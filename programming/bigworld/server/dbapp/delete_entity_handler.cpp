#include "script/first_include.hpp"

#include "delete_entity_handler.hpp"

#include "dbapp.hpp"

#include "db_storage/db_entitydefs.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor. For deleting entity by database ID.
 */
DeleteEntityHandler::DeleteEntityHandler( EntityTypeID typeID, DatabaseID dbID,
		const Mercury::Address& srcAddr, Mercury::ReplyID replyID ) :
	replyBundle_(),
	srcAddr_(srcAddr),
	ekey_( typeID, dbID )
{
	replyBundle_.startReply(replyID);
}


/**
 *	Starts the process of deleting the entity.
 */
void DeleteEntityHandler::deleteEntity()
{
	if (DBApp::instance().getEntityDefs().isValidEntityType( ekey_.typeID ))
	{
		// See if it is checked out
		DBApp::instance().getEntity( ekey_, NULL, true, *this );
		// When getEntity() completes, onGetEntityCompleted() is called.
	}
	else
	{
		ERROR_MSG( "DeleteEntityHandler::deleteEntity: Invalid entity type "
				"%d\n", int(ekey_.typeID) );
		this->addFailure();

		this->sendReply();
	}
}


/**
 *	GetEntityHandler overrides
 */
void DeleteEntityHandler::onGetEntityCompleted( bool isOK,
		const EntityDBKey & entityKey,
		const EntityMailBoxRef * pBaseEntityLocation )
{
	ekey_ = entityKey;

	if (isOK)
	{
		if (this->isActiveMailBox( pBaseEntityLocation ))
		{
			TRACE_MSG( "DeleteEntityHandler::onGetEntityCompleted: "
					"Entity checked out\n" );
			// tell the caller where to find it
			replyBundle_ << *pBaseEntityLocation;
		}
		else
		{	// __kyl__ TODO: Is it a problem if we delete the entity when it's awaiting creation?
			DBApp::instance().delEntity( ekey_, 0, *this );
			// When delEntity() completes, onDelEntityComplete() is called.
			return;	// Don't send reply just yet.
		}
	}
	else
	{	// Entity doesn't exist
		TRACE_MSG( "DeleteEntityHandler::onGetEntityCompleted: "
				"No such entity\n" );
		this->addFailure();
	}

	this->sendReply();
}


/**
 *	IDatabase::IDelEntityHandler overrides
 */
void DeleteEntityHandler::onDelEntityComplete( bool isOK )
{
	if (isOK)
	{
		TRACE_MSG( "DeleteEntityHandler::onDelEntityComplete: succeeded\n" );
	}
	else
	{
		ERROR_MSG( "DeleteEntityHandler::onDelEntityComplete: "
				"Failed to delete entity '%s' (%" FMT_DBID ") of type %d\n",
			ekey_.name.c_str(), ekey_.dbID, ekey_.typeID );
		this->addFailure();
	}

	this->sendReply();
}


/**
 *	This method adds failure data to the reply bundle.
 */
void DeleteEntityHandler::addFailure()
{
	// Just needs to be anything that's a different size than a mailbox.
	replyBundle_ << int32(-1);
}


/**
 *	This method sends the reply.
 */
void DeleteEntityHandler::sendReply()
{
	DBApp::getChannel( srcAddr_ ).send( &replyBundle_ );
	delete this;
}

BW_END_NAMESPACE

// delete_entity_handler.cpp
