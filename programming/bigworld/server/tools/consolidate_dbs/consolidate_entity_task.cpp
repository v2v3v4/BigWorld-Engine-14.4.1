#include "script/first_include.hpp"

#include "consolidate_entity_task.hpp"

#include "db_storage_mysql/mappings/entity_type_mapping.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
ConsolidateEntityTask::ConsolidateEntityTask(
						const EntityTypeMapping * pEntityTypeMapping,
						DatabaseID databaseID,
						BinaryIStream & stream,
						GameTime time,
						IDatabase::IPutEntityHandler & handler ) :
	PutEntityTask( pEntityTypeMapping,
			databaseID, 0, &stream, NULL, /*removeBaseMailbox*/ false, 
			/*putExplicitID*/ false, UPDATE_AUTO_LOAD_RETAIN, 
			handler, &time_ ),
	time_( time )
{
}


/**
 *
 */
void ConsolidateEntityTask::performBackgroundTask( MySql & conn )
{
	if (!entityTypeMapping_.hasNewerRecord( conn, dbID_, time_ ))
	{
		this->PutEntityTask::performBackgroundTask( conn );
	}
}

BW_END_NAMESPACE

// consolidate_entity_task.cpp
