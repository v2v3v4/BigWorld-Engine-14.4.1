#include "pch.hpp"
#include "request.hpp"

#include "event_dispatcher.hpp"
#include "reply_order.hpp"
#include "request_manager.hpp"
#include "udp_channel.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	Constructor.
 */
Request::Request( int replyID,
			const ReplyOrder & replyOrder, Channel * pChannel,
			RequestManager * pRequestManager, EventDispatcher & dispatcher ) :
		replyID_( replyID ),
		timerHandle_(),
		pHandler_( replyOrder.handler ),
		arg_( replyOrder.arg ),
		pChannel_( pChannel )
{
	if (!pChannel)
	{
		MF_ASSERT( replyOrder.microseconds > 0 );
		timerHandle_ = dispatcher.addOnceOffTimer(
							replyOrder.microseconds, this, pRequestManager );
	}
}


/**
 *	Destructor.
 */
Request::~Request()
{
	timerHandle_.cancel();
}


/**
 *	This method returns whether the input address is a valid source for
 *	responding to this request.
 */
bool Request::isValidSource( const Address & source ) const
{
	return (pChannel_ == NULL) || (source == pChannel_->addr());
}


/**
 *	This method handles timeouts for user requests. It informs the handler of
 *	this failure case.
 */
void Request::handleTimeout( TimerHandle /*handle*/, void * arg )
{
	static_cast< RequestManager * >( arg )->failRequest( *this,
			REASON_TIMER_EXPIRED );
}


/**
 *	This method handles a response message to this request.
 */
void Request::handleMessage( const Address & source,
	UnpackedMessageHeader & header,
	BinaryIStream & data )
{
	pHandler_->handleMessage( source, header, data, arg_ );

	this->finish();
}


/**
 *	This method handles failure of the request. This may be caused by failure of
 *	the channel or the request timing out.
 */
void Request::handleFailure( Reason reason )
{
	// now call the exception function of the user's handler
	NubException e( reason );

	if (reason != REASON_SHUTTING_DOWN)
	{
		pHandler_->handleException( e, arg_ );
	}
	else
	{
		pHandler_->handleShuttingDown( e, arg_ );
	}

	this->finish();
}


/**
 *	This method will delete this request.
 */
void Request::finish()
{
	if (timerHandle_.isSet())
	{
		// Cancelling the timer will call onRelease which will delete this.
		timerHandle_.cancel();
	}
	else
	{
		delete this;
	}
}


/**
 *	This is an override from TimerHandler.
 */
void Request::onRelease( TimerHandle handle, void * pUser )
{
	// and finally delete ourselves
	delete this;
}

} // namespace Mercury

BW_END_NAMESPACE

// request.cpp
