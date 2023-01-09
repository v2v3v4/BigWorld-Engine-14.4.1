#ifndef LOGIN_HANDLER_HPP
#define LOGIN_HANDLER_HPP

#include "get_entity_handler.hpp"

#include "connection/log_on_params.hpp"
#include "db_storage/billing_system.hpp"
#include "db_storage/idatabase.hpp"

#include "network/interfaces.hpp"
#include "network/udp_bundle.hpp"


BW_BEGIN_NAMESPACE

class LoginHandler;

/**
 *  This simple class is used to handle the different versions of
 *  onPutEntityComplete.
 */
class PutEntityHandler : public IDatabase::IPutEntityHandler
{
public:
    typedef void (LoginHandler::*MemberFunc)( bool isOK, DatabaseID );

    PutEntityHandler( LoginHandler & loginHandler, MemberFunc memberFunc );

    virtual void onPutEntityComplete( bool isOK, DatabaseID dbID );

private:
    LoginHandler & loginHandler_;
    MemberFunc memberFunc_;
};


/**
 *  This class is used to receive the reply from a createEntity call to
 *  BaseAppMgr.
 */
class LoginHandler : public Mercury::ReplyMessageHandler,
                     public IGetEntityKeyForAccountHandler,
                     public ISetEntityKeyForAccountHandler,
                     public GetEntityHandler,
                     public IDatabase::IPutEntityHandler
{
public:
    LoginHandler(
            LogOnParamsPtr pParams,
            const Mercury::Address & clientAddr,
            const Mercury::Address & replyAddr,
            Mercury::ReplyID replyID );
    ~LoginHandler();

    void login();

    // Mercury::ReplyMessageHandler overrides
    virtual void handleMessage( const Mercury::Address & source,
        Mercury::UnpackedMessageHeader & header,
        BinaryIStream & data, void * arg );

    virtual void handleException( const Mercury::NubException & ne,
        void * arg );
    virtual void handleShuttingDown( const Mercury::NubException & ne,
        void * arg );

    // IGetEntityKeyForAccountHandler overrides
    virtual void onGetEntityKeyForAccountSuccess( const EntityKey & ekey,
            const BW::string & clientData,
            const BW::string & billingData );

    virtual void onGetEntityKeyForAccountLoadFromUsername(
            EntityTypeID entityType,
            const BW::string & username,
            bool shouldCreateNewOnLoadFailure,
            bool shouldRememberNewOnLoadFailure,
            const BW::string & clientData,
            const BW::string & billingData );

    virtual void onGetEntityKeyForAccountCreateNew( EntityTypeID entityType,
            bool shouldRemember,
            const BW::string & clientData,
            const BW::string & billingData );

    virtual void onGetEntityKeyForAccountFailure( LogOnStatus status,
            const BW::string & errorMsg );

    // ISetEntityKeyForAccountHandler overrides
    virtual void onSetEntityKeyForAccountSuccess( const EntityKey & ekey );
    virtual void onSetEntityKeyForAccountFailure( const EntityKey & ekey );

    // DBApp::getEntityHandler override
    virtual void onGetEntityCompleted( bool isOK,
                const EntityDBKey & entityKey,
                const EntityMailBoxRef * pBaseEntityLocation );

    // Called by the different PutEntityHandlers
    void onPutEntityComplete( bool isOK, DatabaseID dbID );
    void onReservedBaseMailbox( bool isOK, DatabaseID dbID );
    void onSetBaseMailbox( bool isOK, DatabaseID dbID );

private:
    void handleFailure( BinaryIStream * pData, LogOnStatus reason );
    void checkOutEntity();
    void createNewEntity( EntityTypeID entityTypeID, bool shouldRemember );
    void loadEntity( const EntityDBKey & ekey );
    void sendCreateEntityMsg();
    void sendReply();
    void sendFailureReply( LogOnStatus status, const char * msg = NULL );

    EntityDBKey         entityKey_;
    LogOnParamsPtr      pParams_;
    Mercury::Address    clientAddr_;
    Mercury::Address    replyAddr_;
    Mercury::ReplyID    replyID_;

    Mercury::UDPBundle  bundle_;
    EntityMailBoxRef    baseRef_;
    EntityMailBoxRef *  pBaseRef_;

    PutEntityHandler    putEntityHandler_;
    PutEntityHandler    reserveBaseMailboxHandler_;
    PutEntityHandler    setBaseMailboxHandler_;

    DatabaseID *        pStrmDbID_;

    bool                shouldCreateNewOnLoadFailure_;
    bool                shouldRememberNewOnLoadFailure_;

    BW::string          dataForClient_;
    BW::string          dataForBaseEntity_;
};

BW_END_NAMESPACE

#endif // LOGIN_HANDLER_HPP
