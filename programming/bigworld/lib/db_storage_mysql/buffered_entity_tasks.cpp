#include "buffered_entity_tasks.hpp"

#include "db_storage/idatabase.hpp" // For EntityKey

#include "tasks/entity_task.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Helper functions
// -----------------------------------------------------------------------------

namespace
{

bool isValidDBID( DatabaseID id )
{
	return (id != 0) && (id != PENDING_DATABASE_ID);
}


template < class MAP, class ID >
bool grabLockT( MAP & tasks, ID id )
{
	typedef typename MAP::iterator iterator;
	std::pair< iterator, iterator > range = tasks.equal_range( id );

	if (range.first != range.second)
	{
		return false;
	}

	tasks.insert( range.second, std::make_pair( id,
				EntityTaskPtr( NULL ) ) );

	return true;
}


template < class MAP, class ID >
void bufferT( MAP & tasks, ID id, const EntityTaskPtr & pTask )
{
	typedef typename MAP::iterator iterator;
	std::pair< iterator, iterator > range = tasks.equal_range( id );

	MF_ASSERT( range.first != range.second );
	MF_ASSERT( range.first->second == NULL );

	// Make sure that it is inserted at the end.
	tasks.insert( range.second, std::make_pair( id, pTask ) );
}

} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: BufferedEntityTasks
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param bgTaskManager The manager where the tasks will be run.
 */
BufferedEntityTasks::BufferedEntityTasks( TaskManager & bgTaskManager ) :
	bgTaskManager_( bgTaskManager ),
	shouldDelayAdds_( false )
{
}


/**
 *	This method attempts to "grab a lock" for writing an entity. This class
 *	stores a list of the outstanding tasks for an entity. If there are any
 *	outstanding tasks, this method will return false.
 *
 *	If there are no tasks, a new NULL task is added, blocking others from
 *	grabbing the lock until that task has finished.
 */
bool BufferedEntityTasks::grabLock( const EntityTaskPtr & pTask )
{
	DatabaseID dbID = pTask->dbID();
	EntityID entityID = pTask->entityID();

	if (dbID == PENDING_DATABASE_ID)
	{
		NewEntityMap::const_iterator iter = newEntityMap_.find( entityID );

		if (iter != newEntityMap_.end())
		{
			dbID = iter->second;
			pTask->dbID( dbID );
		}
	}
	else if (dbID != 0)
	{
		newEntityMap_.erase( entityID );
	}

	if (isValidDBID( dbID ))
	{
		return grabLockT( tasks_, pTask->entityKey() );
	}

	if (entityID != 0)
	{
		return grabLockT( priorToDBIDTasks_, entityID );
	}

	// Always allow a task with no databaseID or entityID to proceed.
	return true;
}


/**
 *	This method buffers a task to be performed once the other outstanding
 *	tasks for this entity have completed.
 */
void BufferedEntityTasks::addBackgroundTask( const EntityTaskPtr & pTask )
{
	if (this->grabLock( pTask ))
	{
		this->doTask( pTask );
	}
	else
	{
		this->buffer( pTask );
	}
}


/**
 *	This method adds the provided task to the collection of buffered tasks.
 */
void BufferedEntityTasks::buffer( const EntityTaskPtr & pTask )
{
	const EntityKey & entityKey = pTask->entityKey();

	// INFO_MSG( "BufferedEntityTasks::buffer: "
	//			"Buffering task. dbID = %" FMT_DBID ". entityID = %d\n",
	//		entityKey.dbID, pTask->entityID() );

	pTask->bufferTimestamp( timestamp() );

	if (isValidDBID( entityKey.dbID ))
	{
		bufferT( tasks_, entityKey, pTask );
	}
	else
	{
		bufferT( priorToDBIDTasks_, pTask->entityID(), pTask );
	}
}


/**
 *	This method should be called when the currently performing task has
 *	completed. It will remove its entry from the collection and will perform
 *	the next task (if any).
 */
void BufferedEntityTasks::onFinished( const EntityTaskPtr & pTask )
{
	const EntityKey & entityKey = pTask->entityKey();

	if (!this->playNextTask( tasks_, entityKey ))
	{
		if (entityKey.dbID != 0)
		{
			this->onFinishedNewEntity( pTask );
		}
		else
		{
			// The write failed. Play the next task even though we know it is
			// going to fail. This could be optimised but not worthwhile for
			// such a rare event.
			this->playNextTask( priorToDBIDTasks_, pTask->entityID() );
		}
	}
}


/**
 *	This method is called when a task has finished to check whether there are
 *	any tasks buffered by EntityID that should now be run.
 */
void BufferedEntityTasks::onFinishedNewEntity( const EntityTaskPtr & pFinishedTask )
{
	const EntityKey & entityKey = pFinishedTask->entityKey();
	EntityID entityID = pFinishedTask->entityID();

	newEntityMap_[ entityID ] = entityKey.dbID;

	if (entityID == 0)
	{
		return;
	}

	typedef EntityIDMap Map;
	Map & tasks = priorToDBIDTasks_;

	std::pair< Map::iterator, Map::iterator > range =
			tasks.equal_range( entityID );

	MF_ASSERT( range.first != range.second );
	MF_ASSERT( range.first->second == NULL );

	Map::iterator iter = range.first;
	++iter;

	EntityTaskPtr pTaskToPlay = NULL;

	while (iter != range.second)
	{
		EntityTaskPtr pTask = iter->second;

		pTask->dbID( entityKey.dbID );

		if (grabLockT( tasks_, pTask->entityKey() ))
		{
			MF_ASSERT( pTaskToPlay == NULL );
			pTaskToPlay = pTask;
		}
		else
		{
			MF_ASSERT( pTaskToPlay != NULL );
			this->buffer( pTask );
		}

		++iter;
	}

	tasks.erase( range.first, range.second );

	if (pTaskToPlay)
	{
		this->doTask( pTaskToPlay );
	}
}


/**
 *	This method schedules a task to be performed by a background thread.
 */
void BufferedEntityTasks::doTask( const EntityTaskPtr & pTask )
{
	// So that it can inform us when it is done.
	pTask->pBufferedEntityTasks( this );

	uint64 bufferTimestamp = pTask->bufferTimestamp();

	double duration = bufferTimestamp == 0 ?
		0 : TimeStamp::toSeconds( timestamp() - pTask->bufferTimestamp() );

	if (duration > 1.0)
	{
		WARNING_MSG( "%s for dbID %" FMT_DBID ", type %d was buffered for "
				"%.2f seconds\n", pTask->name(), pTask->dbID(),
				pTask->entityKey().typeID, duration );
	}

	if (shouldDelayAdds_)
	{
		bgTaskManager_.addDelayedBackgroundTask( pTask );
	}
	else
	{
		bgTaskManager_.addBackgroundTask( pTask );
	}
}


/**
 *	This template method removes the lock associated with the current task and
 *	plays the next one, if any.
 *
 *	@return true on success, false if there was no lock to remove.
 */
template < class MAP, class ID >
bool BufferedEntityTasks::playNextTask( MAP & tasks, ID id )
{
	typedef typename MAP::iterator iterator;
	std::pair< iterator, iterator > range = tasks.equal_range( id );

	if ((range.first == range.second) ||
			(range.first->second != NULL))
	{
		// Not from this map.
		return false;
	}

	// There must be tasks and the first (the one being deleted) must be
	// NULL.
	MF_ASSERT( range.first != range.second );
	MF_ASSERT( range.first->second == NULL );

	iterator nextIter = range.first;
	++nextIter;
	EntityTaskPtr pNextTask = NULL;

	if (nextIter != range.second)
	{
		pNextTask = nextIter->second;

		nextIter->second = NULL;
	}

	tasks.erase( range.first );

	if (pNextTask)
	{
		INFO_MSG( "Playing buffered task %s for dbID %" FMT_DBID ", type %d\n",
				pNextTask->name(), pNextTask->dbID(),
				pNextTask->entityKey().typeID );

		if (pNextTask->dbID() == PENDING_DATABASE_ID)
		{
			pNextTask->dbID( 0 );
		}

		this->doTask( pNextTask );
	}

	return true;
}

BW_END_NAMESPACE

// buffered_entity_tasks.cpp
