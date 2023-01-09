#ifndef CELLAPPMGR_GATEWAY_HPP
#define CELLAPPMGR_GATEWAY_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/watcher.hpp"
#include "network/basictypes.hpp"
#include "network/channel_owner.hpp"
#include "network/misc.hpp"

#include "server/common.hpp"
#include "server/manager_app_gateway.hpp"


BW_BEGIN_NAMESPACE

typedef uint8 SharedDataType;

class CellApp;
class Cells;

/**
 * 	This is a simple helper class that is used to represent the remote cell
 * 	manager.
 */
class CellAppMgrGateway : public ManagerAppGateway
{
public:
	CellAppMgrGateway( Mercury::NetworkInterface & interface );

	void add( const Mercury::Address & addr, uint16 viewerPort,
			Mercury::ReplyMessageHandler * pReplyHandler );

	void informOfLoad( float load );

	void handleCellAppDeath( const Mercury::Address & addr );

	void setSharedData( const BW::string & key, const BW::string & value,
		   SharedDataType type );
	void delSharedData( const BW::string & key, SharedDataType type );

	void ackCellAppDeath( const Mercury::Address & deadAddr );
	void ackShutdown( ShutDownStage stage );

	void updateBounds( const Cells & cells );

	void onManagerRebirth( CellApp & cellApp, const Mercury::Address & addr );

	void shutDownSpace( SpaceID spaceID );

	// TODO:BAR Remove these
	const Mercury::Address & addr() const 	{ return channel_.addr(); }
	Mercury::Bundle & bundle() 				{ return channel_.bundle(); }
	void send() 							{ channel_.send(); }
};

BW_END_NAMESPACE

#endif // CELLAPPMGR_GATEWAY_HPP
