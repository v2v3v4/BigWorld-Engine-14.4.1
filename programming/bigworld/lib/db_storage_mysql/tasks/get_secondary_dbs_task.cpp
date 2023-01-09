#include "get_secondary_dbs_task.hpp"

#include "../query.hpp"
#include "../result_set.hpp"

#include <arpa/inet.h>
#include <sstream>


BW_BEGIN_NAMESPACE

/**
 * 	This method executes SELECTs ip,port,location FROM
 * 	bigworldSecondaryDatabases with the WHERE clause and returns the results
 * 	in entries.
 */
void getSecondaryDBEntries( MySql & connection,
		IDatabase::SecondaryDBEntries & entries,
		const BW::string & condition )
{
	BW::stringstream getStmtStrm;
	getStmtStrm << "SELECT ip,port,location FROM "
			"bigworldSecondaryDatabases" << condition;

	Query query( getStmtStrm.str() );
	ResultSet resultSet;

	query.execute( connection, &resultSet );

	Mercury::Address addr;
	BW::string location;

	while (resultSet.getResult( addr.ip, addr.port, location ))
	{
		entries.push_back( IDatabase::SecondaryDBEntry(
				htonl( addr.ip ),
				htons( addr.port ),
				location ) );
	}
}


/**
 *	Constructor.
 */
GetSecondaryDBsTask::GetSecondaryDBsTask( Handler & handler ) :
	MySqlBackgroundTask( "GetSecondaryDBsTask" ),
	handler_( handler ),
	entries_()
{
}


/**
 *
 */
void GetSecondaryDBsTask::performBackgroundTask( MySql & conn )
{
	getSecondaryDBEntries( conn, entries_ );
}


/**
 *
 */
void GetSecondaryDBsTask::performMainThreadTask( bool succeeded )
{
	handler_.onGetSecondaryDBsComplete( entries_ );
}

BW_END_NAMESPACE

// get_secondary_dbs_task.cpp
