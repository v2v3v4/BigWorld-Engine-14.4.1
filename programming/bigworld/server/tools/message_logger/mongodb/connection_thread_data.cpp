#include "connection_thread_data.hpp"

#include "connection_info.hpp"
#include "log_storage.hpp"
#include "types.hpp"
#include "user_log_writer.hpp"

#include "../types.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/debug.hpp"

#include "mongo/client/dbclient.h"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

// -----------------------------------------------------------------------------
// Section: ConnectionThreadData
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ConnectionThreadData::ConnectionThreadData( 
		const ConnectionInfo & connectionInfo, uint64 expireSeconds,
		const BW::string & loggerID ) :
	conn_( true, NULL, TCP_TIME_OUT ),
	loggerID_( loggerID ),
	connectionInfo_( connectionInfo ),
	expireSeconds_( expireSeconds ),
	isCluster_( false ),
	connectedFlag_( false ),
	allowReconnectFlag_( true ),
	abortFurtherProcessingFlag_( false )
{
}


/**
 *  This method occurs at the start of a background thread, initialises a
 *  MongoDB connection and prepares a userLogWriter for each user DB.
 *
 *  Its processing occurs within the child thread and therefore must be
 *  threadsafe.
 */
bool ConnectionThreadData::onStart( BackgroundTaskThread & thread )
{
	DEBUG_MSG( "ConnectionThreadData::onStart: Connecting to MongoDB.\n" );

	if (!LogStorageMongoDB::connectToDB( conn_, connectionInfo_ ))
	{
		ERROR_MSG( "ConnectionThreadData::connectToDB: "
			"Failed to connect to MongoDB\n" );
		return false;
	}

	DEBUG_MSG( "ConnectionThreadData::onStart: Initialising cluster status.\n" );

	// check if connected to a cluster via a router or to a single instance
	if (!this->initClusterStatus())
	{
		ERROR_MSG( "ConnectionThreadData::connectToDB: "
			"Failed to init cluster status\n" );
		return false;
	}

	DEBUG_MSG( "ConnectionThreadData::onStart: Initialising UserLogWriters.\n" );

	// init existing user logs info
	if (!this->initUserLogWriters())
	{
		ERROR_MSG( "ConnectionThreadData::init: "
			"Failed to init user log collections.\n" );
		return false;
	}

	DEBUG_MSG( "ConnectionThreadData::onStart: Purging expired data.\n" );

	// Initial purge of expired data.
	UserLogWriterMap::iterator it = userLogWriters_.begin();
	while (it != userLogWriters_.end())
	{
		it->second->purgeExpiredColls( expireSeconds_ );

		++it;
	}

	// Inform the main thread that it is safe to start its flush timer.
	this->setConnected( true );

	return true;
}


/*
 * Check if we are connecting to a MongoDB cluster or a single instance
 */
bool ConnectionThreadData::initClusterStatus()
{
	mongo::BSONObj result;

	try
	{
		bool ok = conn_.runCommand( ADMIN_DB_NAME,
				mongo::BSONObjBuilder().append( "ismaster", 1 ).obj(), result );

		if (ok)
		{
			BW::string msg = result.getStringField( "msg" );
			if (msg.find( "isdbgrid" ) != BW::string::npos)
			{
				isCluster_ = true;
			}
			else
			{
				isCluster_ = false;
			}
		}
		else
		{
			BW::string errMsg = result.getStringField( "errmsg" );
			ERROR_MSG( "ConnectionThreadData::initClusterStatus: "
				"MongoDB returned error when init cluster status: %s\n",
				errMsg.c_str() );

			return false;
		}
	}
	catch (mongo::SocketException & ex)
	{
		ERROR_MSG( "ConnectionThreadData::initClusterStatus: "
				"Got exception: %s.\n", ex.what() );

		this->setConnected( false );
		return false;
	}
	catch (std::exception & ex)
	{
		ERROR_MSG( "ConnectionThreadData::initClusterStatus: "
				"Got exception: %s.\n", ex.what() );
		return false;
	}

	return true;
}


/*
 * Init user log collections, create a map of uid and user log collection name
 */
bool ConnectionThreadData::initUserLogWriters()
{
	UserDbNameMap userDBNameMap;

	bool result = this->getUserDatabases( userDBNameMap );
	if (!result)
	{
		ERROR_MSG( "ConnectionThreadData::initUserLogWriters: "
				"An error occurred reading user databases." );
		return false;
	}

	UserDbNameMap::iterator iter = userDBNameMap.begin();

	while (iter!= userDBNameMap.end())
	{
		BW::string username = iter->first;
		BW::string dbName = iter->second;
		++iter;
		MessageLogger::UID uid;

		if (!this->getUidFromDB( dbName, uid ))
		{
			ERROR_MSG( "ConnectionThreadData::initUserLogWriters: "
					"Couldn't read uid from db( %s ).\n", dbName.c_str());

			// Try to check if there is logs. If cannot find logs, drop the
			// database, otherwise return false. When you connect to MongoDB
			// and use a database, it will be created automatically. In this
			// case it will result in an empty database, so we'd better drop
			// it.
			try
			{
				CollList collList;
				if (!ConnectionThreadData::getCollectionNames( conn_, dbName,
						collList ))
				{
					ERROR_MSG( "ConnectionThreadData::initUserLogWriters: "
						"Failed to get collections from db: %s.\n",
						dbName.c_str() );

					return false;
				}

				if (collList.empty())
				{
					ERROR_MSG(
						"ConnectionThreadData::initUserLogWriters: "
						"No log is found in db (%s). Dropping it because it "
						"has no UID.\n", dbName.c_str() );

					conn_.dropDatabase( dbName.c_str() );
				}
				else
				{
					ERROR_MSG(
						"ConnectionThreadData::initUserLogWriters: "
						"Log is found but uid is not found from %s. "
						"Exiting.\n", dbName.c_str() );

					return false;
				}

			}
			catch (mongo::SocketException & ex)
			{
				ERROR_MSG( "ConnectionThreadData::initUserLogWriters: "
					"Got error ( %s ) when trying to read log from db (%s).\n",
					ex.what(), dbName.c_str() );

				this->setConnected( false );
				return false;
			}
			catch (std::exception ex)
			{
				ERROR_MSG( "ConnectionThreadData::initUserLogWriters: "
					"Got error ( %s ) when trying to read log from db (%s).\n",
					ex.what(), dbName.c_str() );

				return false;
			}
		}
		else
		{
			userLogWriters_[ uid ] = new UserLogWriter( *this, uid, username,
				loggerID_ );
		}
	}

	return true;
}


/**
 * Get the user log writer by uid
 */
UserLogWriterPtr ConnectionThreadData::getUserLogWriter(
		MessageLogger::UID uid, const BW::string & username )
{
	UserLogWriterPtr pUserLogWriter = NULL;

	// The following insert NULL allows for performing a dual-purpose
	// lookup-if-exists/insert-if-doesn't. We can set the iterator later since a
	// new one has been returned.
	std::pair< UserLogWriterMap::iterator, bool > insertResult =
		userLogWriters_.insert(
			std::pair< MessageLogger::UID, UserLogWriterPtr > (
				uid, pUserLogWriter ) );

	UserLogWriterMap::iterator it = insertResult.first;

	if (!insertResult.second)
	{
		// Writer already exists, return it.
		return it->second;
	}

 	// Create a new user log writer
	pUserLogWriter = new UserLogWriter( *this, uid, username, loggerID_ );

	if (!pUserLogWriter->initDB())
	{
		// Failed to initialise, delete the newly added iterator
		userLogWriters_.erase( it );

		ERROR_MSG( "ConnectionThreadData::getUserLogWriter: "
			"Couldn't init database of user %s.\n", username.c_str() );

		return NULL;
	}

	it->second = pUserLogWriter;

	return pUserLogWriter;
}


bool ConnectionThreadData::rollUserLogs()
{
	bool ok = true;

	// The collections whose last entry is older than this seconds will be
	// deleted from database
	UserLogWriterMap::iterator it = userLogWriters_.begin();
	while (it != userLogWriters_.end())
	{
		ok = ok && it->second->rollAndPurge( expireSeconds_ );

		++it;
	}

	if (!ok)
	{
		ERROR_MSG( "ConnectionThreadData::rollUserLogs: "
			"An error occurred when rolling user logs.\n" );
	}

	return ok;
}


/**
 * Get the user database name list. User database name starts with bw_ml_user_
 */
bool ConnectionThreadData::getUserDatabases(
	ConnectionThreadData::UserDbNameMap & userDBNameMap )
{
	mongo::BSONObj info;

	try
	{
		conn_.runCommand( ADMIN_DB_NAME,
			mongo::BSONObjBuilder().append( "listDatabases", 1 ).obj() , info );

		MongoDB::string e = conn_.getLastError();

		if (!e.empty())
		{
			ERROR_MSG( "ConnectionThreadData::getUserDatabases: "
				"Couldn't get user databases: %s\n", e.c_str() );
			return false;
		}
	}
	catch (mongo::SocketException & ex)
	{
		ERROR_MSG( "ConnectionThreadData::getUserDatabases: "
			"Got exception when retrieving user databases: %s\n", ex.what() );

		this->setConnected( false );
		return false;
	}
	catch (std::exception & ex)
	{
		ERROR_MSG( "ConnectionThreadData::getUserDatabases: "
			"Got exception when retrieving user databases: %s\n", ex.what() );
		return false;
	}

	BW::string prefix = MongoDB::USER_DB_PREFIX;
	size_t prefixLen = prefix.length();
	BW::string suffix = MongoDB::LOGGER_ID_SEPARATOR + loggerID_;
	size_t suffixLen = suffix.length();

	BW::string dbName;
	BW::string username;

	mongo::BSONObjIterator iter( info["databases"].embeddedObjectUserCheck() );
	while (iter.more())
	{
		dbName = iter.next().embeddedObjectUserCheck()["name"].valuestr();
		size_t dbNameLen = dbName.length();

		// check if the database was created by this Message Logger
		if (dbName.compare( 0, prefixLen, prefix ) == 0 &&
				(dbNameLen > suffixLen) && dbName.compare(
					dbNameLen - suffixLen, suffixLen, suffix ) == 0 )
		{
			uint32 usernameLen = dbNameLen - prefixLen - suffixLen;
			username = dbName.substr( prefixLen, usernameLen );
			userDBNameMap[ username ] = dbName;
		}
	}

	return true;
}


/**
 * Get user id from specified user database.
 */
bool ConnectionThreadData::getUidFromDB( const BW::string & dbName,
		MessageLogger::UID & uid )
{
	BW::string uidCollName = dbName + "." + USER_COLL_UID;

	try
	{
		std::auto_ptr<mongo::DBClientCursor> cursor = conn_.query(
			uidCollName.c_str(), mongo::BSONObj() );

		if (!cursor.get())
		{
			ERROR_MSG( "ConnectionThreadData::getUidFromDB: "
				"Failed to get cursor when reading uid from %s.\n",
				dbName.c_str() );

			return false;
		}

		mongo::BSONObj rec = cursor->next();

		if (!rec.hasField( USER_UID_KEY ))
		{
			ERROR_MSG( "ConnectionThreadData::getUidFromDB: "
				"Not uid filed in query result: %s\n",
				rec.jsonString().c_str() );
			return false;
		}

		uid = rec.getIntField( USER_UID_KEY );
	}
	catch (mongo::SocketException & ex)
	{
		ERROR_MSG( "ConnectionThreadData::getUidFromDB: "
			"Got exception when reading uid from %s: %s\n",
			dbName.c_str(), ex.what() );

		this->setConnected( false );
		return false;
	}
	catch( std::exception & ex)
	{
		ERROR_MSG( "ConnectionThreadData::getUidFromDB: "
			"Got exception when reading uid from %s: %s\n",
			dbName.c_str(), ex.what() );
		return false;
	}

	return true;
}


/**
 * Write a BSON object to a MongoDB collection.
 */
DBTaskStatus ConnectionThreadData::addRecordToCollection(
	const BW::string & collectionName, const mongo::BSONObj & newRecord )
{
	try
	{
		conn_.insert( collectionName.c_str(), newRecord );

		MongoDB::string e = conn_.getLastError();

		if (!e.empty())
		{
			ERROR_MSG( "ConnectionThreadData::addRecordToCollection: "
				"Couldn't insert record to collection '%s': %s\n", 
				collectionName.c_str(), e.c_str() );
			return DB_TASK_GENERAL_ERROR;
		}
	}
	catch (mongo::SocketException & ex)
	{
		ERROR_MSG( "ConnectionThreadData::addRecordToCollection: "
			"Got exception when inserting record into collection '%s', "
			"exception: %s\n", collectionName.c_str(), ex.what() );

		this->setConnected( false );
		return DB_TASK_CONNECTION_ERROR;
	}
	catch (std::exception & ex)
	{
		ERROR_MSG( "ConnectionThreadData::addRecordToCollection: "
			"Got exception when inserting record into collection '%s', "
			"exception: %s\n", 
			collectionName.c_str(), ex.what() );
		return DB_TASK_GENERAL_ERROR;
	}

	return DB_TASK_SUCCESS;
}


/**
 * Update an existing MongoDB record with an new one.
 */
DBTaskStatus ConnectionThreadData::updateRecordInCollection(
	const BW::string & collectionName, const mongo::BSONObj & existingRecord,
	const mongo::BSONObj & newRecord )
{
	try
	{
		conn_.update( collectionName.c_str(), existingRecord,
			newRecord );

		MongoDB::string e = conn_.getLastError();

		if (!e.empty())
		{
			ERROR_MSG( "ConnectionThreadData::updateRecordInCollection: "
				"Couldn't update record in collection '%s': %s\n", 
				collectionName.c_str(), e.c_str() );
			return DB_TASK_GENERAL_ERROR;
		}
	}
	catch (mongo::SocketException & ex)
	{
		ERROR_MSG( "ConnectionThreadData::updateRecordInCollection: "
			"Got exception when updating record in collection '%s', "
			"exception: %s\n", 
			collectionName.c_str(), ex.what() );

		this->setConnected( false );
		return DB_TASK_CONNECTION_ERROR;
	}
	catch (std::exception & ex)
	{
		ERROR_MSG( "ConnectionThreadData::updateRecordInCollection: "
			"Got exception when updating record in collection '%s', "
			"exception: %s\n", 
			collectionName.c_str(), ex.what() );
		return DB_TASK_GENERAL_ERROR;
	}

	return DB_TASK_SUCCESS;
}


/**
 *  This method blocks the background thread until a connection becomes
 *  available or the main thread signals for us to abort.
 *
 *  This function should be checked at the beginning of background tasks that
 *  are going to need to utilise the connection.
 *
 *  It performs two services:
 *  - It allows the main thread to be notified of a successful reconnect (so
 *  that it can resume logging)
 *  - It prevents randomly spammed error messages from various background tasks
 *  attempting to repeatedly use the connection and failing.
 *
 *  It doesn't need to be called before every single connection usage because
 *  the connection has an auto reconnect and tasks should handle a failed
 *  connection mid-task gracefully.
 */
bool ConnectionThreadData::reconnectIfNecessary()
{
	if (this->isConnected())
	{
		return true;
	}

	INFO_MSG( "ConnectionThreadData::reconnectIfNecessary:: "
		"Attempting to reconnect to MongoDB.\n" );

	while (true)
	{
		if (this->shouldAbortReconnect())
		{
			DEBUG_MSG( "ConnectionThreadData::reconnectIfNecessary:: "
				"Aborted MongoDB reconnection.\n" );
			return false;
		}

		if (LogStorageMongoDB::connectToDB( conn_, connectionInfo_ ))
		{
			INFO_MSG( "ConnectionThreadData::reconnectIfNecessary:: "
				"Reconnected to MongoDB.\n" );

			this->setConnected( true );
			break;
		}

		if (connectionInfo_.waitBetweenReconnects_ )
		{
			sleep( connectionInfo_.waitBetweenReconnects_ );
		}
	}

	return true;
}


/**
 *  Sets the thread connected status for the main thread to check.
 */
void ConnectionThreadData::setConnected( bool connected )
{
	SimpleMutexHolder smh( statusFlagsMutex_ );
	connectedFlag_ = connected;
}


/**
 *  Reports if the thread is connected.
 */
bool ConnectionThreadData::isConnected()
{
	SimpleMutexHolder smh( statusFlagsMutex_ );
	return connectedFlag_;
}


/**
 *  Sets the thread connected status for the main thread to check.
 */
void ConnectionThreadData::stopReconnectAttempts()
{
	SimpleMutexHolder smh( statusFlagsMutex_ );
	allowReconnectFlag_ = false;
}


/**
 *  Reports if the thread is connected.
 */
bool ConnectionThreadData::shouldAbortReconnect()
{
	SimpleMutexHolder smh( statusFlagsMutex_ );
	return !allowReconnectFlag_;
}


/**
 *  Sets the thread connected status for the main thread to check.
 */
void ConnectionThreadData::abortFurtherProcessing()
{
	SimpleMutexHolder smh( statusFlagsMutex_ );
	abortFurtherProcessingFlag_ = false;
}


/**
 *  Reports if the thread is connected.
 */
bool ConnectionThreadData::shouldAbortFurtherProcessing()
{
	SimpleMutexHolder smh( statusFlagsMutex_ );
	return abortFurtherProcessingFlag_;
}


bool ConnectionThreadData::getCollectionNames( mongo::DBClientConnection & conn,
		const BW::string & dbName, CollList & rCollList )
{
	try
	{
		// This is for ticket BWT-28917. This is to work around a crash issue
		// caused by MonogDB driver when target MongoDB server is unreachable.
		// This code simulated how MongoDB driver retrieves collections names
		// but avoid the crash issue. This fix may get obsolete in future if
		// MongoDB changes its own db schema. However this is easy to test out,
		// so we just adopt this solution.
		BW::string ns = dbName + ".system.namespaces";
		std::auto_ptr<mongo::DBClientCursor> cursor = conn.query(
				ns.c_str(), mongo::BSONObj() );

		if (!cursor.get())
		{
			ERROR_MSG( "ConnectionThreadData::getCollectionNames: "
				"Failed to get cursor when trying to get collections from db "
				"(%s).\n", dbName.c_str() );

			return false;
		}

		while (cursor->more())
		{
			BW::string name = cursor->next()["name"].valuestr();
			if ( name.find( "$" ) != BW::string::npos )
			{
					continue;
			}

			rCollList.push_back( name );
		}
	}
	catch (std::exception ex)
	{
		ERROR_MSG( "ConnectionThreadData::getCollectionNames: "
			"Got error ( %s ) when trying to get collections from db (%s).\n",
			ex.what(), dbName.c_str() );

		return false;
	}

	return true;
}


} // namespace MongoDB

BW_END_NAMESPACE

// connection_thread_data.cpp
