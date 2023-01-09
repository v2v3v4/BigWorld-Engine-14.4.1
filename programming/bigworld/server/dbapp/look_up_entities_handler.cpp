#include "script/first_include.hpp"

#include "look_up_entities_handler.hpp"

#include "dbapp.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
LookUpEntitiesHandler::LookUpEntitiesHandler( DBApp & dbApp, 
			const Mercury::Address & srcAddr,
			Mercury::ReplyID replyID ):
		dbApp_( dbApp ),
		replyBundle_(),
		srcAddr_( srcAddr )
{
	replyBundle_.startReply( replyID );
}


/**
 *	Start the lookup.
 */
void LookUpEntitiesHandler::lookUpEntities( EntityTypeID entityTypeID,
		const LookUpEntitiesCriteria & criteria )
{		
	dbApp_.getIDatabase().lookUpEntities( entityTypeID, criteria, *this );
}


/**
 *	Override from IDatabase::ILookUpEntitiesHandler.
 */
void LookUpEntitiesHandler::onLookUpEntitiesStart( size_t numEntities )
{
	replyBundle_ << int( numEntities );
}


/**
 *	Override from IDatabase::ILookUpEntitiesHandler.
 */
void LookUpEntitiesHandler::onLookedUpEntity( DatabaseID databaseID,
	const EntityMailBoxRef & baseEntityLocation )
{
	replyBundle_ << databaseID << baseEntityLocation;
}


/**
 *	Override from IDatabase::ILookUpEntitiesHandler.
 */
void LookUpEntitiesHandler::onLookUpEntitiesEnd( bool hasError )
{
	if (hasError)
	{
		replyBundle_ << int( -1 );
	}

	this->sendReply();

	delete this;
}


/**
 *	This method sends the appropriate reply back to the requestor.
 */
void LookUpEntitiesHandler::sendReply()
{
	Mercury::NetworkInterface & interface = dbApp_.interface();
	interface.sendOnExistingChannel( srcAddr_, replyBundle_ );
}

BW_END_NAMESPACE

// look_up_entities_handler.cpp

