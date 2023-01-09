#include "script/first_include.hpp"

#include "put_entity_task.hpp"

#include "../buffered_entity_tasks.hpp"
#include "db_storage_mysql/mappings/entity_type_mapping.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PutEntityTask
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	Stores all required information so that the task can be executed in a
 *	separate thread.
 */
PutEntityTask::PutEntityTask( const EntityTypeMapping * pEntityTypeMapping,
								DatabaseID databaseID,
								EntityID entityID,
								BinaryIStream * pStream,
								const EntityMailBoxRef * pBaseMailbox,
								bool removeBaseMailbox,
								bool putExplicitID,
								UpdateAutoLoad updateAutoLoad,
								IDatabase::IPutEntityHandler & handler,
								GameTime * pGameTime ) :
	EntityTaskWithID( *pEntityTypeMapping, databaseID, entityID, "PutEntityTask" ),
	writeEntityData_( false ),
	writeBaseMailbox_( false ),
	removeBaseMailbox_( removeBaseMailbox ),
	putExplicitID_( putExplicitID ),
	updateAutoLoad_( updateAutoLoad ),
	handler_( handler ),
	pGameTime_( pGameTime )
{
	if (pStream != NULL)
	{
		stream_.transfer( *pStream, pStream->remainingLength() );
		writeEntityData_ = true;
	}

	if (pBaseMailbox)
	{
		baseMailbox_ = *pBaseMailbox;
		writeBaseMailbox_ = true;
	}
}


/**
 *	This method writes the entity data into the database.
 */
void PutEntityTask::performBackgroundTask( MySql & conn )
{
	bool definitelyExists = false;

	MF_ASSERT( dbID_ != PENDING_DATABASE_ID );

	if (writeEntityData_)
	{
		if (dbID_ != 0 && !putExplicitID_)
		{
			if (!entityTypeMapping_.update( conn, dbID_, stream_, pGameTime_ ))
			{
				ERROR_MSG( "PutEntityTask::performBackgroundTask: "
					"Failed to update Entity record "
						"('%s', dbID %" FMT_DBID ")\n",
					entityTypeMapping_.typeName().c_str(), dbID_ );
			}
		}
		else
		{
			if (!putExplicitID_)
			{
				// pGameTime_ only used by consolidate_dbs which should always have
				// a DatabaseID.
				MF_ASSERT( pGameTime_ == NULL );
				dbID_ = entityTypeMapping_.insertNew( conn, stream_ );
			}
			else
			{
				dbID_ = entityTypeMapping_.insertExplicit( conn, dbID_, stream_ );
			}

			if (dbID_ == 0)
			{
				ERROR_MSG( "PutEntityTask::performBackgroundTask: "
					"Failed to create new Entity '%s'\n",
						entityTypeMapping_.typeName().c_str() );
			}
		}

		definitelyExists = true;
	}

	if (writeBaseMailbox_)
	{
		// Check for existence to prevent adding invalid LogOn records
		if (definitelyExists ||
			entityTypeMapping_.checkExists( conn, dbID_ ))
		{
			// Add or update the log on record.
			entityTypeMapping_.addLogOnRecord( conn,
					dbID_, baseMailbox_ );
		}
	}
	else if (removeBaseMailbox_)
	{
		entityTypeMapping_.removeLogOnRecord( conn, dbID_ );
	}

	if (updateAutoLoad_ != UPDATE_AUTO_LOAD_RETAIN)
	{
		entityTypeMapping_.updateAutoLoad( conn, dbID_, 
			(updateAutoLoad_ == UPDATE_AUTO_LOAD_TRUE) );
	}
}


/**
 *
 */
void PutEntityTask::performEntityMainThreadTask( bool succeeded )
{
	handler_.onPutEntityComplete( succeeded, dbID_ );
}


/**
 *	This method is called if the background task fails and should be retried.
 */
void PutEntityTask::onRetry()
{
	stream_.rewind();
}

BW_END_NAMESPACE

// put_entity_task.cpp
