#include "script/first_include.hpp"

#include "baseappmgr_gateway.hpp"

#include "baseapp.hpp"
#include "base.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"

#include "network/channel_owner.hpp"
#include "network/bundle.hpp"

#include "server/backup_hash.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

BaseAppMgrGateway::BaseAppMgrGateway( Mercury::NetworkInterface & interface ) :
		ManagerAppGateway( interface, BaseAppMgrInterface::retireApp )
{
}


void BaseAppMgrGateway::onManagerRebirth( BaseApp & baseApp,
		const Mercury::Address & addr )
{
	channel_.addr( addr );

	Mercury::Bundle & bundle = channel_.bundle();

	bundle.startMessage( BaseAppMgrInterface::recoverBaseApp );
	baseApp.addBaseAppMgrRebirthData( bundle );

	channel_.send();
}


void BaseAppMgrGateway::add( const Mercury::Address & addrForCells,
		const Mercury::Address & addrForClients, bool isServiceApp,
		Mercury::ReplyMessageHandler * pHandler )
{
	Mercury::Bundle	& bundle = channel_.bundle();

	BaseAppMgrInterface::addArgs * args =
		(BaseAppMgrInterface::addArgs*)bundle.startStructRequest(
			BaseAppMgrInterface::add, pHandler );

	args->addrForCells = addrForCells;
	args->addrForClients = addrForClients;
	args->isServiceApp = isServiceApp;

	channel_.send();
}


void BaseAppMgrGateway::useNewBackupHash( const BackupHash & entityToAppHash,
		const BackupHash & newEntityToAppHash )
{
	Mercury::Bundle & bundle = channel_.bundle();

	bundle.startMessage( BaseAppMgrInterface::useNewBackupHash );
	bundle << entityToAppHash;
	bundle << newEntityToAppHash;

	channel_.send();
}


void BaseAppMgrGateway::informOfArchiveComplete(
		const Mercury::Address & deadBaseAppAddr )
{
	Mercury::Bundle & bundle = channel_.bundle();

	bundle.startMessage( BaseAppMgrInterface::informOfArchiveComplete );

	bundle << deadBaseAppAddr;

	channel_.send();
}


void BaseAppMgrGateway::finishedInit()
{
	channel_.bundle().startMessage( BaseAppMgrInterface::finishedInit );
	channel_.send();
}


void BaseAppMgrGateway::registerBaseGlobally( const BW::string & pickledKey,
		const EntityMailBoxRef & mailBox,
		Mercury::ReplyMessageHandler * pHandler )
{
	Mercury::Bundle & bundle = channel_.bundle();

	bundle.startRequest( BaseAppMgrInterface::registerBaseGlobally, pHandler );
	bundle << pickledKey;
	bundle << mailBox;

	channel_.send();
}


void BaseAppMgrGateway::deregisterBaseGlobally( const BW::string & pickledKey )
{
	Mercury::Bundle & bundle = channel_.bundle();

	bundle.startMessage( BaseAppMgrInterface::deregisterBaseGlobally );
	bundle << pickledKey;

	channel_.channel().delayedSend();
}


void BaseAppMgrGateway::registerServiceFragment( Base * pBaseEntity )
{
	Mercury::Bundle & bundle = channel_.bundle();

	bundle.startMessage( BaseAppMgrInterface::registerServiceFragment );
	bundle << pBaseEntity->pType()->description().name();
	bundle << pBaseEntity->baseEntityMailBoxRef();

	channel_.send();
}


void BaseAppMgrGateway::deregisterServiceFragment(
	const BW::string & serviceName )
{
	Mercury::Bundle & bundle = channel_.bundle();
	bundle.startMessage( BaseAppMgrInterface::deregisterServiceFragment );

	bundle << serviceName;

	channel_.send();
}

BW_END_NAMESPACE

// baseappmgr_gateway.cpp

