#include "script/first_include.hpp"

#include "baseapp.hpp"

#include "add_to_baseappmgr_helper.hpp"
#include "archiver.hpp"
#include "backed_up_base_apps.hpp"
#include "backup_sender.hpp"
#include "base_message_forwarder.hpp"
#include "baseapp_config.hpp"
#include "initial_connection_filter.hpp"
#include "baseapp_int_interface.hpp"
#include "client_stream_filter_factory.hpp"
#include "controlled_shutdown_handler.hpp"
#include "dead_cell_apps.hpp"
#include "download_streamer.hpp"
#include "entity_channel_finder.hpp"
#include "entity_creator.hpp"
#include "global_bases.hpp"
#include "loading_thread.hpp"
#include "login_handler.hpp"
#include "message_handlers.hpp"
#include "offloaded_backups.hpp"
#include "ping_manager.hpp"
#include "proxy.hpp"
#include "replay_data_file_writer.hpp"
#include "script_bigworld.hpp"
#include "service_starter.hpp"
#include "shared_data_manager.hpp"
#include "sqlite_database.hpp"
#include "worker_thread.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"

#include "cellappmgr/cellappmgr_interface.hpp"

#include "db/db_config.hpp"
#include "db/dbapp_interface.hpp"
#include "db/dbapp_interface_utils.hpp"

#include "common/py_network.hpp"

#include "chunk/user_data_object.hpp"

#include "connection/baseapp_ext_interface.hpp"

#include "cstdmf/bw_set.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/timestamp.hpp"

#include "delegate_interface/game_delegate.hpp"

#include "entitydef/constants.hpp"

#include "network/blocking_reply_handler.hpp"
#include "network/bundle_sending_map.hpp"
#include "network/channel_sender.hpp"
#include "network/interface_table.hpp"
#include "network/logger_message_forwarder.hpp"
#include "network/machined_utils.hpp"
#include "network/portmap.hpp"
#include "network/udp_bundle.hpp"
#include "network/watcher_nub.hpp"

#include "pyscript/pickler.hpp"
#include "pyscript/py_traceback.hpp"
#include "pyscript/script.hpp"
#include "pyscript/personality.hpp"
#include "pyscript/pywatcher.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/file_system.hpp"
#include "resmgr/multi_file_system.hpp"

#include "server/auto_backup_and_archive.hpp"
#include "server/backup_hash_chain.hpp"
#include "server/bwconfig.hpp"
#include "server/entity_profiler.hpp"
#include "server/plugin_library.hpp"
#include "server/time_keeper.hpp"
#include "server/util.hpp"
#include "server/writedb.hpp"

#ifdef BUILD_PY_URLREQUEST
#include "urlrequest/manager.hpp"
#endif

#include <algorithm>

#include <sys/types.h>
#include <sys/wait.h>


DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "baseapp.ipp"
#endif

extern int Math_token;
extern int ResMgr_token;
extern int PyScript_token;
extern int AppScriptTimers_token;
static int s_moduleTokens BW_UNUSED_ATTRIBUTE =
	Math_token | ResMgr_token | PyScript_token;
extern int force_link_UDO_REF;
static int s_tokenSet BW_UNUSED_ATTRIBUTE =
	force_link_UDO_REF | AppScriptTimers_token;


/// BaseApp Singleton.
BW_SINGLETON_STORAGE( BaseApp )

namespace // (anonymous)
{

// This static variable stores the timestamp of the last tick
uint64 s_lastTimestamp = timestamp();


} // end namespace (anonymous)


// -----------------------------------------------------------------------------
// Section: BaseApp main
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
BaseApp::BaseApp( Mercury::EventDispatcher & mainDispatcher,
	   Mercury::NetworkInterface & interface, bool isServiceApp ) :
	EntityApp( mainDispatcher, interface ),
	TimerHandler(),
	Mercury::ChannelListener(),
	Singleton< BaseApp >(),
	extInterface_( &mainDispatcher,
			Mercury::NETWORK_INTERFACE_EXTERNAL,
			htons( BWConfig::get( "baseApp/externalPorts/port",
				BWConfig::get( "baseApp/externalPort", 0 ) ) ),
			Config::externalInterface().c_str() ),
	pExternalInterfaceFilter_( InitialConnectionFilter::create() ),
	pEntityCreator_(),
	baseAppMgr_( interface ),
	cellAppMgr_(),
	dbApps_(),
	dbAppAlpha_( interface ),
	pSqliteDB_( NULL ),
	bwtracer_(),
	proxies_(),
	bases_(),
	id_( 0 ),
	baseForCall_( NULL ),
	lastMissedBaseForCall_( 0 ),
	forwardingEntityIDForCall_( 0 ),
	baseForCallIsProxy_( false ),
	isExternalCall_( false ),
	isServiceApp_( isServiceApp ),
	shutDownTime_( 0 ),
	shutDownReplyID_( Mercury::REPLY_ID_NONE ),
	retireStartTime_( 0 ),
	lastRetirementOffloadTime_( 0 ),
	pTimeKeeper_( NULL ),
	gameTimer_(),
	timeoutPeriod_( 0.f ),
	pWorkerThread_( NULL ),
	pGlobalBases_( NULL ),
	pPyServicesMap_( NULL ),
	pPickler_( NULL ),
	isBootstrap_( false ),
	didAutoLoadEntitiesFromDB_( false ),
	waitingFor_( READY_BASE_APP_MGR ),
	load_( 0.f ),
	pLoginHandler_( new LoginHandler ),
	pBackedUpBaseApps_( new BackedUpBaseApps ),
	pDeadCellApps_( new DeadCellApps ),
	pBackupSender_(),
	pArchiver_(),
	pSharedDataManager_(),
	pBaseMessageForwarder_( new BaseMessageForwarder( interface ) ),
	pBackupHashChain_( new BackupHashChain() ),
	pOffloadedBackups_( new OffloadedBackups() ),
	pServiceStarter_(),
	baseAppExtAddresses_(),
	pStreamFilterFactory_(),
	tcpServer_( extInterface_, Config::tcpServerBacklog() )
{
	srand( (unsigned int)timestamp() );

	extInterface_.pExtensionData( static_cast< ServerApp * >( this ) );

	BW::string extInterfaceConfig = Config::externalInterface();
	DataSectionPtr pPorts = BWConfig::getSection( "baseApp/externalPorts" );

	Ports externalPorts;
	if (pPorts)
	{
		pPorts->readInts( "port", externalPorts );
	}

	bool didBind = this->bindToPrescribedPort( extInterface_, tcpServer_,
			extInterfaceConfig, externalPorts );

	if (!didBind && 
			(externalPorts.empty() || !Config::shouldShutDownIfPortUsed()))
	{
		this->bindToRandomPort( extInterface_, tcpServer_, extInterfaceConfig );
	}

	if (BWConfig::get( "shouldUseWebSockets" ,true ))
	{
		pStreamFilterFactory_.reset( new ClientStreamFilterFactory );
	}

	// Our channel to the BaseAppMgr is irregular until we have started our game
	// tick timer.
	baseAppMgr_.isRegular( false );

	dbAppAlpha_.channel().isLocalRegular( false );
	dbAppAlpha_.channel().isRemoteRegular( false );

	// Register the indexed channel finder for the internal interface
	static EntityChannelFinder channelFinder;
	this->intInterface().registerChannelFinder( &channelFinder );

	pWorkerThread_ = new WorkerThread();

	PROC_IP_INFO_MSG( "Internal address = %s\n",
		this->intInterface().address().c_str() );
	PROC_IP_INFO_MSG( "External address = %s\n",
		extInterface_.address().c_str() );

	extInterface_.pOffChannelFilter( pExternalInterfaceFilter_ );

	// TODO: Move to BaseAppConfig
	if (!Config::warnOnNoDef())
	{
		CONFIG_NOTICE_MSG( "It is recommended to have baseApp/warnOnNoDef set "
				"to true and to make use of <TempProperties> section in the "
				".def files\n" );
	}

	pBackupSender_.reset( new BackupSender( *this ) );
	pArchiver_.reset( new Archiver() );
}


/**
 *	Destructor.
 */
BaseApp::~BaseApp()
{
	INFO_MSG( "BaseApp::~BaseApp: Stopping background threads.\n" );
	bgTaskManager_.stopAll();
	INFO_MSG( "BaseApp::~BaseApp: Stopped background threads.\n" );

	this->extInterface().prepareForShutdown();

	delete pTimeKeeper_;

	bases_.discardAll();

	if (!bases_.empty() || !proxies_.empty() || !localServiceFragments_.empty())
	{
		INFO_MSG( "Bases::~BaseApp: bases = %" PRIzu ". proxies = %" PRIzu ". " \
			"services = %" PRIzu "\n",
			bases_.size(), proxies_.size(), localServiceFragments_.size() );
	}

	gameTimer_.cancel();
	this->intInterface().processUntilChannelsEmpty();

	if (!this->isShuttingDown() && pEntityCreator_)
	{
		// Do not bother returning the ids when shutting down entire server.
		// pEntityCreator_->returnIDs();
		pEntityCreator_.reset();
	}

	// KeepAliveTimerHandlers hold onto a BasePtr, release them
	this->timeQueue().clear();

	baseForCall_ = NULL;	// Clear this prior to Script::fini()
	bases_.clear();
	proxies_.clear();

	if (IGameDelegate::instance())
	{
		IGameDelegate::instance()->shutDown();
	}
	
	delete pWorkerThread_;
	pWorkerThread_ = NULL;

	Py_XDECREF( pGlobalBases_ );
	pGlobalBases_ = NULL;

	Py_XDECREF( pPyServicesMap_ );
	pPyServicesMap_ = NULL;

	pSharedDataManager_.reset();

	delete pPickler_;
	pPickler_ = NULL;

	// May be NULL
	delete pSqliteDB_;
	pSqliteDB_ = NULL;

	EntityType::clearStatics();
	DataType::clearStaticsForReload();

	MetaDataType::fini();

	PingManager::fini();
}



namespace
{
// This set stores the id of the entity channels that are in danger of
// overflowing.
typedef BW::set< Mercury::ChannelID > OverflowIDs;
OverflowIDs s_overflowIDs;

// This callback function is called when an entity channel is in danger of
// overflowing.
void onSendWindowOverflow( const Mercury::UDPChannel & channel )
{
	IF_NOT_MF_ASSERT_DEV( channel.isIndexed() )
	{
		return;
	}

	s_overflowIDs.insert( channel.id() );
}

} // anonymous namespace


/**
 *	This method initialises this object.
 *
 *	@param argc	The number of elements in argv.
 *	@param argv	An array of argument strings.
 */
bool BaseApp::init( int argc, char * argv[] )
{
	if (!this->EntityApp::init( argc, argv ))
	{
		return false;
	}

 	if (!this->intInterface().isGood())
 	{
 		NETWORK_WARNING_MSG( "BaseApp::init: "
 			"Failed to bind to internal interface.\n" );
 		return false;
 	}

 	if (!extInterface_.isGood())
 	{
 		NETWORK_WARNING_MSG( "BaseApp::init: "
 			"Failed to bind to external interface.\n" );
 		return false;
 	}

 	if (!tcpServer_.isGood())
 	{
		ERROR_MSG( "BaseApp::init: Unable to bind TCP server.\n" );
		return false;
	}

	tcpServer_.pStreamFilterFactory( pStreamFilterFactory_.get() );

	if (!Config::isProduction())
	{
		// Display warnings for potentially hazardous non-production settings

		if (Config::shouldResolveMailBoxes())
		{
			CONFIG_WARNING_MSG( "BaseApp::init: "
				"shouldResolveMailBoxes is enabled. "
				"This can lead to writing script that makes bad assumptions "
				"as to locality of mailboxes.\n" );
		}
	}

	if (!PingManager::init( mainDispatcher_, extInterface_ ))
	{
		return false;
	}

	{
		Mercury::UDPBundle b;
		int maxSize = b.pFirstPacket()->freeSpace() - 32;

		int bytesPerPacketToClient = Config::bytesPerPacketToClient();

		if (bytesPerPacketToClient < 0 || bytesPerPacketToClient > maxSize)
		{
			int updateHertz = Config::updateHertz();
			CONFIG_ERROR_MSG( "BaseApp::init: "
					"Bandwidth must be in range %d to %d. Currently %d bps\n",
				ServerUtil::packetSizeToBPS( 0, updateHertz ),
				ServerUtil::packetSizeToBPS( maxSize, updateHertz ),
				ServerUtil::packetSizeToBPS( bytesPerPacketToClient,
					updateHertz ));

			return false;
		}
	}

	if (!this->findOtherProcesses())
	{
		return false;
	}

	// serve our interfaces
	this->serveInterfaces();

	extInterface_.isVerbose( Config::verboseExternalInterface() );

	if (!PluginLibrary::loadAllFromDirRelativeToApp( true, "-extensions" ))
	{
		ERROR_MSG( "BaseApp::init: Failed to load plugins.\n" );
		return false;
	}

	if (IGameDelegate::instance() != NULL) 
	{
		BW::string resPaths;

		for (int i = 0; i < BWResource::getPathNum(); ++i)
		{
			if (i!=0)
			{
				resPaths += ";";
			}
			resPaths += BWResource::getPath( i );
		}

		INFO_MSG( "BaseApp::init: Initializing IGameDelegate with resource paths: "
				" %s\n", resPaths.c_str() );
		
		if (!IGameDelegate::instance()->initialize( resPaths.c_str() ))
		{
			ERROR_MSG( "BaseApp::init: Failed to initialize IGameDelegate.\n" );
			return false;
		}
	}
	
	bgTaskManager_.initWatchers( this->getAppName() );
	bgTaskManager_.startThreads( "BGTask Manager", 1 );

	// start the scripting system
	if (!this->initScript())
	{
		SCRIPT_ERROR_MSG( "BaseApp::init: Script system init failed.\n" );
		return false;
	}

	this->addWatchers();

	bwtracer_.init( extInterface_ );

	extInterface_.setLatency(
			Config::externalLatencyMin(), Config::externalLatencyMax() );

	extInterface_.setLossRatio( Config::externalLossRatio() );

	if (extInterface_.hasArtificialLossOrLatency())
	{
		NETWORK_WARNING_MSG(
				"BaseApp::init: External interface loss/latency enabled\n" );
	}

	extInterface_.maxSocketProcessingTime( 
		Config::maxExternalSocketProcessingTime() );

	DataSectionPtr pWV = BWConfig::getSection( "baseApp/watcherValues" );
	if (pWV)
	{
		pWV->setWatcherValues();
	}

	Mercury::UDPChannel::sendWindowCallbackThreshold(
			Config::sendWindowCallbackThreshold() );

	Mercury::UDPChannel::setSendWindowCallback( onSendWindowOverflow );

	// We need to create the UDO_REF base type in the base app so entities with
	// links are created properly on the baseapp and the cellapp.
	if (!UserDataObject::createRefType())
	{
		return false;
	}

	ProfileVal::setWarningPeriod( stampsPerSecond() / Config::updateHertz() );

	if (isServiceApp_)
	{
		pServiceStarter_.reset( new ServiceStarter() );

		if (!pServiceStarter_->init())
		{
			return false;
		}
	}

	// Add ourselves to the BaseAppMgr.  Init will continue once the reply to
	// this message is received.  This object deletes itself.
	new AddToBaseAppMgrHelper( *this );

	return true;
}


/**
 *	This method starts the appropriate services on this ServiceApp.
 */
bool BaseApp::startServiceFragments()
{
	if (pServiceStarter_)
	{
		if (!pServiceStarter_->run( *pEntityCreator_ ))
		{
			return false;
		}

		pServiceStarter_.reset();
	}

	return true;
}


/**
 *	This method finds the other processes that this BaseApp needs to know about.
 */
bool BaseApp::findOtherProcesses()
{
	const int numStartupRetries = Config::numStartupRetries();

	INFO_MSG( "Finding other processes. Max retries = %d\n",
			numStartupRetries );

	Mercury::Address baseAppMgrAddr;

	if (!baseAppMgr_.init( "BaseAppMgrInterface", numStartupRetries,
			Config::maxMgrRegisterStagger() ))
	{
		NETWORK_ERROR_MSG( "BaseApp::findOtherProcesses: "
			"could not find BaseAppMgr\n" );
		return false;
	}

	if (Mercury::MachineDaemon::findInterface( "CellAppMgrInterface", 0,
			cellAppMgr_, numStartupRetries ) != Mercury::REASON_SUCCESS)
	{
		NETWORK_ERROR_MSG( "BaseApp::findOtherProcesses: "
			"could not find CellAppMgr\n" );
		return false;
	}

	return true;
}


/**
 *	This class is used to get the initialisation data required to create the
 *	secondary database. The primary database is queried for this information in
 *	a blocking manner.
 */
class SecondaryDBIniter : public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	SecondaryDBIniter() :
		filename_(),
		isOkay_( true )
	{
	}

	/**
	 *	This method queries DBApp for secondary db details using a blocking
	 *	request.
	 */
	bool init( Mercury::NetworkInterface & interface,
			Mercury::UDPChannel & channel )
	{
		Mercury::BlockingReplyHandler handler( interface, this );
		Mercury::Bundle & bundle = channel.bundle();

		bundle.startRequest( DBAppInterface::getSecondaryDBDetails, &handler );
		channel.send();

		return (handler.waitForReply( &channel ) == Mercury::REASON_SUCCESS) &&
			isOkay_;
	}

	const BW::string & filename() const	{ return filename_; }

private:
	void handleMessage( const Mercury::Address & src,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data, void * arg )
	{
		data >> filename_;
	}

	void handleException( const Mercury::NubException & exception, void * arg )
	{
		SECONDARYDB_ERROR_MSG( "SecondaryDBIniter::handleException: %s\n",
			   Mercury::reasonToString( exception.reason() ) );
		isOkay_ = false;
	}

	BW::string filename_;
	bool isOkay_;
};


/**
 *	This method initialises the secondary database.
 */
bool BaseApp::initSecondaryDB()
{
	const DBConfig::Config & dbConfig = DBConfig::get();

	if (!dbConfig.secondaryDB.enable())
	{
		return true;
	}

	if (!pArchiver_->isEnabled())
	{
		SECONDARYDB_WARNING_MSG( "BaseApp::initSecondaryDB: "
				"Disabling secondary database as it does not function "
				"correctly when archiving is disabled\n" );
		return true;
	}

	SecondaryDBIniter initer;
	if (!initer.init( this->intInterface(), this->dbApp().channel() ))
	{
		SECONDARYDB_WARNING_MSG( "BaseApp::initSecondaryDB: "
				"Failed to retrieve initialisation data\n" );
		return false;
	}

	if (initer.filename().empty())
	{
		SECONDARYDB_INFO_MSG( "BaseApp::initSecondaryDB: "
				"Disabling secondary database as requested by DBApp\n" );
		return true;
	}

	BW::string dbDir = dbConfig.secondaryDB.directory();

	if (BWResource::resolveToAbsolutePath( dbDir ) !=
			IFileSystem::FT_DIRECTORY)
	{
		SECONDARYDB_ERROR_MSG( "BaseApp::initSecondaryDB: "
				"Configuration setting <%s> "
				"specifies a non-existent directory: %s. Please create "
				"this directory.\n",
			dbConfig.secondaryDB.directory.path().c_str(),
			dbDir.c_str() );
		return false;
	}

	MD5::Digest digest( EntityType::s_persistentPropertiesMD5_ );

	// TODO: Split this out into an init method that can be used to get
	// a result.
	pSqliteDB_ = new SqliteDatabase( initer.filename(), dbDir );

	if (!pSqliteDB_->init( digest.quote() ))
	{
		delete pSqliteDB_;
		pSqliteDB_ = NULL;
		return false;
	}

	// Only send our DBAppInterface::secondaryDBRegistration after
	// DBApp is ready. If we send it now, our database risks
	// getting consolidated and deleted.

	return true;
}


/**
 *  This method does the portion of the init after this app has been
 *  successfully added to the BaseAppMgr.
 */
bool BaseApp::finishInit( const BaseAppInitData & initData,	
		BinaryIStream & stream )
{
	// Make sure that nothing is read in the main thread.
	BWResource::watchAccessFromCallingThread( true );

	id_ = initData.id;

	LoggerMessageForwarder::pInstance()->registerAppID( id_ );

	this->updateDBAppHash( stream );

	if (!this->initSecondaryDB())
	{
		SECONDARYDB_ERROR_MSG( "BaseApp::updateDBAppHash: "
			"Failed to init secondary database\n" );

		return false;
	}

	pEntityCreator_ = EntityCreator::create( this->dbApp(),
		this->intInterface() );

	if (!pEntityCreator_)
	{
		ERROR_MSG( "BaseApp::updateDBAppHash: "
				"Failed to create EntityCreator\n" );
		return false;
	}

	if (isServiceApp_)
	{
		BaseAppIntInterface::registerWithMachinedAs( "ServiceAppInterface",
				this->intInterface(), id_ );
	}
	else
	{
		BaseAppIntInterface::registerWithMachined( this->intInterface(), id_ );
	}

	baseAppMgr_.finishedInit();

	timeoutPeriod_ = initData.timeoutPeriod;

	if (pServiceStarter_)
	{
		int32 layoutId = (int32)id_ - 1;
		if (!pServiceStarter_->finishInit( layoutId ))
		{
			return false;
		}
	}

	bool isBaseAppMgrReady = initData.isReady;

	// Note that time_ may be later overridden by a startup message
	// (if the cellAppMgr was not ready when we registered)
	this->setStartTime( initData.time );

	if (isBaseAppMgrReady)
	{
		this->startGameTickTimer();
	}

	if (!this->startServiceFragments())
	{
		return false;
	}

	// Register our birth listeners
	Mercury::MachineDaemon::registerBirthListener(
			this->intInterface().address(),
			BaseAppIntInterface::handleBaseAppMgrBirth, "BaseAppMgrInterface" );
	Mercury::MachineDaemon::registerBirthListener(
			this->intInterface().address(),
			BaseAppIntInterface::handleCellAppMgrBirth, "CellAppMgrInterface" );

	// set up the watcher stuff
	const char * abrvFmt = "baseapp%02d";
	int defaultPythonPort = PORT_PYTHON_BASEAPP + id_;

	if (isServiceApp_)
	{
		abrvFmt = "serviceapp%02d";
		defaultPythonPort = PORT_PYTHON_SERVICEAPP + id_;
	}

	char	abrv[32];
	bw_snprintf( abrv, sizeof(abrv), abrvFmt, (int)id_ );

	BW_REGISTER_WATCHER( id_, abrv, "baseApp", mainDispatcher_,
			this->intInterface().address() );

	int pythonPort = BWConfig::get( "baseApp/pythonPort", defaultPythonPort );
	this->startPythonServer( pythonPort, id_ );

	if (isBaseAppMgrReady)
	{
		this->ready( READY_BASE_APP_MGR );
		this->registerSecondaryDB();
	}

	return true;
}


BW_END_NAMESPACE
#include "cstdmf/profile.hpp"
BW_BEGIN_NAMESPACE

/**
 *
 */
uint64 runningTime()
{
	return ProfileGroup::defaultGroup().runningTime();
}


/**
 *	This class converts Mercury proxy addresses to their string
 *	representations, including the protocol used (TCP/UDP).
 */
class ProxyMapKeyStringConverter : 
		public IMapKeyStringConverter< Mercury::Address >
{
public:
	typedef Mercury::Address Key;

	ProxyMapKeyStringConverter()
	{}

	virtual ~ProxyMapKeyStringConverter() {}

	virtual bool stringToValue( const BW::string & keyString, Key & key )
	{
		if (!watcherStringToValue( keyString.c_str() + 4, key ))
		{
			return false;
		}

		key.salt = (strncmp( keyString.c_str(), "tcp:", 4 ) == 0) ? 1 : 0;

		return true;
	}


	virtual const BW::string valueToString( const Key & address )
	{
		BW::string out = address.salt ? "tcp:" : "udp:";
		return out += address.c_str();
	}

private:
};


/**
 *	This method adds the watchers that are associated with this object.
 */
void BaseApp::addWatchers()
{
	EntityMemberStats::limitForBaseApp();

	Watcher * pRoot = &Watcher::rootWatcher();

	// num clients
	MF_WATCH( "numBases", bases_, &Bases::size ); // Dup'ed in stats/
	if (isServiceApp_)
	{
		MF_WATCH( "numServices", localServiceFragments_,
				&Bases::size ); // Dup'ed in stats/
	}
	MF_WATCH( "numProxies", proxies_, &Proxies::size ); // Dup'ed in stats/

	MF_WATCH( "timeoutPeriod", timeoutPeriod_, Watcher::WT_READ_ONLY );


	Watcher * watchBases = new MapWatcher<Bases>( bases_ );
	watchBases->addChild( "*", new BaseDereferenceWatcher(
		Base::pWatcher() ) );

	pRoot->addChild( "entities", watchBases );

	// proxies

	Watcher * watchProxies = new MapWatcher<Proxies>( proxies_,
		new ProxyMapKeyStringConverter() );
	watchProxies->addChild( "*", new BaseDereferenceWatcher(
		Proxy::pWatcher() ) );

	pRoot->addChild( "proxies", watchProxies );
	// types
	pRoot->addChild( "entityTypes", EntityType::pWatcher() );

	MF_WATCH( "load", load_ ); // Dup'ed in stats/

	MF_WATCH( "stats/stampsPerSecond", &stampsPerSecond );
	MF_WATCH( "stats/runningTime", &runningTime );
	MF_WATCH( "stats/load", load_ );
	MF_WATCH( "stats/numBases", bases_, &Bases::size,
				"The number of base entities." );
	MF_WATCH( "stats/numEntities", bases_, &Bases::size,
				"The number of base entities." );
	if (isServiceApp_)
	{
		MF_WATCH( "stats/numServices", localServiceFragments_, &Bases::size );
	}
	MF_WATCH( "stats/numProxies", proxies_, &Proxies::size );

	MF_WATCH( "isServiceApp", isServiceApp_ , Watcher::WT_READ_ONLY );

	// id
	MF_WATCH( "id", id_ );

	// misc
	pRoot->addChild( "nubExternal",
						Mercury::NetworkInterface::pWatcher(), &extInterface_ );

	baseAppMgr_.addWatchers( "baseAppMgr", *pRoot );
	pRoot->addChild( "dbAppAlpha", Mercury::ChannelOwner::pWatcher(),
		&dbAppAlpha_ );

	pRoot->addChild( "logins",
			new BaseDereferenceWatcher( LoginHandler::pWatcher() ),
			&pLoginHandler_ );

	this->EntityApp::addWatchers( *pRoot );

	pRoot->addChild( "backedUpBaseApps",
			new BaseDereferenceWatcher( BackedUpBaseApps::pWatcher() ),
			&pBackedUpBaseApps_ );

	pRoot->addChild( "dbApps", DBAppsGateway::pWatcher(), &dbApps_ );

	pRoot->addChild( "config/secondaryDB",
		DBConfig::get().secondaryDB.pWatcher(),
		(void*)(&DBConfig::get().secondaryDB) );
}


/**
 *	This method returns the DBApp gateway for the given DBID.
 */
const DBAppGateway & BaseApp::dbAppGatewayFor( DatabaseID dbID ) const
{
	return dbApps_[ dbID ];
}


/**
 * This method raises the flag to commit to secondary DB on the
 * first occasion
 */
void BaseApp::commitSecondaryDB()
{
	pArchiver_->commitSecondaryDB();
}


/**
 *	This method adds a base from this manager.
 */
void BaseApp::addBase( Base * pNewBase )
{
	if (pNewBase->isServiceFragment())
	{
		baseAppMgr_.registerServiceFragment( pNewBase );
		localServiceFragments_.add( pNewBase );
	}
	else
	{
		bases_.add( pNewBase );
	}
}


/**
 *	This method removes a base from this manager.
 */
void BaseApp::removeBase( Base * pToGo )
{
	// TRACE_MSG( "BaseApp: Removing base id: %d\n", pToGo->id() );

	if (pToGo->isServiceFragment())
	{
		baseAppMgr_.deregisterServiceFragment(
				pToGo->pType()->description().name() );
		localServiceFragments_.erase( pToGo->id() );
	}
	else
	{
		bases_.erase( pToGo->id() );
	}

	if (baseForCall_ == pToGo)
	{
		baseForCall_ = NULL;
	}
}


/**
 *	This method adds a proxy from this manager.
 */
void BaseApp::addProxy( Proxy * pNewProxy )
{
	Mercury::ChannelPtr pChannel = pNewProxy->pClientChannel();
	Mercury::Address address = pChannel->addr();
	address.salt = (pChannel->isTCP() ? 1 : 0);

	TRACE_MSG( "BaseApp: Adding proxy %u at %s\n",
		pNewProxy->id(), pChannel->c_str() );
	// set ourselves in the map from ip address to proxy

	Proxy *& rpProxy = proxies_[address];
	if (rpProxy == NULL)
	{
		rpProxy = pNewProxy;
	}
	else if (pNewProxy == rpProxy)
	{
		ERROR_MSG( "BaseApp::addProxy: Adding %u at %s when already exists\n",
				pNewProxy->id(), pChannel->c_str() );
	}

	// If we aren't handling multiplexed bot channels, then we have to assume
	// that the client is trying to reconnect over the same channel, so we
	// kill off the original proxy and map in the new one
	else
	{
		WARNING_MSG( "Old proxy at %s disconnected in favour of new one\n",
					 pChannel->c_str() );
		rpProxy->onClientDeath( CLIENT_DISCONNECT_GIVEN_TO_OTHER_PROXY );

		// We can't use rpProxy again here because onClientDeath() will
		// typically delete the mapping from proxies_ and thus invalidate
		// the reference.

		proxies_[address] = pNewProxy;
	}

	pChannel->pChannelListener( this );
}


/**
 *	This method removes a proxy from this manager.
 */
void BaseApp::removeProxy( Proxy * pToGo )
{
	TRACE_MSG( "BaseApp::removeProxy: Removing proxy id %u addr %s\n",
		pToGo->id(), pToGo->c_str() );

	// find it and remove it from the list
	Proxies::iterator found = proxies_.find( pToGo->clientAddr() );

	if (found != proxies_.end())
	{
		proxies_.erase( found );

		// Clear the proxy for call if it was this one, so later messages in the
		// current bundle cannot get to it.
		if (isExternalCall_ &&
				this->getProxyForCall( true /*okIfNull*/ ) == pToGo)
		{
			this->clearProxyForCall();
		}
	}
	else
	{
		ERROR_MSG( "BaseApp::removeProxy: "
				"Could not find proxy id %d addr %s\n",
			pToGo->id(), pToGo->c_str() );
	}
}


/**
 *	This method adds a proxy to the pending logins list.
 */
SessionKey BaseApp::addPendingLogin( Proxy * pProxy,
	const Mercury::Address & addr )
{
	return pLoginHandler_->add( pProxy, addr );
}


// -----------------------------------------------------------------------------
// Section: Base saving for callbacks
// -----------------------------------------------------------------------------


/**
 *	Set the current base or proxy for call.
 *
 *	@param pBase			The base or proxy to set for subsequent calls.
 *	@param isExternalCall 	Whether this is for client-initiated calls or not. 
 */
void BaseApp::setBaseForCall( Base * pBase, bool isExternalCall )
{
	MF_ASSERT( pBase );

	baseForCall_ = pBase;
	forwardingEntityIDForCall_ = 0;
	baseForCallIsProxy_ = pBase && pBase->isProxy();
	isExternalCall_ = isExternalCall;
}


/**
 *	Get the current base.
 *
 *	@param okayIfNull 	If false, then an error message will be output if no
 *						base for call is set, otherwise, no error message will
 *						be output.
 *
 *	@return 			The base for call if it is set, NULL otherwise.
 */
Base * BaseApp::getBaseForCall( bool okayIfNull )
{
	if (!baseForCall_ && !okayIfNull)
	{
		ERROR_MSG( "BaseApp::getBaseForCall: "
						"No base set. lastMissedBaseForCall = %d\n",
					lastMissedBaseForCall_ );
	}
	return baseForCall_.get();
}


/**
 *	Get the current proxy for call.
 *
 *	@param okayIfNull 	If false, then an error message will be output if no
 *						base for call is set, or the base is not a proxy,
 *						otherwise, no error message will be output. 
 *
 *	@return 	The current proxy for call if it is set, otherwise NULL is
 *				returned. If a current base for call is set but not a proxy,
 *				NULL is returned. 
 */
ProxyPtr BaseApp::getProxyForCall( bool okayIfNull )
{
	if (!baseForCall_)
	{
		if (!okayIfNull)
		{
			ERROR_MSG( "BaseApp::getProxyForCall: No proxy set!\n" );
		}

		return NULL;
	}
	else if (!baseForCallIsProxy_)
	{
		if (!okayIfNull)
		{
			ERROR_MSG( "BaseApp::getProxyForCall: Base id %u is not a proxy!\n",
				baseForCall_->id() );
		}

		return NULL;
	}

	return (Proxy*)baseForCall_.get();
}


/**
 *	Clears the current proxy for call and returns it.
 *
 *	@return 	The previous current proxy for call, or NULL if none existed or
 *				a non-proxy base was set for call.
 */
ProxyPtr BaseApp::clearProxyForCall()
{
	ProxyPtr pRet =
		baseForCallIsProxy_ ? (Proxy*)baseForCall_.get() : NULL;
	baseForCall_ = NULL;
	baseForCallIsProxy_ = false;
	lastMissedBaseForCall_ = 0;

	if (forwardingEntityIDForCall_ != 0)
	{
		pBaseMessageForwarder_->
			getForwardingChannel( forwardingEntityIDForCall_ )->send();
	}
	forwardingEntityIDForCall_ = 0;
	return pRet;
}


// -----------------------------------------------------------------------------
// Section: Misc
// -----------------------------------------------------------------------------

/**
 *	This helper method pickles the input object.
 */
BW::string BaseApp::pickle( ScriptObject pObj ) const
{
	if (pPickler_ != NULL)
	{
		return pPickler_->pickle( pObj );
	}

	SCRIPT_CRITICAL_MSG( "BaseApp::pickle: pPickler_ is NULL\n" );
	return "";
}


/**
 *	This helper method unpickles an object from the input string.
 */
ScriptObject BaseApp::unpickle( const BW::string & str ) const
{
	if (pPickler_ != NULL)
	{
		return pPickler_->unpickle( str );
	}

	SCRIPT_CRITICAL_MSG( "BaseApp::unpickle: pPickler_ is NULL\n" );
	return ScriptObject();
}


/**
 * 	This method initialises the scripting environment and sets up the
 * 	the special additions of the Base App.
 */
bool BaseApp::initScript()
{
	this->scriptEvents().createEventType( "onAppReady" );
	this->scriptEvents().createEventType( "onAppShutDown" );
	this->scriptEvents().createEventType( "onBaseAppData" );
	this->scriptEvents().createEventType( "onBaseAppDeath" );
	this->scriptEvents().createEventType( "onCellAppDeath" );
	this->scriptEvents().createEventType( "onServiceAppDeath" );
	this->scriptEvents().createEventType( "onDelBaseAppData" );
	this->scriptEvents().createEventType( "onDelGlobalData" );
	this->scriptEvents().createEventType( "onGlobalData" );
	this->scriptEvents().createEventType( "onAppShuttingDown" );

	// These are deprecated. The generic versions (e.g. onAppReady) should be
	// used. The reason to only encourage the generic callback (and use
	// BigWorld.isServiceApp) is to encourage common code between these
	// processes and making the ordering explicit.
	if (isServiceApp_)
	{
		// These are deliberately not documented as they were never used.
		this->scriptEvents().createEventType( "onServiceAppReady" );
		this->scriptEvents().createEventType( "onServiceAppShuttingDown" );
		this->scriptEvents().createEventType( "onServiceAppShutDown" );
	}
	else
	{
		this->scriptEvents().createEventType( "onBaseAppReady" );
		this->scriptEvents().createEventType( "onBaseAppShuttingDown" );
		this->scriptEvents().createEventType( "onBaseAppShutDown" );
	}

	if (isServiceApp_)
	{
		if (!this->ScriptApp::initScript( "service",
			EntityDef::Constants::entitiesServicePath(),
			EntityDef::Constants::entitiesBasePath()))
		{
			return false;
		}
	}
	else
	{
		if (!this->ScriptApp::initScript( "base",
			EntityDef::Constants::entitiesBasePath()))
		{
			return false;
		}
	}

	if (!PyNetwork::init( this->mainDispatcher(), interface_ )
#ifdef BUILD_PY_URLREQUEST
			|| !URL::Manager::init( this->mainDispatcher() )
#endif
	   )
	{
		return false;
	}

	Script::initExceptionHook( &mainDispatcher_ );

	PyObject * pModule = PyImport_AddModule( "BigWorld" );

	if (PyObject_SetAttrString( pModule, "isServiceApp",
				isServiceApp_ ? Py_True : Py_False ) == -1)
	{
		return false;
	}

	if (!BigWorldBaseAppScript::init( bases_,
				isServiceApp_ ? &localServiceFragments_ : NULL,
				mainDispatcher_, this->intInterface(), isServiceApp_ ))
	{
		return false;
	}

	ScriptObject func = Personality::instance().getAttribute(
		"onShuttingDown", ScriptErrorClear() );
	
	if (func)
	{
		WARNING_MSG( "BaseApp::initScript: onShuttingDown is deprecated. "
				"Use onAppShuttingDown instead.\n" );

		this->scriptEvents().addEventListener( "onAppShuttingDown",
				func.get(), 0 );
	}

	pPickler_ = new Pickler;

	pGlobalBases_ = new GlobalBases;

	if (PyObject_SetAttrString( pModule, "globalBases", pGlobalBases_ ) == -1)
	{
		return false;
	}

	pPyServicesMap_ = new PyServicesMap;

	if (PyObject_SetAttrString( pModule, "services", pPyServicesMap_ ) == -1)
	{
		return false;
	}

	AutoBackupAndArchive::addNextOnlyConstant( pModule );

	pSharedDataManager_ = SharedDataManager::create( pPickler_ );

	return pSharedDataManager_ != NULL;
}


/**
 *	This method handles the Mercury updateDBAppHash message.
 */
void BaseApp::updateDBAppHash( BinaryIStream & data )
{
	if (!dbApps_.updateFromStream( data ))
	{
		CRITICAL_MSG( "BaseApp::updateDBAppHash: "
				"Failed to de-stream DBApp hash\n" );
		return;
	}

	if (dbApps_.alpha().address() != dbAppAlpha_.addr())
	{
		// Switch to the new DBApp Alpha.
		dbAppAlpha_.addr( dbApps_.alpha().address() );

		DEBUG_MSG( "BaseApp::updateDBAppHash: new DBApp Alpha: %s\n",
			dbAppAlpha_.addr().c_str() );
	}
}


/*~ function BigWorld.isNextTickPending
 *	@components{ base }
 *	The function isNextTickPending returns whether or not the current tick
 *	has completed. This allows your script code to determine whether it should
 *	relinquish control of expensive computations to allow the next tick to
 *	execute.
 *
 *	This can be controlled by the baseApp/reservedTickFraction setting in bw.xml.
 *
 *	@return isNextTickPending returns True if the next tick is pending, False
 *	otherwise.
 */
bool isNextTickPending()
{
	return BaseApp::instance().nextTickPending();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, isNextTickPending, END, BigWorld )


/**
 *	This method returns the percentage of time in this quantum that
 *	has passed. Budget-conscious scripts can use this to schedule
 *	themselves intelligently.
 */
int quantumPassedPercent( uint64 currentTimestamp = timestamp() )
{
	uint64 delta = (currentTimestamp - s_lastTimestamp) * uint64(1000);
	delta /= stampsPerSecond();	// careful - don't overflow the uint64
	int msBetweenTicks = int( delta );

	static int msExpected = 1000 / BaseAppConfig::updateHertz();
	return msBetweenTicks * 100 / msExpected;
}

/*~ function BigWorld.quantumPassedPercent
 *	@components{ base }
 *	Note: This method has been deprecated, use BigWorld.isNextTickPending
 *	instead.
 *
 *	The function quantumPassedPercent returns the percentage of execution time
 *	that the application is through the current tick. The returned value is an
 *	integer from 0 upwards, and could conceivably be larger than 100 if the
 *	current tick execution time is longer than the expected interval between
 *	ticks.
 *	@return quantumPassedPercent returns the percentage of execution time that
 *	the application is through the current tick as an integer, truncated to the
 *	nearest percent.
 */
PY_AUTO_MODULE_FUNCTION( RETDATA, quantumPassedPercent, END, BigWorld )


/**
 *	This method returns whether or not the next tick is pending. If the previous
 *	tick was over budget, then the next tick is considered pending. This is to
 *	break timer-handler loops where no network input is processed.
 */
bool BaseApp::nextTickPending() const
{
	return gameTimer_.isSet() &&
		((timestamp() + Config::reservedTickTime()) >
					mainDispatcher_.timerDeliveryTime( gameTimer_ ));
}


/**
 *	This method checks whether or not the current tick has run late. This
 *	includes time taken to respond to network (and timer) events called outside
 *	the timeQueue's process dispatcher.
 *
 *	@return The time of the last tick in stamps.
 */
static uint64 checkTickNotLate()
{
	uint64 currentTimestamp = timestamp();

	int percent = quantumPassedPercent( currentTimestamp );

	if (percent > 200)
	{
		WARNING_MSG( "BaseApp::handleTimeout: "
				"Last tick quantum took %d%% of allocation (%.2f seconds)! "
				"Proxy clients may have been starved!\n",
			percent,
			float( percent )/100.f/float( BaseAppConfig::updateHertz() ) );
	}

	uint64 elapsedTime = currentTimestamp - s_lastTimestamp;
	s_lastTimestamp = currentTimestamp;
	return elapsedTime;
}


/**
 *	This method handles timeout events.
 */
void BaseApp::handleTimeout( TimerHandle /*handle*/, void * arg )
{
	uintptr timerType = reinterpret_cast<uintptr>( arg );

	// TODO: Should investigate whether all this can be done in tickGameTime()
	// instead, since we only seem to start a timer with TIMEOUT_GAME_TICK.

	// Secondary database is used even during shutdown
	if (pSqliteDB_ && (timerType == TIMEOUT_GAME_TICK))
	{
		pArchiver_->tickSecondaryDB( pSqliteDB_ );
	}

	bgTaskManager_.tick();

	if (this->inShutDownPause())
	{
		if (shutDownReplyID_ != Mercury::REPLY_ID_NONE)
		{
			baseAppMgr_.bundle().startReply( shutDownReplyID_ );
			baseAppMgr_.send();

			// No longer regularly sending the load from now on.
			baseAppMgr_.isRegular( false );

			shutDownReplyID_ = Mercury::REPLY_ID_NONE;
		}

		return;
	}

	switch (timerType)
	{
		case TIMEOUT_GAME_TICK:
			this->tickGameTime();
			break;
	}
}


/**
 *	This method ticks the game time.
 */
void BaseApp::tickGameTime()
{
	AUTO_SCOPED_PROFILE( "tickGameTime" )

	// make sure we did not run late
	uint64 lastTickInStamps = checkTickNotLate();
	double spareTimeFraction = 1.0;
	double lastTickPeriod = stampsToSeconds( lastTickInStamps );

	if (lastTickInStamps != 0)
	{
		spareTimeFraction =
			double(mainDispatcher_.getSpareTime())/double(lastTickInStamps);
	}
	mainDispatcher_.clearSpareTime();

	this->tickProfilers( lastTickInStamps );

	if (!Config::allowInteractiveDebugging() &&
		lastTickPeriod > timeoutPeriod_)
	{
		CRITICAL_MSG( "BaseApp::tickGameTime: "
			"Last game tick took %.2f seconds."
			"baseAppMgr/baseAppTimeout is %.2f. "
			"This process should have been stopped by BaseAppMgr. This "
			"process will now be terminated.\n",
			lastTickPeriod,
			timeoutPeriod_ );
	}

	// we are done with that tick, time to start the next one
	this->advanceTime();

	if ((spareTimeFraction < 0.f) || (1.f < spareTimeFraction))
	{
		if (g_timingMethod == RDTSC_TIMING_METHOD)
		{
			CRITICAL_MSG( "BaseApp::tickGameTime: "
				"Invalid timing result %.3f.\n"
				"This is likely due to running on a multicore system that "
				"does not have synchronised Time Stamp Counters. Refer to "
				"server_installation_guide.html regarding TimingMethod.\n",
				spareTimeFraction );
		}
		else
		{
			CRITICAL_MSG( "BaseApp::tickGameTime: "
				"Invalid timing result %.3f.\n", spareTimeFraction );
		}
	}

	// Update the load
	float load = Math::clamp( 0.f, 1.f - float(spareTimeFraction), 1.f );

	float loadSmoothingBias = Config::loadSmoothingBias();
	load_ = (1-loadSmoothingBias)*load_ + loadSmoothingBias*load;

	// now do all the stuff that wants to happen in this tick
	pLoginHandler_->tick();

	this->backup();
	this->archive();

	if ((time_ % Config::timeSyncPeriodInTicks()) == 0)
	{
		// Sync our clock with the Cell App Manager's clock.
		pTimeKeeper_->synchroniseWithMaster();
	}

	// Inform the BaseAppMgr of our load.
	Mercury::Bundle & bundle = baseAppMgr_.bundle();
	BaseAppMgrInterface::informOfLoadArgs & args =
		BaseAppMgrInterface::informOfLoadArgs::start( bundle );

	args.load = this->getLoad();
	args.numBases = bases_.size();
	args.numProxies = proxies_.size();
	baseAppMgr_.send();

	TickedWorkerJob::tickJobs();

	this->tickRateLimitFilters();
	this->checkSendWindowOverflows();
	this->sendIdleProxyChannels();

	static const uint64 RETIRE_ACK_WAIT_PERIOD = 60 * stampsPerSecond();

	uint64 now = timestamp();

	if (this->isRetiring() && 
			pBackupSender_->isOffloading() &&
			(lastRetirementOffloadTime_ == retireStartTime_) &&
			!pOffloadedBackups_->isEmpty())
	{
		// Update the last retirement offload time, so that we can start to
		// wait from this time, instead of when the retirement started.
		lastRetirementOffloadTime_ = now;
	}

	if (this->isRetiring() && 
			bases_.empty() && 
			localServiceFragments_.empty() &&
			!pBackedUpBaseApps_->isBackingUpOthers() &&
			pOffloadedBackups_->isEmpty() &&
			(!this->extInterface().hasUnackedPackets() || 
				(now > (lastRetirementOffloadTime_ + RETIRE_ACK_WAIT_PERIOD))))
	{
		INFO_MSG( "BaseApp::tick: retirement completed in %.03f seconds "
					"(%.03f spent waiting for external channels), "
				"shutting down\n",
			(now - retireStartTime_) / stampsPerSecondD(),
			(now - lastRetirementOffloadTime_) / stampsPerSecondD() );

		this->shutDown();
	}

	this->tickStats();

	pDeadCellApps_->tick( bases_, this->intInterface() );
}


// -----------------------------------------------------------------------------
// Section: Mercury interface
// -----------------------------------------------------------------------------

/**
 * This method receives a forwarded watcher set request from the BaseAppMgr
 * and performs the set operation on the current BaseApp.
 * parameters in 'data'.
 *
 * @param srcAddr  The address from which this request originated.
 * @param header   The mercury header.
 * @param data     The data stream containing the watcher data to set.
 */
void BaseApp::callWatcher( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data)
{
	BW::string path;
	data >> path;

	DeferrableWatcherPathRequest * pPathRequest = 
		new DeferrableWatcherPathRequest( path, interface_, srcAddr,
				header.replyID );

	pPathRequest->setPacketData( data );
	pPathRequest->setWatcherValue();
}


/**
 *	This method creates a base entity on this app. It is used to create a client
 *	proxy or base entities.
 */
void BaseApp::createBaseWithCellData( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data )
{
	pEntityCreator_->createBaseWithCellData( srcAddr, header, data,
			pLoginHandler_.get() );
}


/**
 *
 */
void BaseApp::createBaseFromDB( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data )
{
	pEntityCreator_->createBaseFromDB( srcAddr, header, data );
}


// Handles request create a base from template
/**
 *	This method creates a base entity on this app. It uses provided template id
 *	to populate the properties from local data.
 */
void BaseApp::createBaseFromTemplate( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data )
{
	pEntityCreator_->createBaseFromTemplate( srcAddr, header, data );
}

/**
 *	This method handles a message from the database telling us that a player is
 *	trying to log on to an active entity.
 */
void BaseApp::logOnAttempt( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	MF_ASSERT( srcAddr == this->dbApp().addr() );

	EntityID id;
	Mercury::Address clientAddr;
	data >> id >> clientAddr;

	Base * pBase = bases_.findEntity( id );

	if (pBase == NULL)
	{
		WARNING_MSG( "BaseApp::logOnAttempt: No base %u\n", id );

		Mercury::ChannelSender sender( this->dbApp().channel() );
		sender.bundle().startReply( header.replyID );
		sender.bundle() << BaseAppIntInterface::LOG_ON_ATTEMPT_WAIT_FOR_DESTROY;
		return;
	}

	// We never expect this to happen, but reject the login just in case
	IF_NOT_MF_ASSERT_DEV( pBase->isProxy() )
	{
		ERROR_MSG( "BaseApp::logOnAttempt:"
					"%u is not a proxy, rejecting login attempt.\n",
				id );

		Mercury::ChannelSender sender( this->dbApp().channel() );
		sender.bundle().startReply( header.replyID );
		sender.bundle() << BaseAppIntInterface::LOG_ON_ATTEMPT_REJECTED;
		return;
	}

	Proxy * pProxy = static_cast< Proxy * >( pBase );
	PyObject * pResult;

	BW::string encryptionKey;
	data >> encryptionKey;

	BW::string logOnData;
	data >> logOnData;

	bool tookControl;
	PyObject* pFunction = PyObject_GetAttrString( pBase, "onLogOnAttempt" );
	if (pFunction)
	{
		PyObject * pArgs = PyTuple_New( 3 );
		PyTuple_SET_ITEM( pArgs, 0, Script::getData( clientAddr.ip ) );
		PyTuple_SET_ITEM( pArgs, 1, Script::getData( clientAddr.port ) );
		PyTuple_SET_ITEM( pArgs, 2, Script::getData( logOnData ) );
		pResult = Script::ask( pFunction, pArgs, "", true, true );
		// Note: The base may have destroyed itself in the onLogOnAttempt call.

		// If no result was returned, either due to the programmer forgetting
		// or because of a script error, just immediately reject the login.
		// The client will be free to re-attempt the login in case
		// onLogOnAttempt() has destroyed the entity.
		if (pResult == NULL)
		{
			WARNING_MSG( "BaseApp::logOnAttempt: "
							"%s.onLogOnAttempt failed to return an "
							"authorisation status for %u.\n",
						pBase->pType()->name(), id );

			Mercury::ChannelSender sender( this->dbApp().channel() );
			sender.bundle().startReply( header.replyID );
			sender.bundle() << BaseAppIntInterface::LOG_ON_ATTEMPT_REJECTED;
			return;
		}

		tookControl = !!PyObject_IsTrue( pResult );
		bool isDestroy = PyInt_Check( pResult ) &&
			(PyInt_AsLong( pResult ) == LOG_ON_WAIT_FOR_DESTROY);
		Py_DECREF( pResult );

		if (pBase->isDestroyed() || isDestroy)
		{
			NOTICE_MSG( "BaseApp::logOnAttempt: "
							"Rejecting relogin attempt. Waiting for previous "
							"entity destruction.\n" );
			// Let the database handle this. It should see the old entity log
			// out and then log in the new one.

			Mercury::ChannelSender sender( this->dbApp().channel() );
			sender.bundle().startReply( header.replyID );
			sender.bundle() <<
					BaseAppIntInterface::LOG_ON_ATTEMPT_WAIT_FOR_DESTROY;
			return;
		}
	}
	else
	{
		NOTICE_MSG( "BaseApp::logOnAttempt: "
				"Rejecting relogon attempt for entity %u. "
				"No script method %s.onLogOnAttempt\n",
			id, pBase->pType()->name() );
		PyErr_Clear();
		tookControl = false;
	}

	if (tookControl)
	{
		if (!clientAddr.ip)
		{
			// only clear base's client channel if this is not a web login
			// (check clientAddr.ip)
			INFO_MSG( "BaseApp::logOnAttempt: "
						"For %u from web login.\n", pBase->id() );
		}
		else 
		{
			if (pProxy->isGivingClientAway())
			{
				// We can't discard the existing client connection if it's
				// already being given away, and waiting for the acceptClient
				// reply.
				// This should only be a small window, but there are two hops,
				// one to the remote BaseApp, and one to the client.
				pProxy->prepareForReLogOnAfterGiveClientToSuccess( clientAddr,
					header.replyID,
					encryptionKey );
				return;
			}

			pProxy->logOffClient( /* shouldCondemnChannel */ true );
		}

		pProxy->completeReLogOnAttempt( clientAddr, header.replyID,
			encryptionKey );
	}
	else
	{
		NOTICE_MSG( "BaseApp::logOnAttempt: "
						"Rejecting relogin attempt. " \
						"Have not taken control.\n" );

		Mercury::ChannelSender sender( this->dbApp().channel() );
		Mercury::Bundle & bundle = sender.bundle();
		bundle.startReply( header.replyID );

		bundle << BaseAppIntInterface::LOG_ON_ATTEMPT_REJECTED;
	}
}


/**
 *	This method handles a message from the BaseAppMgr to inform us that there is
 *	a new global base.
 */
void BaseApp::addGlobalBase( BinaryIStream & data )
{
	pGlobalBases_->add( data );
}


/**
 *	This method handles a message from the BaseAppMgr to inform us that a global
 *	base has gone.
 */
void BaseApp::delGlobalBase( BinaryIStream & data )
{
	pGlobalBases_->remove( data );
}


/**
 *	This method handles a message from the BaseAppMgr to inform us that there is
 *	a new global base.
 */
void BaseApp::addServiceFragment( BinaryIStream & data )
{
	BW::string serviceName;
	EntityMailBoxRef fragmentMailBox;

	data >> serviceName >> fragmentMailBox;

	this->servicesMap().addFragment( serviceName, fragmentMailBox );
}


/**
 *	This method handles a message from the BaseAppMgr to inform us that a global
 *	base has gone.
 */
void BaseApp::delServiceFragment( BinaryIStream & data )
{
	BW::string serviceName;
	Mercury::Address address;

	data >> serviceName >> address;
	this->servicesMap().removeFragment( serviceName, address );
}


/**
 *	This method is called when a new Cell App Manager is started.
 */
void BaseApp::handleCellAppMgrBirth(
		const BaseAppIntInterface::handleCellAppMgrBirthArgs & args )
{
	INFO_MSG( "BaseApp::handleCellAppMgrBirth: %s\n", args.addr.c_str() );
	cellAppMgr_ = args.addr;

	if (pTimeKeeper_)
	{
		pTimeKeeper_->masterAddress( cellAppMgr_ );
	}
}


/**
 *	This method is called when a new BaseAppMgr is started.
 */
void BaseApp::handleBaseAppMgrBirth(
		const BaseAppIntInterface::handleBaseAppMgrBirthArgs & args )
{
	INFO_MSG( "BaseApp::handleBaseAppMgrBirth: %s\n", args.addr.c_str() );
	baseAppMgr_.onManagerRebirth( *this,  args.addr );
}


/**
 *
 */
void BaseApp::addBaseAppMgrRebirthData( BinaryOStream & stream )
{
	stream << this->intInterface().address();
	stream << extInterface_.address();
	stream << id_;
	stream << isServiceApp_;
	stream << time_;

	pBackupSender_->addToStream( stream );

	pSharedDataManager_->addToStream( stream );

	localServiceFragments_.addServicesToStream( stream );

	pGlobalBases_->addLocalsToStream( stream );
}


/**
 *	This method is called when a cell application has died unexpectedly.
 */
void BaseApp::handleCellAppDeath( BinaryIStream & data )
{
	MF_ASSERT( data.remainingLength() >= int(sizeof( Mercury::Address )) );
	Mercury::Address addr;
	data >> addr;

	NOTICE_MSG( "BaseApp::handleCellAppDeath: CellApp %s is dead\n",
		addr.c_str() );

	pDeadCellApps_->addApp( addr, data );
	pDeadCellApps_->tick( bases_, this->intInterface() );
}


/**
 *  This method will call Base::emergencySetCurrentCell if the base is
 *  available, otherwise it will ack the message anyway.
 */
void BaseApp::emergencySetCurrentCell( const Mercury::Address & srcAddr,
	const Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data )
{
	Base * pEntity = BaseApp::instance().getBaseForCall( true );

	if (pEntity)
	{
		AUTO_SCOPED_ENTITY_PROFILE( pEntity );
		pEntity->emergencySetCurrentCell( srcAddr, header, data );
	}
	else
	{
		INFO_MSG( "BaseApp::emergencySetCurrentCell: "
				"No such entity. Last missed: %d\n", lastMissedBaseForCall_ );

		// ACK to the CellApp anyway.
		Mercury::ChannelSender sender( BaseApp::getChannel( srcAddr ) );
		sender.bundle().startReply( header.replyID );
	}
}


/**
 *	This method is called by the BaseAppMgr on all BaseApps when the system is
 *	ready to start, however, only one base app will have the 'bootstrap'
 *	argument passed as true.
 */
void BaseApp::startup( const BaseAppIntInterface::startupArgs & args )
{
	isBootstrap_ = args.bootstrap;
	didAutoLoadEntitiesFromDB_ = args.didAutoLoadEntitiesFromDB;

	if (didAutoLoadEntitiesFromDB_)
	{
		INFO_MSG( "Starting from auto-loaded state. "
				"To clear auto-load data for MySQL databases, use the "
				"clear_auto_load command tool.\n"
				"To clear auto-load data for XML databases, remove the "
				"AutoLoad sections under the _BigWorldInfo section in "
				"db.xml.\n" );
	}

	// Do this as soon as possible so that the timer is started as close to the
	// correct time as possible.
	this->ready( READY_BASE_APP_MGR );
	INFO_MSG( "Starting time = %.1f\n", this->gameTimeInSeconds() );

	// Send secondary DB registration now that we're sure DBApp is ready.
	this->registerSecondaryDB();
}


/**
 *  This method starts the game tick timer for this app.
 */
void BaseApp::startGameTickTimer()
{
	MF_ASSERT( !gameTimer_.isSet() );

	gameTimer_ = mainDispatcher_.addTimer( 1000000/Config::updateHertz(),
						this, reinterpret_cast< void * >( TIMEOUT_GAME_TICK ),
						"GameTick" );

	s_lastTimestamp = timestamp();
	mainDispatcher_.clearSpareTime();

	// Since we're now sending load updates to the BaseAppMgr, we're no longer
	// irregular.
	baseAppMgr_.isRegular( true );
}


/**
 *	This method is used to identify when all components are ready so that we
 *	can work out when we are ready.
 */
void BaseApp::ready( int component )
{
	// Will this make it 0?
	if (component == waitingFor_)
	{
		// registerTimer should be done as soon as possible so that it is not
		// too out-of-sync with the game time that the BaseAppMgr has told us.
		if (!gameTimer_.isSet())
		{
			this->startGameTickTimer();
		}

		pTimeKeeper_ = new TimeKeeper( this->intInterface(), gameTimer_,
							time_, Config::updateHertz(),
							cellAppMgr_,
							&CellAppMgrInterface::gameTimeReading );
		INFO_MSG( "BaseApp::ready: Starting game time from %u\n", time_ );

		this->scriptEvents().triggerTwoEvents( "onAppReady",
				isServiceApp_ ? "onServiceAppReady" : "onBaseAppReady",
				PyTuple_Pack( 2,
							isBootstrap_ ? Py_True : Py_False,
							didAutoLoadEntitiesFromDB_ ? Py_True: Py_False ) );
	}

	waitingFor_ &= ~component;
}


/**
 * This method registers the secondary database
 */
void BaseApp::registerSecondaryDB()
{
	if (!this->pSqliteDB())
	{
		return;
	}

	Mercury::Bundle & bundle = this->dbApp().bundle();
	bundle.startMessage( DBAppInterface::secondaryDBRegistration );
	bundle << this->intInterface().address() << this->pSqliteDB()->dbFilePath();
	this->dbApp().send();

	this->pSqliteDB()->isRegistered( true );
}


/**
 *	Find out the external address of another baseapp
 */
Mercury::Address 
BaseApp::getExternalAddressFor( const Mercury::Address & intAddr ) const
{
	BaseAppExtAddresses::const_iterator iter =
		baseAppExtAddresses_.find( intAddr );
	if (iter != baseAppExtAddresses_.end())
	{
		return iter->second;
	}
	return Mercury::Address::NONE;
}


/**
 *	Request that this BaseApp start retirement.
 */
void BaseApp::requestRetirement()
{
	this->EntityApp::requestRetirement();
	lastRetirementOffloadTime_ = retireStartTime_ = timestamp();
}


/**
 *	This method records and sets up the forwarding for an offloaded base
 *	entity.
 */
void BaseApp::addForwardingMapping( EntityID entityID, 
		const Mercury::Address & addr )
{
	pBaseMessageForwarder_->addForwardingMapping( entityID, addr );
}


/**
 *	This message forwards a message to a recently offloaded base entity.
 */
bool BaseApp::forwardBaseMessageIfNecessary( EntityID entityID, 
		const Mercury::Address & srcAddr, 
		const Mercury::UnpackedMessageHeader & header, 
		BinaryIStream & data )
{
	return pBaseMessageForwarder_->forwardIfNecessary( 
		forwardingEntityIDForCall_, srcAddr, header, data );
}


/**
 *	Return true if the given entity ID has been offloaded to another BaseApp
 *	due to retirement.
 */
bool BaseApp::entityWasOffloaded( EntityID entityID ) const
{
	return pOffloadedBackups_->wasOffloaded( entityID );
}


/**
 *	This method handles the acceptClient request.
 */
void BaseApp::acceptClient( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	EntityID entityID;
	data >> entityID;

	Base * pBase = bases_.findEntity( entityID );
	if (!pBase || !pBase->isProxy())
	{
		NOTICE_MSG( "BaseApp::acceptClient: "
				"called for proxy %d that doesn't exist",
			entityID );
		data.finish();

		Mercury::UDPChannel & channel = BaseApp::getChannel( srcAddr );
		Mercury::Bundle & bundle = channel.bundle();
		bundle.startReply( header.replyID );
		bundle << false;
		channel.send();
	}
	else
	{
		static_cast< Proxy * >( pBase )->acceptClient( srcAddr, header, data );
	}
}


/**
 *	This method handles the forwarding of a message for a Base forwarded on
 *	from a BaseApp that had offloaded the Base to this location.
 */
void BaseApp::forwardedBaseMessage( BinaryIStream & data )
{
	Mercury::Address originalSrcAddr;
	Mercury::UnpackedMessageHeader fakeHeader;

	data >> originalSrcAddr >> fakeHeader.identifier >> fakeHeader.replyID;

	fakeHeader.length = data.remainingLength();
	fakeHeader.pInterface = &this->intInterface();

	Mercury::InterfaceElement & ie = 
		this->interface().interfaceTable()[fakeHeader.identifier];

	ie.pHandler()->handleMessage( originalSrcAddr, fakeHeader, data );
}


/**
 *	This method is called by the BaseAppMgr to tell us to shut down.
 */
void BaseApp::shutDown( const BaseAppIntInterface::shutDownArgs & /*args*/ )
{
	this->shutDown();
}


/**
 *	This method shuts down this application.
 */
void BaseApp::shutDown()
{
	if (!ReplayDataFileWriter::haveAllClosed())
	{
		// Close file writers before we stop ticking.

		static const float BGTASK_MGR_TICK_TIME = 0.1;

		// Don't finalise if we are being killed individually.
		const bool shouldFinalise = this->isShuttingDown();

		ReplayDataFileWriter::closeAll( /* pListener */ NULL, shouldFinalise );

		uint64 lastReport = timestamp();

		while (!ReplayDataFileWriter::haveAllClosed()) 
		{
			usleep( int( BGTASK_MGR_TICK_TIME * 1000000 ) );
			bgTaskManager_.tick();

			uint64 now = timestamp();
			static const float REPORT_PERIOD_SECONDS = 5.0;
			float secondsSinceReport = ((now - lastReport) / stampsPerSecondD());

			if (secondsSinceReport > REPORT_PERIOD_SECONDS)
			{
				NOTICE_MSG( "BaseApp::shutDown: Still waiting for replay data file "
						"writers to finalise\n" );
				lastReport = now;
			}
		}
	}

	this->ServerApp::shutDown();
}


/**
 *  Returns true if a CellApp at the specified address is known to have died
 *  recently.
 */
bool BaseApp::isRecentlyDeadCellApp( const Mercury::Address & addr ) const
{
	return pDeadCellApps_->isRecentlyDead( addr );
}


/**
 *	This method is called by the BaseAppMgr to tell us to shut down.
 */
void BaseApp::controlledShutDown( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data )
{
	ShutDownStage stage;
	GameTime shutDownTime;
	data >> stage >> shutDownTime;

	INFO_MSG( "BaseApp::controlledShutDown: stage = %s\n", 
		ServerApp::shutDownStageToString( stage ) );

	switch (stage)
	{
		case SHUTDOWN_INFORM:
		{
			// Make sure that we no longer process the external nub.
			extInterface_.detach();

			shutDownTime_ = shutDownTime;
			shutDownReplyID_ = header.replyID;

			if (this->hasStarted())
			{
				this->scriptEvents().triggerTwoEvents( "onAppShuttingDown",
						isServiceApp_ ? 
							"onServiceAppShuttingDown" :
							"onBaseAppShuttingDown",
						Py_BuildValue( "(d)",
							(double)shutDownTime/Config::updateHertz() ) );

				// Don't send reply immediate to allow scripts to do some stuff.
			}
			else
			{
				baseAppMgr_.bundle().startReply( shutDownReplyID_ );
				baseAppMgr_.send();
			}
		}
		break;

		case SHUTDOWN_DISCONNECT_PROXIES:
		{
			if (this->hasStarted())
			{
				this->callShutDownCallback( 0 );

				// TODO: Should probably spread this out over time.
				typedef BW::vector< SmartPointer< Proxy > > CopiedProxies;
				CopiedProxies copyOfProxies;

				{
					copyOfProxies.reserve( proxies_.size() );

					Proxies::iterator iter = proxies_.begin();

					while (iter != proxies_.end())
					{
						copyOfProxies.push_back( iter->second );
						++iter;
					}
				}

				{
					CopiedProxies::iterator iter = copyOfProxies.begin();

					while (iter != copyOfProxies.end())
					{
						(*iter)->onClientDeath( CLIENT_DISCONNECT_SHUTDOWN );

						++iter;
					}
				}
			}


			IF_NOT_MF_ASSERT_DEV( baseAppMgr_.addr() == srcAddr )
			{
				break;
			}

			baseAppMgr_.bundle().startReply( header.replyID );
			baseAppMgr_.send();

			break;
		}

		case SHUTDOWN_PERFORM:
		{
			if (this->hasStarted())
			{
				this->callShutDownCallback( 1 );
			}

			ControlledShutdown::start( pSqliteDB_,
					bases_, localServiceFragments_,
					header.replyID, srcAddr );

			break;
		}

		case SHUTDOWN_NONE:
		case SHUTDOWN_REQUEST:
		case SHUTDOWN_TRIGGER:
			break;
	}
}


/**
 *	This method calls the onAppShutDown callback with the given stage argument.
 */
void BaseApp::callShutDownCallback( int stage )
{
	this->scriptEvents().triggerTwoEvents( "onAppShutDown",
			isServiceApp_ ?
				"onServiceAppShutDown" : "onBaseAppShutDown",
			Py_BuildValue( "(i)", stage ) );
}


/**
 *	This method is called to set the information that is used to decide which
 *	BaseApp BigWorld.createBaseAnywhere should create base entities on.
 */
void BaseApp::setCreateBaseInfo( BinaryIStream & data )
{
	if (pEntityCreator_)
	{
		pEntityCreator_->setCreateBaseInfo( data );
	}
}


// -----------------------------------------------------------------------------
// Section: New-style BaseApp backup
// -----------------------------------------------------------------------------

/**
 *	This method handles a message telling this BaseApp to start handling the
 *	backup of a subset of another BaseApp's entities.
 */
void BaseApp::startBaseEntitiesBackup(
		const BaseAppIntInterface::startBaseEntitiesBackupArgs & args )
{
	pBackedUpBaseApps_->startAppBackup(
			args.realBaseAppAddr,
			args.index,
			args.hashSize,
			args.prime,
			args.isInitial );
}


/**
 *	This method handles a message telling this BaseApp to stop handling the
 *	backup of a subset of another BaseApp's entities.
 */
void BaseApp::stopBaseEntitiesBackup(
		const BaseAppIntInterface::stopBaseEntitiesBackupArgs & args )
{
	pBackedUpBaseApps_->stopAppBackup(
		args.realBaseAppAddr,
		args.index,
		args.hashSize,
		args.prime,
		args.isPending );
}


void BaseApp::sendAckOffloadBackup( EntityID entityID, 
									const Mercury::Address & dstAddr )
{
	Mercury::UDPChannel & channel = 
		this->interface().findOrCreateChannel( dstAddr );
	Mercury::Bundle & bundle = channel.bundle();

	bundle.startMessage( BaseAppIntInterface::ackOffloadBackup );
	bundle << entityID;
	channel.send();
}


void BaseApp::ackOffloadBackup( BinaryIStream & data )
{
	EntityID entityID;
	data >> entityID;
	pOffloadedBackups_->stopBackingUpEntity( entityID );
}


/**
 *	This class handles the response from a backup call to another baseapp
 *	since we ensure that the backup is no older than the DB entry when the
 *	write operation is complete.
 */
class AcknowledgeOffloadBackupHandler :
	public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	AcknowledgeOffloadBackupHandler( EntityID entityID,
									 const Mercury::Address& srcAddr ) :
		entityID_( entityID ), srcAddr_( srcAddr ) {}
		virtual ~AcknowledgeOffloadBackupHandler() {}

private:
	void handleMessage( const Mercury::Address& /*srcAddr*/,
						Mercury::UnpackedMessageHeader& /*header*/,
						BinaryIStream& /*data*/, void * /*arg*/ )
	{
		BaseApp::instance().sendAckOffloadBackup( entityID_, srcAddr_ );
		delete this;
	}

	void handleException( const Mercury::NubException&, void * )
	{
		delete this;
	}

	EntityID entityID_;
	Mercury::Address srcAddr_;
};


/**
 *
 */
bool BaseApp::backupBaseNow( Base & base, 
							 Mercury::ReplyMessageHandler * pHandler /*=NULL*/ )
{
	Mercury::BundleSendingMap bundles( this->intInterface() );
	if (pBackupSender_->backupBase( base, bundles, pHandler ) )
	{
		bundles.sendAll();
		return true;
	}

	if (pHandler)
	{
		pHandler->handleException( 
			Mercury::NubException( Mercury::REASON_SUCCESS ), NULL );
	}
	return false;
}


/**
 *	This method handles a message containing the backup information for a base
 *	entity on another BaseApp.
 */
void BaseApp::backupBaseEntity( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	bool isOffload;
	data >> isOffload;

	EntityID entityID;
	data >> entityID;

	if (isOffload)
	{
		BasePtr pBase = this->createBaseFromStream( entityID, data );

		if (pBase == NULL)
		{
			WARNING_MSG( "BaseApp::backupBaseEntity: "
				"failed to create base %d for onload\n", entityID );
			return;
		}

		DEBUG_MSG( "BaseApp::backupBaseEntity: Onloaded %s %d\n", 
			pBase->pType()->name(), entityID );

		// This just culls the old backup if appropriate
		pBackedUpBaseApps_->onloadedEntity( srcAddr, entityID );

		// This will send a backup of this entity to another baseapp right now.
		// NOTE: If this baseapp is being retired the entity will be immediately
		// offloaded instead.
		if (pBase->isDestroyed() || !this->backupBaseNow( *pBase,
				new AcknowledgeOffloadBackupHandler( entityID, srcAddr ) ))
		{
			// If there is no backup locations or this entity has chosen to
			// delete itself there is no reasonable expecation
			// of a backup, just acknowledge the offload now.
			this->sendAckOffloadBackup( entityID, srcAddr );
		}
	}
	else
	{
		pBackedUpBaseApps_->backUpEntity( srcAddr, entityID, data );
	}

	if (header.replyID != Mercury::REPLY_ID_NONE)
	{
		Mercury::Channel & channel = BaseApp::getChannel( srcAddr );
		std::auto_ptr< Mercury::Bundle > pBundle( channel.newBundle() );
		pBundle->startReply( header.replyID );
		channel.send( pBundle.get() );
	}
}


/**
 *	This method creates a base entity from the given backup data stream as a
 *	restore or offload.
 */
BasePtr BaseApp::createBaseFromStream( EntityID id, BinaryIStream & stream )
{
	// This can happen when an offloading baseapp offloads a restored entity
	// to the baseapp that has already restored it.
	if (bases_.find( id ) != bases_.end())
	{
		NOTICE_MSG( "BaseApp::createBaseFromStream( %d ): "
			"Entity already exists\n",
					id );
		stream.finish();
		return NULL;
	}
	// This should match the Base::writeBackupData, with the exception that the
	// entity ID has already been streamed off as the given EntityID parameter.

	EntityTypeID typeID;
	BW::string templateID;
	DatabaseID databaseID;
	stream >> typeID >> templateID >> databaseID;

	EntityTypePtr pType = EntityType::getType( typeID );

	if ((pType == NULL) || !pType->canBeOnBase())
	{
		ERROR_MSG( "BaseApp::createBaseFromStream: "
				"Invalid entity type %d for entity %d\n",
			typeID, id );
		stream.finish();
		return NULL;
	}

	BasePtr pBase = pType->newEntityBase( id, databaseID );

	if (!pBase)
	{
		ERROR_MSG( "BaseApp::createBaseFromStream: "
			"Failed to create entity %d of type %d\n",
			id, typeID );
		stream.finish();
		return NULL;
	}
	
	if (!pBase->initDelegate( templateID ))
	{
		ERROR_MSG( "BaseApp::createBaseFromStream: "
			"Failed to initialise delegate of entity %d of type '%s' "
			"with template '%s'\n",
			id, pType->name(), templateID.c_str() );
		stream.finish();
		return NULL;
	}
	
	pBase->readBackupData( stream );
	
	return pBase;
}


/**
 *	Backup the given base locally. When this app is retiring, it is the backup
 *	for all the entities it offloads.
 */
void BaseApp::makeLocalBackup( Base & base )
{
	MemoryOStream dummyStream( 64 * 1024 );
	base.writeBackupData( dummyStream, false );

	pOffloadedBackups_->backUpEntity( pBackupSender_->addressFor( base.id() ), 
									  dummyStream );
}


/**
 *	This method handles a message sent to this BaseApp telling us that we should
 *	no longer back up of specific entity. This is called when the base entity is
 *	destroyed.
 */
void BaseApp::stopBaseEntityBackup( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const BaseAppIntInterface::stopBaseEntityBackupArgs & args )
{
	pBackedUpBaseApps_->stopEntityBackup( srcAddr, args.entityID );
}


/**
 *	Handler when other BaseApps are started. 
 */
void BaseApp::handleBaseAppBirth( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	// We require the external addresses of BaseApps for when switching clients
	// over to a new BaseApp. 

	Mercury::Address intAddr, extAddr;
	data >> intAddr >> extAddr;
	baseAppExtAddresses_[ intAddr ] = extAddr;
}


/**
 *	This method handles a message informing us that another BaseApp has died.
 */
void BaseApp::handleBaseAppDeath( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	Mercury::Address deadBaseAppAddr;
	uint8 isService = 0;
	data >> deadBaseAppAddr >> isService;

	baseAppExtAddresses_.erase( deadBaseAppAddr );
	NETWORK_INFO_MSG( "BaseApp::handleBaseAppDeath: %s\n",
		deadBaseAppAddr.c_str() );

	BackupHash backedUpHash;
	data >> backedUpHash;

	if (backedUpHash.empty())
	{
		WARNING_MSG( "BaseApp::handleBaseAppDeath: "
					"Unable to recover from BaseApp death.\n" );
			return;
	}

	// It's possible entity creation will occur before we receive an update
	// from the BaseAppMgr (eg: in onRestore)
	pEntityCreator_->handleBaseAppDeath( deadBaseAppAddr );

	pBackedUpBaseApps_->handleBaseAppDeath( deadBaseAppAddr );

	// This needs to be done after the entities have been restored but
	// before the mailboxes have been adjusted.
	pGlobalBases_->handleBaseAppDeath( deadBaseAppAddr );

	pBackupHashChain_->adjustForDeadBaseApp( deadBaseAppAddr,
		 backedUpHash );

	pOffloadedBackups_->handleBaseAppDeath( deadBaseAppAddr, 
		*pBackupHashChain_ );

	this->servicesMap().removeFragmentsForAddress( deadBaseAppAddr );

	ServerEntityMailBox::adjustForDeadBaseApp( deadBaseAppAddr,
		*pBackupHashChain_ );

	pGlobalBases_->resolveInvalidMailboxes();

	this->intInterface().onAddressDead( deadBaseAppAddr );

	pBackupSender_->handleBaseAppDeath( deadBaseAppAddr );
	pArchiver_->handleBaseAppDeath( deadBaseAppAddr, bases_, pSqliteDB_ );

	this->scriptEvents().triggerEvent( 
		(isService ? "onServiceAppDeath" : "onBaseAppDeath"),
		Py_BuildValue( "((sH))", 
			deadBaseAppAddr.ipAsString(),
			ntohs( deadBaseAppAddr.port ) ) );
}


/**
 *	This method handles a message from the BaseAppMgr telling us what BaseApps
 *	this BaseApp should back up its entities to.
 */
void BaseApp::setBackupBaseApps( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	pBackupSender_->setBackupBaseApps( data, this->intInterface() );
}


/**
 *	This method returns the backup address for an entity.
 */
Mercury::Address BaseApp::backupAddrFor( EntityID entityID ) const
{
	return pBackupSender_->addressFor( entityID );
}


/**
 *	This method backs up this BaseApp's entities. Each call of this method, a
 *	proportion of the entities are backed up to the BaseApps that they hash to.
 *
 *	This is the first level of fault tolerance for BaseApps. The second level is
 *	archiving. See BaseApp::archive.
 */
void BaseApp::backup()
{
	AUTO_SCOPED_PROFILE( "backup" )
	pBackupSender_->tick( bases_, this->intInterface() );
}


/**
 *	Once we have started retiring, we wait for acknowledgement from the 
 *  BaseAppMgr that it will no longer adjust the backup hash of this app.
 */
void BaseApp::startOffloading( BinaryIStream & stream )
{
	MF_ASSERT( this->isRetiring() );

	INFO_MSG( "BaseApp::startOffloading: Received confirmation of "
			"retirement from BaseAppMgr, destroying %zu local service"
			"fragments and starting to offload %zu entities\n",
		localServiceFragments_.size(),
		bases_.size() );

	localServiceFragments_.discardAll( /*shouldDestroy*/ true );

	pBackupSender_->restartBackupCycle( bases_ );
	pBackupSender_->startOffloading();
}


/**
 *	This method archives this BaseApp's entities. Each call of this method, a
 *	proportion of the entities are backed up to the BaseApps that they hash to.
 *
 *	This is the second level of fault tolerance for BaseApps. The first level is
 *	backing up. See BaseApp::backup.
 */
void BaseApp::archive()
{
	AUTO_SCOPED_PROFILE( "archive" )
	pArchiver_->tick( this->dbApp(), this->baseAppMgr(), bases_, pSqliteDB_ );
}


/**
 *	This method checks whether there are any entity channels that are close to
 *	overflowing. A callback is called on those that are in danger.
 */
void BaseApp::checkSendWindowOverflows()
{
	OverflowIDs::iterator iter = s_overflowIDs.begin();

	while (iter != s_overflowIDs.end())
	{
		Base * pEntity = bases_.findEntity( *iter );

		if (pEntity)
		{
			Script::call( PyObject_GetAttrString( pEntity, "onWindowOverflow" ),
					PyTuple_New( 0 ), "onWindowOverflow", true );
		}
		else
		{
			WARNING_MSG( "BaseApp::checkSendWindowOverflows: "
				"Could not find entity %u\n", *iter );
		}

		++iter;
	}

	s_overflowIDs.clear();
}


/**
 *	Ticks the rate limiter filters on each connected proxy channel.
 */
void BaseApp::tickRateLimitFilters()
{
	typedef BW::vector< RateLimitMessageFilterPtr > Filters;
	Filters filtersToTick;
	filtersToTick.reserve( proxies_.size() );

	Proxies::iterator iProxy = proxies_.begin();
	while (iProxy != proxies_.end())
	{
		filtersToTick.push_back( iProxy->second->pRateLimiter() );
		++iProxy;
	}

	Filters::const_iterator iFilter = filtersToTick.begin();
	while (iFilter != filtersToTick.end())
	{
		(*iFilter)->tick();
		++iFilter;
	}
}


/**
 *	This method checks whether any of the proxes' entity channels need to be
 *	sent.
 *
 *	The sending is usually triggered by a packet from the client. If no client
 *	packet has been received for a while, the ACKs etc should be sent.
 */
void BaseApp::sendIdleProxyChannels()
{
	Proxies::iterator iter = proxies_.begin();

	while (iter != proxies_.end())
	{
		iter->second->channel().sendIfIdle();

		++iter;
	}
}


/**
 * This method ticks entity and entity type profilers
 */
void BaseApp::tickProfilers( uint64 lastTickInStamps )
{
	float smoothingFactor = Config::loadSmoothingBias();

	for (Bases::iterator iter = bases_.begin(); iter != bases_.end(); iter++)
	{
		Base & base = *iter->second;
		EntityTypeProfiler & typeProfiler = base.pType()->profiler();

		base.profiler().tick( lastTickInStamps,
							smoothingFactor,
							typeProfiler );
	}

	EntityType::tickProfilers();
}


// -----------------------------------------------------------------------------
// Section: Shared data
// -----------------------------------------------------------------------------

/**
 *	This method is called by the BaseAppMgr to inform us that a shared value has
 *	changed. This value may be shared between BaseApps or BaseApps and CellApps.
 */
void BaseApp::setSharedData( BinaryIStream & data )
{
	pSharedDataManager_->setSharedData( data );
}


/**
 *	This method is called by the BaseAppMgr to inform us that a shared value was
 *	deleted. This value may be shared between BaseApps or BaseApps and CellApps.
 */
void BaseApp::delSharedData( BinaryIStream & data )
{
	pSharedDataManager_->delSharedData( data );
}


// -----------------------------------------------------------------------------
// Section: Misc
// -----------------------------------------------------------------------------

/**
 *	This method handles the setClient message.
 */
void BaseApp::setClient( const BaseAppIntInterface::setClientArgs & args )
{
	Base * pBase = bases_.findEntity( args.id );

	if (!pBase)
	{
		pBase = localServiceFragments_.findEntity( args.id );
	}

	if (pBase)
	{
		this->setBaseForCall( pBase, /* isExternalCall */ false );
		return;
	}

	Mercury::ChannelPtr pForwardingChannelForCall = 
			 pBaseMessageForwarder_->getForwardingChannel( args.id );

	if (pForwardingChannelForCall != NULL)
	{
		INFO_MSG( "BaseApp::setClient: Forwarding message for id %u\n",
				  args.id );
		baseForCall_ = NULL;


		Mercury::Bundle & bundle = pForwardingChannelForCall->bundle();

		BaseAppIntInterface::setClientArgs::start( bundle ).id = args.id;

		forwardingEntityIDForCall_ = args.id;
	}
	else
	{
		WARNING_MSG( "BaseApp::setClient: Could not find base id %u\n",
			args.id );
		// we can't trust any base messages until the next setClient message
		baseForCall_ = NULL;
		lastMissedBaseForCall_ = args.id;
	}

}


/**
 *	This method handles a message from a CellApp to indicate that the next
 *	should be made reliable. It is needed when volatile position messages are
 *	used to select the entity that subsequent messages will be delivered to,
 *	and also when reference position messages are sent.
 */
void BaseApp::makeNextMessageReliable(
	const BaseAppIntInterface::makeNextMessageReliableArgs & args )
{
	shouldMakeNextMessageReliable_ = args.isReliable;
}


/**
 *	This method handles a message from the client. Should be the first message
 *	received from the client.
 */
void BaseApp::baseAppLogin( const Mercury::Address& srcAddr,
			const Mercury::UnpackedMessageHeader& header,
			const BaseAppExtInterface::baseAppLoginArgs & args )
{
	pLoginHandler_->login( extInterface_, srcAddr, header, args );
}


/**
 *	This method handles a message from the client. It is used to indicate which
 *	proxy subsequent message are destined for.
 */
void BaseApp::authenticate( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const BaseAppExtInterface::authenticateArgs & args )
{

	// This might rarely occur on server restart
	if (header.pChannel == NULL)
	{
		WARNING_MSG( "BaseApp::authenticate(%s): "
				   "Message received with no channel\n",
			srcAddr.c_str() );

		// Bad bundle so break out of dispatching the rest.
		header.breakBundleLoop();
		return;
	}

	// clear an existing proxy if we have one ... which we should not
	this->clearProxyForCall();

	// find the proxy by its channel
	Mercury::Address address = header.pChannel->addr();
	address.salt = header.pChannel->isTCP() ? 1 : 0;
	Proxies::iterator iter = proxies_.find( address );

	if (iter != proxies_.end())
	{
		Proxy & proxy = *iter->second;

		// make sure the authentication session key matches
		if (!proxy.isClientChannel( header.pChannel.get() ))
		{
			ERROR_MSG( "BaseApp::authenticate(%s): "
					   "Message received on incorrect channel\n",
				srcAddr.c_str() );

			// Bad bundle so break out of dispatching the rest.
			header.breakBundleLoop();
		}
		else if (proxy.sessionKey() != args.key)
		{
			ERROR_MSG( "BaseApp::authenticate(%s): "
				"CHEAT: Received wrong session key 0x%x instead of 0x%x\n",
				srcAddr.c_str(), args.key, proxy.sessionKey() );

			// Bad bundle so break out of dispatching the rest.
			header.breakBundleLoop();
		}
		else
		{
			this->setBaseForCall( &proxy, /* isExternalCall */ true );
		}
	}
	else
	{
		WARNING_MSG( "BaseApp::authenticate(%s): "
			"Could not find proxy from that address\n", srcAddr.c_str() );
		// Bad bundle so break out of dispatching the rest.
		header.breakBundleLoop();
	}
}


/**
 *	This method sets up the network interfaces.
 */
int BaseApp::serveInterfaces()
{
	// Internal interface
	BaseAppIntInterface::registerWithInterface( this->intInterface() );
	InternalInterfaceHandlers::init( this->intInterface().interfaceTable() );

	BaseAppExtInterface::registerWithInterface( extInterface_ );
	ExternalInterfaceHandlers::init( extInterface_.interfaceTable() );

	return 0;
}


/**
 *	Common function for obtaining a proxy to send a message to or
 *	complain if it is not able to get a valid one. Only really
 *	makes sense to use in external message handlers.
 */
ProxyPtr BaseApp::getAndCheckProxyForCall(
	Mercury::UnpackedMessageHeader & header, const Mercury::Address & srcAddr )
{
	ProxyPtr pProxy = this->getProxyForCall( true /* okayIfNull*/ );

	if (!pProxy)
	{
		WARNING_MSG( "BaseApp::getAndCheckProxyForCall: "
					 "No proxy for message %s(%d) from %s\n",
					 header.msgName(), header.identifier, srcAddr.c_str() );
		return NULL;
	}

	if ((header.pChannel == NULL) ||
			(!pProxy->isClientChannel( header.pChannel.get() ) &&
				header.pChannel->isExternal()))
	{
		WARNING_MSG( "BaseApp::getAndCheckProxyForCall: "
						 "Message %s(%d) came through wrong channel: ( %s ) "
						 "for entity %u\n",
					 header.msgName(), header.identifier,
					 header.pChannel ? header.pChannel->c_str() : "Null",
					 pProxy->id() );
		return NULL;
	}

	return pProxy;
}


EntityID BaseApp::getID()
{
	return pEntityCreator_->getID();
}

void BaseApp::putUsedID( EntityID id )
{
	if (pEntityCreator_)
	{
		pEntityCreator_->putUsedID( id );
	}
}


/**
 *	Signal handler.
 */
void BaseApp::onSignalled( int sigNum )
{
	if (sigNum == SIGQUIT)
	{
		// Just print out some information, and pass it up to EntityApp to dump
		// core.
		ERROR_MSG( "BaseApp::onSignalled: "
				"load = %f. Time since tick = %f seconds\n",
			this->getLoad(),
			double( timestamp() - s_lastTimestamp ) / stampsPerSecondD() );
	}

	this->EntityApp::onSignalled( sigNum );
}

/**
 *	This method is called on StartOfTick, before the Updaters are updated.
 * We use it here to execute the update step on all the despair objects and
 * components
 */
void BaseApp::onStartOfTick()
{
	if (IGameDelegate::instance() != NULL)
	{
		IGameDelegate::instance()->update();
	}
}

/**
 *	This is a helper class to callback at a time when it's OK to set a
 *	client to be dead.
 */
class ClientDeadCallback : public TimerHandler
{
public:
	ClientDeadCallback( Proxy * p ) :
		p_( p )
	{
		MF_ASSERT( p_ );
	}

	virtual ~ClientDeadCallback() {}

	virtual void handleTimeout( TimerHandle handle, void * arg )
	{
		INFO_MSG( "ClientDeadCallback::handleTimeout: "
				"Channel for entity %d has overflowed.\n", p_->id() );
		p_->onClientDeath( CLIENT_DISCONNECT_TIMEOUT );
	}

	virtual void onRelease( TimerHandle handle, void * pUser )
	{
		delete this;
	}

private:
	ProxyPtr p_;
};


/*
 *	Override from Mercury::ChannelListener.
 */
void BaseApp::onChannelSend( Mercury::Channel & channel )
{
	if (!channel.isTCP())
	{
		// We should only be registered for external channels.
		Mercury::UDPChannel & udpChannel = 
			static_cast< Mercury::UDPChannel & >( channel );

		Proxy * pProxy = reinterpret_cast< Proxy * >( channel.userData() );

		if (udpChannel.sendWindowUsage() > BaseAppConfig::clientOverflowLimit())
		{
			this->mainDispatcher().addOnceOffTimer( 1,
				new ClientDeadCallback( pProxy ) );
		}
	}
}


/*
 *	Override from Mercury::ChannelListener.
 */
void BaseApp::onChannelGone( Mercury::Channel & channel )
{
	TRACE_MSG( "BaseApp::onChannelGone: %s\n", channel.c_str() );

	Proxies::iterator iProxy = proxies_.find( channel.addr() );

	if (iProxy != proxies_.end())
	{
		ProxyPtr pProxy = iProxy->second;
		pProxy->onClientDeath( CLIENT_DISCONNECT_TIMEOUT );
	}
}

/**
 *	This method triggers a controlled shutdown of the cluster.
 */
void BaseApp::triggerControlledShutDown()
{
	BaseAppMgrGateway & baseAppMgr = this->baseAppMgr();

	Mercury::Bundle & bundle = baseAppMgr.bundle();
	BaseAppMgrInterface::controlledShutDownArgs &args =
		BaseAppMgrInterface::controlledShutDownArgs::start( bundle );

	args.stage = SHUTDOWN_TRIGGER;
	args.shutDownTime = 0; // unused on receiving side

	baseAppMgr.send();
}

BW_END_NAMESPACE


// baseapp.cpp
