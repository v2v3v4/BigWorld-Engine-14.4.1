#include "reply_handlers.hpp"

#include "baseappmgr.hpp"

#include "network/channel_sender.hpp"
#include "network/nub_exception.hpp"
#include "network/bundle.hpp"

#include "baseapp/baseapp_int_interface.hpp"
#include "cellappmgr/cellappmgr_interface.hpp"


BW_BEGIN_NAMESPACE


// -----------------------------------------------------------------------------
// Section: CreateBaseReplyHandler
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
CreateBaseReplyHandler::CreateBaseReplyHandler(
		const Mercury::Address & srcAddr,
		Mercury::ReplyID replyID,
		const Mercury::Address & externalAddr ) :
	srcAddr_( srcAddr ),
	replyID_( replyID ),
	externalAddr_( externalAddr )
{
}


/**
 *	This method handles the reply message to a createBase call to a BaseApp.
 */
void CreateBaseReplyHandler::handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, void * arg )
{
	EntityMailBoxRef ref;
	data >> ref;

	Mercury::ChannelSender sender( BaseAppMgr::getChannel( srcAddr_ ) );
	Mercury::Bundle & bundle = sender.bundle();

	bundle.startReply( replyID_ );

	if (ref.addr.ip != 0)
	{
		// Note: If this changes, check that BaseApp::logOnAttempt is ok.
		bundle << externalAddr_;
		// Should be EntityMailBoxRef and sessionKey
		bundle << ref;
		bundle.transfer( data, data.remainingLength() );
	}
	else
	{
		bundle << Mercury::Address( 0, 0 );
		bundle << "Unable to create base";
	}

	delete this;
}


/**
 *	This method handles an exception to a createBase call to a BaseApp.
 */
void CreateBaseReplyHandler::handleException( const Mercury::NubException & ne,
		void* /*arg*/ )
{
	Mercury::ChannelSender sender( BaseAppMgr::getChannel( srcAddr_ ) );
	Mercury::Bundle & bundle = sender.bundle();

	Mercury::Address addr;

	addr.ip = 0;
	addr.port = 0;

	bundle.startReply( replyID_ );
	bundle << addr;
	bundle << Mercury::reasonToString( ne.reason() );

	delete this;
}


void CreateBaseReplyHandler::handleShuttingDown(
		const Mercury::NubException & ne, void * arg )
{
	INFO_MSG( "CreateBaseReplyHandler::handleShuttingDown: Ignoring\n" );

	delete this;
}


// -----------------------------------------------------------------------------
// Section: SyncControlledShutDownHandler
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
SyncControlledShutDownHandler::SyncControlledShutDownHandler(
									ShutDownStage stage, int count ) :
		stage_( stage ),
		count_( count )
{
	MF_ASSERT( count_ > 0 );
}


/**
 *	This method handles the reply message.
 */
void SyncControlledShutDownHandler::handleMessage(
		const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, void * )
{
	this->decCount();
}


/**
 *	This method handles an exception to the request.
 */
void SyncControlledShutDownHandler::handleException(
		const Mercury::NubException & ne, void * )
{
	this->decCount();
}


/**
 *	This method finalises and deletes this handler.
 */
void SyncControlledShutDownHandler::finalise()
{
	BaseAppMgr * pApp = BaseAppMgr::pInstance();

	if (pApp)
	{
		pApp->ackBaseAppsShutDownToCellAppMgr( stage_ );
	}

	delete this;
}


/**
 *	This method decrements the number of outstanding requests.
 */
void SyncControlledShutDownHandler::decCount()
{
	--count_;

	if (count_ == 0)
	{
		this->finalise();
	}
}


// -----------------------------------------------------------------------------
// Section: AsyncControlledShutDownHandler
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
AsyncControlledShutDownHandler::AsyncControlledShutDownHandler(
			ShutDownStage stage, BW::vector< Mercury::Address > & addrs ) :
		stage_( stage ),
		numToSend_( 0 )
{
	addrs_.swap( addrs );
	this->sendNext();
}


/**
 *	This method handles the reply message.
 */
void AsyncControlledShutDownHandler::handleMessage(
		const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, void * )
{
	DEBUG_MSG( "AsyncControlledShutDownHandler::handleMessage: "
			"BaseApp %s has finished stage %s\n",
		srcAddr.c_str(), ServerApp::shutDownStageToString( stage_ ) );

	if (stage_ == SHUTDOWN_PERFORM)
	{
		BaseAppMgr * pApp = BaseAppMgr::pInstance();
		pApp->removeControlledShutdownBaseApp( srcAddr );
	}

	this->sendNext();
}


/**
 *	This method handles an exception to the request.
 */
void AsyncControlledShutDownHandler::handleException(
		const Mercury::NubException & ne, void * )
{
	ERROR_MSG( "AsyncControlledShutDownHandler::handleException: "
		"Reason = %s\n", Mercury::reasonToString( ne.reason() ) );
	this->sendNext();
}


/**
 *	This method sends the next shutdown request.
 */
void AsyncControlledShutDownHandler::sendNext()
{
	bool shouldDeleteThis = true;
	BaseAppMgr * pApp = BaseAppMgr::pInstance();

	if (pApp)
	{
		while (true)
		{
			if (numToSend_ < int(addrs_.size()))
			{
				Mercury::ChannelOwner * pChannelOwner =
					pApp->findChannelOwner( addrs_[ numToSend_ ] );

				if (pChannelOwner != NULL &&
						!pChannelOwner->channel().hasRemoteFailed())
				{
					Mercury::Bundle & bundle = pChannelOwner->bundle();
					bundle.startRequest(
							BaseAppIntInterface::controlledShutDown, this );
					shouldDeleteThis = false;
					bundle << stage_;
					bundle << 0;
					pChannelOwner->send();
				}
				else
				{
					WARNING_MSG( "AsyncControlledShutDownHandler::sendNext: "
									"Could not find channel for %s\n",
							addrs_[ numToSend_ ].c_str() );
					++numToSend_;
					continue;
				}

				++numToSend_;
			}
			else if (stage_ == SHUTDOWN_DISCONNECT_PROXIES)
			{
				// This object deletes itself.
				new AsyncControlledShutDownHandler( SHUTDOWN_PERFORM, addrs_ );
			}
			else
			{
				Mercury::Bundle & bundle = pApp->cellAppMgr().bundle();
				CellAppMgrInterface::ackBaseAppsShutDownArgs &
					rAckBaseAppsShutDown =
						CellAppMgrInterface::ackBaseAppsShutDownArgs::start(
							bundle );

				rAckBaseAppsShutDown.stage = stage_;

				pApp->cellAppMgr().send();
				pApp->shutDown( false );
			}
			break;
		}
	}

	if (shouldDeleteThis)
	{
		delete this;
	}
}


// -----------------------------------------------------------------------------
// Section: CheckStatusReplyHandler
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
CheckStatusReplyHandler::CheckStatusReplyHandler(
			const Mercury::Address & srcAddr,
			Mercury::ReplyID replyID ) :
		ForwardingReplyHandler( srcAddr, replyID,
				BaseAppMgr::instance().interface() )
{
}


/**
 *	This virtual method prepends extra data to the forwarded reply.
 */
void CheckStatusReplyHandler::prependData( BinaryIStream & inStream,
			BinaryOStream & outStream )
{
	int serviceAppCount = BaseAppMgr::instance().numServiceApps();
	int baseAppCount = BaseAppMgr::instance().numBaseApps();

	uint8 isOkay;
	inStream >> isOkay;

	outStream << isOkay << baseAppCount << serviceAppCount;
}

BW_END_NAMESPACE

// reply_handlers.cpp
