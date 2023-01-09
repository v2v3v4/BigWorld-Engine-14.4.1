#include "script/first_include.hpp"

#include "login_app_check_status_reply_handler.hpp"

#include "dbapp.hpp"
#include "dbapp_config.hpp"

#include "network/channel_sender.hpp"
#include "network/bundle.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
LoginAppCheckStatusReplyHandler::LoginAppCheckStatusReplyHandler(
		const Mercury::Address & srcAddr,
		Mercury::ReplyID replyID ) :
	srcAddr_( srcAddr ),
	replyID_ ( replyID )
{
}


/**
 *	This method handles the reply message.
 */
void LoginAppCheckStatusReplyHandler::handleMessage(
		const Mercury::Address & /*srcAddr*/,
		Mercury::UnpackedMessageHeader & /*header*/,
		BinaryIStream & data, void * /*arg*/ )
{
	Mercury::ChannelSender sender( DBApp::getChannel( srcAddr_ ) );
	Mercury::Bundle & bundle = sender.bundle();

	bundle.startReply( replyID_ );

	bool isOkay;
	uint32 numBaseApps;
	uint32 numCellApps;
	uint32 numServiceApps;

	data >> isOkay >> numBaseApps >> numServiceApps >> numCellApps;

	isOkay &= numBaseApps >= DBAppConfig::desiredBaseApps();
	isOkay &= numServiceApps >= DBAppConfig::desiredServiceApps();
	isOkay &= numCellApps >= DBAppConfig::desiredCellApps();

	bundle << uint8( isOkay );

	bundle.transfer( data, data.remainingLength() );

	if (numBaseApps < DBAppConfig::desiredBaseApps())
	{
		bundle << "Not enough BaseApps";
	}

	if (numServiceApps < DBAppConfig::desiredServiceApps())
	{
		bundle << "Not enough ServiceApps";
	}

	if (numCellApps < DBAppConfig::desiredCellApps())
	{
		bundle << "Not enough CellApps";
	}

	delete this;
}


/**
 *	This method handles a failed request.
 */
void LoginAppCheckStatusReplyHandler::handleException(
		const Mercury::NubException & /*ne*/, void * /*arg*/ )
{
	Mercury::ChannelSender sender( DBApp::getChannel( srcAddr_ ) );
	Mercury::Bundle & bundle = sender.bundle();

	bundle.startReply( replyID_ );
	bundle << uint8( false );
	bundle << "No reply from BaseAppMgr";

	delete this;
}


void LoginAppCheckStatusReplyHandler::handleShuttingDown(
		const Mercury::NubException & /*ne*/, void * /*arg*/ )
{
	INFO_MSG( "LoginAppCheckStatusReplyHandler::handleShuttingDown: "
			"Ignoring\n" );
	delete this;
}

BW_END_NAMESPACE

// login_app_check_status_reply_handler.cpp
