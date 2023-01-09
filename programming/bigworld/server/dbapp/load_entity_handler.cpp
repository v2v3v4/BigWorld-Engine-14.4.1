#include "script/first_include.hpp"

#include "load_entity_handler.hpp"

#include "network/channel_sender.hpp"
#include "network/bundle.hpp"


BW_BEGIN_NAMESPACE


void LoadEntityHandler::loadEntity()
{
    // Start reply bundle even though we're not sure the entity exists.
    // This is to take advantage of getEntity() streaming directly into bundle.
    replyBundle_.startReply( replyID_ );
    replyBundle_ << (uint8) LogOnStatus::LOGGED_ON;

    if (ekey_.dbID)
    {
        replyBundle_ << ekey_.dbID;
    }
    else
    {
        // Reserve space for a DBId since we don't know what it is yet.
        pStrmDbID_ = reinterpret_cast< DatabaseID * >(
                        replyBundle_.reserve( sizeof( *pStrmDbID_ ) ) );
    }

    DBApp::instance().getEntity( ekey_, &replyBundle_, true, *this );
    // When getEntity() completes, onGetEntityCompleted() is called.
}


/**
 *  GetEntityHandler override
 */
void LoadEntityHandler::onGetEntityCompleted( bool isOK,
        const EntityDBKey & entityKey,
        const EntityMailBoxRef * pBaseEntityLocation )
{
    ekey_ = entityKey;

    if (pBaseEntityLocation)
    {
        baseRef_ = *pBaseEntityLocation;
    }

    if (isOK)
    {
        if (pBaseEntityLocation) // Already checked out
        {
            this->sendAlreadyCheckedOutReply( *pBaseEntityLocation );
        }
        else if (DBApp::instance().onStartEntityCheckout( entityKey ))
        {
            // Not checked out and not in the process of being checked out.
            if (pStrmDbID_)
            {
                // Now patch the dbID in the stream.
                *pStrmDbID_ = entityKey.dbID;
            }

            // Check out entity.
            baseRef_.init( entityID_, srcAddr_, EntityMailBoxRef::BASE,
                entityKey.typeID );

            DBApp::instance().setBaseEntityLocation( entityKey, baseRef_,
                *this );

            // When completes, onPutEntityComplete() is called.
            // __kyl__ (24/6/2005) Race condition when multiple check-outs of
            // the same entity at the same time. More than one can succeed.
            // Need to check for checking out entity?
            // Also, need to prevent deletion of entity which checking out
            // entity.

            return; // Don't delete ourselves just yet.
        }
        else // In the process of being checked out.
        {
            MF_VERIFY( DBApp::instance().registerCheckoutCompletionListener(
                    entityKey.typeID, entityKey.dbID, *this ) );
            // onCheckoutCompleted() will be called when the entity is fully
            // checked out.
            return; // Don't delete ourselves just yet.
        }
    }
    else
    {
        if (entityKey.dbID)
        {
            ERROR_MSG( "LoadEntityHandler::loadEntity: "
                    "No such entity %" FMT_DBID " of type %d.\n",
                entityKey.dbID, entityKey.typeID );
        }
        else
        {
            NOTICE_MSG( "LoadEntityHandler::loadEntity: "
                    "No such entity %s of type %d.\n",
                entityKey.name.c_str(), entityKey.typeID );
        }
        DBApp::instance().sendFailure( replyID_, srcAddr_,
            LogOnStatus::LOGIN_REJECTED_NO_SUCH_USER, "No such user." );
    }

    delete this;
}


/**
 *  IDatabase::IPutEntityHandler override
 */
void LoadEntityHandler::onPutEntityComplete( bool isOK, DatabaseID )
{
    DBApp & dbApp = DBApp::instance();

    if (isOK)
    {
        DBApp::getChannel( srcAddr_ ).send( &replyBundle_ );

        if (dbApp.shouldCacheLogOnRecords())
        {
            dbApp.logOnRecordsCache().insert( ekey_, baseRef_ );
        }
    }
    else
    {
        // Something horrible like database disconnected or something.
        dbApp.sendFailure( replyID_, srcAddr_,
            LogOnStatus::LOGIN_REJECTED_DB_GENERAL_FAILURE,
            "Unexpected failure from database layer." );
    }
    // Need to call onCompleteEntityCheckout() after we send reply since
    // onCompleteEntityCheckout() can trigger other tasks to send their
    // own replies which assumes that the entity creation has already
    // succeeded or failed (depending on the parameters we pass here).
    // Obviously, if they sent their reply before us, then the BaseApp will
    // get pretty confused since the entity creation is not completed until
    // it gets our reply.
    dbApp.onCompleteEntityCheckout( ekey_, isOK ? &baseRef_ : NULL );

    delete this;
}


/**
 *  DBApp::ICheckoutCompletionListener override
 */
void LoadEntityHandler::onCheckoutCompleted( const EntityMailBoxRef* pBaseRef )
{
    if (pBaseRef)
    {
        this->sendAlreadyCheckedOutReply( *pBaseRef );
    }
    else
    {
        // __kyl__ (11/8/2006) Currently there are no good reason that a
        // checkout would fail. This usually means something has gone horribly
        // wrong. We'll just return an error for now. We could retry the
        // operation from scratch but that's just too much work for now.
        DBApp::instance().sendFailure( replyID_, srcAddr_,
                LogOnStatus::LOGIN_REJECTED_DB_GENERAL_FAILURE,
                "Unexpected failure from database layer." );
    }

    delete this;
}


/**
 *  This method sends back a reply that says that the entity is already
 *  checked out.
 */
void LoadEntityHandler::sendAlreadyCheckedOutReply(
        const EntityMailBoxRef& baseRef )
{
    Mercury::ChannelSender sender( DBApp::getChannel( srcAddr_ ) );
    Mercury::Bundle & bundle = sender.bundle();

    bundle.startReply( replyID_ );
    bundle << uint8( LogOnStatus::LOGIN_REJECTED_ALREADY_LOGGED_IN );
    bundle << ekey_.dbID;
    bundle << baseRef;
}

BW_END_NAMESPACE

// load_entity_handler.cpp
