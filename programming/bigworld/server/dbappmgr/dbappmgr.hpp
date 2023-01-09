#ifndef DBAPP_HPP
#define DBAPP_HPP

#include "dbapp.hpp"

#include "cstdmf/singleton.hpp"

#include "db/dbappmgr_interface.hpp"
#include "db/db_hash_schemes.hpp"

#include "network/channel_owner.hpp"
#include "network/event_dispatcher.hpp"
#include "network/interfaces.hpp"

#include "server/manager_app.hpp"

BW_BEGIN_NAMESPACE

class DBAppMgrConfig;

namespace Mercury
{
class EventDispatcher;
class NetworkInterface;
} // end namespace Mercury


typedef Mercury::ChannelOwner BaseAppMgr;
typedef Mercury::ChannelOwner CellAppMgr;


/**
 *	This class implements the main singleton object in the DBAppMgr application.
 */
class DBAppMgr : public ManagerApp, 
	public TimerHandler,
	public Singleton< DBAppMgr >
{
public:
	MANAGER_APP_HEADER( DBAppMgr, dbAppMgr )

	typedef DBAppMgrConfig Config;

	DBAppMgr( Mercury::EventDispatcher & mainDispatcher,
		Mercury::NetworkInterface & interface );
	~DBAppMgr();

	void controlledShutDown(
		const DBAppMgrInterface::controlledShutDownArgs & args );

	void handleDBAppMgrBirth(
		const DBAppMgrInterface::handleDBAppMgrBirthArgs & args );

	void handleBaseAppDeath( BinaryIStream & data );

	DBApp * findApp( const Mercury::Address & addr );

	size_t numDBApps() const	{ return dbApps_.size(); }

	/** This method returns the DBApp Alpha's DBAppID. */
	DBAppID alphaID() const 
	{ 
		return dbApps_.empty() ? 0 : dbApps_.smallest().second->id();
	}

	DBApp * dbAppAlpha();

	void onBaseAppMgrStartStatusReceived( bool hasStarted );

	// Message handlers
	void addDBApp( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader & header );

	void recoverDBApp( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader & header,
		const DBAppMgrInterface::recoverDBAppArgs & args );

	void handleDBAppDeath( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const DBAppMgrInterface::handleDBAppDeathArgs & args );

	void handleLoginAppDeath(
		const DBAppMgrInterface::handleLoginAppDeathArgs & args );

	void handleBaseAppMgrBirth(
		const DBAppMgrInterface::handleBaseAppMgrBirthArgs & args );

	void handleCellAppMgrBirth(
		const DBAppMgrInterface::handleCellAppMgrBirthArgs & args );

	void addLoginApp( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void recoverLoginApp( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			const DBAppMgrInterface::recoverLoginAppArgs & args );

	/**
	 *	This method handles the message from DBApp-Alpha confirming one-time
	 *	initialisation has successfully completed.
	 */
	void serverHasStarted()
	{
		// This is its own method because BW_EMPTY_MSG won't work with
		// default arguments.
		this->serverHasStarted( /* hasStarted */ true );
	}

	void serverHasStarted( bool hasStarted );

	// Overrides from TimerHandler
	void handleTimeout( TimerHandle handle, void * arg ) /* override */;

private:
	// Overrides from ServerApp
	bool init( int argc, char * argv[] ) /* override */;
	void onSignalled( int sigNum ) /* override */;
	void onStartOfTick() /* override */;

	void sendDBAppHashUpdate( bool haveNewAlpha );
	void sendDBAppHashUpdateToBaseAppMgr();
	void sendDBAppHashUpdateToCellAppMgr();


	void addWatchers();

	void handleDBAppDeath( const Mercury::Address & addr );

	enum TimeoutType
	{
		TIMEOUT_TICK,
		TIMEOUT_GATHER_LOGIN_APPS
	};

	TimerHandle 	tickTimer_;
	bool			isShuttingDown_;

	BaseAppMgr 		baseAppMgr_;
	CellAppMgr		cellAppMgr_;

	typedef DBHashSchemes::DBAppIDBuckets< DBAppPtr >::HashScheme DBApps;
	DBApps 			dbApps_;
	typedef BW::map< Mercury::Address, DBAppPtr > AddressMap;
	AddressMap 		addressMap_;
	TimeStamp 		dbAppAddStartWaitTime_;
	DBAppID 		lastDBAppID_;

	typedef BW::set< Mercury::Address > LoginApps;
	LoginApps 		loginApps_;
	bool 			shouldAcceptLoginApps_;
	TimerHandle 	gatherLoginAppsTimer_;
	LoginAppID 		lastLoginAppID_;

	enum StartupState
	{
		STARTUP_STATE_NOT_STARTED,
		STARTUP_STATE_STARTED,
		STARTUP_STATE_INDETERMINATE
	};
	StartupState 	startupState_;

	static const double ADD_WAIT_TIME_SECONDS;
};

BW_END_NAMESPACE

#endif // DBAPP_HPP
