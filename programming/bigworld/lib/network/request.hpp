#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "misc.hpp"
#include "nub_exception.hpp"

#include "cstdmf/timer_handler.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class Channel;
class EventDispatcher;
class ReplyMessageHandler;
class ReplyOrder;
class RequestManager;

/**
 *	Class to hold the required context for a request and its expected reply
 *	message. 
 */
class Request : public TimerHandler
{
public:
	Request( int replyID, const ReplyOrder & replyOrder,
			Channel * pChannel, RequestManager * pRequestManager,
			EventDispatcher & dispatcher );
	virtual ~Request();

	// Overrides from TimerHandler
	virtual void handleTimeout( TimerHandle handle, void * arg );
	virtual void onRelease( TimerHandle handle, void * pUser );

	void handleMessage( const Address & source,
		UnpackedMessageHeader & header,
		BinaryIStream & data );
	void handleFailure( Reason reason );

	bool matches( Channel * pChannel ) const 
	{ 
		return pChannel_ == pChannel; 
	}

	bool matches( ReplyMessageHandler * pHandler ) const
	{ 
		return pHandler_ == pHandler; 
	}

	bool isValidSource( const Address & source ) const;

	int replyID() const						{ return replyID_; }

private:
	void finish();

private:
	int						replyID_;
	TimerHandle				timerHandle_;
	ReplyMessageHandler * 	pHandler_;
	void *					arg_;
	Channel *				pChannel_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // REQUEST_HPP
