#include "script/first_include.hpp"

#include "mysql_database.hpp"

#include "buffered_entity_tasks.hpp"
#include "database_exception.hpp"
#include "db_config.hpp"
#include "locked_connection.hpp"
#include "named_lock.hpp"
#include "mysql_billing_system.hpp"
#include "query.hpp"
#include "result_set.hpp"
#include "table_inspector.hpp"
#include "table_synchroniser.hpp"
#include "thread_data.hpp"
#include "transaction.hpp"
#include "wrapper.hpp"

#include "mappings/entity_type_mapping.hpp"

#include "tasks/add_secondary_db_entry_task.hpp"
#include "tasks/del_entity_task.hpp"
#include "tasks/execute_raw_command_task.hpp"
#include "tasks/get_base_app_mgr_init_data_task.hpp"
#include "tasks/get_dbid_task.hpp"
#include "tasks/get_entity_task.hpp"
#include "tasks/get_ids_task.hpp"
#include "tasks/get_secondary_dbs_task.hpp"
#include "tasks/look_up_entities_task.hpp"
#include "tasks/put_entity_task.hpp"
#include "tasks/put_ids_task.hpp"
#include "tasks/set_game_time_task.hpp"
#include "tasks/update_secondary_dbs_task.hpp"
#include "tasks/write_space_data_task.hpp"

#include "cstdmf/blob_or_null.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/watcher.hpp"
#include "cstdmf/md5.hpp"

#include "db/db_config.hpp"

#include "db_storage/db_entitydefs.hpp"
#include "db_storage/entity_auto_loader_interface.hpp"

#include "network/event_dispatcher.hpp"

#include "server/bwconfig.hpp"
#include "server/signal_processor.hpp"

#include <arpa/inet.h>

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: class MySqlDatabase
// -----------------------------------------------------------------------------

static int const TASK_WARN_THRESHOLD = 1;

/**
 *	Constructor.
 */
MySqlDatabase::MySqlDatabase(
		Mercury::NetworkInterface & interface,
		Mercury::EventDispatcher & dispatcher ) :
	Mercury::FrequentTask( "MySqlDatabase" ),
		bgTaskManager_(),
		numConnections_( 5 ),
		shouldConsolidate_( true ),
		reconnectTimerHandle_(),
		reconnectCount_( 0 ),
		pBufferedEntityTasks_( new BufferedEntityTasks( bgTaskManager_ ) ),
		interface_( interface ),
		dispatcher_( dispatcher ),
		pEntityDefs_( NULL ),
		entityTypeMappings_(),
		pConnection_( NULL )
{
	dispatcher.addFrequentTask( this );
	bgTaskManager_.initWatchers( "MySqlDatabase", TASK_WARN_THRESHOLD );

	MF_WATCH( "tasks/MySqlDatabase/bufferedQueueSize",
			*pBufferedEntityTasks_, &BufferedEntityTasks::size );

	MF_WATCH( "config/shouldDelayAddBackgroundTask",
			*pBufferedEntityTasks_,
			&BufferedEntityTasks::shouldDelayAdds,
			&BufferedEntityTasks::shouldDelayAdds );
}


/**
 *	Destructor.
 */
MySqlDatabase::~MySqlDatabase()
{
	bgTaskManager_.stopAll();

	reconnectTimerHandle_.cancel();

	delete pBufferedEntityTasks_;
	pBufferedEntityTasks_ = NULL;
}


/*
 *	Override from IDatabase.
 */
bool MySqlDatabase::startup( const EntityDefs & entityDefs,
		Mercury::EventDispatcher & dispatcher,
		int numRetries )
{
	pEntityDefs_ = &entityDefs;

	const DBConfig::ConnectionInfo & info = DBConfig::connectionInfo();

	INFO_MSG( "\tMySql: Configured MySQL server: %s:%d (%s)\n",
				info.host.c_str(),
				info.port,
				info.database.c_str() );

	try
	{
		const DBConfig::ConnectionInfo & connectionInfo = 
			DBConfig::connectionInfo();
		pConnection_ = new MySqlLockedConnection( connectionInfo );

		// TODO: Scalable DB, re-implement locking mechanism for DBApps
		if (!pConnection_->connect( /*shouldLock*/ false ))
		{
			return false;
		}

		MySql & connection = *(pConnection_->connection());

		if (!connection.checkTableEngines())
		{
			ERROR_MSG( "MySqlDatabase::startup: One or more tables are not "
					"using the %s engine type\n", MYSQL_ENGINE_TYPE );
			return false;
		}

		const DBConfig::Config & config = DBConfig::get();

		const bool shouldSyncTablesToDefs = config.mysql.syncTablesToDefs();

		INFO_MSG( "\tMySql: syncTablesToDefs = %s.\n", shouldSyncTablesToDefs ?
				   "True" : "False" );

		if (!isSpecialBigWorldTablesInSync( connection,
				   BWConfig::get( "billingSystem/isPasswordHashed", true ) ) ||
			!isEntityTablesInSync( connection, entityDefs ))
		{
			bool isSynced = false;

			if (shouldSyncTablesToDefs)
			{
				isSynced = syncTablesToDefs();
			}

			if (!isSynced)
			{
				ERROR_MSG( "MySqlDatabase::startup: Cannot use database as "
							"tables are not in sync with entity defs. Please "
							"sync by running sync_db\n" );
				return false;
			}
		}

		numConnections_ = config.mysql.numConnections();

		INFO_MSG( "\tMySql: Number of connections = %d.\n", numConnections_ );

		this->startBackgroundThreads( connectionInfo );

		entityTypeMappings_.init( entityDefs, connection );
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "MySqlDatabase::startup: %s\n", e.what() );
		return false;
	}

	return true;
}


void MySqlDatabase::startBackgroundThreads(
		const DBConfig::ConnectionInfo & connectionInfo )
{
	for (int i = 0; i < numConnections_; ++i)
	{
		bgTaskManager_.startThreads( "MySQL", 1,
				new MySqlThreadData( connectionInfo ) );
	}
}


bool MySqlDatabase::shutDown()
{
	INFO_MSG( "MySqlDatabase::shutDown()\n" );

	try
	{
		delete pConnection_;
		pConnection_ = NULL;
		reconnectTimerHandle_.cancel();

		return true;
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "MySqlDatabase::shutDown: %s\n", e.what() );
		return false;
	}
}


/**
 *	This method implements the FrequentTask method.
 */
void MySqlDatabase::doTask()
{
	bgTaskManager_.tick();
}


/**
 *	This method checks whether there is an unrecoverable error
 */
bool MySqlDatabase::hasUnrecoverableError() const
{
	// It is unrecoverable now. A possible improvement is to check whether
	// the existing running threads can meet the minimal requirement.
	return bgTaskManager_.numRunningThreads() != numConnections_;
}


/**
 *	This method creates the default billing system associated with the MySQL
 *	database.
 */
BillingSystem * MySqlDatabase::createBillingSystem()
{
	return new MySqlBillingSystem( bgTaskManager_, *pEntityDefs_ );
}


/**
 *	This method simply buffers the provided GetEntityTask. It is abstracted
 *	into a method to make the invocation locations clearer.
 *
 *	@param pGetEntityTask A pointer to the GetEntityTask to buffer.
 */
void MySqlDatabase::scheduleGetEntityTask( const GetEntityTaskPtr & pGetEntityTask )
{
	MF_ASSERT( pGetEntityTask );

	pBufferedEntityTasks_->addBackgroundTask( pGetEntityTask );
}


/**
 *	This class handles the result for a DBID lookup and then schedules a
 *	GetEntityTask to be run.
 */
class GetDbIDHandler : public IDatabase::IGetDbIDHandler
{
public:
	GetDbIDHandler( EntityTypeMapping * pEntityTypeMapping,
		const GetEntityTaskPtr & pGetEntityTask,
		MySqlDatabase * pMySqlDatabase ) :
			pEntityTypeMapping_( pEntityTypeMapping ),
			pGetEntityTask_( pGetEntityTask ),
			pMySqlDatabase_( pMySqlDatabase )
	{
		MF_ASSERT( pEntityTypeMapping_ );
		MF_ASSERT( pGetEntityTask_ );
		MF_ASSERT( pMySqlDatabase_ );
	}

	virtual ~GetDbIDHandler() {}

	void onGetDbIDComplete( bool succeeded, const EntityDBKey & entityKey )
	{
		// If the lookup failed, the GetEntityTask must be notified so the
		// invoking process receives a failure response.
		if (!succeeded)
		{
			pGetEntityTask_->onDatabaseIDLookupFailure();
			pGetEntityTask_ = NULL;
			return;
		}

		DEBUG_MSG( "GetDbIDHandler::onGetDbIDComplete: "
				"Entity type: %d, Identifier: '%s' converted to "
					"DatabaseID: %" FMT_DBID,
				entityKey.typeID, entityKey.name.c_str(), entityKey.dbID );

		pEntityTypeMapping_->cacheEntityKey( entityKey );

		// Update the GetEntityTask_ with the newly discovered dbID
		pGetEntityTask_->updateEntityKey( entityKey );

		pMySqlDatabase_->scheduleGetEntityTask( pGetEntityTask_ );

		delete this;
	}

private:
	EntityTypeMapping * pEntityTypeMapping_;
	GetEntityTaskPtr pGetEntityTask_;
	MySqlDatabase * pMySqlDatabase_;
};


/**
 *	Override from IDatabase
 */
void MySqlDatabase::getEntity( const EntityDBKey & entityKey,
		BinaryOStream * pStream,
		bool shouldGetBaseEntityLocation,
		IDatabase::IGetEntityHandler & handler )
{
	bool needsLookup = (entityKey.dbID == 0);

	const EntityTypeMapping * pEntityTypeMapping =
		entityTypeMappings_[ entityKey.typeID ];
	if (pEntityTypeMapping == NULL)
	{
		ERROR_MSG( "MySqlDatabase::getEntity: Entity with id \'%d\' is invalid."
				" Aborting. Please remove from entities.xml or fix def "
				"and script of this entity. ", entityKey.typeID );
		handler.onGetEntityComplete( false, entityKey, NULL );
		return;
	}

	// If we only have an <Identifier> to lookup this entity, check our
	// cached identifiers to prevent having to hit the database.
	if (needsLookup)
	{
		DatabaseID cachedID =
			pEntityTypeMapping->findCachedDatabaseID( entityKey );

		if (cachedID)
		{
			const_cast< EntityDBKey & >( entityKey ).dbID = cachedID;
			needsLookup = false;
		}
	}

	GetEntityTaskPtr pGetEntityTask = new GetEntityTask( pEntityTypeMapping,
		entityKey, pStream, shouldGetBaseEntityLocation, handler );


	// If we still require a DatabaseID, pass all the required data for the
	// GetEntityTask into the GetDbIDTask, as it will buffer the GetEntityTask
	// once it knows the associated DatabaseID.
	if (needsLookup)
	{
		GetDbIDHandler * pGetDbIDHandler =
			new GetDbIDHandler(
				const_cast< EntityTypeMapping * >( pEntityTypeMapping ),
				pGetEntityTask, this );

		bgTaskManager_.addBackgroundTask( 
			new GetDbIDTask(
				pEntityTypeMapping, entityKey,
				*pGetDbIDHandler ) );
	}
	else
	{
		this->scheduleGetEntityTask( pGetEntityTask );
	}
}


/**
 *	Override from IDatabase
 */
void MySqlDatabase::getDatabaseIDFromName( const EntityDBKey & entityKey,
		IDatabase::IGetDbIDHandler & handler )
{
	const EntityTypeMapping * pEntityTypeMapping =
			entityTypeMappings_[ entityKey.typeID ];
	if (pEntityTypeMapping == NULL)
	{
		ERROR_MSG( "MySqlDatabase::getDatabaseIDFromName: Entity with id \'%d\'"
				" is invalid. Aborting. Please remove from entities.xml"
				" or fix def and script of this entity. ", entityKey.typeID );
		handler.onGetDbIDComplete( false, entityKey );
		return;
	}

	bgTaskManager_.addBackgroundTask( 
			new GetDbIDTask(
					pEntityTypeMapping,
				entityKey, handler ) );
}


/**
 *	Override from IDatabase to look up entities of a certain type based on
 *	a string matching on all of the property queries.
 *
 *	@param entityTypeID 		The entity type's ID.
 *	@param criteria				The property queries to match.
 *	@param handler 				The handler to call back on.
 */
void MySqlDatabase::lookUpEntities( EntityTypeID entityTypeID,
		const LookUpEntitiesCriteria & criteria,
		ILookUpEntitiesHandler & handler )
{
	const EntityTypeMapping * pEntityTypeMapping = 
		entityTypeMappings_[entityTypeID];

	if (pEntityTypeMapping == NULL)
	{
		ERROR_MSG( "MySqlDatabase::lookUpEntities: "
					"could not get entity type mapping "
					"for entity type ID %d\n", 
				entityTypeID );

		handler.onLookUpEntitiesEnd( /*hasError:*/ true );

		return;
	}

	bgTaskManager_.addBackgroundTask( 
		new LookUpEntitiesTask( *pEntityTypeMapping, criteria, handler ) );
}


/**
 *	Override from IDatabase
 */
void MySqlDatabase::putEntity( const EntityKey & entityKey,
						EntityID entityID,
						BinaryIStream * pStream,
						const EntityMailBoxRef * pBaseMailbox,
						bool removeBaseMailbox,
						bool putExplicitID,
						UpdateAutoLoad updateAutoLoad,
						IPutEntityHandler & handler )
{
	const EntityTypeMapping * pEntityTypeMapping =
			entityTypeMappings_[ entityKey.typeID ];
	if (pEntityTypeMapping == NULL)
	{
		ERROR_MSG( "MySqlDatabase::putEntity: Entity with id \'%d\' is invalid."
				" Aborting. Please remove from entities.xml or fix def"
				" and script of this entity. ", entityKey.typeID );
		handler.onPutEntityComplete( false, entityKey.dbID );
		return;
	}

	// Note: gameTime is provided to PutEntityTask via the stream
	pBufferedEntityTasks_->addBackgroundTask(
			new PutEntityTask( pEntityTypeMapping,
				entityKey.dbID, entityID,
				pStream, pBaseMailbox, removeBaseMailbox, putExplicitID,
				updateAutoLoad, handler ) );
}


/**
 *	IDatabase override
 */
void MySqlDatabase::delEntity( const EntityDBKey & ekey,
		EntityID entityID,
		IDatabase::IDelEntityHandler & handler )
{
	const EntityTypeMapping * pEntityTypeMapping =
			entityTypeMappings_[ ekey.typeID ];
	if (pEntityTypeMapping == NULL)
	{
		ERROR_MSG("MySqlDatabase::delEntity: Entity with id \'%d\' is invalid."
				" Aborting. Please remove from entities.xml or fix def "
				"and script of this entity. ", ekey.typeID);
		handler.onDelEntityComplete( false );
		return;
	}

	pBufferedEntityTasks_->addBackgroundTask(
			new DelEntityTask( pEntityTypeMapping,
				ekey, entityID, handler ) );
}


void MySqlDatabase::executeRawCommand( const BW::string & command,
	IDatabase::IExecuteRawCommandHandler & handler )
{
	bgTaskManager_.addBackgroundTask(
		new ExecuteRawCommandTask( command, handler ) );
}


void MySqlDatabase::putIDs( int numIDs, const EntityID * ids )
{
	bgTaskManager_.addBackgroundTask( new PutIDsTask( numIDs, ids ) );
}


void MySqlDatabase::getIDs( int numIDs, IDatabase::IGetIDsHandler & handler )
{
	bgTaskManager_.addBackgroundTask( new GetIDsTask( numIDs, handler ) );
}


// -----------------------------------------------------------------------------
// Section: Space related
// -----------------------------------------------------------------------------

/**
 *	This method writes data associated with a space to the database.
 */
void MySqlDatabase::writeSpaceData( BinaryIStream & spaceData )
{
	bgTaskManager_.addBackgroundTask(
							new WriteSpaceDataTask( spaceData ) );
}


/**
 *	This method gets the space ids of the entities that will be auto-loaded.
 *	These are used to create these spaces before the entities are loaded.
 *
 *	Note: This can throw an exception on failure.
 */
void MySqlDatabase::getAutoLoadSpacesFromEntities( SpaceIDSet & spaceIDs )
{
	MySql & connection = *(pConnection_->connection());

	const Query distinctTypes(
		"SELECT DISTINCT bigworldLogOns.typeID, "
				"bigworldEntityTypes.bigworldID, bigworldEntityTypes.name "
			"FROM bigworldLogOns, bigworldEntityTypes "
			"WHERE bigworldLogOns.typeID = bigworldEntityTypes.typeID" );

	ResultSet typesResultSet;
	distinctTypes.execute( connection, &typesResultSet );

	int32 dbTypeID;
	EntityTypeID bwTypeID;
	BW::string typeName;

	while (typesResultSet.getResult( dbTypeID, bwTypeID, typeName ))
	{
		const EntityDescription & entityDesc =
			pEntityDefs_->getEntityDescription( bwTypeID );

		if (entityDesc.canBeOnCell())
		{
			BW::stringstream queryStr;
			queryStr << "SELECT DISTINCT tbl.sm_spaceID "
				"FROM tbl_" << typeName << " = tbl, bigworldLogOns "
				"WHERE tbl.id = bigworldLogOns.databaseID AND "
					"bigworldLogOns.typeID = ?";

			const Query query( queryStr.str() );
			ResultSet spaceIDsResultSet;
			query.execute( connection, dbTypeID, &spaceIDsResultSet );

			SpaceID spaceID;

			while (spaceIDsResultSet.getResult( spaceID ))
			{
				if (spaceID != 0)
				{
					spaceIDs.insert( spaceID );
				}
			}
		}
	}
}


/**
 *	Override from IDatabase.
 */
bool MySqlDatabase::getSpacesData( BinaryOStream & strm )
{
	MySql & connection = *(pConnection_->connection());

	try
	{
		// TODO: Make this handle the case where we are halfway through
		// updating the space data i.e. there are multiple versions present.
		// In that case we should probably use the last complete version
		// instead of the latest incomplete version.

		SpaceIDSet spaceIDs;
		this->getAutoLoadSpacesFromEntities( spaceIDs );

		int32 numSpaces = int32( spaceIDs.size() );
		strm << numSpaces;

		INFO_MSG( "MySqlDatabase::getSpacesData: numSpaces = %d\n", numSpaces );

		const Query spaceDataQuery(
			"SELECT spaceEntryID, entryKey, data "
					"FROM bigworldSpaceData WHERE id = ?" );

		for (SpaceIDSet::iterator iter = spaceIDs.begin();
				iter != spaceIDs.end();
				++iter)
		{
			SpaceID spaceID = *iter;

			uint64 spaceEntryID;
			uint16 spaceDataKey;
			BW::string spaceData;

			strm << spaceID;
			ResultSet spaceDataResultSet;
			spaceDataQuery.execute( connection, spaceID, &spaceDataResultSet );

			strm << spaceDataResultSet.numRows();

			while (spaceDataResultSet.getResult( spaceEntryID,
						spaceDataKey, spaceData ))
			{
				strm << spaceEntryID;
				strm << spaceDataKey;
				strm << spaceData;
			}
		}
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "MySqlDatabase::getSpacesData: Failed to get spaces data: "
				"%s\n", e.what() );
		return false;
	}

	return true;
}


/**
 *	Override from IDatabase.
 */
void MySqlDatabase::autoLoadEntities( IEntityAutoLoader & autoLoader )
{
	MySql & connection = *(pConnection_->connection());

	try
	{
		const Query clearNonAutoloadQuery(
				"DELETE FROM bigworldLogOns WHERE NOT shouldAutoLoad" );
		clearNonAutoloadQuery.execute( connection, NULL );

		// typeID is the id used internally by the database
		// bigworldID is the one used by a running server and is the position in
		// entities.xml.
		const Query query(
			"SELECT logOn.databaseID, entityType.bigworldID "
				"FROM bigworldLogOns logOn, bigworldEntityTypes entityType "
				"WHERE logOn.typeID = entityType.typeID AND "
					"logOn.shouldAutoLoad" );

		ResultSet resultSet;
		query.execute( connection, &resultSet );

		DatabaseID dbID;
		EntityTypeID bwTypeID;

		int numResults = resultSet.numRows();

		if (numResults > 0)
		{
			autoLoader.reserve( numResults );

			while (resultSet.getResult( dbID, bwTypeID ))
			{
				autoLoader.addEntity( bwTypeID, dbID );
			}

			connection.execute( "UPDATE bigworldLogOns SET ip = 0, port = 0" );
		}

		autoLoader.start();
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "MySqlDatabase::autoLoadEntities: "
				"Restore entities failed (%s)\n",
			e.what() );
		autoLoader.abort();
	}
}


/**
 *	This method sets the game time stored in the database.
 */
void MySqlDatabase::setGameTime( GameTime gameTime )
{
	bgTaskManager_.addBackgroundTask( new SetGameTimeTask( gameTime ) );
}


/**
 *	Override from IDatabase.
 */
void MySqlDatabase::getBaseAppMgrInitData(
		IGetBaseAppMgrInitDataHandler & handler )
{
	bgTaskManager_.addBackgroundTask(
			new GetBaseAppMgrInitDataTask( handler ) );
}


/**
 *	Override from IDatabase.
 */
void MySqlDatabase::remapEntityMailboxes( const Mercury::Address & srcAddr,
		const BackupHash & destAddrs )
{
	try
	{
		MySql & connection = *(pConnection_->connection());

		BW::stringstream updateStmtStrm;
		updateStmtStrm << "UPDATE bigworldLogOns SET ip=?, port=? WHERE ip="
				<< ntohl( srcAddr.ip ) << " AND port=" << ntohs( srcAddr.port )
				<< " AND ((((objectID * " << destAddrs.prime()
				<< ") % 0x100000000) >> 8) % ?)=?";

//		DEBUG_MSG( "MySqlDatabase::remapEntityMailboxes: %s\n",
//				updateStmtStrm.str().c_str() );

		const Query updateQuery( updateStmtStrm.str() );

		// TODO: Check if this is needed. If so, could use BufferedEntityTasks
		// and check when it's empty.
#if 0
		// Wait for all tasks to complete just in case some of them updates
		// bigworldLogOns.
		MySqlThreadResPool & threadResPool = this->getThreadResPool();
		threadResPool.threadPool().waitForAllTasks();
#endif

		size_t i = 0;

		while (i < destAddrs.size())
		{
			updateQuery.execute( connection,
					ntohl( destAddrs[i].ip ),
					ntohs( destAddrs[i].port ),
					destAddrs.virtualSizeFor( i ),
					i,
					NULL );

			++i;
		}
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "MySqlDatabase::remapEntityMailboxes: Remap entity "
				"mailboxes failed (%s)\n", e.what() );
	}
}


/**
 *	Overrides IDatabase method.
 */
void MySqlDatabase::addSecondaryDB( const SecondaryDBEntry & entry )
{
	bgTaskManager_.addBackgroundTask( new AddSecondaryDBEntryTask( entry ) );
}


/**
 *	Overrides IDatabase method.
 */
void MySqlDatabase::updateSecondaryDBs( const SecondaryDBAddrs & addrs,
		IUpdateSecondaryDBshandler & handler )
{
	bgTaskManager_.addBackgroundTask(
			new UpdateSecondaryDBsTask( addrs, handler ) );
}


/**
 *	Overrides IDatabase method
 */
void MySqlDatabase::getSecondaryDBs( IGetSecondaryDBsHandler & handler )
{
	bgTaskManager_.addBackgroundTask( new GetSecondaryDBsTask( handler ) );
}


/**
 *	Overrides IDatabase method
 */
uint32 MySqlDatabase::numSecondaryDBs()
{
	MySql & connection = *(pConnection_->connection());

	try
	{
		return ::BW_NAMESPACE numSecondaryDBs( connection );
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "MySqlDatabase::numSecondaryDBs: %s\n", e.what() );
		return -1;
	}
}


/**
 *	Overrides IDatabase method
 */
int MySqlDatabase::clearSecondaryDBs()
{
	bool retry = true;
	int numCleared = 0;

	MySql & connection = *(pConnection_->connection());

	while (retry)
	{
		retry = false;

		try
		{
			connection.query( "DELETE FROM bigworldSecondaryDatabases");
			numCleared = int( connection.affectedRows() );
		}
		catch (DatabaseException & e)
		{
			if (e.shouldRetry())
			{
				retry = true;
			}
		}
	}

	return numCleared;
}


/**
 *	Overrides IDatabase method
 */
bool MySqlDatabase::lockDB()
{
	return pConnection_ && pConnection_->lock();
}


/**
 *	Overrides IDatabase method
 */
bool MySqlDatabase::unlockDB()
{
	return pConnection_ && pConnection_->unlock();
}


/**
 *	This function syncs entity tables to entity definitions by executing
 *	the sync_db command in a child process.
 */
bool MySqlDatabase::syncTablesToDefs() const
{
	INFO_MSG( "MySqlDatabase::syncTablesToDefs: "
			"Running sync_db to update database tables.\n" );

	TableSynchroniser tableSynchroniser;

	if (!tableSynchroniser.run( dispatcher_ ))
	{
		if (tableSynchroniser.didExit())
		{
			ERROR_MSG( "MySqlDatabase::syncTablesToDefs: "
					"sync_db exited with exit code %d\n", 
				tableSynchroniser.exitCode() );
		}
		else if (tableSynchroniser.wasSignalled())
		{
			ERROR_MSG( "MySqlDatabase::syncTablesToDefs: "
					"sync_db was killed by signal %s\n",
				SignalProcessor::signalNumberToString( 
					tableSynchroniser.signal() ) );
		}

		ERROR_MSG( "MySqlDatabase::syncTablesToDefs: sync_db failed, see "
			"SyncDB logs for more details\n" );
		return false;
	}

	INFO_MSG( "MySqlDatabase::syncTablesToDefs: "
			"successfully synchronised tables\n" );
	return true;
}


/*
 *	 Override from IDatabase.
 */
bool MySqlDatabase::resetGameServerState()
{
	if (!pConnection_ || !pEntityDefs_)
	{
		ERROR_MSG( "MySqlDatabase::resetGameServerState: "
			"Has not started yet (or startup failed)\n" );
		return false;
	}

	MySql & connection = *(pConnection_->connection());
	const MD5::Digest & digest = pEntityDefs_->getPersistentPropertiesDigest();

	static const int NUM_ATTEMPTS = 3;
	int numAttemptsLeft = NUM_ATTEMPTS;

	while (numAttemptsLeft > 0)
	{
		try
		{
			MySqlTransaction transaction( connection );

			// Reset entity ID tables.
			connection.execute( "DELETE FROM bigworldUsedIDs" );
			connection.execute( "DELETE FROM bigworldNewID" );
			connection.execute( "INSERT INTO bigworldNewID (id) VALUES (1)" );

			// Set game time to 0 if it doesn't exist.
			// NOTE: This statement assumes bigworldNewID exists and has 1 row
			// in it.
			connection.execute( "INSERT INTO bigworldGameTime "
					"SELECT 0 FROM bigworldNewID "
					"WHERE NOT EXISTS(SELECT * FROM bigworldGameTime)" );

			connection.execute( "DELETE FROM bigworldEntityDefsChecksum" );

			// Set the checksum of all persistent properties
			// TODO: Scalable DB: SyncDB should be doing this on sync. Why are
			// we doing it here? Maybe there's a good reason...
			BW::string stmt;

			stmt = "INSERT INTO bigworldEntityDefsChecksum VALUES ('";
			stmt += digest.quote();
			stmt += "')";
			connection.execute( stmt );

			transaction.commit();

			return true;
		}
		catch (DatabaseException & e)
		{
			if (!e.shouldRetry())
			{
				ERROR_MSG( "MySqlDatabase::resetGameServerState: "
						"Fatal failure: %s\n",
					e.what() );
				numAttemptsLeft = 0;
			}
			else
			{
				--numAttemptsLeft;

				WARNING_MSG( "MySqlDatabase::resetGameServerState: "
						"Retryable failure (%d attempts left): %s\n",
					numAttemptsLeft, e.what() );
			}
		}
	}

	return false;
}

BW_END_NAMESPACE

// mysql_database.cpp
