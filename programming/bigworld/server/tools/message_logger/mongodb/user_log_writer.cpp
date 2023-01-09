#include "user_log_writer.hpp"

#include "connection_thread_data.hpp"
#include "constants.hpp"
#include "types.hpp"

#include "../types.hpp"

#include "mongo/client/dbclient.h"
#include "mongo/bson/bsonmisc.h"

#include <time.h>


BW_BEGIN_NAMESPACE

namespace MongoDB
{

const uint32 COLL_SUFFIX_LENGTH = 14;

/**
 * Constructor.
 * Initialize user database if it's a new user.
 */
UserLogWriter::UserLogWriter(
		ConnectionThreadData & connectionData,
		MessageLogger::UID uid,
		const BW::string & userName,
		const BW::string & loggerID ) :
	connectionData_( connectionData ),
	uid_( uid ),
	userName_( userName ),
	dbName_( USER_DB_PREFIX + userName + LOGGER_ID_SEPARATOR + loggerID )
{
}


/**
 * Initialize user log database, including: initialise the uid collection,
 * server startup collection and shard the database if connecting to a MongoDB
 * cluster.
 */
bool UserLogWriter::initDB()
{
	// just in case
	if (userName_.empty())
	{
		CRITICAL_MSG( "UserLogWriter::initDB: "
				"The user name is empty. This shouldn't happen!\n" );
		return false;
	}

	bool ok = false;

	try
	{
		ok = this->initUIDCollection() &&
				this->initServerStartsColl() &&
				this->initComponentsColl();

		if (ok && connectionData_.getIsCluster())
		{
			// Shard database if we are connecting to a MongoDB cluster. And
			// only do this after initialising uid, server startup and
			// components collection to make sure the database has already
			// existed when sharding it.
			ok = this->shardDatabase();
		}
	}
	catch (mongo::SocketException & ex)
	{
		ERROR_MSG( "UserLogWriter::initDB: "
			"Got exception when initialising database %s: %s.\n",
			dbName_.c_str(), ex.what() );

		connectionData_.setConnected( false );
		ok = false;
	}
	catch (std::exception & ex)
	{
		ERROR_MSG( "UserLogWriter::initDB: "
			"Got exception when initialising database %s: %s.\n",
			dbName_.c_str(), ex.what() );

		ok = false;
	}

	if (!ok)
	{
		ERROR_MSG( "UserLogWriter::initDB: "
			"Failed to initialising database %s. Dropping it.\n",
			dbName_.c_str() );

		connectionData_.conn_.dropDatabase( dbName_.c_str() );
	}

	return ok;
}


/**
 * Write a batch of logs to the database.
 *
 * Clear the batch regardless of whether the flush was successful or not.
 * Losing some logs upon error is better than blowing up the system memory.
 */
bool UserLogWriter::flush( BSONObjBuffer & logBuffer )
{
	// nothing to flush, just return
	if (logBuffer.empty())
	{
		return true;
	}

	// Init collection if no collection is active
	if (activeCollName_.empty() && !this->initActiveColl())
	{
		ERROR_MSG( " UserLogWriter::flush:"
				"Unable to init new collection for user database: %s.\n",
				dbName_.c_str() );

		logBuffer.clear();
		return false;
	}

	bool ok = true;

	try
	{
		connectionData_.conn_.insert( activeCollName_.c_str(), logBuffer );

		MongoDB::string e = connectionData_.conn_.getLastError();
		if (!e.empty())
		{
			ERROR_MSG( "UserLogWriter::flush: "
				"Couldn't insert log to db (%s), error: %s\n",
				activeCollName_.c_str(), e.c_str() );

			ok = false;
		}
	}
	catch (mongo::SocketException & ex)
	{
		ERROR_MSG( "UserLogWriter::flush: "
			"Got exception when writing log to %s: %s\n",
			activeCollName_.c_str(), ex.what() );

		connectionData_.setConnected( false );
		ok = false;
	}
	catch (std::exception & ex)
	{
		ERROR_MSG( "UserLogWriter::flush: "
			"Got exception when writing log to %s: %s\n",
			activeCollName_.c_str(), ex.what() );

		ok = false;
	}

	logBuffer.clear();

	return ok;
}


/**
 * Write component info to the database.
 *
  */
bool UserLogWriter::writeComponent( mongo::BSONObj & obj )
{
	bool ok = true;
	BW::string componentCollName = dbName_ + "." + COMPONENTS_COLL_NAME;
	MongoDB::CollList collList;

	if (!this->getCollList( collList ))
	{
		ERROR_MSG( "UserLogWriter::writeComponent: "
			"Failed to get collection names from db:%s\n", dbName_.c_str() );
		return false;
	}

	MongoDB::CollList::const_iterator iter = std::find( collList.begin(),
			collList.end(), componentCollName.c_str() );

	if (iter == collList.end())
	{
		if (!connectionData_.conn_.createCollection(
								componentCollName.c_str() ))
		{
			ERROR_MSG( "UserLogWriter::writeComponent: "
					"Failed to create components collection (%s)\n",
					componentCollName.c_str() );
			return false;
		}
	}

	try
	{
		mongo::BSONObjBuilder objBuilder;
		objBuilder.append( COMPONENTS_KEY_HOST,
				obj.getIntField( COMPONENTS_KEY_HOST ) );
		objBuilder.append( COMPONENTS_KEY_PID,
				obj.getIntField( COMPONENTS_KEY_PID ) );
		objBuilder.append( COMPONENTS_KEY_COMPONENT,
				obj.getIntField( COMPONENTS_KEY_COMPONENT ) );
		std::auto_ptr< mongo::DBClientCursor > cursor =
			connectionData_.conn_.query(
			componentCollName.c_str(),
			mongo::BSONObj( objBuilder.obj() ));

		if (!cursor.get())
		{
			ERROR_MSG( "UserLogWriter::writeComponent: Couldn't get cursor.\n" );
			return false;
		}

		// check if the component already has the info in the collection
		if (!cursor->more())
		{
			connectionData_.conn_.insert( componentCollName.c_str(), obj );

			MongoDB::string e = connectionData_.conn_.getLastError();
			if (!e.empty())
			{
				ERROR_MSG( "UserLogWriter::writeComponent: "
					"Couldn't insert component to db (%s), error: %s\n",
					componentCollName.c_str(), e.c_str() );

				ok = false;
			}
		}
	}
	catch (mongo::SocketException & ex)
	{
		ERROR_MSG( "UserLogWriter::writeComponent: "
			"Got exception when writing log to %s: %s\n",
			componentCollName.c_str(), ex.what() );

		connectionData_.setConnected( false );
		ok = false;
	}
	catch (std::exception & ex)
	{
		ERROR_MSG( "UserLogWriter::writeComponent: "
			"Got exception when writing log to %s: %s\n",
			componentCollName.c_str(), ex.what() );

		ok = false;
	}

	return ok;
}


/**
 * Called when server start message detected. Write one startup record to db
 */
bool UserLogWriter::onServerStart( uint64 timestamp )
{
	BW::string startupCollName = dbName_ + "."
			+ SERVER_STARTS_COLL_NAME;

	mongo::Date_t timeVal( timestamp );
	mongo::BSONObj recObj = BSON( SERVER_STARTS_KEY_TIME <<
			timeVal );

	try
	{
		connectionData_.conn_.insert( startupCollName.c_str(), recObj );

		MongoDB::string e = connectionData_.conn_.getLastError();
		if (!e.empty())
		{
			ERROR_MSG( "UserLogWriter::onServerStart: "
					"Couldn't insert server start time to db %s: %s\n",
					startupCollName.c_str(), e.c_str() );

			return false;
		}
	}
	catch (mongo::SocketException & ex)
	{
		ERROR_MSG( "UserLogWriter::onServerStart: "
			"Got exception when inserting server start time to %s, error: %s\n",
			startupCollName.c_str(), ex.what() );

		connectionData_.setConnected( false );
		return false;
	}
	catch (std::exception & ex)
	{
		ERROR_MSG( "UserLogWriter::onServerStart: "
			"Got exception when inserting server start time to %s, error: %s\n",
			startupCollName.c_str(), ex.what() );

		return false;
	}
	
	return true;
}


/**
 * Roll up current active collection and purge expired log collections. Log
 * collections whose last entry is older than seconds will be seen as expired.
 */
bool UserLogWriter::rollAndPurge( uint64 seconds )
{
	// Roll up active collection by clearing the active collection name. A new
	// collection will be created when new log arrives. We don't create a new
	// one here because we may end up with empty collection if no new logs.
	activeCollName_.clear();

	return this->purgeExpiredColls( seconds );
}


/**
 * Purge expired log collections. Log collections whose last entry is older than
 * seconds will be seen as expired.
 */
bool UserLogWriter::purgeExpiredColls( uint64 seconds )
{
	INFO_MSG( "UserLogWriter::purgeExpiredColls:"
		"Purging expired log collections of user: %s\n", userName_.c_str() );

	bool ok = true;

	CollList collList;

	if (!this->getCollList( collList ))
	{
		ERROR_MSG( "UserLogWriter::purgeExpiredColls:"
			"Failed to get collections.\n" );

		return false;
	}

	BW::string entryCollNamePrefix;
	entryCollNamePrefix.append( dbName_ ).append( "." ).append(
			USER_COLL_ENTRIES_PREFIX );

	uint32 entryCollNameLength = entryCollNamePrefix.length() +
				COLL_SUFFIX_LENGTH;

	CollList::iterator iter = collList.begin();
	while (iter != collList.end())
	{
		BW::string collNamespace =  iter->c_str();

		if (( collNamespace.length() == entryCollNameLength ) &&
			( collNamespace.compare( 0, entryCollNamePrefix.length(),
					entryCollNamePrefix ) == 0 ))
		{
			BW::string timeSuffix = collNamespace.substr(
					entryCollNamePrefix.length() );

			// drop the collection if it's a entry collection and expired
			if (this->isCollectionExpired( collNamespace, timeSuffix, seconds ))
			{
				ok = ok && this->dropCollection( collNamespace );
			}
		}

		++iter;
	}

	if (!ok)
	{
		ERROR_MSG( "UserLogWriter::purgeExpiredColls:"
			"Failed to purge all expired collections.\n" );
	}

	return ok;
}


/**
 * Initialize the UID collection of this user's database
 * Exceptions raised by MongoDB should be handled by caller.
 */
bool UserLogWriter::initUIDCollection()
{
	// create uid collection
	BW::string uidCollNamespace = dbName_ + "." + BW::string( USER_COLL_UID );
	if (!connectionData_.conn_.createCollection( uidCollNamespace.c_str() ))
	{
		ERROR_MSG( "UserLogWriter::initUIDCollection: "
				"Failed to create uid collection for %s( %hu ).\n",
			userName_.c_str(), uid_ );

		return false;
	}

	// insert uid
	mongo::BSONObj uidObj = BSON( USER_UID_KEY << uid_ );

	connectionData_.conn_.insert( uidCollNamespace.c_str(), uidObj );
	MongoDB::string e = connectionData_.conn_.getLastError();

	if (!e.empty())
	{
		ERROR_MSG( "UserLogWriter::initUIDCollection: "
				"Couldn't insert uid into database %s, error: %s.\n",
				dbName_.c_str(), e.c_str() );

		return false;
	}

	return true;
}


/**
 * Create the server start ups collection for the user database. Limit the total
 * size of it by creating it as Capped Collection.
 *
 * Exceptions raised by MongoDB should be handled by caller.
 */
bool UserLogWriter::initServerStartsColl()
{
	BW::string startupsCollName = dbName_ + "."
			+ SERVER_STARTS_COLL_NAME;

	if (!connectionData_.conn_.createCollection( startupsCollName.c_str(),
			SERVER_STARTS_SIZE_LIMIT,
			true,
			SERVER_STARTS_COUNT_LIMIT ))
	{
		ERROR_MSG( "UserLogWriter::initServerStartsColl: "
			"Failed to create startup collection for database %s.\n",
			dbName_.c_str() );

		return false;
	}

	return true;
}


/**
 * Create the components collection for the user database.
 *
 * Exceptions raised by MongoDB should be handled by caller.
 */
bool UserLogWriter::initComponentsColl()
{
	BW::string componentCollName = dbName_ + "." + COMPONENTS_COLL_NAME;
	if (!connectionData_.conn_.createCollection( componentCollName.c_str() ))
	{
		ERROR_MSG( "UserLogWriter::initComponentsColl: "
			"Failed to create components collection for database %s.\n",
			dbName_.c_str() );

		return false;
	}

	return true;
}


/**
 * Enable the sharding of a database. In MongoDB, to shard a collection, the
 * parent database should be sharded as well. This should only be called when
 * Message Logger is connecting to a MongoDB cluster.
 *
 * Exceptions raised by MongoDB should be handled by caller.
 */
bool UserLogWriter::shardDatabase()
{
	mongo::BSONObj result;
	bool ok;

	mongo::BSONObj shardObj = BSON(	"enablesharding" << dbName_.c_str() );

	ok = connectionData_.conn_.runCommand( ADMIN_DB_NAME, shardObj,
			result );
	if (!ok)
	{
		BW::string errMsg = result.getStringField( "errmsg" );
		ERROR_MSG( "UserLogWriter::shardDatabase: "
			"MongoDB returned error when sharding user db %s, error: %s\n",
			dbName_.c_str(), errMsg.c_str() );

		return false;
	}

	return true;
}


/**
 * Create a new collection and use it as active collection. New logs after this
 * will be inserted into this active collection.
 * Enable sharding of the new collection if Message Logger is connected with a
 * MongoDB cluster.
 */
bool UserLogWriter::initActiveColl()
{
	BW::string newCollName = this->getNewCollName();
	newCollName = dbName_ + "." + newCollName;

	if (newCollName == activeCollName_)
	{
		INFO_MSG( "UserLogWriter::initActiveColl: "
			"Collection with name %s is already active.\n",
			newCollName.c_str() );
		return true;
	}

	try
	{
		// create collection
		if (!connectionData_.conn_.createCollection( newCollName.c_str() ))
		{
			ERROR_MSG( "UserLogWriter::initActiveColl: "
					"Failed to create log collection: %s.\n",
					newCollName.c_str() );

			return false;
		}

		// create index on the collection
		if (!this->createLogIndex( newCollName ))
		{
			this->dropCollection( newCollName );

			return false;
		}

		// enable sharding of the collection if connected to MongoDB cluster
		if (connectionData_.getIsCluster())
		{
			mongo::BSONObj result;
			bool ok;

			mongo::BSONObjBuilder objBuilder;
			objBuilder.append( "shardcollection", newCollName.c_str() );

			// The shard key has two parts:
			// - low cardinality field from LogStorageMongoDB::getShardKey that
			//   groups records and distributes evenly
			// - high cardinality field to allow easy chunk splits and
			//   efficient index updates
			objBuilder.append( "key", mongo::BSONObjBuilder().append(
									ENTRIES_KEY_SHARD_KEY, 1 ).append(
									ENTRIES_KEY_TIMESTAMP, 1 ).obj() );

			ok = connectionData_.conn_.runCommand( ADMIN_DB_NAME,
					objBuilder.done(), result );

			if (!ok)
			{
				BW::string errMsg = result.getStringField( "errmsg" );
				ERROR_MSG( "UserLogWriter::initActiveColl: "
					"MongoDB returned error when sharding collection(%s): %s\n",
					newCollName.c_str(), errMsg.c_str() );

				this->dropCollection( newCollName );

				return false;
			}
		}
	}
	catch (mongo::SocketException & ex)
	{
		ERROR_MSG( "UserLogWriter::initActiveColl: "
			"Got exception when initialising collection %s, exception: %s.\n",
			newCollName.c_str(), ex.what() );

		connectionData_.setConnected( false );
		return false;
	}
	catch (std::exception & ex)
	{
		ERROR_MSG( "UserLogWriter::initActiveColl: "
			"Got exception when initialising collection %s, exception: %s.\n",
			newCollName.c_str(), ex.what() );

		return false;
	}

	activeCollName_ = newCollName;

	return true;
}


/**
 * Create index on user's log collection. This serves two purposes:
 *		1. Maintain the insertion order in query result
 *		2. Limit the log scan range of query
 */
bool UserLogWriter::createLogIndex( const BW::string & logCollNamespace )
{
	mongo::BSONObjBuilder objBuilder;
	objBuilder.append( ENTRIES_KEY_TIMESTAMP, 1 );
	objBuilder.append( ENTRIES_KEY_COUNTER,  1);

	return connectionData_.conn_.ensureIndex( logCollNamespace.c_str(),
			objBuilder.obj() );
}


/**
 * Get new collection name, in the form entries_<timestamp>. The timestamp is in
 * the form YYYYMMDDHHMMSS of current UTC time, like entries_20140820101400
 */
BW::string UserLogWriter::getNewCollName()
{
	BW::string collName = USER_COLL_ENTRIES_PREFIX;

	collName.append( this->getTimeStampSuffix() );

	return collName;
}


/**
 * Get the timestamp suffix based on current time and in the form YYYYMMDDHHMMSS
 * of UTC time, like entries_20140820101400
 */
BW::string UserLogWriter::getTimeStampSuffix()
{
	time_t now = time( NULL );

	struct tm tmStruct;
	gmtime_r( &now, &tmStruct );
	
	char buf[ COLL_SUFFIX_LENGTH + 1 ] = {0};
	bw_snprintf( buf, COLL_SUFFIX_LENGTH + 1, "%04d%02d%02d%02d%02d%02d",
			tmStruct.tm_year + 1900, tmStruct.tm_mon + 1, tmStruct.tm_mday,
			tmStruct.tm_hour, tmStruct.tm_min, tmStruct.tm_sec );

	return buf;
}


/**
 * Get the collections of this user log database
 */
bool UserLogWriter::getCollList( CollList & collList )
{
	if (!ConnectionThreadData::getCollectionNames(
				connectionData_.conn_, dbName_, collList ))
	{
		ERROR_MSG( "UserLogWriter::getCollList: "
			"Failed to get collection names from db:%s\n", dbName_.c_str() );
			return false;
	}

	return true;
}


/**
 * Check if the given collection is expired by checking if its last record is
 * older than seconds.
 */
bool UserLogWriter::isCollectionExpired( const BW::string & collNS, const
	BW::string & timeSuffix, uint64 seconds )
{
	time_t now = time( NULL );

	// Check the collection creation time first. If the collection is created no
	// earlier than seconds, it shouldn't be expired
	struct tm timeInfo;
	int count = sscanf( timeSuffix.c_str(), "%04d%02d%02d%02d%02d%02d",
			&timeInfo.tm_year, &timeInfo.tm_mon, &timeInfo.tm_mday,
			&timeInfo.tm_hour, &timeInfo.tm_min, &timeInfo.tm_sec );

	if (count != 6)
	{
		ERROR_MSG( "UserLogWriter::isCollectionExpired:"
				"Failed to get time info from the suffix %s of collection :%s\n",
				timeSuffix.c_str(), collNS.c_str() );

		return false;
	}

	timeInfo.tm_year -= 1900;
	timeInfo.tm_mon -= 1;

	time_t collTime = timegm( &timeInfo );
	double diffInSec = difftime( now, collTime );

	if (diffInSec < seconds )
	{
		return false;
	}


	// Check the last record in the collection now
	try
	{
		// find the last record
		mongo::Query query;

		//sort timestamp and counter in descending order
		query.sort( BSON( ENTRIES_KEY_TIMESTAMP << -1 <<
				ENTRIES_KEY_COUNTER << -1 ) );

		// only return timestamp and counter so that the query can be covered
		// by index
		mongo::BSONObj returnPattern = BSON(
				ENTRIES_KEY_TIMESTAMP << 1 <<
				ENTRIES_KEY_COUNTER << 1 );

		mongo::BSONObj lastEntry = connectionData_.conn_.findOne( collNS.c_str(),
			query, &returnPattern );

		if (lastEntry.isEmpty())
		{
			ERROR_MSG( "UserLogWriter::isCollectionExpired: "
				"Collection %s is empty. Assuming it is expired.\n",
				collNS.c_str() );

			return true;
		}

		time_t logTime = lastEntry.getField(
				ENTRIES_KEY_TIMESTAMP ).date().toTimeT();


		diffInSec = difftime( now, logTime );
		if (diffInSec > seconds )
		{
			return true;
		}

	}
	catch (mongo::SocketException & ex)
	{
		ERROR_MSG( "UserLogWriter::isCollectionExpired: "
			"Got exception when examining collection %s, exception: %s.\n",
				collNS.c_str(), ex.what() );

		connectionData_.setConnected( false );
		return false;
	}
	catch (std::exception & ex)
	{
		ERROR_MSG( "UserLogWriter::isCollectionExpired: "
			"Got exception when examining collection %s, exception: %s.\n",
				collNS.c_str(), ex.what() );
		return false;
	}

	return false;
}


/**
 * Drop the collection from database
 */
bool UserLogWriter::dropCollection( const BW::string & collNS )
{
	INFO_MSG( "UserLogWriter::dropCollection: Dropping collection:%s\n",
			collNS.c_str());

	bool ok = true;

	try
	{
		ok = connectionData_.conn_.dropCollection( collNS.c_str() );

		if (!ok)
		{
			ERROR_MSG( "UserLogWriter::dropCollection: "
				"Failed to drop collection: %s\n", collNS.c_str() );

			ok = false;
		}
	}
	catch (mongo::SocketException & ex)
	{
		ERROR_MSG( "UserLogWriter::dropCollection: "
			"Got exception when dropping collection %s, exception: %s.\n",
			collNS.c_str(), ex.what() );

		connectionData_.setConnected( false );
		ok = false;
	}
	catch (std::exception & ex)
	{
		ERROR_MSG( "UserLogWriter::dropCollection: "
			"Got exception when dropping collection %s, exception: %s.\n",
			collNS.c_str(), ex.what() );

		ok = false;
	}

	return ok;
}

} // namespace MongoDB

BW_END_NAMESPACE

// user_log_writer.cpp
