#ifndef DBAPPMGR_GATEWAY_HPP
#define DBAPPMGR_GATEWAY_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/watcher.hpp"
#include "network/basictypes.hpp"
#include "network/channel_owner.hpp"
#include "network/misc.hpp"

#include "server/common.hpp"
#include "server/manager_app_gateway.hpp"


BW_BEGIN_NAMESPACE

typedef uint8 SharedDataType;

class DBApp;

/**
 * 	This is a simple helper class that is used to represent the remote db
 * 	manager.
 */
class DBAppMgrGateway : public ManagerAppGateway
{
public:
	DBAppMgrGateway( Mercury::NetworkInterface & interface );
	const Mercury::Address & address() const 
	{
		return this->ManagerAppGateway::address();
	}

	void address( const Mercury::Address & address );

	void addDBApp( Mercury::ReplyMessageHandler * pReplyHandler );
	void notifyServerStartupComplete();
	void recoverDBApp( DBAppID id );

	void requestControlledShutDown();
};

BW_END_NAMESPACE

#endif // DBAPPMGR_GATEWAY_HPP
