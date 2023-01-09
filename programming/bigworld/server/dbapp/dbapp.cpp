#include "script/first_include.hpp"     // See http://docs.python.org/api/includes.html

#include "dbapp.hpp"

#ifndef CODE_INLINE
#include "dbapp.ipp"
#endif

#include "add_to_dbappmgr_helper.hpp"
#include "authenticate_account_handler.hpp"
#include "consolidator.hpp"
#include "custom.hpp"
#include "dbapp_config.hpp"
#include "delete_entity_handler.hpp"
#include "entity_auto_loader.hpp"
#include "get_entity_handler.hpp"
#include "load_entity_handler.hpp"
#include "login_app_check_status_reply_handler.hpp"
#include "login_conditions_config.hpp"
#include "login_handler.hpp"
#include "look_up_dbid_handler.hpp"
#include "look_up_entities_handler.hpp"
#include "look_up_entity_handler.hpp"
#include "relogon_attempt_handler.hpp"
#include "write_entity_handler.hpp"

#include "common/py_network.hpp"

#include "baseapp/baseapp_int_interface.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"

#include "cstdmf/build_config.hpp"
#include "cstdmf/debug_message_categories.hpp"
#include "cstdmf/memory_stream.hpp"

#include "db/db_config.hpp"

#include "db_storage/billing_system_creator.hpp"
#include "db_storage/db_engine_creator.hpp"
#include "db_storage/db_entitydefs.hpp"

#include "entitydef/constants.hpp"

#include "network/blocking_reply_handler.hpp"
#include "network/channel_sender.hpp"
#include "network/event_dispatcher.hpp"
#include "network/logger_message_forwarder.hpp"
#include "network/machine_guard.hpp"
#include "network/machined_utils.hpp"
#include "network/portmap.hpp"
#include "network/smart_bundles.hpp"
#include "network/udp_bundle.hpp"
#include "network/watcher_nub.hpp"

#include "pyscript/py_import_paths.hpp"
#include "pyscript/py_traceback.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/dataresource.hpp"
#include "resmgr/xml_section.hpp"

#include "server/bwconfig.hpp"
#include "server/look_up_entities.hpp"
#include "server/plugin_library.hpp"
#include "server/reviver_subject.hpp"
#include "server/signal_processor.hpp"
#include "server/util.hpp"
#include "server/writedb.hpp"

#ifdef BUILD_PY_URLREQUEST
#include "urlrequest/manager.hpp"
#endif

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

/// DBApp Singleton.
BW_SINGLETON_STORAGE( DBApp )


extern int BgTaskManager_token;
extern int force_link_UDO_REF;
extern int ResMgr_token;
extern int PyScript_token;
extern int BlobOrNull_token;
extern int PyURLRequest_token;

namespace // (anonymous)
{

int s_moduleTokens BW_UNUSED_ATTRIBUTE = ResMgr_token | PyScript_token |
    force_link_UDO_REF | BgTaskManager_token | BlobOrNull_token | 
    PyURLRequest_token;

// -----------------------------------------------------------------------------
// Section: Constants
// -----------------------------------------------------------------------------
const char * UNSPECIFIED_ERROR_STR = "Unspecified error";


// -----------------------------------------------------------------------------
// Section: Functions
// -----------------------------------------------------------------------------

bool commandShutDown( BW::string & output, BW::string & value )
{
    DBApp * pDBApp = DBApp::pInstance();
    if (pDBApp != NULL)
    {
        pDBApp->shutDown();
    }

    return true;
}

} // end anonymous namespace


// -----------------------------------------------------------------------------
// Section: DBApp
// -----------------------------------------------------------------------------

/**
 *  Constructor.
 */
DBApp::DBApp( Mercury::EventDispatcher & mainDispatcher,
       Mercury::NetworkInterface & interface ) :
    ScriptApp( mainDispatcher, interface ),
    id_( 0 ),
    dbApps_(),
    pEntityDefs_( NULL ),
    pDatabase_( NULL ),
    pBillingSystem_( NULL ),
    status_(),
    dbAppMgr_( interface_ ),
    baseAppMgr_( interface_ ),
    initState_( 0 ),
    shouldAlphaResetGameServerState_( false ),
    statusCheckTimer_(),
    gameTimer_(),
    curLoad_( 1.f ),
    hasOverloadedCellApps_( true ),
    overloadStartTime_( 0 ),
    mailboxRemapCheckCount_( 0 ),
    secondaryDBPrefix_(),
    secondaryDBIndex_( 0 ),
    pConsolidator_( NULL ),
    shouldCacheLogOnRecords_( false )
{
    mainDispatcher.maxWait( 0.02 );

    // The channel to the BaseAppMgr is irregular
    baseAppMgr_.channel().isLocalRegular( false );
    baseAppMgr_.channel().isRemoteRegular( false );
}


/**
 *  Destructor.
 */
DBApp::~DBApp()
{
    interface_.cancelRequestsFor( &baseAppMgr_.channel() );

    statusCheckTimer_.cancel();
    gameTimer_.cancel();

    bw_safe_delete( pBillingSystem_ );
    pBillingSystem_ = NULL;

    bw_safe_delete( pDatabase_ );
    // Destroy entity descriptions before calling Script::fini() so that it
    // can clean up any PyObjects that it may have.
    bw_safe_delete( pEntityDefs_ );
    DataType::clearStaticsForReload();

    // Now done in ~ScriptApp
    // Script::fini();
}

// ----------------------------------------------------------------------------
// Section: Initialisation
// ----------------------------------------------------------------------------

/**
 *  This method initialises the application.
 */
bool DBApp::init( int argc, char * argv[] )
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );

    // If changing the order or adding/removing initialisation steps, be sure
    // to update the order in the header too, as it is a handy reference.

    return this->initNetwork() &&
        this->initBaseAppMgr() &&
        this->initScript( argc, argv ) &&
        this->initEntityDefs() &&
        this->initExtensions() &&
        this->initDatabaseCreation() &&
        this->initDBAppMgrAsync();

    // DBAppMgr registration is asynchronous. We will complete initialisation
    // in onDBAppMgrRegistrationCompleted.
}


/**
 *  This method initialises the network.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initNetwork()
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );

    DEBUG_MSG( "DBApp::initNetwork\n" );

    if (!interface_.isGood())
    {
        NETWORK_ERROR_MSG( "DBApp::initNetwork: "
            "Failed to create network interface.\n");
        return false;
    }

    PROC_IP_INFO_MSG( "Address = %s\n", interface_.address().c_str() );

    DBAppInterface::registerWithInterface( interface_ );

    initState_ |= INIT_STATE_NETWORK;

    return true;
}


/**
 *  This method initialises our connection to BaseAppMgr, and determines our 
 *  recovery start-up mode.
 *
 *  @return     true on success, false otherwise.
 */
bool DBApp::initBaseAppMgr()
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );
    MF_ASSERT( initState_ & INIT_STATE_NETWORK );

    DEBUG_MSG( "DBApp::initBaseAppMgr\n" );

    status_.set( DBStatus::STARTING, "Looking for BaseAppMgr" );

    Mercury::MachineDaemon::registerBirthListener( interface_.address(),
            DBAppInterface::handleBaseAppMgrBirth, "BaseAppMgrInterface" );

    // Find the BaseAppMgr interface, but don't retry.
    Mercury::Address baseAppMgrAddr;
    Mercury::Reason reason =
        Mercury::MachineDaemon::findInterface( "BaseAppMgrInterface", 0,
            baseAppMgrAddr );

    if (reason == Mercury::REASON_SUCCESS)
    {
        baseAppMgr_.addr( baseAppMgrAddr );

        PROC_IP_INFO_MSG( "DBApp::initBaseAppMgr: BaseAppMgr at %s\n",
            baseAppMgr_.c_str() );
    }
    else if (reason == Mercury::REASON_TIMER_EXPIRED)
    {
        // This is OK, we can wait till later.
        INFO_MSG( "DBApp::initBaseAppMgr: BaseAppMgr is not ready yet.\n" );
    }
    else
    {
        ERROR_MSG( "DBApp::initBaseAppMgr: Failed to find BaseAppMgr due to "
                "network error: %s\n",
            Mercury::reasonToString( reason ) );
        return false;
    }

    return true;
}


/**
 *  This method initialises the script layer.
 *
 *  @param argc     The number of command line arguments in argv.
 *  @param argv     The command line arguments, which may contain res paths.
 *
 *  @return         true on success, false otherwise.
 */
bool DBApp::initScript( int argc, char ** argv )
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );

    DEBUG_MSG( "DBApp::initScript" );

    status_.set( DBStatus::STARTING, "Initialising script system" );

    if (!this->ScriptApp::init( argc, argv ))
    {
        return false;
    }

    if (!this->ScriptApp::initScript( "database",
                EntityDef::Constants::databasePath() ) ||
#ifdef BUILD_PY_URLREQUEST
            !URL::Manager::init( this->mainDispatcher() ) ||
#endif
            !PyNetwork::init( mainDispatcher_, interface_ ))
    {
        return false;
    }

    this->scriptEvents().createEventType( "onAppReady" );
    this->scriptEvents().createEventType( "onDBAppReady" );

    Script::initExceptionHook( &mainDispatcher_ );

    ScriptModule module = ScriptModule::import( "BigWorld",
        ScriptErrorPrint( "Failed to import BigWorld module" ) );

    MF_ASSERT( module );
    module.setAttribute( "EXPOSE_LOCAL_ONLY", CallableWatcher::LOCAL_ONLY,
        ScriptErrorPrint( "Failed to set EXPOSE_LOCAL_ONLY attribute" ) );

    if (this->initPersonality())
    {
        if (!this->triggerOnInit())
        {
            ERROR_MSG( "DBApp::initScript: "
                    "BWPersonality.onInit event listener failed.\n" );
            return false;
        }
    }

    return true;
}


/**
 *  This method reads the entity definitions.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initEntityDefs()
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );
    MF_ASSERT( Py_IsInitialized() );
    
    DEBUG_MSG( "DBApp::initEntityDefs\n" );

    status_.set( DBStatus::STARTING, "Loading entity definitions" );

    pEntityDefs_ = new EntityDefs();
    if (!pEntityDefs_->init(
        BWResource::openSection( EntityDef::Constants::entitiesFile() ) ))
    {
        bw_safe_delete( pEntityDefs_ );
        return false;
    }

    INFO_MSG( "DBApp::initEntityDefs: Expected digest is %s\n",
            pEntityDefs_->getDigest().quote().c_str() );

    pEntityDefs_->debugDump( Config::dumpEntityDescription() );

    return true;
}


/**
 *  This method loads any extensions (which live in shared libraries).
 *
 *  @return     true on success, false otherwise.
 */
bool DBApp::initExtensions()
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );

    INFO_MSG( "DBApp::initExtensions\n" );
    status_.set( DBStatus::STARTING, "Loading database plugins" );

    if (!PluginLibrary::loadAllFromDirRelativeToApp( true, "-extensions" ))
    {
        ERROR_MSG( "DBApp::initExtensions: Failed to load plugins.\n" );
        return false;
    }

    initState_ |= INIT_STATE_EXTENSIONS;

    return true;
}


/**
 *  This method creates the database implementation object.
 *
 *  @return     true on success, false otherwise.
 */
bool DBApp::initDatabaseCreation()
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );
    MF_ASSERT( initState_ & INIT_STATE_NETWORK );
    MF_ASSERT( initState_ & INIT_STATE_EXTENSIONS );

    DEBUG_MSG( "DBApp::initDatabaseCreation\n" );

    status_.set( DBStatus::STARTING, "Initialising database layer" );

    if (!DBConfig::get().isGood())
    {
        DBENGINE_ERROR_MSG( "DBApp::initDatabaseCreation: "
                "Config was not read successfully\n" );
        return false;
    }

    // Create the engine instance
    const BW::string & databaseType = DBConfig::get().type();

    DBENGINE_INFO_MSG( "Database engine = %s\n", databaseType.c_str() );

    DatabaseEngineData dbEngineData( this->interface(),
        this->mainDispatcher(),
        DBAppConfig::isProduction() );

    pDatabase_ = DatabaseEngineCreator::createInstance( databaseType,
        dbEngineData );

    if (pDatabase_ == NULL)
    {
        DBENGINE_ERROR_MSG( "DBApp::initDatabase: "
                "Failed to create database of type: %s\n",
            databaseType.c_str() );
        return false;
    }

    return true;
}


/**
 *  This method finds DBAppMgr and registers this DBApp with it, retrieving
 *  back a DBApp ID and the DBApp hash.
 *
 *  When registration completes, DBApp::onDBAppMgrRegistrationCompleted() will
 *  be called.
 *
 *  @return     true on success, false otherwise.
 */
bool DBApp::initDBAppMgrAsync()
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );
    MF_ASSERT( initState_ & INIT_STATE_NETWORK );

    DEBUG_MSG( "DBApp::initDBAppMgrAsync\n" );

    status_.set( DBStatus::STARTING, "Waiting for DBAppMgr to start" );
    // Find the DBAppMgr interface
    if (!dbAppMgr_.init( "DBAppMgrInterface", Config::numStartupRetries(),
            Config::maxMgrRegisterStagger() ))
    {
        NETWORK_DEBUG_MSG( "DBApp::initDBAppMgrAsync: "
            "Failed to find the DBAppMgr.\n" );
        return false;
    }

    status_.set( DBStatus::STARTING, "Registering with DBAppMgr" );

    // Add ourselves to the DBAppMgr. Once the reply is received this object
    // calls onDBAppMgrRegistrationCompleted() and deletes itself.
    new AddToDBAppMgrHelper( *this );

    return true;
}


/**
 *  This method handles the portion of init after registering with the DBAppMgr.
 *
 *  @param data     The initialisation data from the DBAppMgr.
 *
 *  @return         true on success, false otherwise.
 */
bool DBApp::onDBAppMgrRegistrationCompleted( BinaryIStream & data )
{
    MF_ASSERT( id_ == 0 );

    uint8 shouldAlphaResetGameServerState = 0;

    data >> id_ >> shouldAlphaResetGameServerState;


    if (!dbApps_.updateFromStream( data ))
    {
        ERROR_MSG( "DBApp::onDBAppMgrRegistrationCompleted: "
            "Could not de-stream DBApp hash\n" );
        return false;
    }

    CONFIG_INFO_MSG( "DBApp ID = %d (out of %" PRIzu ")\n",
        id_, dbApps_.size() );

    if (this->isAlpha())
    {
        CONFIG_INFO_MSG( "Alpha = true (%s reset of game server state)\n",
            shouldAlphaResetGameServerState ? "Performing" : "Skipping");

        // Set this so we reset the game server state as part of the
        // initialisation started by initDBAppAlpha().
        shouldAlphaResetGameServerState_ = shouldAlphaResetGameServerState;
    }
    else
    {
        CONFIG_INFO_MSG( "Alpha = false\n" );
    }

    // If changing the order or adding/removing initialisation steps, be sure
    // to update the order in the header too, as it is a handy reference.
    if (!(this->initAppIDRegistration() &&
            this->initDatabaseStartup() &&
            this->initBirthDeathListeners() &&
            this->initWatchers() &&
            this->initConfig() &&
            this->initReviver() &&
            this->initGameSpecific()))
    {
        return false;
    }

    // TODO: Scalable DB: Potentially have alpha-specific operations
    // implemented in a separate (sub-)class or object like Entity/RealEntity.
    if (this->isAlpha())
    {
        return this->initDBAppAlpha();

        // initTimers() will eventually be called after DBApp Alpha specific
        // stuff is done.
    }
    else
    {
        MF_ASSERT( shouldAlphaResetGameServerState == 0 );

        if (!(this->initTimers() &&
                this->initScriptAppReady()))
        {
            return false;
        }

        // As DBApp non-alpha, we're done now.
        this->onInitCompleted();
    }

    // If the server is starting up (we are not recovering), eventually
    // startServerBegin() will be called.

    return true;
}


/**
 *  This method performs initialisation for our newly received DBApp ID.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initAppIDRegistration()
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );
    MF_ASSERT( initState_ & INIT_STATE_NETWORK );
    MF_ASSERT( id_ != 0 );
    MF_ASSERT( Py_IsInitialized() );

    DEBUG_MSG( "DBApp::initAppIDRegistration\n" );

    // Send app id to loggers
    LoggerMessageForwarder::pInstance()->registerAppID( id_ );

    if (DBAppInterface::registerWithMachined( interface_, id_ ) !=
            Mercury::REASON_SUCCESS)
    {
        NETWORK_ERROR_MSG( "DBApp::initAppIDRegistration: "
            "Unable to register with interface. Is machined running?\n" );
        return false;
    }

    int pythonPort = BWConfig::get( "dbApp/pythonPort", 0 );
    this->startPythonServer( pythonPort, /* appID */ id_ );

    initState_ |= INIT_STATE_APP_ID_REGISTRATION;

    return true;
}


/**
 *  This method starts up the database.
 *
 *  @return     true on success, false otherwise.
 */
bool DBApp::initDatabaseStartup()
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );
    MF_ASSERT( pDatabase_ != NULL );
    MF_ASSERT( pEntityDefs_ != NULL );
    MF_ASSERT( id_ != 0 );
    MF_ASSERT( !dbApps_.empty() );

    DEBUG_MSG( "DBApp::initDatabaseStartup\n" );

    if (!this->isAlpha() && !pDatabase_->supportsMultipleDBApps())
    {
        DBENGINE_ERROR_MSG( "DBApp::initDatabaseStartup: Database type \"%s\" "
                "does not support the use of multiple DBApps\n",
            DBConfig::get().type().c_str() );

        return false;
    }

    // Note: Startup has been temporarily moved to before bwmachined
    // registration in order to avoid DBApp from blocking when SyncDB takes a
    // long time to complete.
    // TODO: Make startup synchronisation asynchronous (see consolidator class)
    if (!pDatabase_->startup( *pEntityDefs_, mainDispatcher_,
            DBAppConfig::numDBLockRetries() ))
    {
        return false;
    }
    
    initState_ |= INIT_STATE_DATABASE_STARTUP;

    return true;
}


/**
 *  This method registers any required birth and death listeners with machined.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initBirthDeathListeners()
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );
    MF_ASSERT( initState_ & INIT_STATE_NETWORK );

    DEBUG_MSG( "DBApp::initBirthDeathListeners\n" );

    Mercury::Reason reason = Mercury::MachineDaemon::registerBirthListener(
        interface_.address(), DBAppInterface::handleDBAppMgrBirth,
        "DBAppMgrInterface" );

    if (reason != Mercury::REASON_SUCCESS)
    {
        ERROR_MSG( "DBApp::initBirthDeathListeners: "
                "Failed to register birth listener for DBAppMgr: %s\n",
            Mercury::reasonToString( reason ) );
        return false;
    }

    reason = Mercury::MachineDaemon::registerDeathListener( interface_.address(),
        DBAppInterface::handleDBAppMgrDeath, "DBAppMgrInterface" );

    if (reason != Mercury::REASON_SUCCESS)
    {
        ERROR_MSG( "DBApp::initBirthDeathListeners: "
                "Failed to register death listener for DBAppMgr: %s\n",
            Mercury::reasonToString( reason ) );
        return false;
    }

    initState_ |= INIT_STATE_BIRTH_DEATH_LISTENERS;

    return true;
}


/**
 *  This method initialises watchers.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initWatchers()
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );
    MF_ASSERT( initState_ & INIT_STATE_NETWORK );
    MF_ASSERT( id_ != 0 );

    DEBUG_MSG( "DBApp::initWatchers\n" );

    char    abrv[32];
    bw_snprintf( abrv, sizeof(abrv), "dbapp%02d", id_ );
    BW_REGISTER_WATCHER( id_, abrv, "dbApp",
            mainDispatcher_, this->interface().address() );

    this->addWatchers( Watcher::rootWatcher() );

    initState_ |= INIT_STATE_WATCHERS;

    return true;
}


/**
 *  This method adds the watchers associated with this object.
 */
void DBApp::addWatchers( Watcher & watcher )
{
    this->ScriptApp::addWatchers( watcher );

    watcher.addChild( "id", makeWatcher( &DBApp::id_ ), this );
    watcher.addChild( "isAlpha", makeWatcher( &DBApp::isAlpha ), this );
    watcher.addChild( "isReady", makeWatcher( &DBApp::isReady ), this );

    status_.registerWatchers( watcher );

    // TODO: Consider removing this.
    watcher.addChild( "hasStartBegun",
        makeWatcher( MF_ACCESSORS( bool, DBApp, hasStartBegun ) ), this );

    watcher.addChild( "load", makeWatcher( &DBApp::curLoad_ ), this );

    watcher.addChild( "hasOverloadedCellApps",
            makeWatcher( &DBApp::hasOverloadedCellApps_ ), this );

    watcher.addChild( "dbApps", DBAppsGateway::pWatcher(), &dbApps_ );

    watcher.addChild( "config/db", DBConfig::get().pWatcher(),
        (void*)&DBConfig::get() );
}


/**
 *  This method does any initialisation related to setting up state based on
 *  configuration data in bw.xml.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initConfig()
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );
    MF_ASSERT( id_ != 0 );
    MF_ASSERT( !dbApps_.empty() );

    DEBUG_MSG( "DBApp::initConfig\n" );

    // During recovery the log on records cache will not contain entries
    // for entities already checked out.

    // TODO: We could fix this by having BaseApps tell DBApp Alpha's when they
    // change.  about their persistent entities when they go down and get
    // revived.

    // TODO: Scalable DB: When we are sharding storage, this will need to be
    // done over all shards not just for DBApp Alpha.

    shouldCacheLogOnRecords_ =
        (Config::shouldCacheLogOnRecords() && 
            shouldAlphaResetGameServerState_);

    initState_ |= INIT_STATE_CONFIG;

    return true;
}


/**
 *  This method initialises the reviver system.
 */
bool DBApp::initReviver()
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );
    MF_ASSERT( initState_ & INIT_STATE_NETWORK );

    DEBUG_MSG( "DBApp::initReviver\n" );

    ReviverSubject::instance().init( &interface_, "dbApp" );

    initState_ |= INIT_STATE_REVIVER;

    return true;
}


/**
 *  This method initialises any special game-specific initialisation that is
 *  required.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initGameSpecific()
{
    MF_ASSERT( status_.status() == DBStatus::STARTING );

    DEBUG_MSG( "DBApp::initGameSpecific\n" );

    initState_ |= INIT_STATE_GAME_SPECIFIC;

    return true;
}


/**
 *  This method initialises the DBApp Alpha. This can be called at
 *  initialisation, or when a non-Alpha DBApp is promoted to Alpha.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initDBAppAlpha()
{
    MF_ASSERT( id_ != 0 );
    MF_ASSERT( !dbApps_.empty() );
    MF_ASSERT( this->isAlpha() );

    DEBUG_MSG( "DBApp::initDBAppAlpha: %s resetting of game server state\n",
        shouldAlphaResetGameServerState_ ? "Performing" : "Skipping" );

    if (this->initAcquireDBLock() && this->initBillingSystem())
    {
        if (shouldAlphaResetGameServerState_)
        {
            return this->initResetGameServerState();
        }

        if (status_.status() < DBStatus::WAITING_FOR_APPS)
        {
            return this->initWaitForAppsToBecomeReadyAsync();
        }
        else
        {
            // If this is a promotion then we are already up and 
            // running, so no need to do anything else
            return true;
        }
    }

    return false;
}


/**
 * This method acquires the db lock, so another Bigworld process ( like
 * consolidate_dbs ) won't use the database for exclusive actions ( like
 * consolidate_dbs --clear ) when DBApp Alpha is running.
 */
bool DBApp::initAcquireDBLock()
{
    DEBUG_MSG( "DBApp::initAcquireDBLock\n" );
    return pDatabase_->lockDB();
}


/**
 *  This method initialises the connection with the billing system.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initBillingSystem()
{
    MF_ASSERT( pEntityDefs_ != NULL );
    MF_ASSERT( !dbApps_.empty() );
    MF_ASSERT( this->isAlpha() );

    DEBUG_MSG( "DBApp::initBillingSystem\n" );

    pBillingSystem_ = BillingSystemCreator::createFromConfig( *pEntityDefs_,
        *this );

    return (pBillingSystem_ != NULL);
}


/**
 *  This method resets the game server state, performing any first-time
 *  initialisation that needs to run from DBApp-Alpha, that should only be run
 *  once per server run.
 *
 *  @return false on error, true otherwise.
 */
bool DBApp::initResetGameServerState()
{
    MF_ASSERT( this->isAlpha() );
    MF_ASSERT( shouldAlphaResetGameServerState_ );

    DEBUG_MSG( "DBApp::initResetGameServerState\n" );

    return this->initSecondaryDBsAsync();
    // onSecondaryDBsInitCompleted() is called when completed.
}


/**
 *  This method starts the process for initialising the secondary DBs, and is
 *  asynchronous.
 *
 *  On completion, onSecondaryDBsInitCompleted() will be called on success.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initSecondaryDBsAsync()
{
    MF_ASSERT( pDatabase_ != NULL );
    MF_ASSERT( !dbApps_.empty() );
    MF_ASSERT( this->isAlpha() );

    DEBUG_MSG( "DBApp::initSecondaryDBsAsync\n" );

    if (DBConfig::get().type() == "xml")
    {
        // Nothing to do, secondary databases not supported, skip ahead to the
        // next step.
        return this->onSecondaryDBsInitCompleted();
    }

    initState_ |= INIT_STATE_SECONDARY_DBS;

    this->initSecondaryDBPrefix();

    if (!pDatabase_->shouldConsolidate())
    {
        // If we are recovering from BaseAppMgr death or DBApp Alpha death,
        // then we don't need to try to consolidate.
        return this->onSecondaryDBsInitCompleted();
    }

    this->consolidateData();

    // This will eventually call onSecondaryDBsInitCompleted() via
    // onConsolidateProcessEnd() if successful, or shutdown if a consolidation
    // failure occurs.

    return true;
}


/**
 *  This method initialises the prefix should for the secondary database files.
 */
void DBApp::initSecondaryDBPrefix()
{
    MF_ASSERT( dbAppMgr_.address() != Mercury::Address::NONE );
    MF_ASSERT( id_ != 0 );
    MF_ASSERT( !dbApps_.empty() );
    MF_ASSERT( this->isAlpha() );

    // Generate the run ID.
    // Theoretically, using local time will not generate a unique run ID
    // across daylight savings transitions but good enough.
    time_t epochTime = ::time( NULL );
    tm timeAndDate;
    localtime_r( &epochTime, &timeAndDate );

    // Get username for run ID
    uid_t       uid = getuid();
    passwd *    pUserDetail = getpwuid( uid );
    BW::string username;

    if (pUserDetail)
    {
        username = pUserDetail->pw_name;
    }
    else
    {
        SECONDARYDB_WARNING_MSG( "DBApp::initSecondaryDBPrefix: "
            "Using '%u' as the username due to uid to name lookup failure\n",
            uid );

        BW::stringstream ss;
        ss << uid;
        username = ss.str();
    }

    // Really generate run ID
    char runIDBuf[ BUFSIZ ];
    snprintf( runIDBuf, sizeof(runIDBuf),
        "%s_%04d%02d%02d_%02d%02d%02d",
        username.c_str(),
        timeAndDate.tm_year + 1900, timeAndDate.tm_mon + 1,
        timeAndDate.tm_mday,
        timeAndDate.tm_hour, timeAndDate.tm_min, timeAndDate.tm_sec );

    secondaryDBPrefix_ = runIDBuf;

    SECONDARYDB_INFO_MSG( "DBApp::initSecondaryDBPrefix: \"%s\"\n",
            secondaryDBPrefix_.c_str() );
}


/**
 *  This method continues initialisation after secondary DBs have been
 *  initialised successfully.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::onSecondaryDBsInitCompleted()
{
    if (status_.status() >= DBStatus::RUNNING)
    {
        // We are recovering after a DBApp Alpha failure, and don't need to
        // do anything further here.
        DEBUG_MSG( "DBApp::onSecondaryDBsInitCompleted: "
            "Finished our promotion to DBApp Alpha\n" );
        return true;
    }

    return this->initBaseAppMgrInitData() &&
        this->initDatabaseResetGameServerState() &&
        this->initWaitForAppsToBecomeReadyAsync();
}


/**
 *  This method kicks off sending game init data to BaseAppMgr from the
 *  database.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initBaseAppMgrInitData()
{
    MF_ASSERT( !dbApps_.empty() );
    MF_ASSERT( this->isAlpha() );
    MF_ASSERT( initState_ & INIT_STATE_DATABASE_STARTUP );

    DEBUG_MSG( "DBApp::initBaseAppMgrInitData\n" );

    if (baseAppMgr_.addr() != Mercury::Address::NONE)
    {
        this->sendBaseAppMgrInitData();
    }
    // Otherwise we wait until we hear of BaseAppMgr birth, or we have already
    // sent it, or have already started to send it.

    return true;
}


/**
 *  This method resets per-server-run state in the databases.
 */
bool DBApp::initDatabaseResetGameServerState()
{
    DEBUG_MSG( "DBApp::initDatabaseResetGameServerState\n" );

    return pDatabase_->resetGameServerState();
}


/**
 *  This method starts the waiting for other apps to become ready.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initWaitForAppsToBecomeReadyAsync()
{
    MF_ASSERT( status_.status() < DBStatus::WAITING_FOR_APPS );
    MF_ASSERT( dbAppMgr_.address() != Mercury::Address::NONE );
    MF_ASSERT( id_ != 0 );
    MF_ASSERT( !dbApps_.empty() );
    MF_ASSERT( this->isAlpha() );

    DEBUG_MSG( "DBApp::initWaitForAppsToBecomeReadyAsync\n" );

    status_.set( DBStatus::WAITING_FOR_APPS,
            "Waiting for other components to become ready" );

    // We just need our TIMER_STATUS_CHECK timer.
    return this->initTimers();
}


/**
 *  This method is called when the other requisite server components have
 *  started up, and are waiting for the server to "start".
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::onAppsReady()
{
    // We can be called for non-alpha DBApps.

    if (!this->initScriptAppReady())
    {
        return false;
    }

    if (shouldAlphaResetGameServerState_)
    {
        MF_ASSERT( this->isAlpha() );

        // We need to perform the initialisation that's done once per
        // server-run.
        status_.set( DBStatus::RESTORING_STATE, "Loading game state" );

        return this->initSendSpaceData() &&
            this->initEntityAutoLoadingAsync();
    }

    // No further initialisation to be done, we are not the first instance
    // of DBApp-Alpha to have started for this server run.
    this->onInitCompleted();
    return true;
}


/**
 *  This method tells the script layer that this app is ready.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initScriptAppReady()
{
    MF_ASSERT( status_.status() < DBStatus::RESTORING_STATE );
    MF_ASSERT( dbAppMgr_.address() != Mercury::Address::NONE );
    MF_ASSERT( id_ != 0 );
    MF_ASSERT( !dbApps_.empty() );

    // These are prerequisites for what onAppReady / onDBAppReady should be
    // able to expect at this point.
    MF_ASSERT( Py_IsInitialized() );
    MF_ASSERT( pEntityDefs_ );
    MF_ASSERT( initState_ & INIT_STATE_NETWORK );
    MF_ASSERT( initState_ & INIT_STATE_DATABASE_STARTUP );
    MF_ASSERT( initState_ & INIT_STATE_GAME_SPECIFIC );
    MF_ASSERT( initState_ & INIT_STATE_WATCHERS );

    DEBUG_MSG( "DBApp::initScriptAppReady\n" );

    if (!this->scriptEvents().triggerTwoEvents( "onAppReady", "onDBAppReady",
            PyTuple_New( 0 ) ))
    {
        return false;
    }

    initState_ |= INIT_STATE_SCRIPT_APP_READY;

    return true;
}


/** 
 *  This method loads stored space data from the database to give to CellAppMgr 
 *  (via BaseAppMgr).
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initSendSpaceData()
{
    MF_ASSERT( status_.status() == DBStatus::RESTORING_STATE );
    MF_ASSERT( initState_ & INIT_STATE_DATABASE_STARTUP );
    MF_ASSERT( baseAppMgr_.addr() != Mercury::Address::NONE );

    DEBUG_MSG( "DBApp::initSendSpaceData\n" );

    MemoryOStream spacesData;
    if (!pDatabase_->getSpacesData( spacesData ))
    {
        ERROR_MSG( "DBApp::initSendSpaceData: "
                "Failed to read space data from the database\n" );
        this->startSystemControlledShutdown();

        return false;
    }

    Mercury::Bundle & bundle = baseAppMgr_.bundle();
    bundle.startMessage( BaseAppMgrInterface::spaceDataRestore );
    bundle.transfer( spacesData, spacesData.remainingLength() );
    baseAppMgr_.send();

    initState_ |= INIT_STATE_SPACE_DATA_RESTORE;

    return true;
}

/**
 *  This method initialises the loading of game state from the database. This
 *  includes auto-loaded entities and space data.
 */
bool DBApp::initEntityAutoLoadingAsync()
{
    MF_ASSERT( status_.status() == DBStatus::RESTORING_STATE );
    MF_ASSERT( pEntityDefs_ );
    MF_ASSERT( initState_ & INIT_STATE_DATABASE_STARTUP );
    MF_ASSERT( initState_ & INIT_STATE_SCRIPT_APP_READY );
    MF_ASSERT( initState_ & INIT_STATE_SPACE_DATA_RESTORE );
    MF_ASSERT( !dbApps_.empty() );
    MF_ASSERT( this->isAlpha() );

    DEBUG_MSG( "DBApp::initEntityAutoLoadingAsync\n" );

    initState_ |= INIT_STATE_AUTO_LOADING;

    // When autoLoadEntities() finishes onEntitiesAutoLoadCompleted() or
    // onEntitiesAutoLoadError() will be called, and the EntityAutoLoader
    // instance will be deleted.
    EntityAutoLoader * pAutoLoader = new EntityAutoLoader;
    pDatabase_->autoLoadEntities( *pAutoLoader );

    return true;
}


/**
 *  This method is called by EntityAutoLoader if the entity auto-loading fails.
 */
void DBApp::onEntitiesAutoLoadError()
{
    this->startSystemControlledShutdown();
}


/**
 *  This method is called by EntityAutoLoader if the entity auto-loading has
 *  finished successfully.
 *
 *  @param didAutoLoad  Whether at least one entity was auto-loaded from the
 *                      database.
 */
void DBApp::onEntitiesAutoLoadCompleted( bool didAutoLoad )
{
    if (status_.status() >= DBStatus::SHUTTING_DOWN)
    {
        // We are shutting down, likely waiting for consolidation, no need to
        // continue with initialisation.
        return;
    }

    MF_ASSERT( status_.status() == DBStatus::RESTORING_STATE );
    MF_ASSERT( this->isAlpha() );
    MF_ASSERT( shouldAlphaResetGameServerState_ );

    if (!this->initNotifyServerStartup( didAutoLoad ))
    {
        this->startSystemControlledShutdown();
    }

    this->onInitCompleted();
}


/**
 *  This method signals to the rest of the server that all components have
 *  started, and to start running the game code.
 *
 *  @param didAutoLoad  Whether at least one entity was auto-loaded from the
 *                      database.
 */
bool DBApp::initNotifyServerStartup( bool didAutoLoad )
{
    MF_ASSERT( status_.status() == DBStatus::RESTORING_STATE );
    MF_ASSERT( initState_ & INIT_STATE_AUTO_LOADING );
    MF_ASSERT( shouldAlphaResetGameServerState_ );
    MF_ASSERT( baseAppMgr_.addr() != Mercury::Address::NONE );

    DEBUG_MSG( "DBApp::initNotifyServerStartup: didAutoLoad = %s\n",
        didAutoLoad ? "true" : "false" );

    Mercury::ChannelSender sender( this->baseAppMgr().channel() );

    Mercury::Bundle & bundle = sender.bundle();
    bundle.startMessage( BaseAppMgrInterface::startup );
    bundle << didAutoLoad;

    dbAppMgr_.notifyServerStartupComplete();

    return true;
}


/**
 *  This method sets up required timers.
 *
 *  @return true on success, false otherwise.
 */
bool DBApp::initTimers()
{
    MF_ASSERT( status_.status() < DBStatus::RUNNING );

    // A one second timer to check all sorts of things, including whether to
    // start the server running if we are waiting for other components to
    // be ready.
    statusCheckTimer_ = mainDispatcher_.addTimer( 1000000, this,
            reinterpret_cast< void * >( TIMEOUT_STATUS_CHECK ),
            "StatusCheck" );

    // NOTE: DBApp's time is not synchronised with the rest of the cluster.
    gameTimer_ = mainDispatcher_.addTimer( 1000000/Config::updateHertz(), this,
            reinterpret_cast< void * >( TIMEOUT_GAME_TICK ),
            "GameTick" );

    return true;
}


/**
 *  This method is called when initialisation has completed successfully.
 *
 *  On initialisation failure, the app/server should shutdown as gracefully as
 *  it can.
 */
void DBApp::onInitCompleted()
{
    MF_ASSERT( status_.status() < DBStatus::RUNNING );

    DEBUG_MSG( "DBApp::onInitCompleted\n" );

    // Check that we have done all the initialisation we're supposed to.
    MF_ASSERT( (initState_ & INIT_STATE_NON_ALPHA_MASK) ==
        INIT_STATE_NON_ALPHA_MASK );

    status_.set( DBStatus::RUNNING, "Running" );
}


/**
 *  This method starts the data consolidation process.
 */
void DBApp::consolidateData()
{
    if (status_.status() <= DBStatus::STARTING)
    {
        status_.set( DBStatus::STARTUP_CONSOLIDATING, "Consolidating data" );
    }
    else if (status_.status() >= DBStatus::SHUTTING_DOWN)
    {
        status_.set( DBStatus::SHUTDOWN_CONSOLIDATING, "Consolidating data" );
    }
    else
    {
        SECONDARYDB_CRITICAL_MSG( "DBApp::consolidateData: "
                "Not a valid state to be running data consolidation!" );
        return;
    }

    uint32 numSecondaryDBs = pDatabase_->numSecondaryDBs();
    if (numSecondaryDBs > 0)
    {
        SECONDARYDB_TRACE_MSG( "Starting data consolidation\n" );
        if (!this->startConsolidationProcess())
        {
            this->onConsolidateProcessEnd( false );
        }
    }
    else
    {
        this->onConsolidateProcessEnd( true );
    }
}


/**
 *  This method runs an external command to consolidate data from secondary
 *  databases.
 *
 *  @returns true if a new consolidation process was started, false otherwise.
 */
bool DBApp::startConsolidationProcess()
{
    if (this->isConsolidating())
    {
        SECONDARYDB_TRACE_MSG( "DBApp::startConsolidationProcess: "
                "Ignoring second attempt to consolidate data while data "
                "consolidation is already in progress\n" );
        return false;
    }

    pConsolidator_.reset( new Consolidator( *this ) );
    bool wasConsolidationStarted = pConsolidator_->startConsolidation();
    if (!wasConsolidationStarted)
    {
        SECONDARYDB_ERROR_MSG( "DBApp::startConsolidationProcess: "
            "Failed to start consolidation.\n" );
    }

    return wasConsolidationStarted;
}


/**
 *  This method is called when the consolidation process exits.
 */
void DBApp::onConsolidateProcessEnd( bool isOK )
{
    pConsolidator_.reset();

    if (status_.status() == DBStatus::STARTUP_CONSOLIDATING)
    {
        if (isOK)
        {
            if (!this->onSecondaryDBsInitCompleted())
            {
                this->startSystemControlledShutdown();
            }
        }
        else
        {
            // Prevent trying to consolidate again during controlled shutdown.
            pDatabase_->shouldConsolidate( false );

            this->startSystemControlledShutdown();
        }
    }
    else if (status_.status() == DBStatus::SHUTDOWN_CONSOLIDATING)
    {
        this->shutDown();
    }
    else
    {
        SECONDARYDB_CRITICAL_MSG( "DBApp::onConsolidateProcessEnd: "
                "Invalid state %d at the end of data consolidation\n", 
            status_.status() );
    }
}


/**
 *  This method handles the checkStatus messages request from the LoginApp.
 */
void DBApp::checkStatus( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data )
{
    Mercury::UDPChannel & channel = this->baseAppMgr().channel();

    Mercury::ReplyMessageHandler * pHandler =
            new LoginAppCheckStatusReplyHandler( srcAddr, header.replyID );

    if (channel.isEstablished())
    {
        Mercury::ChannelSender sender( channel );

        sender.bundle().startRequest( BaseAppMgrInterface::checkStatus,
            pHandler );
    }
    else
    {
        Mercury::NubException e( Mercury::REASON_CHANNEL_LOST );
        pHandler->handleException( e, NULL ); //deletes itself
    }
}


/**
 *  This method handles the replies from the checkStatus requests.
 */
void DBApp::handleStatusCheck( BinaryIStream & data )
{
    uint8 isOkay = 0;
    uint32 numBaseApps = 0;
    uint32 numCellApps = 0;
    uint32 numServiceApps = 0;
    data >> isOkay >> numBaseApps >> numServiceApps >> numCellApps;
    INFO_MSG( "DBApp::handleStatusCheck: "
                "baseApps = %u/%u. cellApps = %u/%u, serviceApps = %u/%u\n",
        numBaseApps, Config::desiredBaseApps(),
        numCellApps, Config::desiredCellApps(),
        numServiceApps, Config::desiredServiceApps() );

    // Ignore other status information
    data.finish();

    if ((status_.status() <= DBStatus::WAITING_FOR_APPS) &&
            !data.error() &&
            (numBaseApps >= Config::desiredBaseApps()) &&
            (numCellApps >= Config::desiredCellApps()) &&
            (numServiceApps >= Config::desiredServiceApps()))
    {
        this->onAppsReady();
    }
}


/**
 *  This method handles the checkStatus request's reply.
 */
class CheckStatusReplyHandler : public Mercury::ShutdownSafeReplyMessageHandler
{
public:
    CheckStatusReplyHandler( DBApp & dbApp ) : dbApp_( dbApp ) {}

private:
    virtual void handleMessage( const Mercury::Address & /*srcAddr*/,
            Mercury::UnpackedMessageHeader & /*header*/,
            BinaryIStream & data, void * /*arg*/ )
    {
        dbApp_.handleStatusCheck( data );
        delete this;
    }

    virtual void handleException( const Mercury::NubException & /*ne*/,
        void * /*arg*/ )
    {
        delete this;
    }

    DBApp & dbApp_;
};


// ----------------------------------------------------------------------------
// Section: Secondary DBs 
// ----------------------------------------------------------------------------

/**
 *  This method handles a secondary database registration message.
 */
void DBApp::secondaryDBRegistration( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
{
    IDatabase::SecondaryDBEntry secondaryDBEntry;

    data >> secondaryDBEntry.addr >> secondaryDBEntry.location;
    pDatabase_->addSecondaryDB( secondaryDBEntry );
}


/**
 *  This method handles an update secondary database registration message.
 *  Secondary databases registered by a BaseApp not in the provided list are
 *  deleted.
 */
void DBApp::updateSecondaryDBs( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
{
    SecondaryDBAddrs addrs;
    data >> addrs;

    pDatabase_->updateSecondaryDBs( addrs, *this );
    // updateSecondaryDBs() calls onUpdateSecondaryDBsComplete() it completes.
}


/**
 *  This method handles the request to get information for creating a new
 *  secondary database. It replies with the name of the new database.
 */
void DBApp::getSecondaryDBDetails( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
{
    Mercury::ChannelSender sender( DBApp::getChannel( srcAddr ) );
    Mercury::Bundle & bundle = sender.bundle();
    bundle.startReply( header.replyID );

    if ((initState_ & INIT_STATE_SECONDARY_DBS) == 0)
    {
        // An empty string indicates that secondary databases are disabled
        bundle << "";
        return;
    }

    MF_ASSERT( !secondaryDBPrefix_.empty() );

    char buf[ BUFSIZ ];
    ++secondaryDBIndex_;
    snprintf( buf, sizeof( buf ), "%s-%d.db",
            secondaryDBPrefix_.c_str(),
            secondaryDBIndex_ );

    bundle << buf;
}


/**
 *  Deletes secondary databases whose registration have just been removed.
 */
void DBApp::onUpdateSecondaryDBsComplete(
        const IDatabase::SecondaryDBEntries& removedEntries )
{
    for (IDatabase::SecondaryDBEntries::const_iterator pEntry =
            removedEntries.begin(); pEntry != removedEntries.end(); ++pEntry )
    {
        if (this->sendRemoveDBCmd( pEntry->addr.ip, pEntry->location ))
        {
            SECONDARYDB_TRACE_MSG( "DBApp::onUpdateSecondaryDBsComplete: "
                    "Deleting secondary database file %s on %s\n",
                pEntry->location.c_str(), pEntry->addr.ipAsString() );
        }
        else
        {
            SECONDARYDB_ERROR_MSG( "DBApp::onUpdateSecondaryDBsComplete: "
                    "Failed to delete secondary database file %s on %s. It "
                    "should be manually deleted to prevent disk space "
                    "exhaustion.\n",
                pEntry->location.c_str(), pEntry->addr.ipAsString() );
        }
    }
}


/**
 *  This method handles an update from DBAppMgr when the DB App hash has
 *  changed.
 */
void DBApp::updateDBAppHash( BinaryIStream & data )
{
    const bool wasAlpha = !dbApps_.empty() && this->isAlpha();

    uint8 shouldAlphaResetGameServerState = 0;
    data >> shouldAlphaResetGameServerState;

    if (!dbApps_.updateFromStream( data ))
    {
        CRITICAL_MSG( "DBApp::updateDBAppHash: "
            "Failed to de-stream DBApp hash\n" );
    }

    if (!wasAlpha && (id_ == dbApps_.alpha().id()))
    {
        INFO_MSG( "DBApp::updateDBAppHash: Promoting to DBApp Alpha\n" );

        shouldAlphaResetGameServerState_ = shouldAlphaResetGameServerState;

        this->initDBAppAlpha();
    }
}


/**
 *  This method sends a message to the destination BWMachined that will cause
 *  the database at dbLocation to be removed.
 */
bool DBApp::sendRemoveDBCmd( uint32 destIP, const BW::string & dbLocation )
{
    CreateWithArgsMessage cm;
    cm.uid_ = getUserId();
    cm.config_ = BW_COMPILE_TIME_CONFIG;
    cm.recover_ = 0;
    cm.name_ = "commands/remove_db";
    cm.fwdIp_ = 0;
    cm.fwdPort_ = 0;

    cm.args_.push_back( dbLocation );

    Endpoint ep;
    ep.socket( SOCK_DGRAM );

    return (ep.good() && (ep.bind() == 0) &&
            cm.sendto( ep, htons( PORT_MACHINED ), destIP ));
}


/**
 *  This method handles timer events. It is called every second.
 */
void DBApp::handleTimeout( TimerHandle handle, void * arg )
{
    switch (reinterpret_cast< uintptr >( arg ))
    {
    case TIMEOUT_GAME_TICK:
        this->advanceTime();
        break;

    case TIMEOUT_STATUS_CHECK:
        this->checkStatus();
        break;
    }
}


/**
 *  This method handles the timer event to check the DBApp status.
 */
void DBApp::checkStatus()
{
    // See if we are ready to start.
    if (baseAppMgr_.channel().isEstablished() &&
            (status_.status() == DBStatus::WAITING_FOR_APPS))
    {
        Mercury::Bundle & bundle = baseAppMgr_.bundle();

        bundle.startRequest( BaseAppMgrInterface::checkStatus,
               new CheckStatusReplyHandler( *this ) );

        baseAppMgr_.send();

        mainDispatcher_.clearSpareTime();
    }

    // Update our current load so we can know whether or not we are overloaded.
    if (status_.status() > DBStatus::WAITING_FOR_APPS)
    {
        uint64 spareTime = mainDispatcher_.getSpareTime();
        mainDispatcher_.clearSpareTime();

        curLoad_ = 1.f - float( double(spareTime) / stampsPerSecondD() );

        // TODO: Consider asking DB implementation if it is overloaded too...
    }

    // Check whether we should end our remapping of mailboxes for a dead BaseApp
    if (--mailboxRemapCheckCount_ == 0)
    {
        this->endMailboxRemapping();
    }

    this->checkPendingLoginAttempts();

    if (pDatabase_->hasUnrecoverableError())
    {
        ERROR_MSG( "DBApp::checkStatus: "
                "Found an unrecoverable error.\n" );
        this->startSystemControlledShutdown();
    }
}


/**
 *  This method checks whether there are any relogin attempts that are too old.
 *  This is attempting to track down a problem where it looks as though entries
 *  are being left in the pendingAttempts_ table.
 */
void DBApp::checkPendingLoginAttempts()
{
    static double maxPendingLoginAge =
        BWConfig::get( "dbApp/maxPendingLoginAge", 60.0 );

    PendingAttempts::iterator iter = pendingAttempts_.begin();

    while (iter != pendingAttempts_.end())
    {
        EntityKey entityKey = iter->first;
        double age = iter->second->ageInSeconds();

        // Iterate first because we may remove the current element
        ++iter;

        if (age > maxPendingLoginAge)
        {
            ERROR_MSG( "DBApp::checkPendingLoginAttempts: Removing login "
                    "attempt for dbID %" FMT_DBID " of age %.2f seconds\n",
                entityKey.dbID, age );
            this->onCompleteRelogonAttempt( entityKey.typeID,
                    entityKey.dbID );
        }
    }
}


// -----------------------------------------------------------------------------
// Section: Manager lifetimes
// -----------------------------------------------------------------------------


/**
 *  This method is called when a new BaseAppMgr is started.
 */
void DBApp::handleBaseAppMgrBirth( 
        const DBAppInterface::handleBaseAppMgrBirthArgs & args )
{
    baseAppMgr_.addr( args.addr );

    PROC_IP_INFO_MSG( "DBApp::handleBaseAppMgrBirth: BaseAppMgr is at %s\n",
        baseAppMgr_.c_str() );

    if (id_ != 0 && this->isAlpha() )
    {
        // If we haven't got an ID yet, we don't know whether we are Alpha or
        // whether the Alpha should reset game server state - this is deferred
        // until onSecondaryDBsInitCompleted().

        this->sendBaseAppMgrInitData();
    }
}


/**
 *  This method is called when a new DBAppMgr is started.
 */
void DBApp::handleDBAppMgrBirth( 
        const DBAppInterface::handleDBAppMgrBirthArgs & args )
{
    INFO_MSG( "DBApp::handleDBAppMgrBirth( id = %d ): %s\n",
        id_, args.addr.c_str() );
    dbAppMgr_.address( args.addr );
    dbAppMgr_.recoverDBApp( id_ );
}


/**
 *  This method is called when a DBAppMgr dies.
 */
void DBApp::handleDBAppMgrDeath( 
        const DBAppInterface::handleDBAppMgrDeathArgs & args )
{
    if (dbAppMgr_.address() != args.addr)
    {
        // Ignore - the new one may have taken over already.
        return;
    }

    dbAppMgr_.address( Mercury::Address::NONE );

    NOTICE_MSG( "DBApp::handleDBAppMgrDeath( id = %d ): %s\n",
        id_, args.addr.c_str() );
}


/**
 *  This method handles the shutDown message.
 */
void DBApp::shutDown( const DBAppInterface::shutDownArgs & /*args*/ )
{
    this->shutDown();
}


/**
 *  This method starts a controlled shutdown for the entire system.
 */
void DBApp::startSystemControlledShutdown()
{
    if (baseAppMgr_.channel().isEstablished())
    {
        BaseAppMgrInterface::controlledShutDownArgs args;
        args.stage = SHUTDOWN_TRIGGER;
        args.shutDownTime = 0;
        baseAppMgr_.bundle() << args;
        baseAppMgr_.send();
    }
    else
    {
        WARNING_MSG( "DBApp::startSystemControlledShutdown: "
                "No known BaseAppMgr, only shutting down self\n" );
        this->shutDownNicely();
    }
}


/**
 *  This method starts shutting down DBApp.
 */
void DBApp::shutDownNicely()
{
    if (status_.status() >= DBStatus::SHUTTING_DOWN)
    {
        WARNING_MSG( "DBApp::shutDownNicely: Ignoring second shutdown\n" );
        return;
    }

    TRACE_MSG( "DBApp::shutDownNicely: Shutting down\n" );

    status_.set( DBStatus::SHUTTING_DOWN, "Shutting down" );

    interface_.processUntilChannelsEmpty();

    if (this->isAlpha() && pDatabase_ && pDatabase_->shouldConsolidate())
    {
        // TODO: Scalable DB: When we shard storage, this will be required
        // for each shard not just for DBApp Alpha.
        this->consolidateData();
    }
    else
    {
        this->shutDown();
    }
}


/**
 *  This method shuts this process down.
 */
void DBApp::shutDown()
{
    TRACE_MSG( "DBApp::shutDown\n" );

    mainDispatcher_.breakProcessing();
}


/**
 *  This method handles telling us to shut down in a controlled manner.
 */
void DBApp::controlledShutDown(
        const DBAppInterface::controlledShutDownArgs & args )
{
    DEBUG_MSG( "DBApp::controlledShutDown: stage = %s\n", 
        ServerApp::shutDownStageToString( args.stage ) );

    // Make sure we no longer send to anonymous channels etc.
    interface_.stopPingingAnonymous();

    switch (args.stage)
    {
    case SHUTDOWN_REQUEST:
    {
        dbAppMgr_.requestControlledShutDown();

        break;
    }

    case SHUTDOWN_PERFORM:
        this->shutDownNicely();
        break;

    default:
        ERROR_MSG( "DBApp::controlledShutDown: Stage %s not handled.\n",
            ServerApp::shutDownStageToString( args.stage ) );
        break;
    }
}


/**
 *  This method is run when the runloop has finished.
 */
void DBApp::onRunComplete()
{
    this->ScriptApp::onRunComplete();
    this->finalise();
}


/**
 *  This method performs some clean-up at the end of the shut down process.
 */
void DBApp::finalise()
{
    if (pDatabase_)
    {
        pDatabase_->shutDown();
    }
}


/**
 *  This method handles telling us that a CellApp is overloaded.
 */
void DBApp::cellAppOverloadStatus( 
        const DBAppInterface::cellAppOverloadStatusArgs & args )
{
    hasOverloadedCellApps_ = args.hasOverloadedCellApps;
}


// -----------------------------------------------------------------------------
// Section: IDatabase overrides
// -----------------------------------------------------------------------------

/**
 *  This method is meant to be called instead of IDatabase::getEntity() so that
 *  we can muck around with stuff before passing it to IDatabase.
 */
void DBApp::getEntity( const EntityDBKey & entityKey,
            BinaryOStream * pStream,
            bool shouldGetBaseEntityLocation,
            GetEntityHandler & handler )
{
    pDatabase_->getEntity( entityKey, pStream, shouldGetBaseEntityLocation,
            handler );
}


/**
 *  This method is meant to be called instead of IDatabase::putEntity() so that
 *  we can muck around with stuff before passing it to IDatabase.
 */
void DBApp::putEntity( const EntityKey & entityKey,
        EntityID entityID,
        BinaryIStream * pStream,
        EntityMailBoxRef * pBaseMailbox,
        bool removeBaseMailbox,
        bool putExplicitID,
        UpdateAutoLoad updateAutoLoad,
        IDatabase::IPutEntityHandler& handler )
{
    // Update mailbox for dead BaseApps.
    if (this->hasMailboxRemapping() && pBaseMailbox)
    {
        // Update mailbox for dead BaseApps.
        this->remapMailbox( *pBaseMailbox );
    }

    pDatabase_->putEntity( entityKey, entityID,
            pStream, pBaseMailbox, removeBaseMailbox, 
            putExplicitID, updateAutoLoad, handler );
}


/**
 *  This method is meant to be called instead of IDatabase::delEntity() so that
 *  we can muck around with stuff before passing it to IDatabase.
 */
void DBApp::delEntity( const EntityDBKey & ekey, EntityID entityID,
        IDatabase::IDelEntityHandler& handler )
{
    pDatabase_->delEntity( ekey, entityID, handler );
}


// -----------------------------------------------------------------------------
// Section: Entity entry database requests
// -----------------------------------------------------------------------------

/**
 *  This method handles a logOn request.
 */
void DBApp::logOn( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data )
{
    Mercury::Address addrForProxy;
    LogOnParamsPtr pParams = new LogOnParams();

    data >> addrForProxy >> *pParams;

    if (pParams->digest() != this->getEntityDefs().getDigest())
    {
        ERROR_MSG( "DBApp::logOn: Incorrect digest\n" );
        this->sendFailure( header.replyID, srcAddr,
            LogOnStatus::LOGIN_REJECTED_BAD_DIGEST,
            "Defs digest mismatch." );
        return;
    }

    this->logOn( srcAddr, header.replyID, pParams, addrForProxy );
}


/**
 *  This method attempts to log on a player.
 */
void DBApp::logOn( const Mercury::Address & srcAddr,
        Mercury::ReplyID replyID,
        LogOnParamsPtr pParams,
        const Mercury::Address & addrForProxy )
{
    if (status_.status() != DBStatus::RUNNING)
    {
        INFO_MSG( "DBApp::logOn: "
            "Login failed for %s. Server not ready.\n",
            pParams->username().c_str() );

        this->sendFailure( replyID, srcAddr,
            LogOnStatus::LOGIN_REJECTED_SERVER_NOT_READY,
            "Server not ready." );

        return;
    }

    if (!pBillingSystem_)
    {
        // We haven't finished registering with DBAppMgr yet, or we have not
        // finished recovering after a DBApp Alpha death.
        
        this->sendFailure( replyID, srcAddr,
            LogOnStatus::LOGIN_REJECTED_SERVER_NOT_READY,
            "Server not ready." );

        return;
    }


    bool isOverloaded = curLoad_ > LoginConditionsConfig::maxLoad();

    if (this->calculateOverloaded( isOverloaded ))
    {
        INFO_MSG( "DBApp::logOn: "
                    "Login failed for %s. We are overloaded "
                    "(load=%.02f > max=%.02f)\n",
                    pParams->username().c_str(),
                    curLoad_,
                    LoginConditionsConfig::maxLoad() );

        this->sendFailure( replyID, srcAddr,
                            LogOnStatus::LOGIN_REJECTED_DBAPP_OVERLOAD,
                            "DBApp is overloaded." );

        return;
    }

    if (hasOverloadedCellApps_)
    {
        INFO_MSG( "DBApp::logOn: "
            "Login failed for %s. At least one CellApp is overloaded.\n",
            pParams->username().c_str() );

        this->sendFailure( replyID, srcAddr,
            LogOnStatus::LOGIN_REJECTED_CELLAPP_OVERLOAD,
            "At least one CellApp is overloaded." );

        return;
    }

    LoginHandler * pHandler =
        new LoginHandler( pParams, addrForProxy, srcAddr, replyID );

    pHandler->login();
}


/**
 *  This method performs checks to see whether we should see ourselves as being
 *  overloaded.
 */
bool DBApp::calculateOverloaded( bool isOverloaded )
{
    if (!isOverloaded)
    {
        // We're not overloaded, stop the overload timer.
        overloadStartTime_ = 0;
        return false;
    }

    uint64 overloadTime;

    // Start rate limiting logins
    if (overloadStartTime_ == 0)
    {
        overloadStartTime_ = timestamp();
    }

    overloadTime = timestamp() - overloadStartTime_;
    INFO_MSG( "DBApp::calculateOverloaded: Overloaded for %" PRIu64 "ms\n",
                overloadTime/(stampsPerSecond()/1000) );

    return (overloadTime >=
            LoginConditionsConfig::overloadTolerancePeriodInStamps());
}


/**
 *  This method is called when there is a log on request for an entity that is
 *  already logged on.
 */
void DBApp::onLogOnLoggedOnUser( EntityTypeID typeID, DatabaseID dbID,
    LogOnParamsPtr pParams,
    const Mercury::Address & clientAddr, const Mercury::Address & replyAddr,
    Mercury::ReplyID replyID, const EntityMailBoxRef * pExistingBase,
    const BW::string & dataForClient, const BW::string & dataForBaseEntity )
{
    // TODO: Could add a config option to speedup denial of relogon attempts
    //       eg <shouldAttemptRelogon>

    if (this->getInProgRelogonAttempt( typeID, dbID ) != NULL)
    {
        // Another re-logon already in progress.
        INFO_MSG( "DBApp::onLogOnLoggedOnUser: %s already logged on\n",
            pParams->username().c_str() );

        this->sendFailure( replyID, replyAddr,
                LogOnStatus::LOGIN_REJECTED_ALREADY_LOGGED_IN,
                "A relogin of same name still in progress." );
        return;
    }

    if (!GetEntityHandler::isActiveMailBox( pExistingBase ))
    {
        // Another logon still in progress.
        WARNING_MSG( "DBApp::onLogOnLoggedOnUser: %s already logging in\n",
            pParams->username().c_str() );

        this->sendFailure( replyID, replyAddr,
            LogOnStatus::LOGIN_REJECTED_ALREADY_LOGGED_IN,
           "Another login of same name still in progress." );
        return;
    }

    INFO_MSG( "DBApp::onLogOnLoggedOnUser: name = %s. databaseID = "
                "%" FMT_DBID ". typeID = %d. entityID = %d. BaseApp = %s\n",
                pParams->username().c_str(),
                dbID,
                typeID,
                pExistingBase->id,
                pExistingBase->addr.c_str() );

    // Log on to existing base
    Mercury::ChannelSender sender(
        DBApp::getChannel( pExistingBase->addr ) );

    Mercury::Bundle & bundle = sender.bundle();
    bundle.startRequest( BaseAppIntInterface::logOnAttempt,
        new RelogonAttemptHandler( pExistingBase->type(), dbID,
            replyAddr, replyID, pParams, clientAddr, dataForClient ) );

    bundle << pExistingBase->id;
    bundle << clientAddr;
    bundle << pParams->encryptionKey();
    bundle << dataForBaseEntity;
}


/*
 *  This method creates a default entity (via createNewEntity() in custom.cpp)
 *  and serialises it into the stream.
 *
 *  @param  The type of entity to create.
 *  @param  The name of the entity (for entities with a name property)
 *  @param  The stream to serialise entity into.
 *  @return True if successful.
 */
bool DBApp::defaultEntityToStrm( EntityTypeID typeID,
    const BW::string & name, BinaryOStream & strm ) const
{
    DataSectionPtr pSection = createNewEntity( typeID, name );
    bool isCreated = pSection.exists();
    if (isCreated)
    {
        const EntityDescription& desc =
            this->getEntityDefs().getEntityDescription( typeID );
        desc.addSectionToStream( pSection, strm,
            EntityDescription::BASE_DATA | EntityDescription::CELL_DATA |
            EntityDescription::ONLY_PERSISTENT_DATA );

        if (desc.canBeOnCell())
        {
            Vector3 defaultVec( 0, 0, 0 );

            strm << defaultVec; // position
            strm << defaultVec; // direction
            strm << SpaceID(0); // space ID
        }
    }

    return isCreated;
}


/*
 *  This method inserts the "header" info into the bundle for a
 *  BaseAppMgrInterface::createEntity message, up till the point where entity
 *  properties should begin.
 *
 *  @return If dbID is 0, then this function returns the position in the
 *  bundle where you should put the DatabaseID.
 */
DatabaseID* DBApp::prepareCreateEntityBundle( EntityTypeID typeID,
        DatabaseID dbID, const Mercury::Address & addrForProxy,
        Mercury::ReplyMessageHandler * pHandler, Mercury::Bundle & bundle,
        LogOnParamsPtr pParams, BW::string * pDataForBaseEntity )
{
    bundle.startRequest( BaseAppMgrInterface::createEntity, pHandler, 0,
        Mercury::DEFAULT_REQUEST_TIMEOUT + 1000000 ); // 1 second extra

    // This data needs to match BaseAppMgr::createBaseWithCellData.
    bundle  << EntityID( 0 ) << typeID;

    DatabaseID *    pDbID = NULL;

    if (dbID)
    {
        bundle << dbID;
    }
    else
    {
        pDbID = reinterpret_cast< DatabaseID * >(
                    bundle.reserve( sizeof( *pDbID ) ) );
    }

    // This is the client address. It is used if we are making a proxy.
    bundle << addrForProxy;

    bundle << ((pParams != NULL) ? pParams->encryptionKey() : "");

    if (pDataForBaseEntity == NULL)
    {
        bundle << false;
    }
    else
    {
        bundle << true;
        bundle << *pDataForBaseEntity;
    }

    bundle << true;     // Has persistent data only

    return pDbID;
}


/**
 *  This helper method sends a failure reply.
 */
void DBApp::sendFailure( Mercury::ReplyID replyID,
        const Mercury::Address & dstAddr,
        LogOnStatus reason, const char * pDescription )
{
    MF_ASSERT( reason != LogOnStatus::LOGGED_ON );

    Mercury::AutoSendBundle spBundle( this->interface(), dstAddr );

    Mercury::Bundle & bundle = spBundle;

    bundle.startReply( replyID );
    bundle << uint8( reason.value() );

    if (pDescription == NULL)
    {
        pDescription = UNSPECIFIED_ERROR_STR;
    }

    bundle << pDescription;
}


/**
 *  This method handles the writeEntity mercury message.
 */
void DBApp::writeEntity( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data )
{
    AUTO_SCOPED_PROFILE( "writeEntity" );

    WriteDBFlags flags;
    data >> flags;
    // if this fails then the calling component had no need to call us
    MF_ASSERT( flags &
            (WRITE_BASE_CELL_DATA | WRITE_LOG_OFF | WRITE_AUTO_LOAD_MASK) );

    EntityDBKey ekey( 0, 0 );
    data >> ekey.typeID >> ekey.dbID;

    // TRACE_MSG( "DBApp::writeEntity: %lld flags=%i\n",
    //         ekey.dbID, flags );

    bool isOkay = this->getEntityDefs().isValidEntityType( ekey.typeID );

    if (!isOkay)
    {
        ERROR_MSG( "DBApp::writeEntity: Invalid entity type %d\n",
            ekey.typeID );

        if (header.replyID != Mercury::REPLY_ID_NONE)
        {
            Mercury::ChannelSender sender( DBApp::getChannel( srcAddr ) );
            sender.bundle().startReply( header.replyID );
            sender.bundle() << isOkay << ekey.dbID;
        }
    }
    else
    {
        EntityID entityID;
        data >> entityID;

        WriteEntityHandler* pHandler =
            new WriteEntityHandler( ekey, entityID, flags,
                header.replyID, srcAddr );

        if (flags & WRITE_DELETE_FROM_DB)
        {
            pHandler->deleteEntity();
        }
        else
        {
            pHandler->writeEntity( data, entityID );
        }
    }
}


/**
 *  This method is called when we've just logged off an entity.
 *
 *  @param  typeID The type ID of the logged off entity.
 *  @param  dbID The database ID of the logged off entity.
 */
void DBApp::onEntityLogOff( EntityTypeID typeID, DatabaseID dbID )
{
    // Notify any re-logon handler waiting on this entity that it has gone.
    RelogonAttemptHandler * pHandler =
            this->getInProgRelogonAttempt( typeID, dbID );

    if (pHandler)
    {
        pHandler->onEntityLogOff();
    }
}


/**
 *  This method handles a message to authenticate a username and password.
 */
void DBApp::authenticateAccount( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data )
{
    AuthenticateAccountHandler::handle( srcAddr, header, data,
            *pBillingSystem_, this->interface() );
}


/**
 *  This method handles a message to load an entity from the database.
 */
void DBApp::loadEntity( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & input )
{
    EntityDBKey ekey( 0, 0 );
    DataSectionPtr pSection;
    bool byName;
    EntityID entityID;
    input >> ekey.typeID >> entityID >> byName;

    if (!this->getEntityDefs().isValidEntityType( ekey.typeID ))
    {
        ERROR_MSG( "DBApp::loadEntity: Invalid entity type %d\n",
            ekey.typeID );
        this->sendFailure( header.replyID, srcAddr,
            LogOnStatus::LOGIN_CUSTOM_DEFINED_ERROR,
            "Invalid entity type" );
        return;
    }

    if (byName)
    {
        if (this->getEntityDefs().getEntityDescription( ekey.typeID ).pIdentifier() == NULL)
        {
            ERROR_MSG( "DBApp::loadEntity: Attempted to load entity type "
                "%d using identifier but no Identifier field is defined.\n",
                ekey.typeID );
            this->sendFailure( header.replyID, srcAddr,
                            LogOnStatus::LOGIN_REJECTED_DB_GENERAL_FAILURE,
                            "No identifier field for entity type" );
            input.finish();
            return;
        }

        input >> ekey.name;
    }
    else
    {
        input >> ekey.dbID;
    }

    LoadEntityHandler * pHandler =
        new LoadEntityHandler( ekey, srcAddr, entityID, header.replyID );
    pHandler->loadEntity();
}


/**
 *  This message deletes the specified entity if it exists and is not checked
 *  out.
 *
 *  On success, an empty reply is sent. If the entity is currently active, its
 *  mailbox is returned. If the entity did not exist, -1 is return as int32.
 */
void DBApp::deleteEntity( const Mercury::Address & srcAddr,
    const Mercury::UnpackedMessageHeader & header,
    const DBAppInterface::deleteEntityArgs & args )
{
    DeleteEntityHandler* pHandler = new DeleteEntityHandler( args.entityTypeID,
            args.dbid, srcAddr, header.replyID );
    pHandler->deleteEntity();
}


/**
 *  This message looks up the specified entity if it exists and is checked
 *  out and returns a mailbox to it. If it is not checked out it returns
 *  an empty message. If it does not exist, it returns -1 as an int32.
 */
void DBApp::lookupEntity( const Mercury::Address & srcAddr,
    const Mercury::UnpackedMessageHeader & header,
    const DBAppInterface::lookupEntityArgs & args )
{
    LookUpEntityHandler* pHandler = new LookUpEntityHandler( srcAddr,
            header.replyID );
    pHandler->lookUpEntity( args.entityTypeID, args.dbid );
}


/**
 *  This message looks up the specified entity if it exists and is checked
 *  out and returns a mailbox to it. If it is not checked out it returns
 *  an empty message. If it does not exist, it returns -1 as an int32.
 */
void DBApp::lookupEntityByName( const Mercury::Address & srcAddr,
    const Mercury::UnpackedMessageHeader & header,
    BinaryIStream & data )
{
    EntityTypeID    entityTypeID;
    BW::string      name;
    data >> entityTypeID >> name;
    LookUpEntityHandler* pHandler = new LookUpEntityHandler(
        srcAddr, header.replyID );
    pHandler->lookUpEntity( entityTypeID, name );
}


/**
 *  This message looks up the DBID of the entity. The DBID will be 0 if the
 *  entity does not exist.
 */
void DBApp::lookupDBIDByName( const Mercury::Address & srcAddr,
    const Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
{
    EntityTypeID    entityTypeID;
    BW::string      name;
    data >> entityTypeID >> name;

    LookUpDBIDHandler* pHandler =
                            new LookUpDBIDHandler( srcAddr, header.replyID );
    pHandler->lookUpDBID( entityTypeID, name );
}


/**
 *  This message (expected to be a request) looks up entities based on matching
 *  a property value with a query string.
 *
 *  The reply consists of the number of entities (int) followed by the database
 *  ID and mailbox location for each entity. The mailbox IP address and entity
 *  ID will be 0 to indicate that it has not been checked out.
 *
 *  On error, -1 is passed as the number of entities.
 */
void DBApp::lookupEntities( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data )
{
    EntityTypeID entityTypeID;
    LookUpEntitiesCriteria criteria;

    data >> entityTypeID >> criteria;

    LookUpEntitiesHandler * pHandler = 
        new LookUpEntitiesHandler( *this, srcAddr, header.replyID );

    pHandler->lookUpEntities( entityTypeID, criteria );
}


// -----------------------------------------------------------------------------
// Section: Miscellaneous database requests
// -----------------------------------------------------------------------------

/**
 *  This class represents a request to execute a raw database command.
 */
class ExecuteRawCommandHandler : public IDatabase::IExecuteRawCommandHandler
{
public:
    ExecuteRawCommandHandler( DBApp & dbApp, const Mercury::Address srcAddr,
            Mercury::ReplyID replyID ) :
        dbApp_( dbApp ), replyBundle_(), srcAddr_(srcAddr)
    {
        replyBundle_.startReply(replyID);
    }
    virtual ~ExecuteRawCommandHandler() {}

    void executeRawCommand( const BW::string& command )
    {
        dbApp_.getIDatabase().executeRawCommand( command, *this );
    }

    // IDatabase::IExecuteRawCommandHandler overrides
    virtual BinaryOStream& response()   {   return replyBundle_;    }
    virtual void onExecuteRawCommandComplete()
    {
        DBApp::getChannel( srcAddr_ ).send( &replyBundle_ );

        delete this;
    }

private:
    DBApp & dbApp_;

    Mercury::UDPBundle      replyBundle_;
    Mercury::Address    srcAddr_;
};


/**
 *  This method executaes a raw database command specific to the present
 *  implementation of the database interface.
 */
void DBApp::executeRawCommand( const Mercury::Address & srcAddr,
    const Mercury::UnpackedMessageHeader & header,
    BinaryIStream & data )
{
    BW::string command( (char*)data.retrieve( header.length ), header.length );
    ExecuteRawCommandHandler* pHandler =
        new ExecuteRawCommandHandler( *this, srcAddr, header.replyID );
    pHandler->executeRawCommand( command );
}

/**
 *  This method stores some previously used ID's into the database
 */
void DBApp::putIDs( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream& input )
{
    int numIDs = input.remainingLength() / sizeof(EntityID);
    INFO_MSG( "DBApp::putIDs: storing %d id's\n", numIDs);
    pDatabase_->putIDs( numIDs,
            static_cast< const EntityID * >(
                input.retrieve( input.remainingLength() ) ) );
}

/**
 *  This class represents a request to get IDs from the database.
 */
class GetIDsHandler : public IDatabase::IGetIDsHandler
{
public:
    GetIDsHandler( DBApp & dbApp, const Mercury::Address & srcAddr,
            Mercury::ReplyID replyID ) :
        dbApp_( dbApp ), srcAddr_(srcAddr), replyID_( replyID ), replyBundle_()
    {
        replyBundle_.startReply( replyID );
    }
    virtual ~GetIDsHandler() {}

    void getIDs( int numIDs )
    {
        dbApp_.getIDatabase().getIDs( numIDs, *this );
    }

    virtual BinaryOStream& idStrm() { return replyBundle_; }
    virtual void resetStrm()
    {
        replyBundle_.clear();
        replyBundle_.startReply( replyID_ );
    }
    virtual void onGetIDsComplete()
    {
        DBApp::getChannel( srcAddr_ ).send( &replyBundle_ );
        delete this;
    }

private:
    DBApp & dbApp_;
    Mercury::Address    srcAddr_;
    Mercury::ReplyID    replyID_;
    Mercury::UDPBundle  replyBundle_;

};


/**
 *  This methods grabs some more ID's from the database
 */
void DBApp::getIDs( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & input )
{
    int numIDs;
    input >> numIDs;
    INFO_MSG( "DBApp::getIDs: fetching %d id's for %s\n",
            numIDs, srcAddr.c_str() );

    GetIDsHandler * pHandler =
        new GetIDsHandler( *this, srcAddr, header.replyID );
    pHandler->getIDs( numIDs );
}


/**
 *  This method writes information about the spaces to the database.
 */
void DBApp::writeSpaces( const Mercury::Address & /*srcAddr*/,
        const Mercury::UnpackedMessageHeader & /*header*/,
        BinaryIStream & data )
{
    pDatabase_->writeSpaceData( data );
}


/**
 *  This method handles a message from the BaseAppMgr informing us that a
 *  BaseApp has died.
 */
void DBApp::handleBaseAppDeath( BinaryIStream & data )
{
    if (!this->isAlpha())
    {
        // TODO: Scalable DB: Non-alpha DBApps will need to remap mailboxes
        // when we shard the storage.
        data.finish();
        return;
    }

    Mercury::Address remappingSrcAddr;
    data >> remappingSrcAddr;

    uint8 isServiceApp;
    data >> isServiceApp;

    BackupHash & remappingDstAddrs = mailboxRemapInfo_[ remappingSrcAddr ];
    data >> remappingDstAddrs;

    INFO_MSG( "DBApp::handleBaseAppDeath: %s\n", remappingSrcAddr.c_str() );

    pDatabase_->remapEntityMailboxes( remappingSrcAddr, remappingDstAddrs);

    if (shouldCacheLogOnRecords_)
    {
        logOnRecordsCache_.remapMailboxes( remappingSrcAddr,
                remappingDstAddrs );
    }

    mailboxRemapCheckCount_ = 5;    // do remapping for 5 seconds.
}


/**
 *  This method ends the mailbox remapping for a dead BaseApp.
 */
void DBApp::endMailboxRemapping()
{
//  DEBUG_MSG( "DBApp::endMailboxRemapping: End of handleBaseAppDeath\n" );
    mailboxRemapInfo_.clear();
}


/**
 *  This method changes the address of input mailbox to cater for recent
 *  BaseApp death.
 */
void DBApp::remapMailbox( EntityMailBoxRef & mailbox ) const
{
    MailboxRemapInfo::const_iterator found = 
        mailboxRemapInfo_.find( mailbox.addr );

    if ( found != mailboxRemapInfo_.end() )
    {
        const Mercury::Address& newAddr =
                found->second.addressFor( mailbox.id );
        // Mercury::Address::salt must not be modified.
        mailbox.addr.ip = newAddr.ip;
        mailbox.addr.port = newAddr.port;
    }
}


/**
 *  This method writes the game time to the database.
 */
void DBApp::writeGameTime( const DBAppInterface::writeGameTimeArgs & args )
{
    pDatabase_->setGameTime( args.gameTime );
}


/**
 *  Gathers initialisation data to send to BaseAppMgr
 */
void DBApp::sendBaseAppMgrInitData()
{
    // NOTE: Due to the asynchronous call, if two BaseAppMgrs register in
    // quick succession then we'll end up sending the init data twice to the
    // second BaseAppMgr.
    // onGetBaseAppMgrInitDataComplete() will be called.
    pDatabase_->getBaseAppMgrInitData( *this );
}


/**
 *  Sends initialisation data to BaseAppMgr
 */
void DBApp::onGetBaseAppMgrInitDataComplete( GameTime gameTime )
{
    // Once BaseAppMgr receives initData, it can start adding BaseApps, so 
    // this step is a requirement for the completion of
    // initWaitForAppsToBecomeReadyAsync().

    Mercury::Bundle & bundle = baseAppMgr_.bundle();
    bundle.startMessage( BaseAppMgrInterface::initData );
    bundle << gameTime;

    baseAppMgr_.send();
}


/**
 *  This method sets whether we have started. It is used so that the server can
 *  be started from a watcher.
 */
void DBApp::hasStartBegun( bool hasStartBegun )
{
    // TODO: This doesn't appear to ever have been used in the tools. It was
    // added very early on, consider removing it now that we do this
    // automatically based on handleStatusCheck().

}


/**
 *  This method is the called when an entity that is being checked out has
 *  completed the checkout process. onStartEntityCheckout() should've been
 *  called to mark the start of the operation. pBaseRef is the base mailbox
 *  of the now checked out entity. pBaseRef should be NULL if the checkout
 *  operation failed.
 */
bool DBApp::onCompleteEntityCheckout( const EntityKey& entityID,
    const EntityMailBoxRef* pBaseRef )
{
    bool isErased = (inProgCheckouts_.erase( entityID ) > 0);
    if (isErased && (checkoutCompletionListeners_.size() > 0))
    {
        std::pair< CheckoutCompletionListeners::iterator,
                CheckoutCompletionListeners::iterator > listeners =
            checkoutCompletionListeners_.equal_range( entityID );
        for ( CheckoutCompletionListeners::const_iterator iter =
                listeners.first; iter != listeners.second; ++iter )
        {
            iter->second->onCheckoutCompleted( pBaseRef );
        }
        checkoutCompletionListeners_.erase( listeners.first,
                listeners.second );
    }

    return isErased;
}

/**
 *  This method registers listener to be called when the entity identified
 *  by typeID and dbID completes its checkout process. This function will
 *  false and not register the listener if the entity is not currently
 *  in the process of being checked out.
 */
bool DBApp::registerCheckoutCompletionListener( EntityTypeID typeID,
        DatabaseID dbID, DBApp::ICheckoutCompletionListener& listener )
{
    EntityKeySet::key_type key( typeID, dbID );
    bool isFound = (inProgCheckouts_.find( key ) != inProgCheckouts_.end());
    if (isFound)
    {
        CheckoutCompletionListeners::value_type item( key, &listener );
        checkoutCompletionListeners_.insert( item );
    }
    return isFound;
}

BW_END_NAMESPACE

// dbapp.cpp
