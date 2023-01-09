#include "dbappmgr_gateway.hpp"

#include "db/dbappmgr_interface.hpp"

#include "network/bundle.hpp"
#include "network/machined_utils.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: DB App Manager
// -----------------------------------------------------------------------------


/**
 *	The constructor for DBAppMgrGateway.
 */
DBAppMgrGateway::DBAppMgrGateway( Mercury::NetworkInterface & interface ) :
	ManagerAppGateway( interface, DBAppMgrInterface::retireApp )
{}


/**
 *	This method changes the address to DBAppMgr.
 *
 *	@param address 	The new DBAppMgr address.
 */
void DBAppMgrGateway::address( const Mercury::Address & address )
{
	channel_.addr( address );
}


/**
 *	This method is used to register this DBApp to the DBAppMgr.
 */
void DBAppMgrGateway::addDBApp( Mercury::ReplyMessageHandler * pReplyHandler )
{
	Mercury::Bundle & bundle = channel_.bundle();
	bundle.startRequest( DBAppMgrInterface::addDBApp, pReplyHandler );

	channel_.send();
}


/**
 *	This method notifies DBAppMgr from DBApp-Alpha that server startup
 *	initialisation has been completed.
 */
void DBAppMgrGateway::notifyServerStartupComplete()
{
	Mercury::Bundle & bundle = channel_.bundle();
	bundle.startMessage( DBAppMgrInterface::serverHasStarted );

	channel_.send();
}


/**
 *	This method sends recovery information about this DBApp to a newly started
 *	instance of DBAppMgr.
 */
void DBAppMgrGateway::recoverDBApp( DBAppID id )
{
	DBAppMgrInterface::recoverDBAppArgs & args =
		args.start( channel_.bundle() );
	args.id = id;
	channel_.send();
}


/**
 *	This method makes a request to the DBAppMgr to start a controlled shutdown.
 */
void DBAppMgrGateway::requestControlledShutDown()
{
	DBAppMgrInterface::controlledShutDownArgs & args =
		args.start( channel_.bundle() );

	args.stage = SHUTDOWN_REQUEST;

	channel_.send();
}


BW_END_NAMESPACE

// dbappmgr_gateway.cpp
