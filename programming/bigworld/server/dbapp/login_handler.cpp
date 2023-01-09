#include "script/first_include.hpp"

#include "login_handler.hpp"

#include "dbapp.hpp"

#include "connection/log_on_status.hpp"

#include "db_storage/db_entitydefs.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: LoginHandler
// -----------------------------------------------------------------------------

/**
 *  Constructor.
 */
LoginHandler::LoginHandler(
            LogOnParamsPtr pParams,
            const Mercury::Address & clientAddr,
            const Mercury::Address & replyAddr,
            Mercury::ReplyID replyID ) :
        entityKey_( 0, 0 ),
        pParams_( pParams ),
        clientAddr_( clientAddr ),
        replyAddr_( replyAddr ),
        replyID_( replyID ),
        bundle_(),
        baseRef_(),
        putEntityHandler_( *this, &LoginHandler::onPutEntityComplete ),
        reserveBaseMailboxHandler_( *this, 
            &LoginHandler::onReservedBaseMailbox ),
        setBaseMailboxHandler_( *this, &LoginHandler::onSetBaseMailbox ),
        pStrmDbID_( NULL ),
        shouldCreateNewOnLoadFailure_( false ),
        shouldRememberNewOnLoadFailure_( false ),
        dataForClient_(),
        dataForBaseEntity_()
{
}


/**
 *  Destructor.
 */
LoginHandler::~LoginHandler()
{
}


/**
 *  Start the login process
 */
void LoginHandler::login()
{
    DBApp::instance().pBillingSystem()->getEntityKeyForAccount(
        pParams_->username(), pParams_->password(), clientAddr_, *this );

    // When getEntityKeyForAccount() completes, onGetEntityKeyForAccount*()
    // will be called.
}


/*
 *  IGetEntityKeyForAccountHandler override
 */
void LoginHandler::onGetEntityKeyForAccountSuccess( const EntityKey & ekey,
    const BW::string & dataForClient,
    const BW::string & dataForBaseEntity )
{
    dataForClient_ = dataForClient;
    dataForBaseEntity_ = dataForBaseEntity;
    this->loadEntity( EntityDBKey( ekey ) );
}


/*
 *  IGetEntityKeyForAccountHandler override
 */
void LoginHandler::onGetEntityKeyForAccountCreateNew( EntityTypeID typeID,
        bool shouldRemember,
        const BW::string & dataForClient,
        const BW::string & dataForBaseEntity )
{
    dataForClient_ = dataForClient;
    dataForBaseEntity_ = dataForBaseEntity;
    this->createNewEntity( typeID, shouldRemember );
}


/*
 *  IGetEntityKeyForAccountHandler override
 */
void LoginHandler::onGetEntityKeyForAccountLoadFromUsername(
        EntityTypeID entityType, const BW::string & username,
        bool shouldCreateNewOnLoadFailure,
        bool shouldRememberNewOnLoadFailure,
        const BW::string & dataForClient,
        const BW::string & dataForBaseEntity )
{
    dataForClient_ = dataForClient;
    dataForBaseEntity_ = dataForBaseEntity;
    shouldCreateNewOnLoadFailure_ = shouldCreateNewOnLoadFailure;
    shouldRememberNewOnLoadFailure_ = shouldRememberNewOnLoadFailure;
    pParams_->username( username );
    this->loadEntity( EntityDBKey( entityType, 0, username ) );
}


/**
 *  ISetEntityKeyForAccountHandler override.
 */
void LoginHandler::onSetEntityKeyForAccountSuccess( const EntityKey & key )
{
    this->sendCreateEntityMsg();
}


/**
 *  ISetEntityKeyForAccountHandler override.
 */
void LoginHandler::onSetEntityKeyForAccountFailure( const EntityKey & key )
{
    class DelEntityNoOp : public IDatabase::IDelEntityHandler
    {
    public:
        virtual void onDelEntityComplete( bool isOK )
        {
            delete this;
        }
    };

    DBApp::instance().delEntity( EntityDBKey( key ), 0,
        *(new DelEntityNoOp()) );

    this->sendFailureReply(
        LogOnStatus::LOGIN_REJECTED_AUTH_SERVICE_GENERAL_FAILURE,
        "External billing system failed to write new entity." );
}


/**
 *  This method loads the entity with the given key.
 */
void LoginHandler::loadEntity( const EntityDBKey & ekey )
{
    entityKey_ = ekey;

    // Start "create new base" message even though we're not sure entity
    // exists. This is to take advantage of getEntity() streaming properties
    // into the bundle directly.
    pStrmDbID_ = DBApp::prepareCreateEntityBundle( entityKey_.typeID,
        entityKey_.dbID, clientAddr_, this, bundle_, pParams_,
        &dataForBaseEntity_ );

    // Get entity data
    pBaseRef_ = &baseRef_;

    DBApp::instance().getEntity( ekey, &bundle_, true, *this );
    // When getEntity() completes, onGetEntityCompleted() is called.
}


/*
 *  IGetEntityKeyForAccountHandler override
 */
void LoginHandler::onGetEntityKeyForAccountFailure( LogOnStatus status,
    const BW::string & errorMsg )
{
    const char * msg;
    bool isError = false;

    switch (status.value())
    {
        case LogOnStatus::LOGIN_REJECTED_NO_SUCH_USER:
            msg = "Unknown user.";
            break;

        case LogOnStatus::LOGIN_REJECTED_INVALID_PASSWORD:
            msg = "Invalid password.";
            break;

        case LogOnStatus::LOGIN_REJECTED_BAN:
            msg = (errorMsg.empty()) ? "User is banned." : errorMsg.c_str();
            break;

        case LogOnStatus::LOGIN_REJECTED_DB_GENERAL_FAILURE:
            msg = "Unexpected database failure.";
            isError = true;
            break;

        default:
            msg = (errorMsg.empty()) ? "Unspecified error" : errorMsg.c_str();
            isError = true;
            break;
    }

    if (isError)
    {
        ERROR_MSG( "LoginHandler::onMapLoginToEntityKeyComplete: "
                        "Failed for username '%s': (%d) %s\n",
                    pParams_->username().c_str(), status.value(), msg );
    }
    else
    {
        NOTICE_MSG( "LoginHandler::onMapLoginToEntityKeyComplete: "
                        "Failed for username '%s': (%d) %s\n",
                    pParams_->username().c_str(), status.value(), msg );
    }

    DBApp::instance().sendFailure( replyID_, replyAddr_,
        status, msg );
    delete this;
}


/**
 *  DBApp::GetEntityHandler override
 */
void LoginHandler::onGetEntityCompleted( bool isOK,
                    const EntityDBKey & entityKey,
                    const EntityMailBoxRef * pBaseEntityLocation )
{
    if (!isOK)
    {
        if (shouldCreateNewOnLoadFailure_)
        {
            this->createNewEntity( entityKey_.typeID,
                    shouldRememberNewOnLoadFailure_ );
        }
        else
        {
            ERROR_MSG( "LoginHandler::onGetEntityCompleted: "
                            "Entity '%" FMT_DBID "' does not exist\n",
                        entityKey_.dbID );

            this->sendFailureReply(
                LogOnStatus::LOGIN_REJECTED_NO_SUCH_USER,
                "Failed to load entity." );
        }
        return;
    }

    entityKey_ = entityKey;

    if (pBaseEntityLocation != NULL)
    {
        baseRef_ = *pBaseEntityLocation;
        pBaseRef_ = &baseRef_;
    }
    else
    {
        pBaseRef_ = NULL;
    }

    if (pStrmDbID_)
    {
        // Means ekey.dbID was 0 when we called prepareCreateEntityBundle()
        // Now fix up everything.
        *pStrmDbID_ = entityKey.dbID;
    }

    this->checkOutEntity();
}


/**
 *  This function checks out the login entity. Must be called after
 *  entity has been successfully retrieved from the database.
 */
void LoginHandler::checkOutEntity()
{
    if ((pBaseRef_ == NULL) &&
        DBApp::instance().onStartEntityCheckout( entityKey_ ))
    {
        // Not checked out and not in the process of being checked out.
        DBApp::setBaseRefToLoggingOn( baseRef_, entityKey_.typeID );
        pBaseRef_ = &baseRef_;

        DBApp::instance().setBaseEntityLocation( entityKey_, baseRef_,
                reserveBaseMailboxHandler_ );
        // When completes, onReservedBaseMailbox() is called.
    }
    else    // Checked out
    {
        DBApp::instance().onLogOnLoggedOnUser( entityKey_.typeID,
            entityKey_.dbID, pParams_, clientAddr_, replyAddr_, replyID_,
            pBaseRef_, dataForClient_, dataForBaseEntity_ );

        delete this;
    }
}


/**
 *  This method is called when writing the entity has finished.
 */
void LoginHandler::onPutEntityComplete( bool isOK, DatabaseID dbID )
{
    MF_ASSERT( pStrmDbID_ );
    *pStrmDbID_ = dbID;

    DBApp & dbApp = DBApp::instance();

    if (isOK)
    {
        entityKey_.dbID = dbID;

        dbApp.pBillingSystem()->setEntityKeyForAccount(
            pParams_->username(), pParams_->password(), entityKey_, *this );

        if (dbApp.shouldCacheLogOnRecords())
        {
            dbApp.logOnRecordsCache().insert( entityKey_, *pBaseRef_ );
        }
    }
    else
    {   // Failed "rememberEntity" function.
        ERROR_MSG( "LoginHandler::onPutEntityComplete: "
                "Failed to write default entity for username '%s'\n",
            pParams_->username().c_str());

        this->onReservedBaseMailbox( isOK, dbID );
        // Let them log in anyway since this is meant to be a convenience
        // feature during product development.
    }
}


/**
 *  This method is called when the record in bigworldLogOns has been created
 *  or returned.
 */
void LoginHandler::onReservedBaseMailbox( bool isOK, DatabaseID dbID )
{
    if (isOK)
    {
        this->sendCreateEntityMsg();
    }
    else
    {
        DBApp::instance().onCompleteEntityCheckout( entityKey_, NULL );
        // Something horrible like database disconnected or something.
        this->sendFailureReply(
                LogOnStatus::LOGIN_REJECTED_DB_GENERAL_FAILURE,
                "Unexpected database failure." );
    }
}


/**
 *  This method is called when the record in bigworldLogOns has been set.
 */
void LoginHandler::onSetBaseMailbox( bool isOK, DatabaseID dbID )
{
    DBApp::instance().onCompleteEntityCheckout( entityKey_,
            isOK ? &baseRef_ : NULL );

    if (isOK)
    {
        this->sendReply();
    }
    else
    {
        // Something horrible like database disconnected or something.
        this->sendFailureReply(
                LogOnStatus::LOGIN_REJECTED_DB_GENERAL_FAILURE,
                "Unexpected database failure." );
    }
}


/**
 *  This method sends the BaseAppMgrInterface::createEntity message.
 *  Assumes bundle has the right data.
 */
inline void LoginHandler::sendCreateEntityMsg()
{
    INFO_MSG( "DBApp::logOn: %s\n", pParams_->username().c_str() );

    DBApp::instance().baseAppMgr().send( &bundle_ );
}


/**
 *  This method sends the reply to the LoginApp. Assumes bundle already
 *  has the right data.
 *
 *  This should also be the last thing this object is doing so this
 *  also destroys this object.
 */
inline void LoginHandler::sendReply()
{
    DBApp::instance().interface().sendOnExistingChannel(
            replyAddr_, bundle_ );

    delete this;
}


/**
 *  This method sends a failure reply to the LoginApp.
 *
 *  This should also be the last thing this object is doing so this
 *  also destroys this object.
 */
inline void LoginHandler::sendFailureReply( LogOnStatus status,
        const char * msg )
{
    DBApp::instance().sendFailure( replyID_, replyAddr_, status, msg );
    delete this;
}


/**
 *  This function creates a new login entity for the user.
 */
void LoginHandler::createNewEntity( EntityTypeID entityTypeID,
        bool shouldRemember )
{
    entityKey_.typeID = entityTypeID;

    // If a username is specified explicitly in an EntityDBKey use it,
    // otherwise stub with login name (this is used for test purposes only).
    const BW::string & newEntityName = entityKey_.name.empty() ?
        pParams_->username() : entityKey_.name;

    if (pStrmDbID_ == NULL)
    {
        pStrmDbID_ = DBApp::prepareCreateEntityBundle( entityTypeID,
            0, clientAddr_, this, bundle_, pParams_, &dataForBaseEntity_ );
    }

    bool isDefaultEntityOK;

    if (shouldRemember)
    {
        // __kyl__ (13/7/2005) Need additional MemoryOStream because I
        // haven't figured out how to make a BinaryIStream out of a
        // Mercury::Bundle.
        MemoryOStream strm;
        isDefaultEntityOK = DBApp::instance().defaultEntityToStrm(
            entityKey_.typeID, newEntityName, strm );

        if (isDefaultEntityOK)
        {
            bundle_.transfer( strm, strm.size() );

            // If <rememberUnknown> is true we need to append the game time
            // so that secondary databases have a timestamp associated to
            // them.
            strm << GameTime( 0 );
            strm.rewind();

            // Put entity data into DB and set baseref to "logging on".
            DBApp::setBaseRefToLoggingOn( baseRef_, entityKey_.typeID );
            pBaseRef_ = &baseRef_;

            DBApp::instance().putEntity( entityKey_,
                /* entityID: */ 0,
                /* pStream: */ &strm,
                /* pBaseMailbox: */ &baseRef_,
                /* removeBaseMailbox:*/ false,
                /* putExplicitID:*/ false,
                /* updateAutoLoad */ UPDATE_AUTO_LOAD_RETAIN,
                /* handler */ putEntityHandler_ );
            // When putEntity() completes, onPutEntityComplete() is called.
        }
    }
    else
    {
        *pStrmDbID_ = 0;

        // No need for additional MemoryOStream. Just stream into bundle.
        isDefaultEntityOK = DBApp::instance().defaultEntityToStrm(
            entityKey_.typeID, newEntityName, bundle_ );

        if (isDefaultEntityOK)
        {
            this->sendCreateEntityMsg();
        }
    }

    if (!isDefaultEntityOK)
    {
        ERROR_MSG( "LoginHandler::createNewEntity: "
                    "Failed to create default entity for username '%s'\n",
                pParams_->username().c_str());

        this->sendFailureReply( LogOnStatus::LOGIN_CUSTOM_DEFINED_ERROR,
                "Failed to create default entity" );
    }
}


/*
 *  Mercury::ReplyMessageHandler override.
 */
void LoginHandler::handleMessage( const Mercury::Address & source,
    Mercury::UnpackedMessageHeader & header,
    BinaryIStream & data,
    void * arg )
{
    Mercury::Address proxyAddr;

    data >> proxyAddr;

    if (proxyAddr.ip == 0)
    {
        LogOnStatus::Status status;
        switch (proxyAddr.port)
        {
            case BaseAppMgrInterface::CREATE_ENTITY_ERROR_NO_BASEAPPS:
                status = LogOnStatus::LOGIN_REJECTED_NO_BASEAPPS;
                break;
            case BaseAppMgrInterface::CREATE_ENTITY_ERROR_BASEAPPS_OVERLOADED:
                status = LogOnStatus::LOGIN_REJECTED_BASEAPP_OVERLOAD;
                break;
            default:
                status = LogOnStatus::LOGIN_CUSTOM_DEFINED_ERROR;
                break;
        }

        this->handleFailure( &data, status );
    }
    else
    {
        data >> baseRef_;

        bundle_.clear();
        bundle_.startReply( replyID_ );

        // Assume success.
        bundle_ << (uint8)LogOnStatus::LOGGED_ON;
        bundle_ << proxyAddr;

        // session key
        MF_ASSERT_DEV( data.remainingLength() == sizeof( SessionKey ) );
        bundle_.transfer( data, data.remainingLength() );

        bundle_ << dataForClient_;

        if (entityKey_.dbID != 0)
        {
            pBaseRef_ = &baseRef_;
            DBApp::instance().setBaseEntityLocation( entityKey_,
                    baseRef_, setBaseMailboxHandler_ );
            // When completes, onSetBaseMailbox() is called.
        }
        else
        {
            // Must be "createUnknown", and "rememberUnknown" is false.
            this->sendReply();
        }
    }
}


/*
 *  Mercury::ReplyMessageHandler override.
 */
void LoginHandler::handleException( const Mercury::NubException & ne,
    void * arg )
{
    MemoryOStream mos;
    mos << "BaseAppMgr timed out creating entity.";
    this->handleFailure( &mos,
            LogOnStatus::LOGIN_REJECTED_BASEAPPMGR_TIMEOUT );
}


void LoginHandler::handleShuttingDown( const Mercury::NubException & ne,
    void * arg )
{
    INFO_MSG( "LoginHandler::handleShuttingDown: Ignoring\n" );
    delete this;
}


/**
 *  Handles a failure to create entity base.
 */
void LoginHandler::handleFailure( BinaryIStream * pData, LogOnStatus reason )
{
    bundle_.clear();
    bundle_.startReply( replyID_ );

    bundle_ << (uint8)reason.value();

    bundle_.transfer( *pData, pData->remainingLength() );

    if (entityKey_.dbID != 0)
    {
        pBaseRef_ = NULL;
        DBApp::instance().clearBaseEntityLocation( entityKey_,
                setBaseMailboxHandler_ );
        // When completes, onSetBaseMailbox() is called.
    }
    else
    {
        // Must be "createUnknown" and "rememberUnknown" is false.
        this->sendReply();
    }
}


// -----------------------------------------------------------------------------
// Section: PutEntityHandler
// -----------------------------------------------------------------------------

/**
 *  Constructor.
 */
PutEntityHandler::PutEntityHandler( LoginHandler & loginHandler,
        MemberFunc memberFunc ) :
    loginHandler_( loginHandler ),
    memberFunc_( memberFunc )
{
}


/**
 *  This method calls through to the appropriate member of LoginHandler.
 */
void PutEntityHandler::onPutEntityComplete( bool isOK, DatabaseID dbID )
{
    (loginHandler_.*memberFunc_)( isOK, dbID );
}

BW_END_NAMESPACE

// login_handler.cpp
