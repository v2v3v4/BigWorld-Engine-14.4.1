#include "put_ids_task.hpp"

#include "../query.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PutIDsTask
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PutIDsTask::PutIDsTask( int numIDs, const EntityID * ids ) :
	MySqlBackgroundTask( "PutIDsTask" ),
	ids_( ids, ids + numIDs )
{
}


/**
 *	This method puts unused IDs into the database.
 */
void PutIDsTask::performBackgroundTask( MySql & conn )
{
	static const Query query( "INSERT INTO bigworldUsedIDs (id) VALUES (?)" );

	Container::const_iterator iter = ids_.begin();

	// TODO: ugh... make this not a loop somehow!
	while (iter != ids_.end())
	{
		query.execute( conn, *iter, NULL );

		++iter;
	}
}


/**
 *	This method is called in the main thread after run() completes.
 */
void PutIDsTask::performMainThreadTask( bool succeeded )
{
}

BW_END_NAMESPACE

// put_ids_task.cpp
