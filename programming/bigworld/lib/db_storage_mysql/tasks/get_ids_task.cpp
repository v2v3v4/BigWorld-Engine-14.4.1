#include "get_ids_task.hpp"

#include "../query.hpp"
#include "../result_set.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: GetIDsTask
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
GetIDsTask::GetIDsTask( int numIDs, IDatabase::IGetIDsHandler & handler ) :
	MySqlBackgroundTask( "GetIDsTask" ),
	numIDs_( numIDs ),
	handler_( handler )
{
}

namespace
{
// For returned ids
const Query getIDsQuery( "SELECT id FROM bigworldUsedIDs LIMIT ? FOR UPDATE" );
const Query delIDsQuery( "DELETE FROM bigworldUsedIDs LIMIT ?" );

// For new ids
const Query incIDQuery( "UPDATE bigworldNewID SET id=id+?" );
const Query getIDQuery( "SELECT id FROM bigworldNewID LIMIT 1 FOR UPDATE" );
const Query setIDQuery( "UPDATE bigworldNewID SET id=?" );
}


/**
 *	This method gets some unused IDs from the database. May be executed in a
 *	separate thread.
 */
void GetIDsTask::performBackgroundTask( MySql & conn )
{
	BinaryOStream & stream = handler_.idStrm();

	int numRemaining = this->getUsedIDs( conn, numIDs_, stream );

	if (numRemaining > 0)
	{
		this->getNewIDs( conn, numRemaining, stream );
	}
}


/**
 *	This gets ids from the pool of previously used ids.
 *
 *	@return The number of ids that it failed to get.
 */
int GetIDsTask::getUsedIDs( MySql & conn, int numIDs, BinaryOStream & stream )
{
	// Reuse any id's we can get our hands on
	ResultSet resultSet;
	getIDsQuery.execute( conn, numIDs, &resultSet );

	int numIDsRetrieved = 0;

	EntityID entityID;

	while (resultSet.getResult( entityID ))
	{
		stream << entityID;
		++numIDsRetrieved;
	}

	if (numIDsRetrieved > 0)
	{
		delIDsQuery.execute( conn, numIDsRetrieved, NULL );
	}

	return numIDs - numIDsRetrieved;
}


/**
 *	This gets ids from the pool of new ids.
 */
void GetIDsTask::getNewIDs( MySql & conn, int numIDs, BinaryOStream & stream )
{
	ResultSet resultSet;
	getIDQuery.execute( conn, &resultSet );
	incIDQuery.execute( conn, numIDs, NULL );

	EntityID newID;
	MF_VERIFY( resultSet.getResult( newID ) );

	for (int i = 0; i < numIDs; ++i)
	{
		stream << newID++;

		// TODO: In theory, this shouldn't interfere with
		// getUsedIDs(). Because getUsedIDs doesn't get
		// called atm, it should be safe to just wrap
		// the ID around hoping nobody uses FIRST_ENTITY_ID
		// after ~2.1 bn generated IDs.
		if (newID >= FIRST_LOCAL_ENTITY_ID)
		{
			WARNING_MSG( "GetIDsTask::getNewIDs: "
					"reached entity ID limit (%d), "
					"started generating entity IDs from %d",
					FIRST_LOCAL_ENTITY_ID, FIRST_ENTITY_ID );

			newID = FIRST_ENTITY_ID;

			EntityID dbEntityID = FIRST_ENTITY_ID + (numIDs - i - 1);
			setIDQuery.execute( conn, dbEntityID, NULL );
		}
	}
}


/**
 *
 */
void GetIDsTask::onRetry()
{
	handler_.resetStrm();
}


/**
 *	This method is called in the main thread after run() completes.
 */
void GetIDsTask::performMainThreadTask( bool succeeded )
{
	handler_.onGetIDsComplete();
}

BW_END_NAMESPACE

// get_ids_task.cpp
