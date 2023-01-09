#ifndef MANAGER_APP_HPP
#define MANAGER_APP_HPP

#include "server_app.hpp"

#define MANAGER_APP_HEADER SERVER_APP_HEADER


BW_BEGIN_NAMESPACE

/**
 *	This class is a common base class for BaseAppMgr and CellAppMgr.
 */
class ManagerApp : public ServerApp
{
public:
	ManagerApp( Mercury::EventDispatcher & mainDispatcher,
			Mercury::NetworkInterface & interface );

protected:
	void addWatchers( Watcher & watcher );

private:
};

BW_END_NAMESPACE

#endif // MANAGER_APP_HPP
