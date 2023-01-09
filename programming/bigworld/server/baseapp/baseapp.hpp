#ifndef BASEAPP_HPP
#define BASEAPP_HPP

#include "backed_up_base_app.hpp"
#include "baseapp_int_interface.hpp"
#include "baseappmgr_gateway.hpp"
#include "bases.hpp"
#include "bwtracer.hpp"
#include "proxy.hpp"
#include "rate_limit_message_filter.hpp"

#include "connection/baseapp_ext_interface.hpp"

#include "cstdmf/background_file_writer.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/shared_ptr.hpp"
#include "cstdmf/singleton.hpp"

#include "db/dbapps_gateway.hpp"

#include "network/channel_listener.hpp"
#include "network/channel_owner.hpp"
#include "network/tcp_server.hpp"

#include "script/script_object.hpp"

#include "server/common.hpp"
#include "server/entity_app.hpp"
#include "server/py_services_map.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class Archiver;
class BackedUpBaseApps;
class BackupHashChain;
class BackupSender;
class Base;
class BaseAppConfig;
class BaseMessageForwarder;
class CellEntityMailBox;
class DeadCellApps;
class EntityCreator;
class GlobalBases;
class LoginHandler;
class OffloadedBackups;
class Pickler;
class Proxy;
class ServiceStarter;
class SharedDataManager;
class SqliteDatabase;
class TimeKeeper;
class WorkerThread;
class InitialConnectionFilter;

struct BaseAppInitData;

typedef SmartPointer< Base > BasePtr;
typedef SmartPointer< PyObject > PyObjectPtr;
typedef SmartPointer< InitialConnectionFilter > InitialConnectionFilterPtr;

typedef Mercury::ChannelOwner DBApp;


/**
 *	This class implement the main singleton object in the base application.
 *	Its main responsibility is to manage all of the bases on the local process.
 */
class BaseApp : public EntityApp,
	public TimerHandler,
	public Mercury::ChannelListener,
	public Singleton< BaseApp >
{
public:
	typedef BaseAppConfig Config;

	SERVER_APP_HEADER_CUSTOM_NAME( BaseApp, baseApp )

	BaseApp( Mercury::EventDispatcher & mainDispatcher,
		  Mercury::NetworkInterface & internalInterface,
		  bool isServiceApp = false );
	virtual ~BaseApp();

	virtual void shutDown();

	/* Internal Interface */

	// create this client's proxy
	void createBaseWithCellData( const Mercury::Address& srcAddr,
			const Mercury::UnpackedMessageHeader& header,
			BinaryIStream & data );

	// Handles request create a base from DB
	void createBaseFromDB( const Mercury::Address& srcAddr,
			const Mercury::UnpackedMessageHeader& header,
			BinaryIStream & data );

	// Handles request create a base from template id
	void createBaseFromTemplate( const Mercury::Address& srcAddr,
			const Mercury::UnpackedMessageHeader& header,
			BinaryIStream & data );

	void logOnAttempt( const Mercury::Address& srcAddr,
			const Mercury::UnpackedMessageHeader& header,
			BinaryIStream & data );

	void addGlobalBase( BinaryIStream & data );
	void delGlobalBase( BinaryIStream & data );

	void addServiceFragment( BinaryIStream & data );
	void delServiceFragment( BinaryIStream & data );

	void callWatcher( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data );

	void handleCellAppMgrBirth(
		const BaseAppIntInterface::handleCellAppMgrBirthArgs & args );
	void handleBaseAppMgrBirth(
		const BaseAppIntInterface::handleBaseAppMgrBirthArgs & args );
	void addBaseAppMgrRebirthData( BinaryOStream & stream );

	void handleCellAppDeath( BinaryIStream & data );

	void emergencySetCurrentCell( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data );

	void startup( const BaseAppIntInterface::startupArgs & args );

	// shut down this app nicely
	void shutDown( const BaseAppIntInterface::shutDownArgs & args );
	void controlledShutDown( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data );

	void triggerControlledShutDown();

	void setCreateBaseInfo( BinaryIStream & data );

	// New-style BaseApp backup
	void startBaseEntitiesBackup(
			const BaseAppIntInterface::startBaseEntitiesBackupArgs & args );

	void stopBaseEntitiesBackup(
			const BaseAppIntInterface::stopBaseEntitiesBackupArgs & args );

	void backupBaseEntity( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void ackOffloadBackup( BinaryIStream & data );

	void makeLocalBackup( Base & base );

	BasePtr createBaseFromStream( EntityID id, BinaryIStream & stream );

	void stopBaseEntityBackup( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			const BaseAppIntInterface::stopBaseEntityBackupArgs & args );

	void handleBaseAppBirth( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void handleBaseAppDeath( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void setBackupBaseApps( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void startOffloading( BinaryIStream & stream );

	Mercury::Address backupAddrFor( EntityID entityID ) const;

	// Shared Data message handlers
	void setSharedData( BinaryIStream & data );
	void delSharedData( BinaryIStream & data );

	void updateDBAppHash( BinaryIStream & data );

	// set the proxy to receive future messages
	void setClient( const BaseAppIntInterface::setClientArgs & args );

	void makeNextMessageReliable(
		const BaseAppIntInterface::makeNextMessageReliableArgs & args );

	/* External Interface */
	// let the proxy know who we really are
	void baseAppLogin( const Mercury::Address& srcAddr,
			const Mercury::UnpackedMessageHeader& header,
			const BaseAppExtInterface::baseAppLoginArgs & args );

	// let the proxy know who we really are
	void authenticate( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const BaseAppExtInterface::authenticateArgs & args );

	/* C++ stuff */

	bool initScript();

	float getLoad() const						{ return load_; }

	Mercury::NetworkInterface & intInterface()	{ return interface_; }
	Mercury::NetworkInterface & extInterface()	{ return extInterface_; }

	static Mercury::UDPChannel & getChannel( const Mercury::Address & addr )
	{
		return BaseApp::instance().intInterface().findOrCreateChannel( addr );
	}

	BaseAppMgrGateway & baseAppMgr()				{ return baseAppMgr_; }
	const BaseAppMgrGateway & baseAppMgr() const	{ return baseAppMgr_; }

	const Mercury::Address & cellAppMgrAddr() const	{ return cellAppMgr_; }

	/**
	 *	This method returns the DBApp alpha channel owner.
	 */
	DBApp & dbApp()						{ return dbAppAlpha_; }

	const DBAppGateway & dbAppGatewayFor( DatabaseID dbID ) const;

	/**
	 *	This method returns the DBApps gateway.
	 */
	DBAppsGateway & dbApps() 			{ return dbApps_; }

	SqliteDatabase* pSqliteDB() const	{ return pSqliteDB_; }
	void commitSecondaryDB();

	BackupHashChain & backupHashChain()			{ return *pBackupHashChain_; }

	void addBase( Base * pNewBase );
	void addProxy( Proxy * pNewProxy );

	void removeBase( Base * pToGo );
	void removeProxy( Proxy * pToGo );

	SessionKey addPendingLogin( Proxy * pProxy, const Mercury::Address & addr );

	void impendingDataPresentLocally( uint32 version );
	bool allImpendingDataSent( int urgency );
	bool allReloadedByClients( int urgency );

	static void reloadResources( void * arg );
	void reloadResources();

	void callShutDownCallback( int stage );

	void setBaseForCall( Base * pBase, bool isExternalCall );
	Base * getBaseForCall( bool okayIfNull = false );
	ProxyPtr getProxyForCall( bool okayIfNull = false );
	ProxyPtr clearProxyForCall();
	ProxyPtr getAndCheckProxyForCall( Mercury::UnpackedMessageHeader & header,
									  const Mercury::Address & srcAddr );

	EntityID lastMissedBaseForCall() const { return lastMissedBaseForCall_; }

	BW::string pickle( ScriptObject pObj ) const;
	ScriptObject unpickle( const BW::string & str ) const;

	const Bases & bases() const		{ return bases_; }
	const Bases & localServiceFragments() const
		{ return localServiceFragments_; }

	shared_ptr< EntityCreator > pEntityCreator() const
									{ return pEntityCreator_; }

	bool isServiceApp() const	{ return isServiceApp_; }

	bool nextTickPending() const;

	WorkerThread & workerThread()	{ return *pWorkerThread_; }

	GlobalBases * pGlobalBases() const	{ return pGlobalBases_; }
	ServicesMap & servicesMap() const	{ return pPyServicesMap_->map(); }

	bool inShutDownPause() const
			{ return (shutDownTime_ != 0) && (this->time() >= shutDownTime_); }

	bool hasStarted() const				{ return waitingFor_ == 0; }
	bool isShuttingDown() const			{ return shutDownTime_ != 0; }

	void startGameTickTimer();
	void ready( int component );
	void registerSecondaryDB();

	INLINE bool shouldMakeNextMessageReliable();

	EntityID getID();
	void putUsedID( EntityID id );

	enum
	{
		READY_BASE_APP_MGR	= 0x1,
	};

	bool isRecentlyDeadCellApp( const Mercury::Address & addr ) const;

	Mercury::Address 
		getExternalAddressFor( const Mercury::Address & intAddr ) const;

	virtual void requestRetirement();
	bool isRetiring() const 			{ return (retireStartTime_ != 0); }

	void addForwardingMapping( EntityID entityID, 
		const Mercury::Address & addr );

	bool forwardBaseMessageIfNecessary( EntityID entityID, 
		const Mercury::Address & srcAddr, 
		const Mercury::UnpackedMessageHeader & header, 
		BinaryIStream & data );

	void forwardedBaseMessage( BinaryIStream & data );

	bool backupBaseNow( Base & base, 
						Mercury::ReplyMessageHandler * pHandler = NULL );
	void sendAckOffloadBackup( EntityID entityID, 
							   const Mercury::Address & dstAddr );

	bool entityWasOffloaded( EntityID entityID ) const;

	void acceptClient( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data );

	virtual const char * getAppName() const
	{
		return isServiceApp_ ? "ServiceApp" : "BaseApp";
	}

	// Override from Mercury::ChannelListener
	virtual void onChannelSend( Mercury::Channel & channel );
	virtual void onChannelGone( Mercury::Channel & channel );

private:
	friend class AddToBaseAppMgrHelper;

	// From ServerApp
	virtual bool init( int argc, char *argv[] );
	virtual void onSignalled( int sigNum );
	virtual void onStartOfTick();

	bool startServiceFragments();

	bool finishInit( const BaseAppInitData & initData, BinaryIStream & data );

	bool initSecondaryDB();

	bool findOtherProcesses();

	int serveInterfaces();
	void addWatchers();
	void backup();
	void archive();

	// Override from EntityApp
	ManagerAppGateway * pManagerAppGateway() /* override */
	{
		return &baseAppMgr_;
	}

	// Override from TimerHandler
	virtual void handleTimeout( TimerHandle handle, void * arg );
	void tickGameTime();

	void checkSendWindowOverflows();

	void tickRateLimitFilters();
	void sendIdleProxyChannels();
	void tickProfilers( uint64 lastTickInStamps );

	// Data members

	Mercury::NetworkInterface		extInterface_;
	InitialConnectionFilterPtr		pExternalInterfaceFilter_;
	// Must be before dbAppAlpha_ as dbAppAlpha_ destructor can cancel pending
	// requests and call back to EntityCreator's idClient_.
	shared_ptr< EntityCreator >     pEntityCreator_;

	BaseAppMgrGateway				baseAppMgr_;
	Mercury::Address				cellAppMgr_;

	DBAppsGateway					dbApps_;
	DBApp							dbAppAlpha_;

	SqliteDatabase *				pSqliteDB_;

	BWTracerHolder					bwtracer_;

	typedef BW::map< Mercury::Address, Proxy * > Proxies;
	Proxies 						proxies_;

	Bases 							bases_;
	Bases							localServiceFragments_;

	BaseAppID						id_;

	BasePtr							baseForCall_;
	EntityID						lastMissedBaseForCall_;
	EntityID						forwardingEntityIDForCall_;
	bool							baseForCallIsProxy_;
	bool							isExternalCall_;
	bool							isServiceApp_;

	bool							shouldMakeNextMessageReliable_;

	GameTime						shutDownTime_;
	Mercury::ReplyID				shutDownReplyID_;
	uint64							retireStartTime_;
	uint64							lastRetirementOffloadTime_;

	TimeKeeper *					pTimeKeeper_;
	TimerHandle						gameTimer_;

	float							timeoutPeriod_;

	enum TimeOutType
	{
		TIMEOUT_GAME_TICK,
	};

	WorkerThread * 					pWorkerThread_;

	GlobalBases *					pGlobalBases_;
	PyServicesMap *					pPyServicesMap_;

	// Misc
	Pickler *						pPickler_;

	bool							isBootstrap_;
	bool							didAutoLoadEntitiesFromDB_;
	int								waitingFor_;

	float							load_;

	shared_ptr< LoginHandler >      pLoginHandler_;
	shared_ptr< BackedUpBaseApps >  pBackedUpBaseApps_;
	shared_ptr< DeadCellApps >      pDeadCellApps_;
	shared_ptr< BackupSender >      pBackupSender_;
	shared_ptr< Archiver >          pArchiver_;
	shared_ptr< SharedDataManager > pSharedDataManager_;
	shared_ptr< BaseMessageForwarder > pBaseMessageForwarder_;
	shared_ptr< BackupHashChain > 	pBackupHashChain_;

	// This is just like BackedUpBaseApps, but it stores backups for entities
	// that we have offloaded.
	shared_ptr< OffloadedBackups >  pOffloadedBackups_;

	shared_ptr< ServiceStarter >	pServiceStarter_;

	typedef BW::map< Mercury::Address, Mercury::Address > BaseAppExtAddresses;
	BaseAppExtAddresses 			baseAppExtAddresses_;

	std::auto_ptr< Mercury::StreamFilterFactory > 
									pStreamFilterFactory_;
	Mercury::TCPServer				tcpServer_;
};


#ifdef CODE_INLINE
#include "baseapp.ipp"
#endif

BW_END_NAMESPACE

#endif // BASEAPP_HPP
