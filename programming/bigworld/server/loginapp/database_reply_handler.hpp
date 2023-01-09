#ifndef DatabaseReplyHandler_HPP
#define DatabaseReplyHandler_HPP

#include "connection/log_on_params.hpp"

#include "network/basictypes.hpp"
#include "network/channel.hpp"
#include "network/interfaces.hpp"


BW_BEGIN_NAMESPACE

class LoginApp;

/**
 *	An instance of this class is used to receive the reply from a call to
 *	the database.
 */
class DatabaseReplyHandler : public Mercury::ReplyMessageHandler
{
public:
	DatabaseReplyHandler(
		LoginApp & loginApp,
		const Mercury::Address & clientAddr,
		Mercury::Channel * pChannel,
		const Mercury::ReplyID replyID,
		LogOnParamsPtr pParams );

	virtual ~DatabaseReplyHandler() {}

	virtual void handleMessage( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		void * arg );

	virtual void handleException( const Mercury::NubException & ne,
		void * arg );
	virtual void handleShuttingDown( const Mercury::NubException & ne,
		void * arg );

private:
	LoginApp & 			loginApp_;
	Mercury::Address	clientAddr_;
	Mercury::ChannelPtr pChannel_;
	Mercury::ReplyID	replyID_;
	LogOnParamsPtr		pParams_;
};

BW_END_NAMESPACE

#endif // DatabaseReplyHandler_HPP

