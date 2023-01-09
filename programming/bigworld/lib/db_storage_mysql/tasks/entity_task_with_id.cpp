#include "entity_task_with_id.hpp"

#include "db_storage_mysql/query.hpp"
#include "db_storage_mysql/result_set.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
EntityTaskWithID::EntityTaskWithID( const EntityTypeMapping & entityTypeMapping,
							DatabaseID databaseID,
							EntityID entityID,
							const char * taskName ) :
	EntityTask( entityTypeMapping, databaseID, taskName ),
	entityID_( entityID )
{
}

BW_END_NAMESPACE

// entity_task_with_id.cpp
