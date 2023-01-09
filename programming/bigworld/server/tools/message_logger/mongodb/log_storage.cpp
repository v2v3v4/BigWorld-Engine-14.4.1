#include "log_storage.hpp"

#include "connection_info.hpp"
#include "connection_thread_data.hpp"
#include "constants.hpp"
#include "types.hpp"
#include "user_log_buffer.hpp"
#include "metadata.hpp"

#include "tasks/add_server_start_task.hpp"
#include "tasks/flush_task.hpp"
#include "tasks/roll_task.hpp"
#include "tasks/reconnect_task.hpp"
#include "tasks/update_appid_task.hpp"
#include "tasks/write_appid_task.hpp"

#include "../constants.hpp"
#include "../types.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/debug_message_priority.hpp"
#include "cstdmf/debug_message_source.hpp"

#include "mongo/client/dbclient.h"


BW_BEGIN_NAMESPACE

/**
 * Constructor.
 */
LogStorageMongoDB::LogStorageMongoDB( Logger & logger ) :
	LogStorage( logger ),
	logger_( logger ),
	currentBufSize_(0),
	maxBufSize_( MongoDB::MAX_LOG_BUFFER_SIZE ),
	flushInterval_( MongoDB::LOG_FLUSH_INTERVAL ),
	expireLogsDays_( MongoDB::EXPIRE_LOGS_DAYS ),
	loggerID_( logger.getLoggerID().empty() ?
			MongoDB::LOGGER_ID_NA : logger.getLoggerID() ),
	commonDBName_( MongoDB::BW_COMMON_DB_NAME + MongoDB::LOGGER_ID_SEPARATOR +
			loggerID_ ),
	conn_( true, NULL, MongoDB::TCP_TIME_OUT ),
	connectionInfo_(),
	pConnectionThreadData_( NULL ),
	mongoDBTaskMgr_( false ),
	canAddLogs_( false ),
	hostnames_( mongoDBTaskMgr_, conn_, commonDBName_ + "." +
			MongoDB::HOST_NAMES_COLL ),
	componentNames_( mongoDBTaskMgr_, conn_, commonDBName_ + "." +
			MongoDB::COMPONENTS_COLL ),
	formatStrings_( mongoDBTaskMgr_, conn_, commonDBName_ + "." +
			MongoDB::FMT_STRS_COLL ),
	categories_( mongoDBTaskMgr_, conn_, commonDBName_ + "." +
			MongoDB::CATEGORIES_COLL ),
	currentSecond_( 0 ),
	currentCounter_( 0 )
{}


/**
 * Destructor.
 */
LogStorageMongoDB::~LogStorageMongoDB()
{
	DEBUG_MSG( "LogStorageMongoDB::~LogStorageMongoDB(): shutting down\n" );

	flushTimer_.cancel();

	this->flushLogs();

	if (pConnectionThreadData_)
	{
		// Inform the connection thread that, if it is attempting to reconnect,
		// stop all attempts (which will allow the thread to exit).
		pConnectionThreadData_->stopReconnectAttempts();
	}
}


/**
 * Initialize the MongoDB driver. This should be done before calling any MongoDB
 * driver API, according to MongoDB C++ legacy driver documentation.
 */
bool LogStorageMongoDB::initMongoDBDriver()
{
	mongo::Status status = mongo::client::initialize();

	if (!status.isOK())
	{
		ERROR_MSG( "LogStorageMongoDB::initMongoDBDriver:"
				"Failed to initialize MongoDB driver: %s.\n",
				status.reason().c_str() );
		return false;
	}

	INFO_MSG( "LogStorageMongoDB::initMongoDBDriver:"
			"Successfully initialized MongoDB driver.\n" );

	return true;
}


/**
 * Shutdown and clean up MongoDB driver
 */
void LogStorageMongoDB::shutdownMongoDBDriver()
{
	mongo::client::shutdown();
}


/**
 * Initialize the log storage, including reading configurations, initializing
 * MongoDB connection and reading necessary data from MongoDB
 */
bool LogStorageMongoDB::init( const ConfigReader & config,
	const char * /*pRoot*/ )
{
	// init MongoDB config
	if (!this->initFromConfig( config ))
	{
		ERROR_MSG( "LogStorageMongoDB::init: Failed to init configurations\n" );
		return false;
	}

	// init MongoDB connection
	if (!this->connectToDB( conn_, connectionInfo_ ))
	{
		ERROR_MSG( "LogStorageMongoDB::init: "
			"Failed to init MongoDB connection\n" );
		return false;
	}

	// init common collections
	if (!this->initCommonCollections())
	{
		ERROR_MSG( "LogStorageMongoDB::init: "
			"Failed to init common collections.\n" );
		return false;
	}

	pConnectionThreadData_ =
		new MongoDB::ConnectionThreadData( connectionInfo_,
			this->getExpireLogsSeconds(), loggerID_ );

	mongoDBTaskMgr_.startThreads( "MongoDBTaskManager", 1,
								pConnectionThreadData_ );

	DEBUG_MSG( "Waiting for thread to connect to MongoDB\n" );

	while (!pConnectionThreadData_->isConnected() &&
			(mongoDBTaskMgr_.numRunningThreads() > 0))
	{
		// Tick the task manager in case anything important comes through while
		// we are waiting.
		mongoDBTaskMgr_.tick();
	}

	if (mongoDBTaskMgr_.numRunningThreads() == 0)
	{
		ERROR_MSG( "MongoDB thread creation failed\n" );
		return false;
	}

	DEBUG_MSG( "Thread connected to MongoDB\n" );

	canAddLogs_ = true;

	this->startFlushTimer();

	return true;
}


/**
 *  Reports if the thread is connected to MongoDB.
 */
bool LogStorageMongoDB::isConnected()
{
	MF_ASSERT( pConnectionThreadData_.get() )

	return pConnectionThreadData_->isConnected();
}


/**
 *  Trigger a tick in the TaskManager, as it expects.
 */
void LogStorageMongoDB::tick()
{
	MF_ASSERT( pConnectionThreadData_.get() )

	mongoDBTaskMgr_.tick();

	if (pConnectionThreadData_->shouldAbortFurtherProcessing())
	{
		// If the child thread has signalled an unrecoverable error then stop
		// communications and exit with a failure state.
		exit( EXIT_FAILURE );
	}
}


/**
 * Roll up user active log collections and purge expired user log collections.
 */
bool LogStorageMongoDB::roll()
{
	MongoDB::RollTask *pRollTask = new MongoDB::RollTask();
	mongoDBTaskMgr_.addBackgroundTask( pRollTask, TaskManager::HIGH );

	return true;
}


/**
 * Set the App Instance ID of a address
 */
bool LogStorageMongoDB::setAppInstanceID( const Mercury::Address & addr,
		ServerAppInstanceID id )
{
	appIDMap_[ addr ] = id;

	ComponentMap::iterator iter = componentMap_.find( addr );
	// check if the component already has the entry
	if (iter != componentMap_.end())
	{
		mongo::BSONObj obj;
		obj = iter->second;
		mongoDBTaskMgr_.addBackgroundTask(
			new MongoDB::UpdateAppIDTask( obj, id ));
	}

	return true;
}


/**
 * Stop logging from the component, delete in-memory data associated with it
 */
bool LogStorageMongoDB::stopLoggingFromComponent(
	const Mercury::Address & addr )
{
	appIDMap_.erase( addr );

	componentMap_.erase( addr );

	return true;
}


/*
 * Validate next hostname, which is done via hostnames_
 */
HostnamesValidatorProcessStatus LogStorageMongoDB::validateNextHostname()
{
	return hostnames_.validateNextHostname();
}


/**
 *  This method overrides the parent class LogStorage::addLogMessage.
 *
 *  If the DB connection is down,
 */
LogStorage::AddLogMessageResult LogStorageMongoDB::addLogMessage(
		const LoggerComponentMessage & componentMessage,
		const Mercury::Address & address, MemoryIStream & inputStream )
{
	// Only trigger a real addLogMessage if we are connected to the DB
	bool connected = this->isConnected();

	if (!connected && canAddLogs_)
	{
		// For the first time we switch canAddLogs_ from true to false, report
		// an error.
		ERROR_MSG( "LogStorageMongoDB::addLogMessage: "
			"Unable to store logs due to a database connection issue. "
			"All logging has been suspended.\n" );
	}

	if (connected && !canAddLogs_)
	{
		INFO_MSG( "LogStorageMongoDB::addLogMessage: "
			"MongoDB connection has been reestablished. Resuming logging.\n" );
	}

	canAddLogs_ = connected;

	if (!canAddLogs_)
	{
		// If there are no tasks queued in the background then nothing will ever
		// attempt to reconnect. Add a manual reconnect task.
		if (mongoDBTaskMgr_.bgTaskListSize() == 0)
		{
			MongoDB::ReconnectTask *pReconnectTask =
				new MongoDB::ReconnectTask();
			mongoDBTaskMgr_.addBackgroundTask( pReconnectTask,
				TaskManager::MAX );
		}
		return LOG_ADDITION_IGNORED;
	}

	return LogStorage::addLogMessage( componentMessage, address, inputStream );
}


/**
 * Write one log line to database.
 */
LogStorage::AddLogMessageResult LogStorageMongoDB::writeLogToDB(
 	const LoggerComponentMessage & componentMessage,
 	const Mercury::Address & address, MemoryIStream & inputStream,
 	const LoggerMessageHeader & header,
	LogStringInterpolator *pHandler,
	MessageLogger::CategoryID categoryID )
{
	// 1. Prepare user log buffer. Create a new one if it's a new user.
	MessageLogger::UID uid = componentMessage.uid_;

	MongoDB::UserLogs::iterator it = userLogs_.find( uid );
	MongoDB::UserLogBufferPtr pUserLogBuffer;

	if (it != userLogs_.end())
	{
		pUserLogBuffer = it->second;
	}
	else
	{
		BW::string username;

		Mercury::Reason reason = this->resolveUID( uid, address.ip, username );

		if (reason != Mercury::REASON_SUCCESS)
		{
			ERROR_MSG( "LogStorageMongoDB::writeLogToDB: "
				"Couldn't resolve uid %d (%s). User database not created.\n",
				uid, Mercury::reasonToString( reason ) );

			return LOG_ADDITION_FAILED;
		}

		// Map the uid to a user log buffer with the located username
		pUserLogBuffer = new MongoDB::UserLogBuffer( username );
		userLogs_[ uid ] = pUserLogBuffer;
	}

	// 2. Parse the log message into a BSON object
	uint64 currTime = this->getCurrentTime();

	mongo::BSONObj logBson;
	try
	{
		logBson = this->createBson( componentMessage, address,
			inputStream, header, pHandler, categoryID, currTime );
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "LogStorageMongoDB::writeLogToDB: Failed to create "
				"BSON object, error:%s\n", e.what() );
		return LOG_ADDITION_FAILED;
	}

	if (BW_TOOLS_COMPONENT_NAME == componentMessage.componentName_)
	{
		BW::string msg = logBson.getStringField(
				MongoDB::ENTRIES_KEY_MESSAGE );

		if (BW_TOOLS_SERVER_START_MSG == msg)
		{
			MongoDB::AddServerStartTask *pAddServerStartTask =
				new MongoDB::AddServerStartTask(
					componentMessage.uid_, pUserLogBuffer->getUsername(),
					currTime );
			mongoDBTaskMgr_.addBackgroundTask( pAddServerStartTask );
		}
	}

	if (writeToStdout_)
	{
		// highly un-usual to use std::cout. tend to use the existing standard
		// which would be bw_fprintf
		std::cout << logBson.jsonString().c_str();
	}

	ComponentMap::iterator iter = componentMap_.find( address );
	// check if this log is the first entry of the component
	if (iter == componentMap_.end())
	{
		// insert into components collection
		BW::string username;
		Mercury::Reason reason = this->resolveUID( uid, address.ip, username );
		if (reason != Mercury::REASON_SUCCESS)
		{
			ERROR_MSG( "LogStorageMongoDB::writeLogToDB: "
				"Couldn't resolve uid %d (%s). User database not created.\n",
				uid, Mercury::reasonToString( reason ) );

			return LOG_ADDITION_FAILED;
		}
		mongo::BSONObj componentBson;
		mongo::BSONObj componentMapBson;
		try
		{
			mongo::BSONObjBuilder objBuilder;
			objBuilder.append( MongoDB::COMPONENTS_KEY_HOST, address.ip );
			objBuilder.append( MongoDB::COMPONENTS_KEY_PID,
					componentMessage.pid_ );
			objBuilder.append( MongoDB::COMPONENTS_KEY_COMPONENT,
				componentNames_.getIDOfComponentName(
					componentMessage.componentName_ ) );
			objBuilder.append( MongoDB::COMPONENTS_KEY_APPID, 0 );
			componentBson = objBuilder.obj();
		}
		catch (std::exception & e)
		{
			ERROR_MSG( "LogStorageMongoDB::writeLogToDB: Failed to create "
					"BSON object, error:%s\n", e.what() );
			return LOG_ADDITION_FAILED;
		}

		try
		{
			mongo::BSONObjBuilder objBuilder;
			objBuilder.append( MongoDB::COMPONENTS_KEY_HOST, address.ip );
			objBuilder.append( MongoDB::COMPONENTS_KEY_PID,
					componentMessage.pid_ );
			objBuilder.append( MongoDB::COMPONENTS_KEY_COMPONENT,
				componentNames_.getIDOfComponentName(
					componentMessage.componentName_ ) );
			objBuilder.append( MongoDB::COMPONENTS_UID, uid );
			objBuilder.append( MongoDB::COMPONENTS_USERNAME, username.c_str() );
			componentMapBson = objBuilder.obj();
		}
		catch (std::exception & e)
		{
			ERROR_MSG( "LogStorageMongoDB::writeLogToDB: Failed to create "
					"BSON object, error:%s\n", e.what() );
			return LOG_ADDITION_FAILED;
		}

		MongoDB::WriteAppIDTask *pWriteAppIDTask =
			new MongoDB::WriteAppIDTask( uid, username, componentBson );

		mongoDBTaskMgr_.addBackgroundTask( pWriteAppIDTask );

		componentMap_[ address ] = componentMapBson;
	}

	// 3. Add the log to user log buffer and flush the buffer if necessary.
	pUserLogBuffer->append( logBson );
	++currentBufSize_;

	if (currentBufSize_ >= maxBufSize_)
	{
		return this->flushLogs();
	}

	return LOG_ADDITION_SUCCESS;
}


/*
 * Init configuration options
 */
bool LogStorageMongoDB::initFromConfig( const ConfigReader & config )
{
	BW::string tmpString;

	// host
	if (config.getValue( MongoDB::CONFIG_NAME_MONGO, "host",
						connectionInfo_.dbHost_ ))
	{
		INFO_MSG( "LogStorageMongoDB::initFromConfig: host: %s\n",
			connectionInfo_.dbHost_.c_str() );
	}
	else
	{
		ERROR_MSG( "LogStorageMongoDB::initFromConfig: "
			"Failed to read MongoDB host.\n" );
		return false;
	}

	// port
	if (config.getValue( MongoDB::CONFIG_NAME_MONGO, "port", tmpString ))
	{
		if (sscanf( tmpString.c_str(), "%hu", &connectionInfo_.dbPort_ ) != 1)
		{
			ERROR_MSG( "LogStorageMongoDB::initFromConfig: "
					"Failed to convert port( %s ) to an integer.\n",
				tmpString.c_str() );
			return false;
		}
		else
		{
			INFO_MSG( "LogStorageMongoDB::initFromConfig: port: %hu\n",
					connectionInfo_.dbPort_ );
		}
	}
	else
	{
		ERROR_MSG( "LogStorageMongoDB::initFromConfig: "
			"Failed to read MongoDB port.\n" );
		return false;
	}

	// user name
	if (config.getValue( MongoDB::CONFIG_NAME_MONGO, "user",
						connectionInfo_.dbUser_ ))
	{
		INFO_MSG( "LogStorageMongoDB::initFromConfig: user: %s\n",
			connectionInfo_.dbUser_.c_str() );
	}
	else
	{
		ERROR_MSG( "LogStorageMongoDB::initFromConfig: "
			"Failed to read MongoDB user name.\n" );
		return false;
	}

	// password
	if (config.getValue( MongoDB::CONFIG_NAME_MONGO, "password",
						connectionInfo_.dbPasswd_ ))
	{
		INFO_MSG( "LogStorageMongoDB::initFromConfig: "
			"Got MongoDB user password\n" );
	}
	else
	{
		ERROR_MSG( "LogStorageMongoDB::initFromConfig: "
			"Failed to read MongoDB user password.\n" );
		return false;
	}

	// tcp time out
	if (config.getValue( MongoDB::CONFIG_NAME_MONGO, "tcp_time_out",
						tmpString ))
	{
		if (sscanf( tmpString.c_str(), "%u", &connectionInfo_.tcpTimeout_ ) != 1)
		{
			ERROR_MSG( "LogStorageMongoDB::initFromConfig: "
					"Failed to convert tcp_time_out( %s ) to an integer.\n",
				tmpString.c_str() );
			return false;
		}
		else
		{
			INFO_MSG( "LogStorageMongoDB::initFromConfig: "
					"tcp_time_out: %u seconds.\n", 
					connectionInfo_.tcpTimeout_ );
		}
	}

	// wait time between reconnect attempts
	if (config.getValue( MongoDB::CONFIG_NAME_MONGO, "wait_between_reconnects",
						tmpString ))
	{
		if (sscanf( tmpString.c_str(), "%hu",
			&connectionInfo_.waitBetweenReconnects_ ) != 1)
		{
			ERROR_MSG( "LogStorageMongoDB::initFromConfig: "
					"Failed to convert wait_between_reconnects( %s ) "
					"to an integer.\n",
				tmpString.c_str() );
			return false;
		}
		else
		{
			INFO_MSG( "LogStorageMongoDB::initFromConfig: "
					"wait_between_reconnects: %hu seconds.\n", 
					connectionInfo_.waitBetweenReconnects_ );
		}
	}

	// max log buffer size
	if (config.getValue( MongoDB::CONFIG_NAME_MONGO, "max_buffered_lines",
						tmpString ))
	{
		if (sscanf( tmpString.c_str(), "%u", &maxBufSize_ ) != 1)
		{
			ERROR_MSG( "LogStorageMongoDB::initFromConfig: "
				"Failed to convert max_buffered_lines( %s ) to an integer.\n",
				tmpString.c_str() );
			return false;
		}
		else
		{
			INFO_MSG( "LogStorageMongoDB::initFromConfig: "
				"max_buffered_lines: %u\n", maxBufSize_ );
		}
	}

	// flush interval
	if (config.getValue( MongoDB::CONFIG_NAME_MONGO, "flush_interval",
						tmpString ))
	{
		if (sscanf( tmpString.c_str(), "%u", &flushInterval_ ) != 1)
		{
			ERROR_MSG( "LogStorageMongoDB::initFromConfig: "
					"Failed to convert flush_interval( %s ) to an integer.\n",
				tmpString.c_str() );
			return false;
		}
		else
		{
			INFO_MSG( "LogStorageMongoDB::initFromConfig: "
					"flush_interval: %u milliseconds.\n", flushInterval_ );
		}
	}

	// how long logs could be kept in database
	if (config.getValue( MongoDB::CONFIG_NAME_MONGO, "expire_logs_days",
						tmpString ))
	{
		if (sscanf( tmpString.c_str(), "%u", &expireLogsDays_ ) != 1)
		{
			ERROR_MSG( "LogStorageMongoDB::initFromConfig: "
				"Failed to convert expire_logs_days( %s ) to an integer.\n",
				tmpString.c_str() );
			return false;
		}
		else
		{
			INFO_MSG( "LogStorageMongoDB::initFromConfig: "
				"expire_logs_days: %u\n", expireLogsDays_ );
		}
	}

	return true;
}


/**
 *  A static method to connect and authenticate a passed-in MongoDB connection
 *  object using the passed-in connection info.
 */
bool LogStorageMongoDB::connectToDB( mongo::DBClientConnection & connection, 
	MongoDB::ConnectionInfo & connectionInfo )
{
	try
	{
		mongo::HostAndPort hostAndPort( connectionInfo.dbHost_.c_str(),
										connectionInfo.dbPort_ );
		MongoDB::string errMsg;

		connection.setSoTimeout( connectionInfo.tcpTimeout_ );

		if (!connection.connect( hostAndPort, errMsg ))
		{
			ERROR_MSG( "LogStorageMongoDB::connectToDB: "
					"Couldn't connect to MongoDB, error: %s\n", errMsg.c_str() );
			return false;
		}

		if (!connection.auth( MongoDB::ADMIN_DB_NAME,
				connectionInfo.dbUser_.c_str(),
				connectionInfo.dbPasswd_.c_str(), errMsg ))
		{
			ERROR_MSG( "LogStorageMongoDB::connectToDB: "
					"Couldn't authenticate against MongoDB with "
					"user %s, error: %s\n",
					connectionInfo.dbUser_.c_str(), errMsg.c_str() );
			return false;
		}
	}
	catch (std::exception & ex)
	{
		ERROR_MSG( "LogStorageMongoDB::connectToDB: Got exception: %s.\n",
				ex.what() );
		return false;
	}

	return true;
}


/*
 * Init common collections
 * Read these from db: hostnames, component names, categories, format strings
 * Write these to db if they don't exist: sources, severities, version
 */
bool LogStorageMongoDB::initCommonCollections()
{
	// read collections from database
	if (!hostnames_.init() || !componentNames_.init() ||
		!formatStrings_.init() || !categories_.init())
	{
		ERROR_MSG( "LogStorageMongoDB::initCommonCollections: "
				"Failed to initialize common collections.\n" );
		return false;
	}

	// get common collection list
	MongoDB::CollList collList;

	if (!MongoDB::ConnectionThreadData::getCollectionNames(
					conn_, commonDBName_, collList ))
	{
		ERROR_MSG( "LogStorageMongoDB::initCommonCollections: "
			"Failed to get common collections.\n" );
		return false;
	}

	MongoDB::CollList::iterator iter = collList.begin();
	BW::string names;

	while (iter != collList.end())
	{
		names.append( iter->c_str() ).append( ", " );
		++iter;
	}

	DEBUG_MSG( "LogStorageMongoDB::initCommonCollections: "
			"Got ( %lu )%s,\n", collList.size(), names.c_str() );

	// Init these collections if necessary
	try
	{
		// create collections if they don't exist
		if (!this->initSourcesColl( collList )
				|| !initSeveritiesColl( collList )
				|| !this->initVersionColl( collList ))
		{
			return false;
		}
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "LogStorageMongoDB::initCommonCollections: "
				"Failed to init common collections, error: %s\n", e.what() );
		return false;
	}

	return true;
}


/**
 * Get log expiration time in seconds.
 *
 * The expiration and purging is over entire collection, which means one
 * collection will only be expired and purged when all of its logs have expired
 * and collections containing expired and non-expired logs will not be purged.
 * So one hour offset will be deducted when checking the timestamp of each
 * collection, to avoid keeping one extra day logs when rolling doesn't happen
 * at the exact second everyday. For example if the expireLogsDays_ is 7, in
 * rare cases only logs of the last 6 days and 23 hours are kept.
 */
uint64 LogStorageMongoDB::getExpireLogsSeconds()
{
	return (uint64( expireLogsDays_ * 24 - 1 )) * 60 * 60;
}


/**
 * Write sources to database if the collection doesn't exist.
 * This is mainly for query
 */
bool LogStorageMongoDB::initSourcesColl(
	const MongoDB::CollList & collList )
{
	BW::string sourceCollName = commonDBName_ + "." + MongoDB::SOURCES_COLL;
	MongoDB::CollList::const_iterator iter =
		std::find( collList.begin(), collList.end(), sourceCollName.c_str() );

	if (iter == collList.end())
	{
		if (!conn_.createCollection( sourceCollName.c_str() ))
		{
			ERROR_MSG( "LogStorageMongoDB::initSourcesColl: "
					"Failed to create sources collection (%s)\n",
					sourceCollName.c_str() );
			return false;
		}

		if (!this->insertSourceToDB( MESSAGE_SOURCE_CPP )
			|| !this->insertSourceToDB( MESSAGE_SOURCE_SCRIPT ))
		{
			conn_.dropCollection( sourceCollName.c_str() );
			ERROR_MSG( "Failed to insert sources. Drop the collection\n" );

			return false;
		}
	}

	return true;
}


/**
 * Insert one source info into database
 */
bool LogStorageMongoDB::insertSourceToDB( DebugMessageSource type )
{
	BW::string sourceCollName = commonDBName_ + "." + MongoDB::SOURCES_COLL;
	mongo::BSONObjBuilder objBuilder;
	objBuilder.append( MongoDB::SOURCES_KEY_ID, type );
	objBuilder.append( MongoDB::SOURCES_KEY_NAME,
			messageSourceAsCString( type ) );

	conn_.insert( sourceCollName.c_str(), objBuilder.obj() );

	MongoDB::string e = conn_.getLastError();
	if (!e.empty())
	{
		ERROR_MSG( "LogStorageMongoDB::insertSourceToDB: "
				"Couldn't insert source type %d to db: %s\n", type, e.c_str() );
		return false;
	}

	return true;
}

/**
 * Write severity levels to database if the collection doesn't exist.
 * This is mainly for query
 */
bool LogStorageMongoDB::initSeveritiesColl(
		const MongoDB::CollList & collList )
{
	BW::string severitiesCollName = commonDBName_ + "." +
			MongoDB::SEVERITIES_COLL;
	MongoDB::CollList::const_iterator iter = std::find(
			collList.begin(), collList.end(), severitiesCollName.c_str() );

	if (iter == collList.end())
	{
		if (!conn_.createCollection( severitiesCollName.c_str() ))
		{
			ERROR_MSG( "LogStorageMongoDB::initSeveritiesColl: "
					"Failed to create severities collection (%s)\n",
					severitiesCollName.c_str() );
			return false;
		}

		if (!this->insertSeverityToDB( MESSAGE_PRIORITY_TRACE )
			|| !this->insertSeverityToDB( MESSAGE_PRIORITY_DEBUG )
			|| !this->insertSeverityToDB( MESSAGE_PRIORITY_INFO )
			|| !this->insertSeverityToDB( MESSAGE_PRIORITY_NOTICE )
			|| !this->insertSeverityToDB( MESSAGE_PRIORITY_WARNING )
			|| !this->insertSeverityToDB( MESSAGE_PRIORITY_ERROR )
			|| !this->insertSeverityToDB( MESSAGE_PRIORITY_CRITICAL )
			|| !this->insertSeverityToDB( MESSAGE_PRIORITY_HACK ))
		{
			conn_.dropCollection( severitiesCollName.c_str() );
			ERROR_MSG( "Failed to insert severities. Drop the collection\n" );

			return false;
		}
	}

	return true;
}


/**
 * Insert one severity level into database
 */
bool LogStorageMongoDB::insertSeverityToDB( DebugMessagePriority level )
{
	BW::string severitiesCollName = commonDBName_ + "." +
			MongoDB::SEVERITIES_COLL;
	mongo::BSONObjBuilder objBuilder;
	objBuilder.append( MongoDB::SEVERITIES_KEY_LEVEL, level );
	objBuilder.append( MongoDB::SEVERITIES_KEY_NAME,
			messagePrefix( level ) );

	conn_.insert( severitiesCollName.c_str(), objBuilder.obj() );

	MongoDB::string e = conn_.getLastError();
	if (!e.empty())
	{
		ERROR_MSG( "LogStorageMongoDB::insertSeverityToDB: "
				"Couldn't insert severity level %d to db: %s\n",
				level, e.c_str() );
		return false;
	}

	return true;
}


/**
 * Write version to database if the collection doesn't exist. Return false if
 * the schema version in the database is different from that one in the code.
 *
 * This is to identify the version of db schema that is used.
 */
bool LogStorageMongoDB::initVersionColl(
		const MongoDB::CollList & collList )
{
	BW::string versionCollName = commonDBName_ + "." +
			MongoDB::VERSION_COLL;
	MongoDB::CollList::const_iterator iter = std::find( collList.begin(),
			collList.end(), versionCollName.c_str() );

	if (iter == collList.end())
	{
		if (!conn_.createCollection( versionCollName.c_str() ))
		{
			ERROR_MSG( "LogStorageMongoDB::initVersionColl: "
					"Failed to create version collection (%s)\n",
					versionCollName.c_str() );
			return false;
		}

		return this->insertVersion();
	}
	else
	{
		std::auto_ptr< mongo::DBClientCursor > cursor = conn_.query(
				versionCollName.c_str(), mongo::BSONObj() );

		if (!cursor.get())
		{
			ERROR_MSG( "LogStorageMongoDB::initVersionColl: "
				"Failed to get cursor when reading version record.\n" );
			return false;
		}

		if (cursor->more())
		{
			mongo::BSONObj rec = cursor->next();
			uint32 version = rec.getIntField(
					MongoDB::VERSION_KEY_VERSION );

			if (version != MongoDB::SCHEMA_VERSION)
			{
				// If the version in database is different, return false so
				// that Message Logger will exit. This is to make sure that
				// we are working with compatible database. And user is
				// reponsible of munually updating data and the version
				// if there is any change to the schema. Hopefull there won't
				// be as the MongoDB is NoSQL and schema-free is what NoSQL for
				ERROR_MSG( "LogStorageMongoDB::initVersionColl: Version in db "
					"(%d) is different from expected (%d)\n",
					version, MongoDB::SCHEMA_VERSION );

				return false;
			}
		}
		else
		{
			return this->insertVersion();
		}
	}

	return true;
}


/**
 * Insert version record into database
 */
bool LogStorageMongoDB::insertVersion()
{
	BW::string versionCollName = commonDBName_ + "." +
			MongoDB::VERSION_COLL;
	mongo::BSONObjBuilder objBuilder;
	objBuilder.append( MongoDB::VERSION_KEY_VERSION,
			MongoDB::SCHEMA_VERSION );
	mongo::BSONObj versionBson = objBuilder.obj();

	conn_.insert( versionCollName.c_str(), versionBson );

	MongoDB::string e = conn_.getLastError();
	if (!e.empty())
	{
		ERROR_MSG( "LogStorageMongoDB::initVersionColl: "
				"Couldn't insert version info to db: %s\n", e.c_str() );
		return false;
	}

	return true;
}


/**
 * Create BSONobj for the log mesaage. Throw exception unpon error.
 */
mongo::BSONObj LogStorageMongoDB::createBson(
	 	const LoggerComponentMessage & componentMessage,
	 	const Mercury::Address & address, MemoryIStream & inputStream,
	 	const LoggerMessageHeader & header,	LogStringInterpolator *pHandler,
		MessageLogger::CategoryID categoryID, uint64 timestamp )
{
	mongo::BSONObjBuilder objBuilder;

	mongo::Date_t time = timestamp;
	objBuilder.append( MongoDB::ENTRIES_KEY_TIMESTAMP, time );

	uint32 counter = this->getCounter( timestamp );
	objBuilder.append( MongoDB::ENTRIES_KEY_COUNTER, counter );

	objBuilder.append( MongoDB::ENTRIES_KEY_CATEGORY, categoryID );
	objBuilder.append( MongoDB::ENTRIES_KEY_SOURCE,
			header.messageSource_ );
	objBuilder.append( MongoDB::ENTRIES_KEY_SEVERITY,
			header.messagePriority_ );
	objBuilder.append( MongoDB::ENTRIES_KEY_HOST, address.ip );
	objBuilder.append( MongoDB::ENTRIES_KEY_PID,
			componentMessage.pid_ );

	BW::string fmtString = pHandler->formatString();
	FormatStringID fmtID = formatStrings_.getIdOfFmtString( fmtString );
	objBuilder.append( MongoDB::ENTRIES_KEY_FMT_STR, fmtID );

	objBuilder.append( MongoDB::ENTRIES_KEY_COMPONENT,
		componentNames_.getIDOfComponentName(
			componentMessage.componentName_ ) );

	objBuilder.append( MongoDB::ENTRIES_KEY_APP_ID,
			this->getAppID( address ) );


	BW::string msg;
	mongo::BSONObj metadataObj;
	this->parseMessage( pHandler, inputStream, address,
		componentMessage.version_, msg, metadataObj );

	// New version apps validate UTF8 on sending side, old ones do not.
	const char * pMsg = msg.c_str();
	if (componentMessage.version_ <= MESSAGE_LOGGER_VERSION_WITHOUT_METADATA)
	{
		if (!isValidUtf8Str( pMsg ))
		{
			// string containing non-utf8 characters, utf8fy it
			BW::string utf8Str;
			toValidUtf8Str( pMsg, utf8Str );
			pMsg = utf8Str.c_str();
		}
	}
	objBuilder.append( MongoDB::ENTRIES_KEY_MESSAGE, pMsg );
	objBuilder.append( MongoDB::ENTRIES_KEY_META_DATA, metadataObj );

	if (pConnectionThreadData_->getIsCluster())
	{
		objBuilder.append( MongoDB::ENTRIES_KEY_SHARD_KEY,
					this->getShardKey( componentMessage ) );
	}
	return objBuilder.obj();
}

/**
 * Generate the cluster shard key for the given message.
 *
 * A good shard key should have two components:
 *	- the first component with low cardinality to both group similar records
 *	  and make even use of the available shards
 *	- the second component with high cardinality to ensure that chunks can
 *	  always be split, and constantly increasing to allow for efficient
 *	  index updates.
 *	This method generates the first component, and the second component is
 *	simply the timestamp field.
 */
int LogStorageMongoDB::getShardKey(
							const LoggerComponentMessage & componentMessage )
{
	// Multiply by constant to spread out the values for consecutive processes,
	// and then modulo 10 to reduce the number of possibilities.
	return componentMessage.pid_ * 7 % 10;
}

/**
 * Get current time in milliseconds
 */
uint64 LogStorageMongoDB::getCurrentTime()
{
	timeval tv;
	gettimeofday(&tv, NULL);

	return (uint64) tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


/**
 * Get counter by time. Counter automatically increase and starts from 1 for
 * every second
 */
uint32 LogStorageMongoDB::getCounter( uint64 milliseconds )
{
	uint64 second = milliseconds / 1000;

	if ( second == currentSecond_ )
	{
		++currentCounter_;
	}
	else
	{
		currentSecond_ = second;
		currentCounter_ = 1;
	}

	return currentCounter_;
}


/**
 * Get the app id of a process by its address
 * Return 0 if the address is not found
 */
ServerAppInstanceID LogStorageMongoDB::getAppID( const Mercury::Address & addr )
{
	AppIDMap::iterator iter = appIDMap_.find( addr );
	if (iter != appIDMap_.end())
	{
		return iter->second;
	}

	return 0;
}


/**
 * Parse out message and meta data from stream
 */
void LogStorageMongoDB::parseMessage( LogStringInterpolator *pHandler,
		MemoryIStream & inputStream,
		const Mercury::Address & address,
		MessageLogger::NetworkVersion version,
		BW::string & msg,
		mongo::BSONObj & metadataObj )
{
	pHandler->streamToString( inputStream, msg, version );

	// inputStream may have 1 byte remaining for NULL.
	if (version >= LOG_METADATA_VERSION && inputStream.remainingLength() > 1)
	{
		MLMetadata::LogMetadataBuilder builder;
		MLMetadata::MongoDBMetadataStream outputStream;
		builder.process( inputStream, outputStream );
		inputStream.finish();
		metadataObj = outputStream.getBSON();
	}
}


/*
 * Start the timer to regularly flush logs to MongoDB server
 */
void LogStorageMongoDB::startFlushTimer()
{
	MF_ASSERT( !flushTimer_.isSet() );

	flushTimer_ = (logger_.getDispatcher())->addTimer( flushInterval_ * 1000,
		this, reinterpret_cast< void * >( TIMEOUT_FLUSH_LOG ), "FlushLog" );
}


/**
 * Handle time out event, to flush logs to MongoDB server
 */
void LogStorageMongoDB::handleTimeout( TimerHandle handle, void * arg )
{
	uintptr timerType = reinterpret_cast<uintptr>( arg );

	if (timerType == TIMEOUT_FLUSH_LOG)
	{
		this->flushLogs();
	}
}


/**
 * Flush log to MongoDB server
 */
LogStorage::AddLogMessageResult LogStorageMongoDB::flushLogs()
{
	AddLogMessageResult result = LOG_ADDITION_SUCCESS;

	MongoDB::UserLogs::iterator it = userLogs_.begin();

	// Logs are stored in per-user databases in MongoDB and therefore are
	// written separately (as locking occurs at the user-database level).
	while (it != userLogs_.end())
	{
		if (it->second->size() == 0)
		{
			++it;
			continue;
		}

		// FlushTask fully consumes the log buffer.
		MongoDB::FlushTask *pFlushTask =
			new MongoDB::FlushTask( it->first, it->second );

		mongoDBTaskMgr_.addBackgroundTask( pFlushTask );

		++it;
	}

	currentBufSize_ = 0;

	return result;
}

BW_END_NAMESPACE
