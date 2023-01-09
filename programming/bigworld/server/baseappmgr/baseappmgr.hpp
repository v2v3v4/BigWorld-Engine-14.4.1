#ifndef BASE_APP_MGR_HPP
#define BASE_APP_MGR_HPP

#include "baseappmgr_interface.hpp"
#include "util.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/singleton.hpp"

#include "db/dbapps_gateway.hpp"

#include "network/channel_owner.hpp"

#include "server/common.hpp"
#include "server/manager_app.hpp"
#include "server/services_map.hpp"

#include "cstdmf/bw_std.hpp"

BW_BEGIN_NAMESPACE

class BackupHashChain;
class BaseApp;
class BaseAppMgr;
class BaseAppMgrConfig;
class TimeKeeper;

typedef Mercury::ChannelOwner CellAppMgr;
typedef Mercury::ChannelOwner DBApp;

namespace Mercury
{
class EventDispatcher;
class NetworkInterface;
}

/**
 *	This class provides a Java-ish style iterator over managed BaseApps with
 *	optional filtering.
 */
class BaseAppsIterator
{
public:
	typedef std::function<bool ( const BaseApp & )> Predicate;
	BaseAppsIterator( const BaseApps & apps,
					  const Predicate & pred = Predicate() );
	const BaseAppPtr & next();
private:
	BaseApps::const_iterator		iter_;
	const BaseApps::const_iterator	end_;
	const Predicate					predicate_;
	static const BaseAppPtr			endOfContainer_;
};


/**
 *	This class encapsulates operations that should be applied only to a subset
 *	of managed BaseApps.
 */
class ManagedAppSubSet
{
public:
	uint size() const
	{
		MF_ASSERT_DEBUG( size_ == this->countApps() );
		return size_;
	}
	BaseApp * findLeastLoadedApp() const;
	float minAppLoad() const;
	float avgAppLoad() const;
	float maxAppLoad() const;

	BaseAppID lastAppID() const { return lastAppID_; }

	void adjustBackupLocations( BaseApp & app, AdjustBackupLocationsOp op );
	virtual void updateCreateBaseInfo() = 0;

	void recoverBaseApp( const Mercury::Address & srcAddr,
		BaseAppMgr & baseAppMgr, const Mercury::Address & addrForCells,
		const Mercury::Address & addrForClients, BaseAppID id,
		bool isServiceApp, BinaryIStream & data );

	void killApp( const BaseApp & baseApp );
	virtual void addApp( const BaseAppPtr & pBaseApp );
	void removeApp( const BaseApp & baseApp );
	virtual bool onBaseAppDeath( BaseApp & app );
	
	BaseAppID getNextAppID();

	/**
	 *	This method should return the minimum number of apps required for a
	 *	particular subset.
	 */
	virtual uint minimumRequiredApps() const = 0;

protected:
	ManagedAppSubSet( BaseApps & apps, CellAppMgr & cellAppMgr );
	typedef BaseAppMgrConfig Config;

	/**
	 *	This method should return an iterator that only enumerates apps from this
	 *	subset.
	 */
	virtual BaseAppsIterator iterator() const = 0;
	virtual ~ManagedAppSubSet() { }

private:
	void stopBackupsTo( const Mercury::Address & addr );
	uint countApps() const;
	bool hasIdealBackups() const;

	/**
	 *	This method should return a camel-case base name (no ID) for an app
	 *	in this subset.
	 */
	virtual const char * appName() const = 0;

	/**
	 *	This method should return @a true if the server should be shut down when
	 *	an app from this subset dies.
	 */
	virtual bool shutDownOnAppDeath() const = 0;

protected:
	BaseApps &		apps_;
	CellAppMgr &	cellAppMgr_;
	BaseAppID		lastAppID_;		//  last id allocated for a BaseApp
	// Cached subset size.
	uint			size_;
	// Whether we've got apps on multiple machines.
	bool			hasIdealBackups_;
};


/**
 *	This class extends @ref ManagedAppSubSet to represent a subset of "real"
 *	BaseApps (not ServiceApps).
 */
class BaseAppSubSet : public ManagedAppSubSet
{
public:
	BaseAppSubSet( BaseApps & apps, CellAppMgr & cellAppMgr );
	void updateBestBaseApp();
	void updateCreateBaseInfo() /* override */;
	void clearBestBaseApp();
	const Mercury::Address & bestBaseApp() const { return bestBaseAppAddr_; }
	void bestBaseApp( const Mercury::Address & addr )
	{
		bestBaseAppAddr_ = addr;
	}
	void addApp( const BaseAppPtr & pBaseApp ) /* override */;
	uint minimumRequiredApps() const /* override */ { return 1; }

private:
	const char * appName() const /* override */ { return "BaseApp"; }
	bool shutDownOnAppDeath() const /* override */;
	BaseAppsIterator iterator() const /* override */;

	Mercury::Address bestBaseAppAddr_;
};


/**
 *	This class extends @ref ManagedAppSubSet to represent a subset ServiceApps.
 */
class ServiceAppSubSet: public ManagedAppSubSet
{
public:
	ServiceAppSubSet( BaseApps & apps, CellAppMgr & cellAppMgr );
	void addServiceFragmentFromStream( const Mercury::Address & srcAddr,
		BinaryIStream & data, bool sendToBaseApps );
	bool deregisterServiceFragment( const Mercury::Address & addr,
		BinaryIStream & data, ShutDownStage shutDownStage );
	void putServiceFragmentsInto( Mercury::Bundle & bundle );
	void recoverServiceFragments( const Mercury::Address & srcAddr,
		BinaryIStream & data );
	bool onBaseAppDeath( BaseApp & app ) /* override */;
	uint minimumRequiredApps() const /* override */;
	void updateCreateBaseInfo() /* override */;

private:
	BaseAppsIterator iterator() const /* override */;
	const char * appName() const /* override */ { return "ServiceApp"; }
	bool shutDownOnAppDeath() const /* override */;

	ServicesMap services_;
};


/**
 *	This singleton class is the global object that is used to manage proxies and
 *	bases.
 */
class BaseAppMgr : public ManagerApp,
	private TimerHandler,
	public Singleton< BaseAppMgr >
{
public:
	MANAGER_APP_HEADER( BaseAppMgr, baseAppMgr )

	typedef BaseAppMgrConfig Config;

	BaseAppMgr( Mercury::EventDispatcher & mainDispatcher,
			Mercury::NetworkInterface & interface );
	virtual ~BaseAppMgr();

	void createEntity( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	// BaseApps register/unregister via the add/del calls.
	void add( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			const BaseAppMgrInterface::addArgs & args );

	void recoverBaseApp( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void del( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader & header,
		const BaseAppMgrInterface::delArgs & args );

	void finishedInit( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader & header);

	void informOfLoad( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader & header,
		const BaseAppMgrInterface::informOfLoadArgs & args );

	virtual void shutDown() // from ServerApp
		{ this->shutDown( false ); }


	void shutDown( bool shutDownOthers );
	void shutDown( const BaseAppMgrInterface::shutDownArgs & args );
	void startup( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void checkStatus( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void controlledShutDown(
			const BaseAppMgrInterface::controlledShutDownArgs & args );

	void requestHasStarted( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header );

	void initData( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void spaceDataRestore( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void setSharedData( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void delSharedData( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void useNewBackupHash( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void informOfArchiveComplete( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void requestBackupHashChain( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void updateDBAppHash( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	BaseApp * findBaseApp( const Mercury::Address & addr );
	Mercury::ChannelOwner * findChannelOwner( const Mercury::Address & addr );

	static Mercury::UDPChannel & getChannel( const Mercury::Address & addr );

	/*
	 * This method returns the number of known BaseApps and ServiceApps.
	 */
	uint numBaseAndServiceApps() const
	{
		return baseAndServiceApps_.size();
	}
	uint numBaseApps() const { return baseApps_.size(); }
	uint numServiceApps() const { return serviceApps_.size(); }

	int numBases() const;
	int numProxies() const;

	CellAppMgr & cellAppMgr()	{ return cellAppMgr_; }
	DBApp & dbAppAlpha()		{ return dbAppAlpha_; }

	// ---- Message Handlers ----
	void handleCellAppMgrBirth(
		const BaseAppMgrInterface::handleCellAppMgrBirthArgs & args );
	void handleBaseAppMgrBirth(
		const BaseAppMgrInterface::handleBaseAppMgrBirthArgs & args );

	void handleCellAppDeath( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );
	void handleBaseAppDeath(
		const BaseAppMgrInterface::handleBaseAppDeathArgs & args );
	void handleBaseAppDeath( const Mercury::Address & addr );

	void registerBaseGlobally( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void deregisterBaseGlobally( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void registerServiceFragment( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void deregisterServiceFragment( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void runScript( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void startAsyncShutDownStage( ShutDownStage stage );

	void controlledShutDownServer();

	bool onBaseAppDeath( const Mercury::Address & addr );

	void removeControlledShutdownBaseApp( const Mercury::Address & addr );

	void onBaseAppRetire( BaseApp & baseApp );

	void ackBaseAppsShutDownToCellAppMgr( ShutDownStage stage );

private:
	virtual bool init( int argc, char* argv[] );

	enum TimeOutType
	{
		TIMEOUT_GAME_TICK
	};

	virtual void handleTimeout( TimerHandle handle, void * arg );

	void startTimer();
	void checkBackups(); // Old-style backup
	void adjustBackupLocations( const Mercury::Address & addr, 
			AdjustBackupLocationsOp op );

	void checkForDeadBaseApps();

	void runScriptAll( BW::string script );
	void runScriptSingle( BW::string script );

	void runScript( const BW::string & script, int8 broadcast );

	void addServiceFragmentFromStream( const Mercury::Address & srcAddr,
		BinaryIStream & data,
		bool sendToBaseApps );

	void recoverGlobalBasesFromStream( const Mercury::Address & srcAddr,
		BinaryIStream & data );

	void recoverSharedDataFromStream( BinaryIStream & data );

	ManagedAppSubSet & getSubSet( const BaseApp & app );
	
	void startupBaseApp( BaseApp & baseApp, bool & shouldSendBootstrap,
		bool didAutoLoadEntitiesFromDB );

	void synchronize( BaseAppPtr baseApp );

	/**
	 *	This method returns a subset instance reference corresponding to the
	 *	given argument.
	 *	@param isServiceApp Which subset to pick.
	 */
	ManagedAppSubSet & getSubSet( bool isServiceApp )
	{
		return isServiceApp ? static_cast<ManagedAppSubSet &>( serviceApps_ )
							: static_cast<ManagedAppSubSet &>( baseApps_ );
	}
	void redirectGlobalBases( const BaseApp & baseApp );
	void sendBaseAppDeathNotifications( const BaseApp & baseApp );
public:
	const BaseApps & baseApps() const { return baseAndServiceApps_; }

private:
	void addWatchers();

	bool calculateOverloaded( bool areBaseAppsOverloaded );

	void informBaseAppsOfShutDown(
		const BaseAppMgrInterface::controlledShutDownArgs & args );

	CellAppMgr				cellAppMgr_;
	DBApp					dbAppAlpha_;
	DBAppsGateway 			dbApps_;

	// A container for all apps added but not ready to process message.
	BaseApps			pendingApps_;

	// A container for all apps ready to process message.
	// NOTE:	The 'baseAndServiceApps_' container should only be modified
	//			through the corresponding subset's 'addApp'/'removeApp' methods.
	const BaseApps			baseAndServiceApps_;
	BaseAppSubSet			baseApps_;
	ServiceAppSubSet		serviceApps_;

	std::auto_ptr< BackupHashChain >	pBackupHashChain_;

	typedef BW::map< BW::string, BW::string > SharedData;
	SharedData 		sharedBaseAppData_; // Authoritative copy
	SharedData 		sharedGlobalData_; // Copy from CellAppMgr

	typedef BW::map< BW::string, EntityMailBoxRef > GlobalBases;
	GlobalBases globalBases_;

	TimeKeeper *	pTimeKeeper_;
	int				syncTimePeriod_;

	bool			isRecovery_;
	bool			hasInitData_;
	bool			hasStarted_;
	bool			shouldShutDownOthers_;

	Mercury::Address	deadBaseAppAddr_;
	unsigned int		archiveCompleteMsgCounter_;

	GameTime		shutDownTime_;
	ShutDownStage	shutDownStage_;

	// Entity creation rate limiting when baseapps are overload
	uint64			baseAppOverloadStartTime_;
	int				loginsSinceOverload_;

	TimerHandle		gameTimer_;

	friend class CreateBaseReplyHandler;
};

#ifdef CODE_INLINE
#include "baseappmgr.ipp"
#endif

BW_END_NAMESPACE

#endif // BASE_APP_MGR_HPP
