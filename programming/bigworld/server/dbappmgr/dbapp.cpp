#include "dbapp.hpp"

#include "dbappmgr.hpp"


BW_BEGIN_NAMESPACE


// -----------------------------------------------------------------------------
// Section: DBApp
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
DBApp::DBApp( Mercury::NetworkInterface & interface,
		const Mercury::Address & addr, DBAppID id,
		Mercury::ReplyID addReplyID ) :
	id_( id ),
	channelOwner_( interface, addr ),
	pendingReplyID_( addReplyID )
{
	channelOwner_.channel().isLocalRegular( false );
	channelOwner_.channel().isRemoteRegular( true );
}


/**
 *	This method handles a retirement message.
 */
void DBApp::retireApp()
{
	this->controlledShutDown( SHUTDOWN_PERFORM );
}


/**
 *	This method informs the DBApp to shut down.
 */
void DBApp::controlledShutDown( ShutDownStage stage )
{
	DBAppInterface::controlledShutDownArgs & args =
		args.start( channelOwner_.bundle() );
	args.stage = stage;
	channelOwner_.send();
}


/**
 *	This method returns the watcher for DBApp instances.
 */
WatcherPtr DBApp::pWatcher()
{
	DirectoryWatcher * pWatcher = new DirectoryWatcher();

	pWatcher->addChild( "id", makeWatcher( &DBApp::id ) );
	pWatcher->addChild( "address", makeWatcher( &DBApp::address ) );

	return pWatcher;
}

BW_END_NAMESPACE

// dbapp.cpp
