#include "script/first_include.hpp"

#include "look_up_entity_handler.hpp"

#include "dbapp.hpp"
#include "dbapp_config.hpp"

#include "db_storage/db_entitydefs.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
LookUpEntityHandler::LookUpEntityHandler( const Mercury::Address & srcAddr,
		Mercury::ReplyID replyID ) :
	replyID_( replyID ),
	srcAddr_( srcAddr )
{
}

/**
 *	Starts the process of looking up the entity.
 *	If entityKey.dbID == 0, that means we are using the name to lookup the
 *	entity, so we need to get the DBID first.
 *	For a look up by name, if the entity does not exist in the database, False
 *	will be called back; for a look up by DBID, if the entity is not logged on,
 *	True will be called back, regardless of whether it exists in the database
 *	or not. This allows us to avoid a database query if we are using the cache.
 */
void LookUpEntityHandler::lookUpEntity( const EntityDBKey & entityKey )
{

	DBApp & db = DBApp::instance();

	if (!db.getEntityDefs().isValidEntityType( entityKey.typeID ))
	{		ERROR_MSG( "LookUpEntityHandler::lookUpEntity: Invalid entity type "
				"%d\n", entityKey.typeID );

		this->sendReply( true/*hasError*/, NULL/*pBaseEntityLocation*/ );
		return;
	}

	if (db.shouldCacheLogOnRecords())
	{
		if (0 == entityKey.dbID)
		{
			db.getIDatabase().getDatabaseIDFromName( entityKey, *this );
			return;
		}
		EntityMailBoxRef mb;
		bool hasBaseLocation = db.logOnRecordsCache().lookUp( entityKey, mb );
		this->onGetEntityComplete( true, entityKey, hasBaseLocation ? &mb : NULL );
		return;
	}

	// When getEntity() completes, onGetEntityCompleted() is called.
	db.getEntity( entityKey, NULL, true, *this );
}


void LookUpEntityHandler::onGetEntityCompleted( bool isOK,
		const EntityDBKey & /*entityKey*/,
		const EntityMailBoxRef * pBaseEntityLocation )
{
	this->sendReply( !isOK,
			isOK && this->isActiveMailBox( pBaseEntityLocation ) ?
				pBaseEntityLocation : NULL );
}

/**
 *	GetEntityHandler override.
 *	This is a callback of getDatabaseIDFromName.
 *	If we can not lookup the DBID by its name, we should return False.
 *	If the desired entity is not in the cache, there is no need to query the
 *	database. We assume that the cache will be exactly the same as the
 *	bigworldLogOns table.
 */
void LookUpEntityHandler::onGetDbIDComplete( bool isOK,
                const EntityDBKey & entityKey )
{
	if (!isOK)
	{
		this->onGetEntityComplete( /* isOK */ false, entityKey, NULL );
		return;
	}

	DBApp & db = DBApp::instance();

	if (db.shouldCacheLogOnRecords())
	{
		EntityMailBoxRef mb;
		bool hasBaseLocation = db.logOnRecordsCache().lookUp( entityKey, mb );
		this->onGetEntityComplete( /* isOK */ true, entityKey,
				hasBaseLocation ? &mb : NULL );
		return;
	}

	db.getEntity( entityKey, NULL, true, *this );
}


/**
 *	This method writes the reply to a bundle
 */
void LookUpEntityHandler::writeReply( bool hasError,
		const EntityMailBoxRef * pBaseEntityLocation,
		Mercury::Bundle & bundle )
{
	bundle.startReply( replyID_ );

	if (hasError)
	{
		bundle << int32(-1);
	}
	else if (pBaseEntityLocation)
	{
		bundle << *pBaseEntityLocation;
	}
}


/**
 *	This method sends the reply and deletes this object.
 */
void LookUpEntityHandler::sendReply( bool hasError,
		const EntityMailBoxRef * pBaseEntityLocation )
{
	Mercury::NetworkInterface & interface = DBApp::instance().interface();
	Mercury::UDPChannel * pChannel = interface.findChannel( srcAddr_ );

	if (pChannel)
	{
		Mercury::Bundle & bundle = pChannel->bundle();
		this->writeReply( hasError, pBaseEntityLocation, bundle );

		if (DBAppConfig::shouldDelayLookUpSend())
		{
			interface.delayedSend( *pChannel );
		}
		else
		{
			pChannel->send();
		}
	}
	else
	{
		// Send off channel
		Mercury::UDPBundle bundle;
		this->writeReply( hasError, pBaseEntityLocation, bundle );
		interface.send( srcAddr_, bundle );
	}

	delete this;
}

BW_END_NAMESPACE

// look_up_entity_handler.cpp
