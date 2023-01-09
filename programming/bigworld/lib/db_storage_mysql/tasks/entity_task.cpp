#include "script/first_include.hpp"

#include "entity_task.hpp"

#include "db_storage_mysql/buffered_entity_tasks.hpp"
#include "db_storage_mysql/mappings/entity_type_mapping.hpp"
#include "db_storage/idatabase.hpp" // For EntityKey


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
EntityTask::EntityTask( const EntityTypeMapping & entityTypeMapping,
		DatabaseID dbID,
		const char * taskName ) :
	MySqlBackgroundTask( taskName ),
	entityTypeMapping_( entityTypeMapping ),
	dbID_( dbID ),
	pBufferedEntityTasks_( NULL ),
	bufferTimestamp_( 0 )
{
}


/**
 *
 */
void EntityTask::performMainThreadTask( bool succeeded )
{
	this->performEntityMainThreadTask( succeeded );

	if (pBufferedEntityTasks_)
	{
		pBufferedEntityTasks_->onFinished( this );
	}
}


/**
 *	This method returns the EntityKey associated with this task.
 */
EntityKey EntityTask::entityKey() const
{
	return EntityKey( entityTypeMapping_.getTypeID(), dbID_ );
}

BW_END_NAMESPACE

// entity_task.cpp
