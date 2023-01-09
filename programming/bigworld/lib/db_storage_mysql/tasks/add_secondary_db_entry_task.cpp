#include "add_secondary_db_entry_task.hpp"

#include "../query.hpp"

#include <arpa/inet.h>


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
AddSecondaryDBEntryTask::AddSecondaryDBEntryTask(
			const IDatabase::SecondaryDBEntry & entry ) :
		MySqlBackgroundTask( "AddSecondaryDBEntryTask" ),
		entry_( entry )
{
}


namespace
{
const Query query( "INSERT INTO bigworldSecondaryDatabases "
					"(ip, port, location) VALUES (?,?,?)" );
}


/**
 *
 */
void AddSecondaryDBEntryTask::performBackgroundTask( MySql & conn )
{
	query.execute( conn, ntohl( entry_.addr.ip ), ntohs( entry_.addr.port ),
			entry_.location, NULL );
}


/**
 *
 */
void AddSecondaryDBEntryTask::performMainThreadTask( bool succeeded )
{
}

BW_END_NAMESPACE

// add_secondary_db_entry_task.cpp
