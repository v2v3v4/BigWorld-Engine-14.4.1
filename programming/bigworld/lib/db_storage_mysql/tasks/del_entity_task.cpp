#include "script/first_include.hpp"

#include "del_entity_task.hpp"

#include "db_storage_mysql/mappings/entity_type_mapping.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
DelEntityTask::DelEntityTask( const EntityTypeMapping * pEntityMapping,
		const EntityDBKey & entityKey,
		EntityID entityID,
		IDatabase::IDelEntityHandler & handler ) :
	EntityTaskWithID( *pEntityMapping, entityKey.dbID , entityID, "DelEntityTask" ),
	entityKey_( entityKey ),
	handler_( handler )
{
}


/**
 *	This method deletes an entity from the database. May be executed in a
 *	separate thread.
 */
void DelEntityTask::performBackgroundTask( MySql & conn )
{
	DatabaseID dbID = entityKey_.dbID;

	MF_ASSERT( dbID != PENDING_DATABASE_ID );

	if (dbID == 0)
	{
		dbID = entityTypeMapping_.getDbID( conn, entityKey_.name );
	}
	else
	{
		// If we have a dbID, ensure that we have a name in our key as well.
		// The name is needed so that removeCachedEntityKey can be performed
		// in the main thread.
		if (entityTypeMapping_.hasIdentifier())
		{
			BW::string identifier;
			if (!entityTypeMapping_.getName( conn,
					entityKey_.dbID, identifier ))
			{
				this->setFailure();
				return;
			}

			entityKey_.name = identifier;
		}
	}

	if (dbID == 0)
	{
		this->setFailure();
		return;
	}

	if (!entityTypeMapping_.deleteWithID( conn, dbID ))
	{
		this->setFailure();
		return;
	}

	entityTypeMapping_.removeLogOnRecord( conn, dbID );
}


/**
 *	This method is called in the main thread after run() completes.
 */
void DelEntityTask::performEntityMainThreadTask( bool succeeded )
{
	const_cast< EntityTypeMapping & >(
		entityTypeMapping_ ).removeCachedEntityKey( entityKey_ );

	handler_.onDelEntityComplete( succeeded );
}

BW_END_NAMESPACE

// del_entity_task.cpp
