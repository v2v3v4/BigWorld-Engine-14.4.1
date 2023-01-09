#include "update_secondary_dbs_task.hpp"

#include "get_secondary_dbs_task.hpp" // For getSecondaryDBEntries

#include "../query.hpp"

#include <sstream>
#include <arpa/inet.h>


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
UpdateSecondaryDBsTask::UpdateSecondaryDBsTask( const SecondaryDBAddrs & addrs,
			IDatabase::IUpdateSecondaryDBshandler & handler ) :
	MySqlBackgroundTask( "UpdateSecondaryDBsTask" ),
	handler_( handler ),
	entries_()
{
	// Create condition from addresses
	if (!addrs.empty())
	{
		BW::stringstream conditionStrm;
		SecondaryDBAddrs::const_iterator iter = addrs.begin();

		conditionStrm << " WHERE ( ip, port ) NOT IN (";

		while (iter != addrs.end())
		{
			if (iter != addrs.begin())
			{
				conditionStrm << ",";
			}

			conditionStrm <<
				"(" << htonl( iter->ip ) << "," << htons( iter->port ) << ")";

			++iter;
		}

		conditionStrm << ')';
		condition_ = conditionStrm.str();
	}
}


/**
 *
 */
void UpdateSecondaryDBsTask::performBackgroundTask( MySql & conn )
{
	// Get the entries that we're going to delete.
	getSecondaryDBEntries( conn, entries_, condition_ );

	// Delete the entries
	BW::stringstream delStmtStrm_;
	delStmtStrm_ << "DELETE FROM bigworldSecondaryDatabases" << condition_;

	Query query( delStmtStrm_.str() );
	query.execute( conn, NULL );
}


/**
 *
 */
void UpdateSecondaryDBsTask::performMainThreadTask( bool succeeded )
{
	handler_.onUpdateSecondaryDBsComplete( entries_ );
}

BW_END_NAMESPACE

// update_secondary_dbs_task.cpp
