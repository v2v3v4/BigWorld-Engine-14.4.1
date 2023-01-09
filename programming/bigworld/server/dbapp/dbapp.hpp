#ifndef DB_APP_HPP
#define DB_APP_HPP

#include "dbappmgr_gateway.hpp"
#include "log_on_records_cache.hpp"

#include "connection/log_on_params.hpp"

#include "cstdmf/singleton.hpp"

#include "db/db_hash_schemes.hpp"
#include "db/dbapp_interface.hpp"
#include "db/dbapps_gateway.hpp"

#include "db_storage/db_status.hpp"
#include "db_storage/idatabase.hpp"

#include "network/basictypes.hpp"
#include "network/channel_owner.hpp"

#include "resmgr/datasection.hpp"

#include "server/backup_hash.hpp"
#include "server/script_app.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE


namespace Mercury
{
    class Bundle;
} // end namespace Mercury

namespace DBConfig
{
    class Server;
} // end namespace DBConfig


class BWResource;
class Consolidator;
class DBAppConfig;
class EntityDefs;
class GetEntityHandler;
class RelogonAttemptHandler;

typedef Mercury::ChannelOwner BaseAppMgr;

/**
 *  This class is used to implement the main singleton object that represents
 *  this application.
 */
class DBApp : public ScriptApp,
    public TimerHandler,
    public IDatabase::IGetBaseAppMgrInitDataHandler,
    public IDatabase::IUpdateSecondaryDBshandler,
    public Singleton< DBApp >
{
private:
    enum TimeOutType
    {
        TIMEOUT_GAME_TICK,
        TIMEOUT_STATUS_CHECK,
        TIMEOUT_GATHER_LOGIN_APPS
    };

public:
    SERVER_APP_HEADER( DBApp, dbApp )

    typedef DBAppConfig Config;

    DBApp( Mercury::EventDispatcher & dispatcher,
            Mercury::NetworkInterface & interface );
    virtual ~DBApp();

    DBAppMgrGateway & dbAppMgr()    { return dbAppMgr_; }

    BaseAppMgr & baseAppMgr()       { return baseAppMgr_; }

    // Overrides
    void handleTimeout( TimerHandle handle, void * arg );

    void handleStatusCheck( BinaryIStream & data );

    // Lifetime messages
    void handleBaseAppMgrBirth(
        const DBAppInterface::handleBaseAppMgrBirthArgs & args );

    void handleDBAppMgrBirth(
        const DBAppInterface::handleDBAppMgrBirthArgs & args );
    void handleDBAppMgrDeath(
        const DBAppInterface::handleDBAppMgrDeathArgs & args );

    void shutDown( const DBAppInterface::shutDownArgs & args );
    void startSystemControlledShutdown();
    void shutDownNicely();
    void shutDown();

    void controlledShutDown(
        const DBAppInterface::controlledShutDownArgs & args );

    // TODO: Scalable DB: Move this to DBAppMgr instead of DBApp Alpha.
    void cellAppOverloadStatus(
        const DBAppInterface::cellAppOverloadStatusArgs & args );


    // Entity messages
    void logOn( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data );

    void logOn( const Mercury::Address & srcAddr,
        Mercury::ReplyID replyID,
        LogOnParamsPtr pParams,
        const Mercury::Address & addrForProxy );

    void onLogOnLoggedOnUser( EntityTypeID typeID, DatabaseID dbID,
        LogOnParamsPtr pParams,
        const Mercury::Address & proxyAddr, const Mercury::Address& replyAddr,
        Mercury::ReplyID replyID, const EntityMailBoxRef * pExistingBase,
        const BW::string & dataForClient,
        const BW::string & dataForBaseEntity );

    void onEntityLogOff( EntityTypeID typeID, DatabaseID dbID );

    bool calculateOverloaded( bool isOverloaded );

    void sendFailure( Mercury::ReplyID replyID,
        const Mercury::Address & dstAddr,
        LogOnStatus reason, const char * pDescription = NULL );

    void authenticateAccount( const Mercury::Address & srcAddr,
            const Mercury::UnpackedMessageHeader & header,
            BinaryIStream & data );

    void loadEntity( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data );

    void writeEntity( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data );

    void deleteEntity( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        const DBAppInterface::deleteEntityArgs & args );

    void lookupEntity( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        const DBAppInterface::lookupEntityArgs & args );
    void lookupEntityByName( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data );
    void lookupDBIDByName( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data );
    void lookupEntity( EntityDBKey & ekey, BinaryOStream & bos );

    void lookupEntities( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data );

    // Misc messages
    void executeRawCommand( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data );

    void putIDs( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data );

    void getIDs( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data );

    void writeSpaces( const Mercury::Address & srcAddr,
        const Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data );

    void writeGameTime( const DBAppInterface::writeGameTimeArgs & args );

    void handleBaseAppDeath( BinaryIStream & data );

    void checkStatus( const Mercury::Address & srcAddr,
            const Mercury::UnpackedMessageHeader & header,
            BinaryIStream & data );

    void secondaryDBRegistration( const Mercury::Address & srcAddr,
            const Mercury::UnpackedMessageHeader & header,
            BinaryIStream & data );

    void updateSecondaryDBs( const Mercury::Address & srcAddr,
            const Mercury::UnpackedMessageHeader & header,
            BinaryIStream & data );
    virtual void onUpdateSecondaryDBsComplete(
            const IDatabase::SecondaryDBEntries& removedEntries );

    void getSecondaryDBDetails( const Mercury::Address & srcAddr,
            const Mercury::UnpackedMessageHeader & header, BinaryIStream & data );

    void updateDBAppHash( BinaryIStream & data );

    // Accessors
    const EntityDefs& getEntityDefs() const
        { return *pEntityDefs_; }
    EntityDefs& swapEntityDefs( EntityDefs& entityDefs )
    {
        EntityDefs& curDefs = *pEntityDefs_;
        pEntityDefs_ = &entityDefs;
        return curDefs;
    }

    Mercury::EventDispatcher & mainDispatcher()     { return mainDispatcher_; }
    Mercury::NetworkInterface & interface()         { return interface_; }
    static Mercury::NetworkInterface & getNub() { return DBApp::instance().interface(); }

    static Mercury::UDPChannel & getChannel( const Mercury::Address & addr )
    {
        return DBApp::instance().interface().findOrCreateChannel( addr );
    }

    IDatabase& getIDatabase()   { MF_ASSERT(pDatabase_); return *pDatabase_; }
    BillingSystem * pBillingSystem()        { return pBillingSystem_; }

    // IDatabase pass-through methods. Call these instead of IDatabase methods
    // so that we can intercept and muck around with stuff.

    void getEntity( const EntityDBKey & entityKey,
            BinaryOStream * pStream,
            bool shouldGetBaseEntityLocation,
            GetEntityHandler & handler );

    void putEntity( const EntityKey & ekey,
            EntityID entityID,
            BinaryIStream * pStream,
            EntityMailBoxRef * pBaseMailbox,
            bool removeBaseMailbox,
            bool putExplicitID,
            UpdateAutoLoad updateAutoLoad,
            IDatabase::IPutEntityHandler& handler );

    void setBaseEntityLocation( const EntityKey & entityKey,
            EntityMailBoxRef & mailbox,
            IDatabase::IPutEntityHandler & handler,
            UpdateAutoLoad updateAutoLoad = UPDATE_AUTO_LOAD_RETAIN )
    {
        this->putEntity( entityKey, mailbox.id, NULL, &mailbox, 
            false, false,
            updateAutoLoad,
            handler );
    }

    void clearBaseEntityLocation( const EntityKey & entityKey,
            IDatabase::IPutEntityHandler & handler )
    {
        this->putEntity( entityKey, 0, NULL, NULL, true, false,
            UPDATE_AUTO_LOAD_RETAIN,
            handler );
    }

    void delEntity( const EntityDBKey & ekey, EntityID entityID,
            IDatabase::IDelEntityHandler& handler );

    // Watcher
    void hasStartBegun( bool hasStartBegun );
    bool hasStartBegun() const
    {
        return status_.status() > DBStatus::WAITING_FOR_APPS;
    }
    bool isConsolidating() const        { return pConsolidator_.get() != NULL; }

    /** Return true if we are DBApp Alpha. */
    bool isAlpha() const                { return id_ == dbApps_.alpha().id(); }

    bool isReady() const
    {
        return status_.status() >= DBStatus::WAITING_FOR_APPS;
    }

    // Sets baseRef to "pending base creation" state.
    static void setBaseRefToLoggingOn( EntityMailBoxRef& baseRef,
        EntityTypeID entityTypeID )
    {
        baseRef.init( 0, Mercury::Address( 0, 0 ),
            EntityMailBoxRef::BASE, entityTypeID );
    }

    bool defaultEntityToStrm( EntityTypeID typeID, const BW::string& name,
        BinaryOStream& strm ) const;

    static DatabaseID* prepareCreateEntityBundle( EntityTypeID typeID,
        DatabaseID dbID, const Mercury::Address& addrForProxy,
        Mercury::ReplyMessageHandler* pHandler, Mercury::Bundle & bundle,
        LogOnParamsPtr pParams = NULL,
        BW::string * pDataForBaseEntity = NULL );

    RelogonAttemptHandler* getInProgRelogonAttempt( EntityTypeID typeID,
            DatabaseID dbID )
    {
        PendingAttempts::iterator iter =
                pendingAttempts_.find( EntityKey( typeID, dbID ) );
        return (iter != pendingAttempts_.end()) ? iter->second : NULL;
    }
    void onStartRelogonAttempt( EntityTypeID typeID, DatabaseID dbID,
            RelogonAttemptHandler* pHandler )
    {
        MF_VERIFY( pendingAttempts_.insert(
                PendingAttempts::value_type( EntityKey( typeID, dbID ),
                        pHandler ) ).second );
    }
    void onCompleteRelogonAttempt( EntityTypeID typeID, DatabaseID dbID )
    {
        MF_VERIFY( pendingAttempts_.erase( EntityKey( typeID, dbID ) ) > 0 );
    }

    bool onStartEntityCheckout( const EntityKey& entityID )
    {
        return inProgCheckouts_.insert( entityID ).second;
    }
    bool onCompleteEntityCheckout( const EntityKey& entityID,
            const EntityMailBoxRef* pBaseRef );

    /**
     *  This interface is used to receive the event that an entity has completed
     *  checking out.
     */
    struct ICheckoutCompletionListener
    {
        // This method is called when the onCompleteEntityCheckout() is
        // called for the entity that you've registered for via
        // registerCheckoutCompletionListener(). After this call, this callback
        // will be automatically deregistered.
        virtual void onCheckoutCompleted( const EntityMailBoxRef* pBaseRef ) = 0;
    };
    bool registerCheckoutCompletionListener( EntityTypeID typeID,
            DatabaseID dbID, ICheckoutCompletionListener& listener );

    bool hasMailboxRemapping() const    { return !mailboxRemapInfo_.empty(); }
    void remapMailbox( EntityMailBoxRef& mailbox ) const;

    void onConsolidateProcessEnd( bool isOK );

    bool shouldCacheLogOnRecords() const { return shouldCacheLogOnRecords_; }
    LogOnRecordsCache & logOnRecordsCache() { return logOnRecordsCache_; }

    // Callbacks used during initialisation.
    bool onDBAppMgrRegistrationCompleted( BinaryIStream & data );
    void onEntitiesAutoLoadCompleted( bool didAutoLoad );
    void onEntitiesAutoLoadError();

    int id() const              { return id_; }

private:
    bool init( int argc, char * argv[] ) /* override */;

    // In order of execution in init().
    bool initNetwork();
    bool initBaseAppMgr();
    bool initScript( int argc, char ** argv );
    bool initEntityDefs();
    bool initExtensions();
    bool initDatabaseCreation();
    bool initDBAppMgrAsync();
    // This is public so AddToDBAppMgrHelper can access it.
    // bool onDBAppMgrRegistrationCompleted( BinaryIStream & data );
    // The init*()'s below require DBAppMgr registration to complete first.
    bool initAppIDRegistration();
    bool initDatabaseStartup();
    bool initBirthDeathListeners();
    bool initWatchers();
    bool initConfig();
    bool initReviver();
    bool initGameSpecific();

    bool initDBAppAlpha();
    //      These indented initialisation steps done if we are the DBApp
    //      alpha.
    bool    initAcquireDBLock();
    bool    initBillingSystem();
    void    initSecondaryDBPrefix();
    //      The initialisation steps for DBApp-alpha below should only run 
    //      once per server-run by DBApp-Alpha.
    bool    initResetGameServerState();
    bool    initSecondaryDBsAsync();
    bool    onSecondaryDBsInitCompleted();
    bool    initBaseAppMgrInitData();
    bool    initDatabaseResetGameServerState();
    bool    initWaitForAppsToBecomeReadyAsync();
//  bool    initTimers();
    bool    onAppsReady();
//  bool    initScriptAppReady();
    bool    initSendSpaceData();
    bool    initEntityAutoLoadingAsync();
    // This is public so EntityAutoLoader can access it.
//  void    onEntitiesAutoLoadCompleted();
//  void    onEntitiesAutoLoadError();
    bool    initNotifyServerStartup( bool didAutoLoad );

    bool initTimers();
    bool initScriptAppReady();
    void onInitCompleted();

    void onRunComplete() /* override */;

    // Override from ServerApp.
    ManagerAppGateway * pManagerAppGateway() /* override */
    {
        return &dbAppMgr_;
    }

    void finalise();

    void addWatchers( Watcher & watcher );

    void endMailboxRemapping();

    void sendBaseAppMgrInitData();
    void onGetBaseAppMgrInitDataComplete( GameTime gameTime ) /* override */;

    void checkPendingLoginAttempts();

    void checkStatus();

    // Data consolidation methods
    void consolidateData();
    bool startConsolidationProcess();
    bool sendRemoveDBCmd( uint32 destIP, const BW::string & dbLocation );

    DBAppID             id_;
    DBAppsGateway       dbApps_;

    EntityDefs*         pEntityDefs_;
    IDatabase *         pDatabase_;
    BillingSystem *     pBillingSystem_;

    DBStatus            status_;

    DBAppMgrGateway     dbAppMgr_;

    BaseAppMgr          baseAppMgr_;

    /// Flags for initialisation, used for checking whether a certain
    /// initialisation step has completed that is not easily verifiable
    /// otherwise.
    enum InitStateFlags
    {
        INIT_STATE_NETWORK                  = (1 << 0),
        INIT_STATE_EXTENSIONS               = (1 << 1),
        INIT_STATE_DATABASE_STARTUP         = (1 << 2),
        INIT_STATE_APP_ID_REGISTRATION      = (1 << 3),
        INIT_STATE_BIRTH_DEATH_LISTENERS    = (1 << 4),
        INIT_STATE_WATCHERS                 = (1 << 5),
        INIT_STATE_CONFIG                   = (1 << 6),
        INIT_STATE_REVIVER                  = (1 << 7),
        INIT_STATE_GAME_SPECIFIC            = (1 << 8),
        INIT_STATE_SCRIPT_APP_READY         = (1 << 9),
        INIT_STATE_NON_ALPHA_MASK           = (INIT_STATE_SCRIPT_APP_READY << 1) - 1,
        INIT_STATE_SECONDARY_DBS            = (1 << 10),
        INIT_STATE_SPACE_DATA_RESTORE       = (1 << 11),
        INIT_STATE_AUTO_LOADING             = (1 << 12)
    };
    uint16              initState_;

    bool                shouldAlphaResetGameServerState_;

    TimerHandle         statusCheckTimer_;
    TimerHandle         gameTimer_;

    friend class RelogonAttemptHandler;
    typedef BW::map< EntityKey, RelogonAttemptHandler * > PendingAttempts;
    PendingAttempts pendingAttempts_;
    typedef BW::set< EntityKey > EntityKeySet;
    EntityKeySet    inProgCheckouts_;

    typedef BW::multimap< EntityKey, ICheckoutCompletionListener* >
            CheckoutCompletionListeners;
    CheckoutCompletionListeners checkoutCompletionListeners_;

    float               curLoad_;
    bool                hasOverloadedCellApps_;
    uint64              overloadStartTime_;

    typedef BW::map< Mercury::Address, BackupHash > MailboxRemapInfo;
    MailboxRemapInfo    mailboxRemapInfo_;
    int                 mailboxRemapCheckCount_;

    BW::string          secondaryDBPrefix_;
    uint                secondaryDBIndex_;

    std::auto_ptr< Consolidator >       pConsolidator_;

    bool                                shouldCacheLogOnRecords_;
    LogOnRecordsCache                   logOnRecordsCache_;
};

#ifdef CODE_INLINE
#include "dbapp.ipp"
#endif

BW_END_NAMESPACE

#endif // DB_APP_HPP
