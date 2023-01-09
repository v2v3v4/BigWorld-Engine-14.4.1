#include "script/first_include.hpp"

#include "rate_limit_message_filter.hpp"

#include "baseapp.hpp"
#include "rate_limit_config.hpp"

#include "cstdmf/timestamp.hpp"
#include "network/interfaces.hpp"
#include "server/server_app_option.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is a buffered message instance, and stores the header, data
 *	stream and the destination message handler for deferred playback.
 *
 *	Note that the source address is not stored, as RateLimitMessageFilters are
 *	per-address already, and it supplies that source address to the dispatch
 *	method.
 *
 *	Application code can derive from BufferedMessage for associating their own
 *	state and/or overriding the dispatch method for doing any custom actions.
 *	They can override RateLimitMessageFilter::Callback::createBufferedMessage()
 *	in order to have the filter create instances of different derived classes.
 */
class BufferedMessage
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param header 		The message header.
	 *	@param data 		The message data stream.
	 *	@param pHandler 	The destination message handler.
	 */
	BufferedMessage( const Mercury::UnpackedMessageHeader & header,
				BinaryIStream & data,
				Mercury::InputMessageHandler * pHandler ):
			header_( header ),
			data_( data.remainingLength() ),
			pHandler_( pHandler )
	{
		data_.transfer( data, data.remainingLength() );
	}


	/**
	 *	Destructor.
	 */
	virtual ~BufferedMessage()
	{}


	/**
	 *	Dispatch a buffered message. Subclasses of BufferedMessage can override
	 *	this method to supply custom functionality.
	 *
	 *	@param pProxy 		The source proxy of the message.
	 */
	virtual void dispatch( ProxyPtr pProxy )
	{
		this->pHandler()->handleMessage( pProxy->clientAddr(), this->header(),
			this->data() );
	}

	/**
	 *	Return the data size of the message. Note that calling this after the
	 *	message has been dispatched will not return the original data size.
	 */
	uint size() const 						{ return data_.remainingLength(); }


	/**
	 *	Return the message header (const version).
	 */
	const Mercury::UnpackedMessageHeader & header() const
												{ return header_; }

	/**
	 *	Return the message header.
	 */
	Mercury::UnpackedMessageHeader & header()	{ return header_; }

protected:
	/**
	 *	Return the message data stream.
	 */
	BinaryIStream & data() 						{ return data_; }

	/**
	 *	Return the message's destination handler.
	 */
	Mercury::InputMessageHandler * pHandler() 	{ return pHandler_; }


private:
	// The message header.
	Mercury::UnpackedMessageHeader 	header_;

	// The message data.
	MemoryOStream 					data_;

	// The destination handler for this message.
	Mercury::InputMessageHandler * 	pHandler_;

};


// ----------------------------------------------------------------------------
// Section: RateLimitMessageFilter
// ----------------------------------------------------------------------------

/**
 *	Constructor.
 */
RateLimitMessageFilter::RateLimitMessageFilter( const Mercury::Address & /*addr*/ ):
		pProxy_( NULL ),
		warnFlags_( 0 ),
		numReceivedSinceLastTick_( 0 ),
		receivedBytesSinceLastTick_( 0 ),
		numDispatchedSinceLastTick_( 0 ),
		dispatchedBytesSinceLastTick_( 0 ),
		queue_(),
		sumBufferedSizes_( 0 )
{
}


/**
 *	Destructor.
 */
RateLimitMessageFilter::~RateLimitMessageFilter()
{
	// remove any buffered messages
	if (!queue_.empty())
	{
		this->clear();
	}
}


/**
 *	This method clears all buffered messages.
 */
void RateLimitMessageFilter::clear()
{
	if ((queue_.size() > 0) && (pProxy_ != NULL))
	{
		DEBUG_MSG( "RateLimitMessageFilter::clear: "
				"removing %" PRIzu " buffered messages for %s\n",
			queue_.size(), pProxy_->c_str() );
	}

	while (!queue_.empty())
	{
		delete queue_.front();
		queue_.pop();
	}
}


/**
 *	This method filters an incoming message and decides whether to handle it
 *	now or buffer for handling later, based on message dispatch statistics for
 *	that source address.
 *
 *	@param srcAddr 		the source address of the message
 *	@param header 		the unpacked message header of the message
 *	@param data			the message data stream
 *	@param pHandler 	the destination handler for the message
 */
void RateLimitMessageFilter::filterMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		Mercury::InputMessageHandler * pHandler )
{
	if (srcAddr != pProxy_->clientAddr())
	{
		// We've probably been given to another client from the last message in
		// the bundle. We can't play these messages for the old proxy on the
		// new proxy.
		WARNING_MSG( "RateLimitMessageFilter::filterMessage: "
				"Dropping message from %s bound for proxy %d (%s)\n",
			srcAddr.c_str(), pProxy_->id(),
			(pProxy_->clientAddr().ip == 0) ?
				"without client" : pProxy_->c_str() );
		return;
	}

	// Update received stats
	++numReceivedSinceLastTick_;
	receivedBytesSinceLastTick_ += data.remainingLength();

	if (queue_.empty() && this->canSendNow( data.remainingLength() ))
	{
		// Filter or the callback says OK!
		this->dispatch( header, data, pHandler );
	}
	else
	{
		BufferedMessage * pMsg = NULL;

		// Create a buffered message - if we have a callback, ask it to create
		// its own (can be a derived class)

		pMsg = new BufferedMessage( header, data, pHandler );

		// Buffer with the address filter for later
		if (!this->buffer( pMsg ))
		{
			if (pProxy_)
			{
				pProxy_->onFilterLimitsExceeded( srcAddr, pMsg );
			}
			delete pMsg;
		}
	}
}


/**
 *	Update this message filter for the next tick. Call this method periodically
 *	in the application code.
 */
void RateLimitMessageFilter::tick()
{
	// Check if we are now over the receive warn limits.
	if (!(warnFlags_ & WARN_MESSAGE_COUNT) &&
			(numReceivedSinceLastTick_ >= Config::warnMessagesPerTick()))
	{
		WARNING_MSG( "RateLimitMessageFilter: "
				"Client %s has sent %u messages since last tick (%d bytes)\n",
			pProxy_->c_str(),
			numReceivedSinceLastTick_,
			receivedBytesSinceLastTick_ );

		warnFlags_ |= WARN_MESSAGE_COUNT;
	}
	else
	{
		warnFlags_ &= ~WARN_MESSAGE_COUNT;
	}

	if (!(warnFlags_ & WARN_MESSAGE_SIZE) &&
			(receivedBytesSinceLastTick_ >= Config::warnBytesPerTick()))
	{
		WARNING_MSG( "RateLimitMessageFilter: "
				"Client %s has sent %u bytes since last tick (%d messages)\n",
			pProxy_->c_str(),
			receivedBytesSinceLastTick_,
			numReceivedSinceLastTick_ );

		warnFlags_ |= WARN_MESSAGE_SIZE;
	}
	else
	{
		warnFlags_ &= ~WARN_MESSAGE_SIZE;
	}


	this->replayAny();


	// Check if we are still over our buffered warn limits.
	if ((warnFlags_ & WARN_MESSAGE_COUNT) &&
			(numDispatchedSinceLastTick_ < Config::warnMessagesPerTick()))
	{
		warnFlags_ &= ~WARN_MESSAGE_COUNT;
	}

	if ((warnFlags_ & WARN_MESSAGE_SIZE) &&
			(dispatchedBytesSinceLastTick_ < Config::warnBytesPerTick()))
	{
		warnFlags_ &= ~WARN_MESSAGE_SIZE;
	}

	// Reset our stats for the next tick
	numReceivedSinceLastTick_ = 0;
	receivedBytesSinceLastTick_ = 0;
	numDispatchedSinceLastTick_ = 0;
	dispatchedBytesSinceLastTick_ = 0;
}


/**
 *	Return true if we can accept a new incoming message with the given length.
 *	Returns false if we cannot accept it due to count or size limits being
 *	exceeded.
 *
 *	@param dataLen		the length of the queried message
 */
bool RateLimitMessageFilter::canSendNow( uint dataLen )
{
	return (numDispatchedSinceLastTick_ <= Config::maxMessagesPerTick()) &&
		(dispatchedBytesSinceLastTick_ + int( dataLen ) <=
			Config::maxBytesPerTick());
}


/**
 *	Pops the next buffered message from the queue. The message is NOT deleted.
 */
void RateLimitMessageFilter::pop_front()
{
	MF_ASSERT( !queue_.empty() );

	BufferedMessage * pMsg = this->front();
	sumBufferedSizes_ -= pMsg->size();

	queue_.pop();

}


/**
 *	Buffer the given message for later handling.
 *
 *	@param pMsg 	the message to buffer
 *	@return 		true if the message was buffered, false if we failed to
 *					buffer the message due to the buffer size limit being
 *					exceeded.
 */
bool RateLimitMessageFilter::buffer( BufferedMessage * pMsg )
{
	if (queue_.size() >= Config::maxMessagesBuffered())
	{
		INFO_MSG( "RateLimitMessageFilter(%s): "
				"Could not buffer further messages, "
				"message count limit reached\n",
			pProxy_->c_str() );
		return false;
	}
	if (sumBufferedSizes_ + pMsg->size() >= Config::maxBytesBuffered())
	{
		INFO_MSG( "RateLimitMessageFilter(%s): "
				"Could not buffer further messages, "
				"message size limit reached\n",
			pProxy_->c_str() );
		return false;
	}

	queue_.push( pMsg );
	sumBufferedSizes_ += pMsg->size();

	if (!(warnFlags_ & WARN_MESSAGE_BUFFERED) &&
			queue_.size() >= Config::warnMessagesBuffered())
	{
		WARNING_MSG( "RateLimitMessageFilter: "
				"%" PRIzu " messages pending in buffer from client %s\n",
			queue_.size(), pProxy_->c_str() );
		warnFlags_ |= WARN_MESSAGE_BUFFERED;
	}

	if (!(warnFlags_ & WARN_BYTES_BUFFERED) &&
			sumBufferedSizes_ >= Config::warnBytesBuffered())
	{
		WARNING_MSG( "RateLimitMessageFilter: "
				"%u bytes pending in buffer from client %s\n",
			sumBufferedSizes_, pProxy_->c_str() );
		warnFlags_ |= WARN_BYTES_BUFFERED;
	}

	return true;
}


/**
 *	Set the callback object for this filter.
 *
 *	@param pProxy 	The new proxy to set.
 */
void RateLimitMessageFilter::setProxy( ProxyPtr pProxy )
{ 
	// Make sure messages buffered for the old proxy are not played back on the
	// new proxy.
	this->clear();
	pProxy_ = pProxy;
}


/**
 *	Dispatch a message now.
 *
 *	@param header 		the unpacked message header
 *	@param data			the data stream
 *	@param pHandler 	the handler for the message
 */
void RateLimitMessageFilter::dispatch(
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, Mercury::InputMessageHandler * pHandler )
{
	// Update our account-keeping.
	++numDispatchedSinceLastTick_;
	dispatchedBytesSinceLastTick_ += data.remainingLength();

	pHandler->handleMessage( pProxy_->clientAddr(), header, data );
}


/**
 *	Dispatch a buffered message now.
 *
 *	@param  pMessage 	the message to be dispatched
 */
void RateLimitMessageFilter::dispatch( BufferedMessage * pMessage )
{
	// Update our account-keeping.
	++numDispatchedSinceLastTick_;
	dispatchedBytesSinceLastTick_ += pMessage->size();

	pMessage->dispatch( pProxy_ );
}


/**
 *	Replay any buffered messages that are eligible to be dispatched now.
 *	This method should be called periodically.
 */
void RateLimitMessageFilter::replayAny()
{
	BaseApp & baseApp = BaseApp::instance();
	BufferedMessage * pMsg = this->front();

	baseApp.setBaseForCall( pProxy_.get(), /* isExternalCall */ true );

	// Saved in case we are passed to another proxy during dispatch(). In this
	// case, clear() should have been called, and BaseApp's proxy-for-call will
	// have been cleared.

	ProxyPtr pProxy = pProxy_;

	while (pMsg && 
			baseApp.getProxyForCall( /*okIfNull: */ false ) == pProxy &&
			this->canSendNow( pMsg->size() ))
	{
		this->pop_front();

		this->dispatch( pMsg );

		delete pMsg;

		pMsg = this->front();
	}

	if (baseApp.getProxyForCall( /*okIfNull: */ true ) != pProxy)
	{
		INFO_MSG( "RateLimitMessageFilter: current proxy was changed "
				"while replaying rate-limited messages for proxy %d\n", 
			pProxy->id() );
	}
	else
	{
		baseApp.clearProxyForCall();
	}

	if (queue_.size() < Config::warnMessagesBuffered())
	{
		warnFlags_ &= ~WARN_MESSAGE_BUFFERED;
	}
	if (sumBufferedSizes_ < Config::warnBytesBuffered())
	{
		warnFlags_ &= ~WARN_BYTES_BUFFERED;
	}
}

BW_END_NAMESPACE

// rate_limit_message_filter.cpp
