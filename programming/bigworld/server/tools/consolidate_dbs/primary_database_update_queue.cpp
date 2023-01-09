#include "script/first_include.hpp"

#include "primary_database_update_queue.hpp"

#include "consolidate_entity_task.hpp"

#include "db_storage_mysql/mappings/entity_type_mapping.hpp"

#include "db_storage_mysql/buffered_entity_tasks.hpp"
#include "db_storage_mysql/thread_data.hpp"

#include <time.h>

DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )


BW_BEGIN_NAMESPACE

/**
 * 	Constructor.
 */
PrimaryDatabaseUpdateQueue::PrimaryDatabaseUpdateQueue( int numConnections ) :
	bgTaskMgr_(),
	pBufferedEntityTasks_( new BufferedEntityTasks( bgTaskMgr_ ) ),
	entityTypeMappings_(),
	hasError_( false ),
	numOutstanding_( 0 ),
	numConnections_( numConnections ),
	consolidatedTimes_()
{}


/**
 * 	Destructor.
 */
PrimaryDatabaseUpdateQueue::~PrimaryDatabaseUpdateQueue()
{
	bgTaskMgr_.stopAll( false );
}


/**
 * 	Initialization function.
 */
bool PrimaryDatabaseUpdateQueue::init( 
		const DBConfig::ConnectionInfo & connectionInfo,
		const EntityDefs & entityDefs )
{
	for (int i = 0; i < numConnections_; ++i)
	{
		bgTaskMgr_.startThreads( 1,
			new MySqlThreadData( connectionInfo ) );
	}

	try
	{
		MySql connection( connectionInfo );
		entityTypeMappings_.init( entityDefs, connection );
	}
	catch ( std::exception& e )
	{
		ERROR_MSG( "PrimaryDatabaseUpdateQueue::init: "
				"Failed to set up a mysql connection\n" );
		return false;
	}

	return true;
}


/**
 * 	Adds an entity update into our queue.
 */
void PrimaryDatabaseUpdateQueue::addUpdate( const EntityKey & key,
		BinaryIStream & data, GameTime time )
{
	// Check if we've already written a newer version of this entity.

	ConsolidatedTimes::const_iterator iEntityGameTime = 
		consolidatedTimes_.find( key );

	if (iEntityGameTime != consolidatedTimes_.end() &&
			time < iEntityGameTime->second)
	{
		return;
	}

	const EntityTypeMapping * pEntityTypeMapping =
			entityTypeMappings_[ key.typeID ];
	if (pEntityTypeMapping == NULL)
	{
		WARNING_MSG( "PrimaryDatabaseUpdateQueue::addUpdate: Entity with id"
				" \'%d\' is invalid. Skipping it.", key.typeID );
		return;
	}

	EntityTask * pTask =
		new ConsolidateEntityTask( pEntityTypeMapping,
				key.dbID, data, time, *this );

	++numOutstanding_;
	pBufferedEntityTasks_->addBackgroundTask( pTask );

	consolidatedTimes_[key] = time;
}


/**
 * 	Waits for all outstanding entity updates to complete.
 */
void PrimaryDatabaseUpdateQueue::waitForUpdatesCompletion()
{
	bgTaskMgr_.tick();

	while (numOutstanding_ > 0)
	{
		usleep( 1000 );
		bgTaskMgr_.tick();

		if (numConnections_ != bgTaskMgr_.numRunningThreads())
		{
			hasError_ = true;
		}
	}
}


/**
 * 	Called by UpdateTask when it completes.
 */
void PrimaryDatabaseUpdateQueue::onPutEntityComplete( bool isOkay,
		DatabaseID dbID )
{
	if (!isOkay)
	{
		ERROR_MSG( "PrimaryDatabaseUpdateQueue::onPutEntity: "
				"could not write entity with DBID=%" FMT_DBID " to database\n",
			dbID );
		hasError_ = true;
	}

	--numOutstanding_;
}

BW_END_NAMESPACE

// primary_database_update_queue.cpp

