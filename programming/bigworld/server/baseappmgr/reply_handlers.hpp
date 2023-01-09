#ifndef REPLY_HANDLERS_HPP
#define REPLY_HANDLERS_HPP

#include "network/basictypes.hpp"
#include "network/forwarding_reply_handler.hpp"
#include "network/interfaces.hpp"
#include "network/misc.hpp"

#include "server/common.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to handle reply messages from BaseApps when a
 *	createBase message has been sent. This sends the base creation reply back
 *	to the DBApp.
 */
class CreateBaseReplyHandler : public Mercury::ReplyMessageHandler
{
public:
	CreateBaseReplyHandler( const Mercury::Address & srcAddr,
		 	Mercury::ReplyID replyID,
			const Mercury::Address & externalAddr );

private:
	void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, void * arg );

	void handleException( const Mercury::NubException & ne, void * arg );
	void handleShuttingDown( const Mercury::NubException & ne, void * arg );

	Mercury::Address 	srcAddr_;
	Mercury::ReplyID	replyID_;
	Mercury::Address 	externalAddr_;
};


/**
 *	This class is used to handle the controlled shutdown stage that can be sent
 *	to all BaseApps at the same time.
 */
class SyncControlledShutDownHandler :
	public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	SyncControlledShutDownHandler( ShutDownStage stage, int count );

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data, void * );

	virtual void handleException( const Mercury::NubException & ne, void * );

	void finalise();
	void decCount();

	ShutDownStage stage_;
	int count_;
};


/**
 *	This class is used to handle the controlled shutdown stage that is sent to
 *	all BaseApps sequentially.
 */
class AsyncControlledShutDownHandler :
	public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	AsyncControlledShutDownHandler( ShutDownStage stage,
			BW::vector< Mercury::Address > & addrs );
	virtual ~AsyncControlledShutDownHandler() {}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data, void * );

	virtual void handleException( const Mercury::NubException & ne, void * );

	void sendNext();

	ShutDownStage stage_;
	BW::vector< Mercury::Address > addrs_;
	int numToSend_;
};


/**
 *	This class is used to handle replies from the CellAppMgr to the checkStatus
 *	request.
 */
class CheckStatusReplyHandler : public ForwardingReplyHandler
{
public:
	CheckStatusReplyHandler( const Mercury::Address & srcAddr,
			Mercury::ReplyID replyID );

	virtual void prependData( BinaryIStream & inStream,
			BinaryOStream & outStream );
};

BW_END_NAMESPACE

#endif // REPLY_HANDLERS_HPP
