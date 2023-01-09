#include "pch.hpp"

#include "deferred_bundle.hpp"

#include "cstdmf/guard.hpp"
#include "cstdmf/memory_stream.hpp"
#include "network/interface_element.hpp"


BW_BEGIN_NAMESPACE


namespace Mercury
{


/**
 *	Constructor.
 */
DeferredBundle::DeferredBundle():
		Bundle(),
		messages_(),
		pData_( new MemoryOStream() )
{}


/**
 *	Destructor.
 */
DeferredBundle::~DeferredBundle()
{

}


/*
 *	Override from Bundle.
 */
void DeferredBundle::startMessage( const InterfaceElement & ie, 
		ReliableType reliable )
{
	if (!messages_.empty())
	{
		messages_.back().finish( pData_->size() );
	}

	DeferredMessage message( pData_->size(), ie, reliable );
	messages_.push_back( message );
}


/*
 *	Override from Bundle.
 */
void DeferredBundle::startRequest( const InterfaceElement & ie,
		ReplyMessageHandler * pRequestHandler,
		void * arg, int timeout, ReliableType reliable )
{
	if (!messages_.empty())
	{
		messages_.back().finish( pData_->size() );
	}

	DeferredMessage message( pData_->size(), ie, reliable,
		pRequestHandler, arg, timeout );
	messages_.push_back( message );
}


/*
 *	Override from Bundle.
 */
void DeferredBundle::startReply( ReplyID id, ReliableType reliable )
{
	if (!messages_.empty())
	{
		messages_.back().finish( pData_->size() );
	}

	DeferredMessage message( pData_->size(), Mercury::InterfaceElement::REPLY,
		reliable );
	messages_.push_back( message );
}


/*
 *	Override from Bundle.
 */
void DeferredBundle::clear( bool newBundle )
{
	messages_.clear();
	pData_.reset( new MemoryOStream() );
}


/*
 *	Override from Bundle.
 */
int DeferredBundle::freeBytesInLastDataUnit() const
{
	// Don't send out any opportunistic data.
	return 0;
}


/*
 *	Override from Bundle.
 */
int DeferredBundle::numDataUnits() const
{
	return 1;
}


/*
 *	Override from Bundle.
 */
int DeferredBundle::numMessages() const
{
	return static_cast<int>(messages_.size());
}


/*
 *	Override from Bundle.
 */
void DeferredBundle::finalise()
{
	if (!messages_.empty())
	{
		messages_.back().finish( pData_->size() );
	}
}


/*
 *	Override from BinaryOStream.
 */
void * DeferredBundle::reserve( int nBytes )
{
	return pData_->reserve( nBytes );
}


/*
 *	Override from BinaryOStream.
 */
int DeferredBundle::size() const
{
	return pData_->size();
}


/**
 *	This method applies all the deferred messages from this bundle onto the
 *	given bundle.
 *
 *	@param bundle	The bundle to apply the messages to.
 */
void DeferredBundle::applyToBundle( Bundle & bundle )
{
	this->finalise();

	pData_->rewind();

	DeferredMessages::iterator iDeferredMessage = messages_.begin();

	while (iDeferredMessage != messages_.end())
	{
		iDeferredMessage->apply( bundle, *pData_ );
		++iDeferredMessage;
	}
}


/**
 *	Constructor.
 *
 *	@param offset 				An offset into the parent's data buffer.
 *	@param ie					The interface element.
 *	@param reliable				The reliability type.
 *	@param pRequestHandler 		The request handler, or NULL for non-request.
 *	@param arg 					The user data for requests. Only used for
 *								requests.
 *	@param timeout 				The timeout for requess. Only used for
 *								requests.
 */
DeferredBundle::DeferredMessage::DeferredMessage( int offset, 
			const InterfaceElement & ie, ReliableType reliable, 
			ReplyMessageHandler * pRequestHandler, void * arg, int timeout ) :
		offset_( offset ),
		length_( -1 ),
		ie_( ie ),
		reliable_( reliable ),
		pRequestHandler_( pRequestHandler ),
		arg_( arg ),
		timeout_( timeout )
{}


/**
 *	This method finalises this message.
 *
 *	@param newOffset 	The new offset, indicating the end of the data payload.
 */
void DeferredBundle::DeferredMessage::finish( int newOffset )
{
	length_ = newOffset - offset_;
}


/**
 *	Apply this message onto the given bundle.
 *
 *	@param bundle 	The bundle to stream a message onto.
 *	@param stream 	The message payload.
 */
void DeferredBundle::DeferredMessage::apply( Bundle & bundle,
		BinaryIStream & stream )
{
	MF_ASSERT( (length_ != -1) && (length_ <= stream.remainingLength()) );

	if (!pRequestHandler_)
	{
		bundle.startMessage( ie_, reliable_ );
	}
	else
	{
		bundle.startRequest( ie_, pRequestHandler_, arg_, timeout_, reliable_ );
	}
	
	bundle.transfer( stream, length_ );
}


} // end namespace Mercury

BW_END_NAMESPACE

// deferred_bundle.cpp

