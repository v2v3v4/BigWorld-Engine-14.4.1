#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "cellappmgr_gateway.hpp"

#include "cellapp.hpp"

#include "network/bundle.hpp"
#include "network/machined_utils.hpp"

#include "cellappmgr/cellappmgr_interface.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Cell App Manager
// -----------------------------------------------------------------------------


/**
 *	The constructor for CellAppMgrGateway.
 */
CellAppMgrGateway::CellAppMgrGateway( Mercury::NetworkInterface & interface ) :
	ManagerAppGateway( interface, CellAppMgrInterface::retireApp )
{}


/**
 *	This method is used to make an add call to the real Cell App Manager.
 */
void CellAppMgrGateway::add( const Mercury::Address & addr, uint16 viewerPort,
		Mercury::ReplyMessageHandler * pReplyHandler )
{
	Mercury::Bundle & bundle = this->bundle();
	bundle.startRequest( CellAppMgrInterface::addApp, pReplyHandler );
	bundle << addr;
	bundle << viewerPort;

	this->send();
}


/**
 *	This method is used to inform the Cell App Manager of this application's
 *	current load.
 */
void CellAppMgrGateway::informOfLoad( float load )
{
	CellAppMgrInterface::informOfLoadArgs args;
	args.load = load;
	const CellApp & app = CellApp::instance();
	args.numEntities = app.numRealEntities();

	this->bundle() << args;

	this->send();
}


/**
 *	This method is used to inform the Cell App Manager of a dead Cell App.
 */
void CellAppMgrGateway::handleCellAppDeath( const Mercury::Address & addr )
{
	CellAppMgrInterface::handleCellAppDeathArgs args;
	args.addr = addr;
	this->bundle() << args;

	this->send();
}


/**
 *	This method sends a message to the CellAppMgr informing it that a shared
 *	data value has changed. This may be data shared between CellApps, BaseApps,
 *	or both.
 */
void CellAppMgrGateway::setSharedData( const BW::string & key,
		const BW::string & value, SharedDataType dataType )
{
	Mercury::Bundle & bundle = this->bundle();
	bundle.startMessage( CellAppMgrInterface::setSharedData );

	bundle << dataType << key << value;

	this->send();
}


/**
 *	This method sends a message to the CellAppMgr informing it that a shared
 *	data value has been deleted. This may be data shared between CellApps,
 *	BaseApps or both.
 */
void CellAppMgrGateway::delSharedData( const BW::string & key,
		SharedDataType dataType )
{
	Mercury::Bundle & bundle = this->bundle();
	bundle.startMessage( CellAppMgrInterface::delSharedData );
	bundle << dataType;
	bundle << key;

	this->send();
}


/**
 *
 */
void CellAppMgrGateway::ackCellAppDeath( const Mercury::Address & deadAddr )
{
	Mercury::Bundle & bundle = channel_.bundle();
	CellAppMgrInterface::ackCellAppDeathArgs & args =
		CellAppMgrInterface::ackCellAppDeathArgs::start( bundle );

	args.deadAddr = deadAddr;
	channel_.send();
}


/**
 *
 */
void CellAppMgrGateway::ackShutdown( ShutDownStage stage )
{
	Mercury::Bundle & bundle = channel_.bundle();
	CellAppMgrInterface::ackCellAppShutDownArgs & rAckCellAppShutDown =
		CellAppMgrInterface::ackCellAppShutDownArgs::start( bundle );

	rAckCellAppShutDown.stage = stage;

	channel_.send();
}


/**
 *	This method informs the CellAppMgr of data that it needs for load balancing.
 *
 *	@param cells	Collection of cells on this CellApp.
 */
void CellAppMgrGateway::updateBounds( const Cells & cells )
{
	// TODO: Put this in the same bundle as cellAppMgr_.informOfLoad
	Mercury::Bundle & bundle = channel_.bundle();
	bundle.startMessage( CellAppMgrInterface::updateBounds );

	cells.writeBounds( bundle );
	channel_.send();
}


/**
 *
 */
void CellAppMgrGateway::onManagerRebirth( CellApp & cellApp,
		const Mercury::Address & addr )
{
	channel_.addr( addr );

	Mercury::Bundle & bundle = channel_.bundle();
	bundle.startMessage( CellAppMgrInterface::recoverCellApp );

	cellApp.addCellAppMgrRebirthData( bundle );

	channel_.send();
}


/**
 *
 */
void CellAppMgrGateway::shutDownSpace( SpaceID spaceID )
{
	CellAppMgrInterface::shutDownSpaceArgs args;
	args.spaceID = spaceID;

	channel_.bundle() << args;

	channel_.send();
}

BW_END_NAMESPACE

// cellappmgr_gateway.cpp
