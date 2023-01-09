#include "manager_app.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ManagerApp
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ManagerApp::ManagerApp( Mercury::EventDispatcher & mainDispatcher,
			Mercury::NetworkInterface & interface ) :
	ServerApp( mainDispatcher, interface )
{
}

/**
 *	This method adds the watchers associated with this class.
 */
void ManagerApp::addWatchers( Watcher & watcher )
{
	ServerApp::addWatchers( watcher );
}

BW_END_NAMESPACE

// manager_app.hpp
