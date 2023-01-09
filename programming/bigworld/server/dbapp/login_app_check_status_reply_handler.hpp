#ifndef LOGINAPP_CHECK_STATUS_REPLY_HANDLER_HPP
#define LOGINAPP_CHECK_STATUS_REPLY_HANDLER_HPP

#include "network/basictypes.hpp"
#include "network/interfaces.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This method handles the checkStatus request's reply.
 */
class LoginAppCheckStatusReplyHandler : public Mercury::ReplyMessageHandler
{
public:
	LoginAppCheckStatusReplyHandler( const Mercury::Address & srcAddr,
			Mercury::ReplyID replyID );

private:
	virtual void handleMessage( const Mercury::Address & /*srcAddr*/,
			Mercury::UnpackedMessageHeader & /*header*/,
			BinaryIStream & data, void * /*arg*/ );

	virtual void handleException( const Mercury::NubException & /*ne*/,
		void * /*arg*/ );
	virtual void handleShuttingDown( const Mercury::NubException & /*ne*/,
		void * /*arg*/ );

	Mercury::Address srcAddr_;
	Mercury::ReplyID replyID_;
};

BW_END_NAMESPACE

#endif // LOGINAPP_CHECK_STATUS_REPLY_HANDLER_HPP
