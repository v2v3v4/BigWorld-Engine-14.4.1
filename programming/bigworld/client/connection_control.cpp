#include "pch.hpp"

#include "connection_control.hpp"

#include "app_config.hpp"
#include "entity_manager.hpp"
#include "script_bigworld.hpp"

#include "chunk/chunk_manager.hpp"

#include "connection/entity_def_constants.hpp"
#include "connection/login_challenge_factory.hpp"
#include "connection/login_handler.hpp"
#include "connection/login_interface.hpp"
#include "connection/loginapp_public_key.hpp"
#include "connection/rsa_stream_encoder.hpp"
#include "connection/smart_server_connection.hpp"

#include "connection_model/bw_connection.hpp"
#include "connection_model/bw_null_connection.hpp"
#include "connection_model/bw_replay_connection.hpp"
#include "connection_model/bw_server_connection.hpp"

#include "network/packet_loss_parameters.hpp"

#include "cstdmf/watcher.hpp"

#include "pyscript/personality.hpp"
#include "pyscript/py_callback.hpp"
#include "pyscript/script_math.hpp"

#include "space/space_manager.hpp"

#include "urlrequest/manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Server", 0 )


BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( ConnectionControl );

static void scriptBandwidthFromServerMutator( int bandwidth );
extern void setBandwidthFromServerMutator( void (*mutatorFn)( int bandwidth ) );


// -----------------------------------------------------------------------------
// Section: LogOnStatus enumeration
// -----------------------------------------------------------------------------

class LogOnStatusEnum : public LogOnStatus
{
    public:
    PY_BEGIN_ENUM_MAP_NOPREFIX( Status )
        PY_ENUM_VALUE( NOT_SET )
        PY_ENUM_VALUE( LOGGED_ON )
        PY_ENUM_VALUE( LOGGED_ON_OFFLINE )
        PY_ENUM_VALUE( CONNECTION_FAILED )
        PY_ENUM_VALUE( DNS_LOOKUP_FAILED )
        PY_ENUM_VALUE( UNKNOWN_ERROR )
        PY_ENUM_VALUE( CANCELLED )
        PY_ENUM_VALUE( ALREADY_ONLINE_LOCALLY )
        PY_ENUM_VALUE( PUBLIC_KEY_LOOKUP_FAILED )
        PY_ENUM_VALUE( LAST_CLIENT_SIDE_VALUE )
        PY_ENUM_VALUE( LOGIN_MALFORMED_REQUEST )
        PY_ENUM_VALUE( LOGIN_BAD_PROTOCOL_VERSION )
        PY_ENUM_VALUE( LOGIN_CHALLENGE_ISSUED ) // Shouldn't actually see this
        PY_ENUM_VALUE( LOGIN_REJECTED_NO_SUCH_USER )
        PY_ENUM_VALUE( LOGIN_REJECTED_INVALID_PASSWORD )
        PY_ENUM_VALUE( LOGIN_REJECTED_ALREADY_LOGGED_IN )
        PY_ENUM_VALUE( LOGIN_REJECTED_BAD_DIGEST )
        PY_ENUM_VALUE( LOGIN_REJECTED_DB_GENERAL_FAILURE )
        PY_ENUM_VALUE( LOGIN_REJECTED_DB_NOT_READY )
        PY_ENUM_VALUE( LOGIN_REJECTED_ILLEGAL_CHARACTERS )
        PY_ENUM_VALUE( LOGIN_REJECTED_SERVER_NOT_READY )
        PY_ENUM_VALUE( LOGIN_REJECTED_UPDATER_NOT_READY )   // No longer used
        PY_ENUM_VALUE( LOGIN_REJECTED_NO_BASEAPPS )
        PY_ENUM_VALUE( LOGIN_REJECTED_BASEAPP_OVERLOAD )
        PY_ENUM_VALUE( LOGIN_REJECTED_CELLAPP_OVERLOAD )
        PY_ENUM_VALUE( LOGIN_REJECTED_BASEAPP_TIMEOUT )
        PY_ENUM_VALUE( LOGIN_REJECTED_BASEAPPMGR_TIMEOUT )
        PY_ENUM_VALUE( LOGIN_REJECTED_DBAPP_OVERLOAD )
        PY_ENUM_VALUE( LOGIN_REJECTED_LOGINS_NOT_ALLOWED )
        PY_ENUM_VALUE( LOGIN_REJECTED_RATE_LIMITED )
        PY_ENUM_VALUE( LOGIN_REJECTED_BAN )
        PY_ENUM_VALUE( LOGIN_REJECTED_CHALLENGE_ERROR )

        PY_ENUM_VALUE( LOGIN_REJECTED_AUTH_SERVICE_NO_SUCH_ACCOUNT )
        PY_ENUM_VALUE( LOGIN_REJECTED_AUTH_SERVICE_LOGIN_DISALLOWED )
        PY_ENUM_VALUE( LOGIN_REJECTED_AUTH_SERVICE_UNREACHABLE )
        PY_ENUM_VALUE( LOGIN_REJECTED_AUTH_SERVICE_INVALID_RESPONSE )
        PY_ENUM_VALUE( LOGIN_REJECTED_AUTH_SERVICE_GENERAL_FAILURE )

        PY_ENUM_VALUE( LOGIN_REJECTED_NO_LOGINAPP )
        PY_ENUM_VALUE( LOGIN_REJECTED_NO_LOGINAPP_RESPONSE )
        PY_ENUM_VALUE( LOGIN_REJECTED_NO_BASEAPP_RESPONSE )

        PY_ENUM_VALUE( LOGIN_REJECTED_IP_ADDRESS_BAN )
        PY_ENUM_VALUE( LOGIN_REJECTED_REGISTRATION_NOT_CONFIRMED )
        PY_ENUM_VALUE( LOGIN_REJECTED_NOT_REGISTERED )
        PY_ENUM_VALUE( LOGIN_REJECTED_ACTIVATING )
        PY_ENUM_VALUE( LOGIN_REJECTED_UNABLE_TO_PARSE_JSON )
        PY_ENUM_VALUE( LOGIN_REJECTED_USERS_LIMIT )
        PY_ENUM_VALUE( LOGIN_REJECTED_LOGIN_QUEUE )

        PY_ENUM_VALUE( LOGIN_CUSTOM_DEFINED_ERROR )
        PY_ENUM_VALUE( LAST_SERVER_SIDE_VALUE )
    PY_END_ENUM_MAP()
};

PY_ENUM_CONVERTERS_DECLARE( LogOnStatusEnum::Status )

PY_ENUM_MAP( LogOnStatusEnum::Status )
PY_ENUM_CONVERTERS_SCATTERED( LogOnStatusEnum::Status )


#if ENABLE_WATCHERS

namespace // (anonymous)
{

/**
 *  This class provides a watcher for the ServerConnection object, if there is
 *  a server connection existing.
 */
class ConnectionControlWatcherProvider : public WatcherProvider
{
public:
    ConnectionControlWatcherProvider( ConnectionControl & connectionControl ) :
        WatcherProvider(),
        connectionControl_( connectionControl )
    {}

    WatcherPtr getWatcher() /* override */;

private:
    ConnectionControl & connectionControl_;
};


/*
 *  Override from WatcherProvider.
 */
WatcherPtr ConnectionControlWatcherProvider::getWatcher() /* override */
{
    static DirectoryWatcherPtr pEmpty( new DirectoryWatcher );

    BWServerConnection * pServerConnectionModel =
        connectionControl_.pServerConnection();

    if (!pServerConnectionModel)
    {
        return pEmpty;
    }

    ServerConnection * pServerConnection =
        pServerConnectionModel->pServerConnection();

    MF_ASSERT( pServerConnection );

    // TODO: Should think about caching this per connection instead
    // of regenerating.

    return new AbsoluteWatcher( ServerConnection::pWatcher(),
        pServerConnection );
}


/**
 *  This class provides a watcher for the BWEntities collection, when there is 
 *  a main connection.
 */
class EntitiesWatcherProvider : public WatcherProvider
{
public:
    EntitiesWatcherProvider( ConnectionControl & connectionControl ) :
        WatcherProvider(),
        connectionControl_( connectionControl )
    {}

    WatcherPtr getWatcher() /* override */;

private:
    ConnectionControl & connectionControl_;
};



/*
 *  Override from WatcherProvider.
 */
WatcherPtr EntitiesWatcherProvider::getWatcher() /* override */
{
    static DirectoryWatcherPtr pEmpty( new DirectoryWatcher );

    BWConnection * pConnectionModel = connectionControl_.pConnection();

    if (!pConnectionModel)
    {
        return pEmpty;
    }

    // TODO: Should think about caching this per connection instead
    // of regenerating.
    WatcherPtr pWatcher = new DirectoryWatcher;

    WatcherPtr pMapWatcher = BWEntities::pWatcher();

    pMapWatcher->addChild( "*",
        new SmartPointerDereferenceWatcher( &Entity::watcher() ) );

    pWatcher->addChild( "Entity Objects", 
        new AbsoluteWatcher( pMapWatcher,
        (void *)(&pConnectionModel->entities()) ) );

    BWEntities * pNullEntities = NULL;

    pWatcher->addChild( "Active Entities",
        new AbsoluteWatcher( makeWatcher( &BWEntities::size ),
            (void *)(&pConnectionModel->entities() )) );

    return pWatcher;
}


/**
 *  This class provides a watcher for the NetworkInterface object attached to
 *  the main connection, if one exists.
 */
class NetworkInterfaceWatcherProvider : public WatcherProvider
{
public:
    NetworkInterfaceWatcherProvider( ConnectionControl & connectionControl ) :
        WatcherProvider(),
        connectionControl_( connectionControl )
    {}

    WatcherPtr getWatcher() /* override */;

private:
    ConnectionControl & connectionControl_;
};


/*
 *  Override from WatcherProvider.
 */
WatcherPtr NetworkInterfaceWatcherProvider::getWatcher() /* override */
{
    static DirectoryWatcherPtr pEmpty( new DirectoryWatcher );

    BWServerConnection * pServerConnectionModel = 
        connectionControl_.pServerConnection();

    if (!pServerConnectionModel)
    {
        // When there's no ServerConnectionModel, we cache the artificialLoss
        // settings and apply them to the new network interface when it got 
        // created. 
        return new AbsoluteWatcher( ConnectionControl::pWatcher(),
            (void *)( &connectionControl_ ) );
    }

    MF_ASSERT( pServerConnectionModel->pServerConnection() );

    Mercury::NetworkInterface * pInterface = 
        &(pServerConnectionModel->pServerConnection()->networkInterface());

    if (!pInterface)
    {
        // Should be rare, when we log into BaseApp.
        return pEmpty;
    }

    return new AbsoluteWatcher( Mercury::NetworkInterface::pWatcher(),
        (void *)(pInterface) );
}

} // end namespace (anonymous)

/**
 *  This method returns a watcher for the artificial loss settings.
 */
WatcherPtr ConnectionControl::pWatcher()
{
    static DirectoryWatcherPtr pWatcher = NULL;

    if (pWatcher == NULL)
    {
        pWatcher = new DirectoryWatcher();

        ConnectionControl * pNull = NULL;
        pWatcher->addChild( "artificialLoss",
                new BaseDereferenceWatcher( 
                    Mercury::PacketLossParameters::pWatcher() ),
                &pNull->pPacketLossParameters_ );
    }
    return pWatcher;
}


#endif // ENABLE_WATCHERS


// -----------------------------------------------------------------------------
// Section: ConnectionControl
// -----------------------------------------------------------------------------


/**
 *  Constructor.
 */
ConnectionControl::ConnectionControl() :
    dispatcher_(),
    entityFactory_(),
    spaceDataStorage_(),
    pConnection_( NULL ),
    connectionHelper_(
        entityFactory_,
        spaceDataStorage_,
        EntityType::entityDefConstants() ),
    condemnedConnections_(),
    clientSpaceConnections_(),
    clientSpaces_(),
    isExpectingPlayerCreation_( false ),
#if ENABLE_WATCHERS
    loggerMessageEndpoint_(),
    pLoggerMessageForwarder_( NULL ),
    pCommsWatcherProvider_( NULL ),
    pEntitiesWatcherProvider_( NULL ),
    pNetworkInterfaceWatcherProvider_( NULL ),
#endif
    pPacketLossParameters_( new Mercury::PacketLossParameters() ),
    shuttingDown_( false ),
    server_(),
    mode_( NONE ),
    isInTick_( false ),
    offlineTime_( 0.0 )
{
    BW_GUARD;

    URL::Manager::init( dispatcher_ );
    setBandwidthFromServerMutator( scriptBandwidthFromServerMutator );

#if ENABLE_WATCHERS
    loggerMessageEndpoint_.socket( SOCK_DGRAM );
    loggerMessageEndpoint_.bind();

    bool shouldLogToML = AppConfig::instance().pRoot()->readBool( 
                "logging/logToMessageLogger", false );

    // Old location, deprecated by BigWorld 2.3
    BW::string loggerID = AppConfig::instance().pRoot()->readString(
        "loggerID" );
    loggerID = AppConfig::instance().pRoot()->readString(
        "logging/loggerID", loggerID );

    if (shouldLogToML && (getUserId() == 0))
    {
        NOTICE_MSG( "ConnectionControl::ConnectionControl: "
            "Not logging to MessageLogger as UID environment "
            "variable has not been set.\n" );
        shouldLogToML = false;
    }

    // (start forwarding logging messages)
    pLoggerMessageForwarder_ = new LoggerMessageForwarder( "Client",
        loggerMessageEndpoint_, dispatcher_, loggerID, shouldLogToML );

    pCommsWatcherProvider_ = new ConnectionControlWatcherProvider( *this );
    pEntitiesWatcherProvider_ = new EntitiesWatcherProvider( *this );
    pNetworkInterfaceWatcherProvider_ = 
        new NetworkInterfaceWatcherProvider( *this );

    Watcher::rootWatcher().addChild( "ConnectionControl", 
        new PolymorphicWatcher( "ConnectionControl" ), 
        &pCommsWatcherProvider_ );

    Watcher::rootWatcher().addChild( "Entities", 
        new PolymorphicWatcher( "BWEntities" ), 
        &pEntitiesWatcherProvider_ );

    Watcher::rootWatcher().addChild( "NetworkInterface", 
        new PolymorphicWatcher( "NetworkInterface" ), 
        &pNetworkInterfaceWatcherProvider_ );

#endif
}


/**
 *  Destructor.
 */
ConnectionControl::~ConnectionControl()
{
    BW_GUARD;

#if ENABLE_WATCHERS
    bw_safe_delete( pCommsWatcherProvider_ );
    bw_safe_delete( pEntitiesWatcherProvider_ );
    bw_safe_delete( pNetworkInterfaceWatcherProvider_ );

    // TODO: It looks like the root watcher is cleared out by the time we are
    // destroyed.
    // MF_VERIFY( Watcher::rootWatcher().removeChild( "ConnectionControl" ) );
    // MF_VERIFY( Watcher::rootWatcher().removeChild( "Entities" ) );
    // MF_VERIFY( Watcher::rootWatcher().removeChild( "NetworkInterface" ) );

    // pLoggerMessageForwarder cancels timers on server connection's nub on
    // destruction
    bw_safe_delete( pLoggerMessageForwarder_ );

#endif // ENABLE_WATCHERS

    bw_safe_delete( pPacketLossParameters_ );

    if (this->pConnection_ != NULL)
    {
        this->destroyMainConnection();
    }

    // This should be true after each call to tick().
    // We should be deleted when App destructs, outside of any tick() calls.
    MF_ASSERT( condemnedConnections_.empty() );

    MF_ASSERT( clientSpaceConnections_.empty() );
    MF_ASSERT( clientSpaces_.empty() );
}


/**
 *  This method finalises the ConnectionControl by clearing all BWConnections.
 */
void ConnectionControl::fini()
{
    // Clear entities in online, offline, replay spaces
    this->clearSpaces();

    // Clear entites in local spaces 
    ClientSpaceConnections::iterator iter = clientSpaceConnections_.begin();
    while (iter != clientSpaceConnections_.end())
    {
        iter->second->clearAllEntities();

        delete iter->second;
        ++iter;
    }
    clientSpaceConnections_.clear();

    // SpaceManager owns the elements
    clientSpaces_.clear();
}


/**
 *  This method initialises the log on parameter encoder, by setting its
 *  LoginApp public key from the given path.
 */
bool ConnectionControl::initLogOnParamsEncoder( BW::string publicKeyPath )
{
    MF_ASSERT( pConnection_ );
    BWServerConnection * pServerConn = this->pServerConnection();

    IF_NOT_MF_ASSERT_DEV( pServerConn )
    {
        return false;
    }

#if defined( PLAYSTATION3 ) || defined( _XBOX360 )
    BW::string keyString( g_loginAppPublicKey );
#else

    // Use a standard key path if none is provided
    if (publicKeyPath.empty())
    {
        publicKeyPath = "loginapp.pubkey";
    }

    DataSectionPtr pSection = BWResource::openSection( publicKeyPath );

    if (!pSection)
    {
        ERROR_MSG( "ConnectionControl::initLogOnParamsEncoder: "
                "Couldn't load private key from non-existent file %s\n",
            publicKeyPath.c_str() );
        return false;
    }

    BinaryPtr pBinData = pSection->asBinary();
    BW::string keyString( pBinData->cdata(), pBinData->len() );

#endif // defined( PLAYSTATION3 ) || defined( _XBOX360 )

    // TODO: We also have a direct fopen/fread initialiser we could use.
    return pServerConn->setLoginAppPublicKeyString( keyString );
}


/**
 *  This is a utility method for calling back on the connection callback.
 */
void ConnectionControl::callConnectionCallback( LogOnStage stage, 
        bool shouldCallNextFrame,
        LogOnStatus status, const BW::string & message )
{
    PyObject * pFunction = progressFn_.get();
    Py_XINCREF( pFunction );

    static const char * ERROR_PREFIX = "BigWorld.connect";

    PyObject * args = Py_BuildValue( "(iNs)", stage,
                Script::getData( status.value() ), 
                message.c_str() );

    if (shouldCallNextFrame)
    {
        Script::callNextFrame( pFunction, args, ERROR_PREFIX );
    }
    else
    {
        Script::call( pFunction, args, ERROR_PREFIX, /* okIfFnNull */ true );
    }
}


/**
 *  This method creates a connection and logs on.
 *
 *  @param server       Server address string.
 *  @param loginParams  Script object with string attributes 'username',
 *                      'password', 'publicKeyPath', 'inactivityTimeout'
 *                      and transport'.
 *  @param progressFn   Script method to listen on for connection progress. It
                        is called with three arguments: the stage, the status,
                        and a message. Note it may be called before this method
                        returns.
 */
void ConnectionControl::connect( const BW::string & server,
    ScriptObject loginParams, ScriptObject progressFn )
{
    BW_GUARD;

    // if already connected, tell OLD progress function step 0 failed.
    if (pConnection_ != NULL)
    {
        this->callConnectionCallback( STAGE_INITIAL,
            /* shouldCallNextFrame */ false );
        return;
    }

    // ok, starting to log on then
    server_ = server;
    MF_ASSERT( progressFn.isCallable() );
    progressFn_ = progressFn;

    if (server_.empty())
    {
        mode_ = OFFLINE;

        SpaceID spaceID = this->nextLocalSpaceID();     

        pConnection_ = connectionHelper_.createNullConnection(
            offlineTime_, spaceID );

        pConnection_->addSpaceDataListener( EntityManager::instance() );
        pConnection_->setStreamDataFallbackHandler( EntityManager::instance() );
        pConnection_->addEntitiesListener( &EntityManager::instance() );

        // Perhaps promote setHandler to BWConnection, then BWNullConnection
        // can handle its own callbacks.
        this->onLoggedOn();
        return;
    }

    mode_ = ONLINE;

    // extract login parameters
    BW::string username;
    BW::string password;
    BW::string publicKeyPath;
    BW::string transport = "udp";

    float inactivityTimeout = 60.f;

    loginParams.getAttribute( "username", username, ScriptErrorClear() );
    loginParams.getAttribute( "password", password, ScriptErrorClear() );
    loginParams.getAttribute( "publicKeyPath", publicKeyPath,
        ScriptErrorClear() );
    loginParams.getAttribute( "inactivityTimeout", inactivityTimeout,
        ScriptErrorClear() );
    loginParams.getAttribute( "transport", transport,
        ScriptErrorClear() );

    ConnectionTransport transportValue = CONNECTION_TRANSPORT_UDP;

    if (transport == "websockets")
    {
        transportValue = CONNECTION_TRANSPORT_WEBSOCKETS;
    }
    else if (transport == "tcp")
    {
        transportValue = CONNECTION_TRANSPORT_TCP;
    }
    else if (transport != "udp")
    {
        PyErr_SetString( PyExc_ValueError,
            "The transport log on param must be one of \"udp\", \"tcp\", "
            "or \"websockets\"" );
        return;
    }

    BWServerConnection * pServerConn = 
        connectionHelper_.createServerConnection( offlineTime_ );

    pConnection_ = pServerConn;

    if (!this->initLogOnParamsEncoder( publicKeyPath ))
    {
#if !ENABLE_UNENCRYPTED_LOGINS
        this->callConnectionCallback( STAGE_LOGIN, 
            /* shouldCallNextFrame */ true,
            LogOnStatus::PUBLIC_KEY_LOOKUP_FAILED );
        this->destroyMainConnection();
        return;
#endif // !ENABLE_UNENCRYPTED_LOGINS
    }

    pConnection_->addSpaceDataListener( EntityManager::instance() );
    pConnection_->setStreamDataFallbackHandler( EntityManager::instance() );
    pConnection_->addEntitiesListener( &EntityManager::instance() );

    pServerConn->setHandler( this );

    // Configure the BWServerConnection's ServerConnection
    ServerConnection * pInternalServerConnection =
        pServerConn->pServerConnection();
    // enable an extra validation packet to be sent to work around
    // client firewalls reassigning ports
    pInternalServerConnection->enableReconfigurePorts();

    pInternalServerConnection->setInactivityTimeout( inactivityTimeout );
    pInternalServerConnection->setArtificialLoss( *pPacketLossParameters_ );

    pInternalServerConnection->pTaskManager( BgTaskManager::pInstance() );

    // start trying to connect then
    if (!pServerConn->logOnTo( server_, username, password, transportValue ))
    {
        // This could be either "server or username empty" or "login currently
        // in progress".
        // The latter was tested earlier, but there's no good error code
        // for the former.
        this->callConnectionCallback( STAGE_INITIAL,
            /* shouldCallNextFrame */ false );
        this->destroyMainConnection();
    }
}


/**
 *  Disconnect if connected.
 */
void ConnectionControl::disconnect( bool shouldCallProgressFn /* = true */ )
{
    BW_GUARD;

    if (pConnection_)
    {
        pConnection_->disconnect();
        this->disconnected( this->isShuttingDown(), shouldCallProgressFn );
    }
}


/**
 *  This method starts a new replay playback from a file on disk if we are
 *  not already playing a replay or connected to a server.
 *
 *  @param filename             The name of the file resource to read from.
 *  @param pHandler             A BWReplayEventHandler or NULL if not desired.
 *  @param publicKey            The EC public key string to use for verifying
 *                              signatures, in PEM format.
 *  @param volatileInjectionPeriod
 *                              The maximum period for injected volatile
 *                              updates or -1 for the application default.
 *
 *  @return                     The SpaceID the playback is playing in, or
 *                              NULL_SPACE_ID if something failed.
 */
SpaceID ConnectionControl::startReplayFromFile( const BW::string & fileName,
    BWReplayEventHandler * pHandler, const BW::string & publicKey,
    int volatileInjectionPeriod )
{
    BW_GUARD;

    // if already connected, tell OLD progress function step 0 failed.
    if (pConnection_ != NULL)
    {
        this->callConnectionCallback( STAGE_INITIAL,
            /* shouldCallNextFrame */ false );
        return NULL_SPACE_ID;
    }

    if (volatileInjectionPeriod < 0)
    {
        volatileInjectionPeriod = AppConfig::instance().pRoot()->readUInt(
            "replay/volatileInjectionPeriodTicks",
            ReplayController::DEFAULT_VOLATILE_INJECTION_PERIOD_TICKS );
    }

    mode_ = REPLAY;

    server_.clear();

    BWReplayConnection * pReplayConn = 
        connectionHelper_.createReplayConnection( offlineTime_ );

    pConnection_ = pReplayConn;

    pConnection_->addSpaceDataListener( EntityManager::instance() );
    pConnection_->setStreamDataFallbackHandler( EntityManager::instance() );
    pConnection_->addEntitiesListener( &EntityManager::instance() );

    pReplayConn->setHandler( pHandler );
    pReplayConn->setReplaySignaturePublicKeyString( publicKey );

    SpaceID spaceID = this->nextLocalSpaceID();

    if (!pReplayConn->startReplayFromFile( FileIOTaskManager::instance(),
        spaceID, fileName, volatileInjectionPeriod ))
    {
        this->destroyMainConnection();
        return NULL_SPACE_ID;
    }

    return pReplayConn->spaceID();
}


/**
 *  This method starts a new replay playback from a file on disk if we are
 *  not already playing a replay or connected to a server.
 *
 *  @param localCacheFileName    The buffer file to use for playback.
 *  @param shouldKeepCacheFile  true if the buffer file should not be deleted
 *                              after playback terminates.
 *  @param pHandler             A BWReplayEventHandler or NULL if not desired.
 *  @param publicKey            The EC public key string to use for verifying
 *                              signatures, in PEM format.
 *  @param volatileInjectionPeriod
 *                              The maximum period for injected volatile
 *                              updates or -1 for the application default.
 *
 *  @return                     The SpaceID the playback is playing in, or
 *                              NULL_SPACE_ID if something failed.
 */
SpaceID ConnectionControl::startReplayFromStream(
    const BW::string & localCacheFileName, bool shouldKeepCacheFile,
    BWReplayEventHandler * pHandler, const BW::string & publicKey,
    int volatileInjectionPeriod )
{
    // if already connected, tell OLD progress function step 0 failed.
    if (pConnection_ != NULL)
    {
        this->callConnectionCallback( STAGE_INITIAL,
            /* shouldCallNextFrame */ false );
        return NULL_SPACE_ID;
    }

    if (volatileInjectionPeriod < 0)
    {
        volatileInjectionPeriod = AppConfig::instance().pRoot()->readUInt(
            "replay/volatileInjectionPeriodTicks",
            ReplayController::DEFAULT_VOLATILE_INJECTION_PERIOD_TICKS );
    }

    mode_ = REPLAY;

    server_.clear();

    BWReplayConnection * pReplayConn = 
        connectionHelper_.createReplayConnection( offlineTime_ );
    pConnection_ = pReplayConn;

    pConnection_->addSpaceDataListener( EntityManager::instance() );
    pConnection_->setStreamDataFallbackHandler( EntityManager::instance() );
    pConnection_->addEntitiesListener( &EntityManager::instance() );

    pReplayConn->setHandler( pHandler );
    pReplayConn->setReplaySignaturePublicKeyString( publicKey );

    SpaceID spaceID = this->nextLocalSpaceID();

    if (!pReplayConn->startReplayFromStreamData( FileIOTaskManager::instance(),
        spaceID, localCacheFileName, shouldKeepCacheFile,
        volatileInjectionPeriod ))
    {
        this->destroyMainConnection();
        return NULL_SPACE_ID;
    }

    return pReplayConn->spaceID();
}


/**
 *  This method terminates the replay, in the same way disconnect terminates
 *  a connection started by connect()
 */
void ConnectionControl::stopReplay()
{
    BW_GUARD;

    BWReplayConnection * pReplayConn;

    if ((pReplayConn = this->pReplayConnection()) != NULL)
    {
        pReplayConn->stopReplay();
    }
    else
    {
        // Not running a replay
        return;
    }

    // TODO: Replay playbacks use their own progress function. We should unify
    // this interface.
    this->disconnected( this->isShuttingDown(),
        /* shouldCallProgressFn */ false );
}


/**
 *  This method clears all the entities and space data from an online, offline
 *  or replay spaces. It will have no effect if the active connection is still
 *  running, i.e. ConnectionControl::disconnect() or
 *  ConnectionControl::stopReplay() has not been called. Once called a new
 *  online, offline connection or replay can be started.
 */
void ConnectionControl::clearSpaces()
{
    BWServerConnection * pServerConn;
    BWNullConnection * pNullConn;
    BWReplayConnection * pReplayConn;

    if (pConnection_ == NULL)
    {
        return;
    }

    if ((pNullConn = this->pNullConnection()) != NULL)
    {
        pNullConn->destroyPlayerCellEntity();
        pNullConn->destroyPlayerBaseEntity();
    }
    else if ((pServerConn = this->pServerConnection()) != NULL &&
        (pServerConn->isLoggingIn() || pServerConn->isOnline()))
    {
        ERROR_MSG( "ConnectionControl::clearSpaces: "
            "Called while still connected to a server. Ignoring request.\n" );
        return;
    }
    else if ((pReplayConn = this->pReplayConnection()) != NULL &&
        pReplayConn->pReplayController() != NULL)
    {
        ERROR_MSG( "ConnectionControl::clearSpaces: "
            "Called while still playing a replay. Ignoring request.\n" );
        return;
    }

    pConnection_->clearAllEntities();

    this->destroyMainConnection();
}


void ConnectionControl::notifyOfShutdown()
{
    shuttingDown_ = true;
}


bool ConnectionControl::isShuttingDown() const
{
    return shuttingDown_;
}


/**
 *  Return the current server, or NULL if not connected
 */
const BW::string & ConnectionControl::server() const
{
    BW_GUARD;
    return server_;
}

/**
 *  Start to determine the status of the given server.
 *
 *  Note that progressFn may be called before this method returns
 */
void ConnectionControl::probe( const BW::string & server,
    ScriptObject progressFn )
{
    BW_GUARD;
    MF_ASSERT( progressFn.isCallable() );
    progressFn.callFunction( ScriptArgs::create( ScriptObject::none() ),
        ScriptErrorPrint() );
}



/**
 *  Tick method for network processing.
 */
void ConnectionControl::tick( float dt )
{
    BW_GUARD;

    MF_ASSERT( !isInTick_ );
    isInTick_ = true;

    if (pConnection_ == NULL)
    {
        offlineTime_ += dt;
    }

    connectionHelper_.update( pConnection_, dt );

    // Tick the dispatcher for any other sockets that might be registered.
    while (dispatcher_.processOnce( /* shouldIdle */ false ) != 0)
    {
        // Do nothing.
    }

    // Update each of our client space connections as well.
    {
        ClientSpaceConnections::iterator iter = clientSpaceConnections_.begin();
        while (iter != clientSpaceConnections_.end())
        {
            // This should never have any non-local entities, so this will have
            // no effect except advancing BWNullConnection::clientTime
            iter->second->update( dt );
            iter->second->updateServer();
            // TODO: I'd like this to be true... But I think server
            // time-correction is causing it to drift.
            /*
            MF_ASSERT( isEqual( pClientSpaceConnection_->clientTime(),
                this->gameTime() ) );
            */

            ++iter;
        }
    }

    // Clear any condemned connections now that we're hopefully outside of any
    // connection's update() method.
    {
        Connections::iterator iter = condemnedConnections_.begin();
        while (iter != condemnedConnections_.end())
        {
            delete *iter;
            ++iter;
        }

        condemnedConnections_.clear();
    }

    isInTick_ = false;
}


/**
 *  This method is called when we are disconnected from a server while
 *  attempting to log on.
 */
void ConnectionControl::onLoggedOff()
{
    this->disconnected( /* isShuttingDown */ false );
}


/**
 *  This method is called when we successfully log on to a server.
 */
void ConnectionControl::onLoggedOn()
{
    BWServerConnection * pServerConn;
    BWNullConnection * pNullConn;
    if ((pServerConn = this->pServerConnection()))
    {
        this->callConnectionCallback( STAGE_LOGIN,
            /* shouldCallNextFrame */ false, LogOnStatus::LOGGED_ON,
            pServerConn->pServerConnection()->serverMsg() );
    }
    else if ((pNullConn = this->pNullConnection()))
    {
        this->callConnectionCallback( STAGE_LOGIN,
            /* shouldCallNextFrame */ false, LogOnStatus::LOGGED_ON_OFFLINE,
            "Single user mode" );
    }
    else
    {
        // No connection? How did we get here?
        MF_ASSERT( false && "onLoggedOn without a connection" );
    }

    isExpectingPlayerCreation_ = true;
}


/**
 *  This method is called if we failed to log on to a server.
 */
void ConnectionControl::onLogOnFailure( const LogOnStatus & status,
    const BW::string & message )
{
    MF_ASSERT( status != LogOnStatus::LOGGED_ON &&
        status != LogOnStatus::LOGGED_ON_OFFLINE );

    BWServerConnection * pServerConn;

    if ((pServerConn = this->pServerConnection()))
    {
        MF_ASSERT( !pServerConn->isLoggingIn() && !pServerConn->isOnline() );
        // This is to ensure we don't still have entities lying around, or the
        // client script may try and generate network traffic in the callback,
        // which'd be bad.
        pServerConn->clearAllEntities();
    }

    this->destroyMainConnection();

    this->callConnectionCallback( STAGE_LOGIN,
        /* shouldCallNextFrame */ false, status, message );

    progressFn_ = ScriptObject();
    return;
}


/**
 *  This method is called to determine if we have a login attempt in progress.
 */
bool ConnectionControl::isLoginInProgress() const
{
    BWServerConnection * pServerConn;
    if ((pServerConn = this->pServerConnection()))
    {
        return pServerConn->isLoggingIn();
    }

    return false;
}


/**
 *  This method is called when we the client receives information about the 
 *  base player from the server.
 */
void ConnectionControl::onBasePlayerCreated()
{
    if (!isExpectingPlayerCreation_)
    {
        // Not the first Base player entity for this connection.
        return;
    }

    BWServerConnection * pServerConn;
    BWNullConnection * pNullConn;
    if ((pServerConn = this->pServerConnection()))
    {
        this->callConnectionCallback( STAGE_DATA,
            /* shouldCallNextFrame */ false, LogOnStatus::LOGGED_ON );
    }
    else if ((pNullConn = this->pNullConnection()))
    {
        this->callConnectionCallback( STAGE_DATA,
            /* shouldCallNextFrame */ false, LogOnStatus::LOGGED_ON_OFFLINE );
    }
    else
    {
        // No connection? How did we get here?
        MF_ASSERT( false && "onLoggedOn without a connection" );
    }

    isExpectingPlayerCreation_ = false;
}


/**
 *  This private method is called when we have been disconnected
 */
void ConnectionControl::disconnected( bool isShuttingDown, 
    bool shouldCallProgressFn /* = true */ )
{
    BW_GUARD;

    if (progressFn_ && shouldCallProgressFn)
    {
        this->callConnectionCallback( STAGE_DISCONNECTED, 
            /* shouldCallNextFrame */ !isShuttingDown );
    }

    EntityManager::instance().clearPendingEntities();
    progressFn_ = ScriptObject();

    server_.clear();
}


/**
 *  This static method fetches the instance's EventDispatcher
 */
/* static */ Mercury::EventDispatcher & ConnectionControl::dispatcher()
{
    return ConnectionControl::instance().dispatcher_;
}


/**
 *  This method returns our BWConnection.
 */
BWConnection * ConnectionControl::pConnection() const
{
    return pConnection_;
}


/**
 *  This method returns our BWServerConnection if we are online, or NULL.
 */
BWServerConnection * ConnectionControl::pServerConnection() const
{
    if (!pConnection_ || mode_ != ONLINE)
    {
        return NULL;
    }
    return static_cast< BWServerConnection * >( pConnection_ );
}


/**
 *  This method returns our BWNullConnection if we are in offline mode, or NULL.
 */
BWNullConnection * ConnectionControl::pNullConnection() const
{
    if (!pConnection_ || mode_ != OFFLINE)
    {
        return NULL;
    }
    return static_cast< BWNullConnection * >( pConnection_ );
}


/**
 *  This method returns our BWReplayConnection if we are playing a replay, or
 *  NULL.
 */
BWReplayConnection * ConnectionControl::pReplayConnection() const
{
    if (!pConnection_ || mode_ != REPLAY)
    {
        return NULL;
    }
    return static_cast< BWReplayConnection * >( pConnection_ );
}


/**
 *  This method returns a BWConnection mapped to a SpaceID. Both server and
 *  client created BWConnections are mapped.
 *
 *  @param spaceID  SpaceID of the BWConnection.
 *
 *  @return BWConnection if found, otherwise NULL.
 */
BWConnection * ConnectionControl::pSpaceConnection( SpaceID spaceID ) const
{
    if (pConnection_ && pConnection_->spaceID() == spaceID)
    {
        return pConnection_;
    }

    ClientSpaceConnections::const_iterator iter =
        clientSpaceConnections_.find( spaceID );

    return iter == clientSpaceConnections_.end() ? NULL : iter->second;
}


/**
 *  This method returns the next local space ID.
 */
SpaceID ConnectionControl::nextLocalSpaceID()
{
    BW_GUARD;

    static SpaceID clientSpaceID = SpaceManager::LOCAL_SPACE_START;

    const SpaceID mainOfflineSpaceID = (this->pNullConnection() ?
        this->pNullConnection()->spaceID() : 
        NULL_SPACE_ID);

    SpaceID newSpaceID = NULL_SPACE_ID;

    do
    {
        newSpaceID = clientSpaceID++;
        if (clientSpaceID == NULL_SPACE_ID)
        {
            clientSpaceID = SpaceManager::LOCAL_SPACE_START;
        }
    } while ((newSpaceID == mainOfflineSpaceID) ||
            clientSpaceConnections_.count( newSpaceID ));

    return newSpaceID;
}


/**
 *  This method creates a new local space.
 *
 *  @return SpaceID of the created local space.
 *
 *  @see ConnectionControl::clearLocalSpace
 */
SpaceID ConnectionControl::createLocalSpace()
{
    BW_GUARD;

    SpaceID spaceID = this->nextLocalSpaceID();
    SpaceManager & spaceManager = SpaceManager::instance();

    ClientSpacePtr pNewSpace = spaceManager.createSpace( spaceID );
    clientSpaces_[spaceID] = pNewSpace;

    clientSpaceConnections_.insert( 
        std::make_pair( spaceID,
            connectionHelper_.createNullConnection(
                this->gameTime(), spaceID ) ) );

    return spaceID;
}


/**
 *  This method clears a local space. Clearing the space clears all its
 *  entities and space data.
 *
 *  @see ConnectionControl::clearLocalSpace
 */
void ConnectionControl::clearLocalSpace( SpaceID spaceID )
{
    {
        ClientSpaceConnections::iterator iter =
            clientSpaceConnections_.find( spaceID );

        if (iter == clientSpaceConnections_.end())
        {
            WARNING_MSG( "ConnectionControl::clearLocalSpace: "
                    "Space %d does not exist\n",
                spaceID );
            return;
        }

        iter->second->clearAllEntities();
        delete iter->second;

        clientSpaceConnections_.erase( iter );
    }

    {
        ClientSpaces::iterator iter = clientSpaces_.find( spaceID );

        IF_NOT_MF_ASSERT_DEV( iter != clientSpaces_.end() )
        {
            return;
        }

        clientSpaces_.erase( iter );
    }

    spaceDataStorage_.deleteSpace( spaceID );
}


/**
 *  This method visits all entities known to ConnectionControl. This includes
 *  server entities (this includes offline and replay spaces) and locally
 *  created client entities.
 */
void ConnectionControl::visitEntities( EntityVisitor & visitor ) const
{
    BW_GUARD;

    // Go through the main connection first.
    if (pConnection_)
    {
        BWEntities::const_iterator iter = pConnection_->entities().begin();
        while (iter != pConnection_->entities().end())
        {
            if (!visitor.onVisitEntity( static_cast< Entity * >(
                    iter->second.get()) ))
            {
                return;
            }
            ++iter;
        }
    }

    // Now loop through all the client spaces.
    ClientSpaceConnections::const_iterator iSpace =
        clientSpaceConnections_.begin();

    while (iSpace != clientSpaceConnections_.end())
    {
        BWNullConnection * pConnection = iSpace->second;

        BWEntities::const_iterator iEntity = pConnection->entities().begin();
        while (iEntity != pConnection->entities().end())
        {
            if (!visitor.onVisitEntity( static_cast< Entity * >(
                    iEntity->second.get()) ))
            {
                return;
            }
            ++iEntity;
        }
        ++iSpace;
    }
}


/**
 *  This method returns the total number of entities being managed.
 */
size_t ConnectionControl::numEntities() const
{
    size_t count = 0;

    if (pConnection_)
    {
        count += pConnection_->entities().size();
    }

    ClientSpaceConnections::const_iterator iSpace =
        clientSpaceConnections_.begin();

    while (iSpace != clientSpaceConnections_.end())
    {
        count += iSpace->second->entities().size();
        ++iSpace;
    }

    return count;
}


/**
 *  This method returns the client time. It ensures that we
 *  continue to have monotonically increasing time no matter whether we're
 *  connected to a server or not.
 */
double ConnectionControl::gameTime() const
{
    if (pConnection_ != NULL)
    {
        return pConnection_->clientTime();
    }
    return offlineTime_;
}


/**
 *  This convenience method returns the current player as an Entity, wrapping
 *  up some dereferencing and a static cast.
 */
Entity * ConnectionControl::pPlayer() const
{
    if (pConnection_ == NULL)
    {
        return NULL;
    }
    return static_cast< Entity * >( pConnection_->pPlayer() );
}


/**
 *  This convenience method returns the requested EntityID as an Entity,
 *  wrapping up some dereferencing and a static cast.
 */
Entity * ConnectionControl::findEntity( EntityID entityID ) const
{
    Entity * pOut = NULL;

    if (BWEntities::isLocalEntity( entityID ))
    {
        // Hopefully not too many of these.
        ClientSpaceConnections::const_iterator iter =
            clientSpaceConnections_.begin();

        while (iter != clientSpaceConnections_.end())
        {
            if (NULL != (pOut = static_cast< Entity * >(
                    iter->second->entities().find( entityID ))))
            {
                return pOut;
            }
            ++iter;
        }
        // Fall through to allow for finding local client entities in the 
        // main connection space.
        // return NULL;
    }

    // Must be a server entity ID.
    if (pConnection_ == NULL)
    {
        return NULL;
    }

    return static_cast< Entity * >( pConnection_->entities().find( entityID ) );
}


/**
 *  This method cleans up and destroys our BWConnection instance
 */
void ConnectionControl::destroyMainConnection()
{
    if (!pConnection_)
    {
        return;
    }

    // We save it away here, because deleting the connection can trigger script
    // which may try to call virtual methods on the destructing connection as 
    // part of various time-keeping methods e.g. App::getGameTimeFrameStart().

    BWConnection * pConnection = pConnection_;
    pConnection_ = NULL;

    offlineTime_ = pConnection->clientTime();

    if (isInTick_)
    {
        condemnedConnections_.push_back( pConnection );
    }
    else
    {
        delete pConnection;
    }

    mode_ = NONE;
}


// -----------------------------------------------------------------------------
// Section: Python stuff
// -----------------------------------------------------------------------------

/*~ function BigWorld connect
 *  This function initiates a connection between the client and a specified
 *  server. If the client is not already connected to a server and is able to
 *  reach the network then all entities which reside on the client are
 *  destroyed in preparation for entering the server's game. This function can
 *  be used to tell the client to run offline, or to pass username, password,
 *  and other login data to the server. The client can later be disconnected
 *  from the server by calling BigWorld.disconnect.
 *  @param server This is a string representation of the name of the server
 *  which the client is to attempt to connect. This can be in the form of an IP
 *  address (eg: "11.12.123.42"), a domain address
 *  (eg: "server.coolgame.company.com"), or an empty string (ie: "" ). If an
 *  empty string is passed in then the client will run in offline mode. If a
 *  connection is successfully established then subsequent calls to
 *  BigWorld.server will return this value.
 *  @param loginParams This is a Python object that may contain members which
 *  provide login data. If a member is not present in this object then a
 *  default value is used instead. These members are:
 *  username - The username for the player who is logging in. This defaults to
 *  "".
 *  password - The password for the player who is logging in. This defaults to
 *  "".
 *  transport - The transport to use for connections. This defaults to "udp".
 *  Other valid values are "tcp" for raw TCP, and "websockets" for WebSockets.
 *  inactivityTimeout - The amount of time the client should wait for a
 *  response from the server before assuming that the connection has failed
 *  in seconds. This defaults to 60.0. This is the only member that is read
 *  from the object but not sent to the server.
 *  publicKeyPath - The location of a keyfile that contains the loginapp's
 *  public key, used to encrypt the login handshake.  This defaults to
 *  "loginapp.pubkey".
 *  Code Example:
 *  @{
 *  # This example defines a class of object, an instance of which could be
 *  # passed as the loginParams argument to BigWorld.connect.
 *  # define LoginInfo
 *  class LoginInfo:
 *      # init
 *      def __init__( self, username, password, transport, inactivityTimeout ):
 *          self.username          = username
 *          self.password          = password
 *          self.transport         = transport
 *          self.inactivityTimeout = inactivityTimeout
 *  @}
 *  This data is only sent by clients which are supplied to licensees of the
 *  BigWorld server technology.
 *  @param progressFn This is a Python function which is called by BigWorld to
 *  report on the status of the connection. This function must take three
 *  arguments, where the first is interpreted as an integer indicating where
 *  in the connection process the report indicated, the second is interpreted as
 *  a string indicating the status at that point and the third is a string
 *  message that the server may have sent.
 *  It should be noted that this function may be called before BigWorld.connect
 *  returns.
 *  Possible (progress, status) pairs that may be passed to this function are
 *  listed below.  Please see bigworld/src/common/login_interface.hpp for the
 *  definitive reference for these status codes.
 *  @{
 *  0, 'NOT_SET' - The connection could not be initiated as either the client was
 *  already connected to a server, the network could not be reached, or the
 *  client was running in offline mode. If the client is running offline then
 *  it must be disconnected before it can connect to a server.
 *  1, 'LOGGED_ON' - The client has successfully logged in to the server. Once the client
 *  begins to receive data (as notified by a call to this function with
 *  arguments 2, 1) it can consider itself to have completed the connection.
 *
 *  1, 'LOGGED_ON_OFFLINE' - The client has successfully simulated a login in
 *  offline mode, and is waiting for the client-side script to create an
 *  entity to simulate the connected player.
 *  @}
 *  The following are client side errors:
 *  @{
 *  1, 'CONNECTION_FAILED' - The client failed to make a connection to the network.
 *  1, 'DNS_LOOKUP_FAILED' - The client failed to locate the server IP address via DNS.
 *  1, 'UNKNOWN_ERROR' - An unknown client-side error has occurred.
 *  1, 'CANCELLED' - The login was cancelled by the client.
 *  1, 'ALREADY_ONLINE_LOCALLY' - The client is already online locally (i.e. exploring a space
 *  offline).
 *  1, 'PUBLIC_KEY_LOOKUP_FAILED' - Lookup for the client public key has failed.
 *  The following are server side errors:
 *  1, 'LOGIN_MALFORMED_REQUEST' - The login packet sent to the server was malformed.
 *  1, 'LOGIN_BAD_PROTOCOL_VERSION' - The login protocol the client used does not match the one on the
 *  server.
 *  1, 'LOGIN_REJECTED_NO_SUCH_USER' - The server database did not contain an entry for the specified
 *  username and was running in a mode that did not allow for unknown users
 *  to connect, or could not create a new entry for the user. The database
 *  would most likely be unable to create a new entry for a user if an
 *  inappropriate entity type being listed in bw.xml as database/entityType.
 *  1, 'LOGIN_REJECTED_INVALID_PASSWORD' - A global password was specified in bw.xml, and it did not match the
 *  password with which the login attempt was made.
 *  1, 'LOGIN_REJECTED_ALREADY_LOGGED_IN' - A client with this username is already logged into the server.
 *  1, 'LOGIN_REJECTED_BAD_DIGEST' - The defs and/or entities.xml are not identical on the client and
 *  the server.
 *  1, 'LOGIN_REJECTED_DB_GENERAL_FAILURE' - A general database error has occurred, for example the database may
 *  have been corrupted.
 *  1, 'LOGIN_REJECTED_DB_NOT_READY' - The database is not ready yet.
 *  1, 'LOGIN_REJECTED_ILLEGAL_CHARACTERS' - There are illegal characters in either the username or password.
 *  1, 'LOGIN_REJECTED_SERVER_NOT_READY' - The server is not ready yet.
 *  1, 'LOGIN_REJECTED_NO_BASEAPPS' - There are no baseapps registered at present.
 *  1, 'LOGIN_REJECTED_BASEAPP_OVERLOAD' - Baseapps are overloaded and logins are being temporarily
 *  disallowed.
 *  1, 'LOGIN_REJECTED_CELLAPP_OVERLOAD' - Cellapps are overloaded and logins are being temporarily
 *  disallowed.
 *  1, 'LOGIN_REJECTED_BASEAPP_TIMEOUT' - The baseapp that was to act as the proxy for the client timed out
 *  on its reply to the dbapp.
 *  1, 'LOGIN_REJECTED_BASEAPPMGR_TIMEOUT' - The baseappmgr is not responding.
 *  1, 'LOGIN_REJECTED_DBAPP_OVERLOAD' - The dbapp is overloaded.
 *  1, 'LOGIN_REJECTED_LOGINS_NOT_ALLOWED' - Logins are being temporarily disallowed.
 *  1, 'LOGIN_REJECTED_RATE_LIMITED' - Logins are temporarily not allowed by
 *  the server due to login rate limit.
 *  2, 'LOGGED_ON' - The client has begun to receive data from the server. This indicates
 *  that the connection process is complete.
 *  2, 'LOGGED_ON_OFFLINE' - The client has completed simulating an offline
 *  version of a connection, and is ready to start running in offline mode.
 *  6, 'NOT_SET' - The client has been disconnected from the server.
 *  @}
 *  @return None
 */
static bool connect( const BW::string & server,
    ScriptObject loginParams,
    ScriptObject progressFn )
{
    BW_GUARD;
    ConnectionControl & connectionControl = ConnectionControl::instance();
    if (connectionControl.isLoginInProgress())
    {
        PyErr_SetString( PyExc_RuntimeError, "An existing login is already in progress" );
        return false;
    }

    connectionControl.connect( server, loginParams, progressFn );
    return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, connect, ARG( BW::string, NZARG(
    ScriptObject, CALLABLE_ARG( ScriptObject, END ) ) ), BigWorld )


/*~ function BigWorld disconnect
 *  If the client is connected to a server then this will terminate the
 *  connection. Otherwise it does nothing. This function does not delete any
 *  entities, however it does stop the flow of data regarding entity status
 *  to and from the server.
 *
 *  The client can be connected to a server via the BigWorld.connect function.
 *  @return None
 */
static void disconnect()
{
    if (ConnectionControl::instance().isShuttingDown())
    {
        WARNING_MSG( "Personality script fini is calling BigWorld.disconnect() "
            "during shutdown, but it is called automatically.\n" );
    }
    else
    {
        ConnectionControl::instance().disconnect();
    }
}
PY_AUTO_MODULE_FUNCTION( RETVOID, disconnect, END, BigWorld )


/*~ function BigWorld server
 *  This function returns the name of the server to which the client is
 *  currently connected. The name is identical to the string which would have
 *  been passed in as the first argument to the BigWorld.connect function to
 *  establish the connection. Usually this is a string representation of an IP
 *  address. If the client is running in offline mode, then this is an empty
 *  string. Otherwise, if the client is neither connected to a server nor
 *  running in offline mode, then this returns None.
 *  @return The name of the server to which the client is connected, an empty
 *  string if the client is running in offline mode, or None if the client is
 *  not connected.
 */
static PyObject * server()
{
    BW_GUARD;
    if (ConnectionControl::instance().pConnection() == NULL)
    {
        Py_RETURN_NONE;
    }

    return Script::getData( ConnectionControl::instance().server() );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, server, END, BigWorld )


static void probe( const BW::string & server, ScriptObject progressFn )
{
    ConnectionControl::instance().probe( server, progressFn );
}

/*~ function BigWorld.probe
 *  The function probe is used to determine the status of a particular server.
 *  This has not yet been implemented.
 *  @param server String. Specifies the name or address of server.
 *  @param progressFn Callback Function (python function object, or an instance of
 *  a python class that implements the __call__ interface). The function to be
 *  called when information on the status for a particular server is available.
 */
PY_AUTO_MODULE_FUNCTION( RETVOID, probe, ARG( BW::string, CALLABLE_ARG(
    ScriptObject, END ) ), BigWorld )


/*~ class BigWorld.LatencyInfo
 *  The LatencyInfo class is a sub class of Vector4Provider that has an
 *  overloaded value attribute. This attribute
 *  is overloaded to return a 4 float sequence consisting of
 *  ( 1, minimum latency, maximum latency, average latency ) if the client is
 *  connected to a server. If the client is not connected to the server then
 *  the value attribute returns ( 0, 0, 0, 0 ).
 *
 *  Call the BigWorld.LatencyInfo function to create an instance of a
 *  LatencyInfo object.
 */

/**
 *  This class provides latency information to Python
 */
class LatencyInfo : public Vector4Provider
{
    Py_Header( LatencyInfo, Vector4Provider )

public:
    LatencyInfo( PyTypeObject * pType = &s_type_ ) :
        Vector4Provider( false, pType )
    {}

    virtual void output( Vector4 & val );

    PY_AUTO_CONSTRUCTOR_FACTORY_DECLARE( LatencyInfo, END )
};

PY_TYPEOBJECT( LatencyInfo )

PY_BEGIN_METHODS( LatencyInfo )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( LatencyInfo )
PY_END_ATTRIBUTES()

/*~ function BigWorld.LatencyInfo
 *  The function LatencyInfo is a factory function to create and return
 *  a LatencyInfo object. A LatencyInfo object is a Vector4Provider with
 *  an overloaded value attribute. This attribute (for a LatencyInfo object)
 *  is overloaded to return a 4 float sequence consisting of
 *  ( 1, minimum latency, maximum latency, average latency ) if the client is
 *  connected to a server. If the client is not connected to the server then
 *  the value attribute returns ( 0, 0, 0, 0 ).
 *  @return A new LatencyInfo object (see also Vector4Provider).
 */
PY_FACTORY_NAMED( LatencyInfo, "LatencyInfo", BigWorld )

/**
 *  Output method ... get the actual data.
 */
void LatencyInfo::output( Vector4 & val )
{
    BW_GUARD;

    BWServerConnection * pServerConn =
        ConnectionControl::instance().pServerConnection();

    if (pServerConn == NULL || !pServerConn->isOnline())
    {
        val.set( 0.f, 0.f, 0.f, 0.f );
        return;
    }

    val.x = 1;
    val.y = pServerConn->pServerConnection()->latency(); // minLatency();
    val.z = pServerConn->pServerConnection()->latency(); // maxLatency();
    val.w = pServerConn->pServerConnection()->latency();
}


/**
 *  This function calls the personality script to send a message to the server
 *  to set the current bandwidth. It is invoked when the server connection's
 *  watcher is set to a different value.
 */
static void scriptBandwidthFromServerMutator( int bandwidth )
{
    BW_GUARD;

    Personality::instance().callMethod( "onInternalBandwidthSetRequest",
        ScriptArgs::create( bandwidth ),
        ScriptErrorPrint( "scriptBandwidthFromServerMutator: " ),
        /* allowNullMethod */ false );
}

BW_END_NAMESPACE

// connection_control.cpp
