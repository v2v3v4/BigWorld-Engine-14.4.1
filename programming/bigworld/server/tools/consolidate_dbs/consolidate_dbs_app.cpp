#include "script/first_include.hpp"

#include "consolidate_dbs_app.hpp"

#include "consolidation_progress_reporter.hpp"
#include "db_file_transfer_error_monitor.hpp"
#include "file_transfer_progress_reporter.hpp"
#include "primary_database_update_queue.hpp"
#include "secondary_database.hpp"
#include "secondary_db_info.hpp"
#include "tcp_listener.hpp"
#include "transfer_db_process.hpp"

#include "cstdmf/debug_message_categories.hpp"

#include "db/db_config.hpp"

#include "db_storage_mysql/db_config.hpp"
#include "db_storage_mysql/query.hpp"
#include "db_storage_mysql/result_set.hpp"
#include "db_storage_mysql/transaction.hpp"

#include "resmgr/bwresource.hpp"

#include "server/bwconfig.hpp"


DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )



BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Link tokens
// -----------------------------------------------------------------------------

extern int force_link_UDO_REF;
extern int ResMgr_token;
extern int PyScript_token;

namespace // anonymous
{

int s_moduleTokens = ResMgr_token | force_link_UDO_REF | PyScript_token;


// -----------------------------------------------------------------------------
// Section: ConsolidateDBsApp
// -----------------------------------------------------------------------------

}

BW_SINGLETON_STORAGE( ConsolidateDBsApp )

/**
 *	Constructor.
 */
ConsolidateDBsApp::ConsolidateDBsApp( bool shouldStopOnError ) :
	DatabaseToolApp(),
	internalIP_( 0 ),
	pDBApp_( NULL ),
	connectionInfo_(),
	consolidationDir_( "/tmp/" ),
	consolidationErrors_(),
	shouldStopOnError_( shouldStopOnError ),
	shouldAbort_( false )
{
}


/**
 *	Destructor.
 */
ConsolidateDBsApp::~ConsolidateDBsApp()
{
}


/**
 * 	This function should be called after constructor to initialise the object.
 * 	Returns true if initialisation succeeded, false if it failed.
 */
bool ConsolidateDBsApp::init( bool isVerbose, bool shouldReportToDBApp,
		bool shouldLock,
		const DBConfig::ConnectionInfo & primaryDBConnectionInfo )
{
	const DBConfig::Config & config = DBConfig::get();

	if (!this->DatabaseToolApp::init( "ConsolidateDBs", "consolidate_dbs",
				isVerbose, shouldLock, primaryDBConnectionInfo ))
	{
		return false;
	}

	this->enableSignalHandler( SIGINT );
	this->enableSignalHandler( SIGHUP );

	internalIP_ = this->watcherNub().udpSocket().getLocalAddress().ip;

	if (shouldReportToDBApp)
	{
		pDBApp_.reset( new DBApp( this->watcherNub() ) );
		if (!pDBApp_->init())
		{
			// No DBApp found!
			return false;
		}
	}

	TRACE_MSG( "ConsolidateDBsApp: Connected to primary database: "
			"host=%s:%d, username=%s, database=%s\n",
		primaryDBConnectionInfo.host.c_str(),
		primaryDBConnectionInfo.port,
		primaryDBConnectionInfo.username.c_str(),
		primaryDBConnectionInfo.database.c_str() );

	connectionInfo_ = primaryDBConnectionInfo;

	consolidationDir_ = config.secondaryDB.consolidation.directory();

	IFileSystem::FileType consolidationDirType =
			BWResource::resolveToAbsolutePath( consolidationDir_ );

	if (consolidationDirType != IFileSystem::FT_DIRECTORY)
	{
		ERROR_MSG( "ConsolidateDBsApp::init: Configuration setting "
				"<%s> specifies a non-existent directory: %s\n",
			config.secondaryDB.consolidation.directory.path().c_str(),
			consolidationDir_.c_str() );
		return false;
	}

	return true;
}


/**
 * 	After initialisation, this method starts the data consolidation process
 */
bool ConsolidateDBsApp::transferAndConsolidate()
{
	// Get the secondary DB info from primary database.
	SecondaryDBInfos secondaryDBs;

	if (!this->getSecondaryDBInfos( secondaryDBs ))
	{
		return false;
	}

	if (secondaryDBs.empty())
	{
		ERROR_MSG( "ConsolidateDBsApp::transferAndConsolidate: "
				"No secondary databases to consolidate\n" );
		return false;
	}

	FileTransferProgressReporter progressReporter( *this, 
		secondaryDBs.size() );
	FileReceiverMgr	fileReceiverMgr( this->dispatcher(), progressReporter,
		secondaryDBs, consolidationDir_ );

	if (!this->transferSecondaryDBs( secondaryDBs, fileReceiverMgr ))
	{
		ERROR_MSG( "ConsolidateDBsApp::transferAndConsolidate: "
				"Failed to transfer secondaryDBs\n" );
		return false;
	}

	// Consolidate databases
	const FileNames & dbFilePaths = fileReceiverMgr.receivedFilePaths();

	if (!this->consolidateSecondaryDBs( dbFilePaths ))
	{
		ERROR_MSG( "ConsolidateDBsApp::transferAndConsolidate: "
				"Failed to consolidate secondaryDBs\n" );
		return false;
	}

	fileReceiverMgr.cleanUpRemoteFiles( consolidationErrors_ );

	this->clearSecondaryDBEntries();

	TRACE_MSG( "ConsolidateDBsApp::transferAndConsolidate: "
			"Completed successfully\n" );

	return true;
}


/**
 *	Transfer the secondary DBs from each location specified.
 */
bool ConsolidateDBsApp::transferSecondaryDBs( 
		const SecondaryDBInfos & secondaryDBs, 
		FileReceiverMgr & fileReceiverMgr )
{
	// Start listening for incoming connections
	TcpListener< FileReceiverMgr > connectionsListener( fileReceiverMgr );

	if (!connectionsListener.init( 0, internalIP_, secondaryDBs.size() ))
	{
		return false;
	}

	// Make our address:port into a string to pass to child processes
	Mercury::Address ourAddr;
	if (!connectionsListener.getBoundAddr( ourAddr ))
	{
		return false;
	}

	INFO_MSG( "ConsolidateDBsApp::transferSecondaryDBs: "
		"Listening on: %s\n", ourAddr.c_str() );

	// Start remote file transfer service
	for (SecondaryDBInfos::const_iterator iSecondaryDBInfo = 
				secondaryDBs.begin();
			iSecondaryDBInfo != secondaryDBs.end();
			++iSecondaryDBInfo)
	{
		TransferDBProcess transferDB( ourAddr );

		if (!transferDB.transfer( iSecondaryDBInfo->hostIP, 
				iSecondaryDBInfo->location ))
		{
			shouldAbort_ = true;
			return false;
		}
	}

	{
		DBFileTransferErrorMonitor errorMonitor( fileReceiverMgr );

		// Wait for file transfer to complete
		this->dispatcher().processUntilBreak();
	}

	return fileReceiverMgr.finished();
}


/**
 *	Consolidates the secondary databases pointed to by filePath into the
 * 	primary database.
 */
bool ConsolidateDBsApp::consolidateSecondaryDBs( const FileNames & filePaths )
{
	const uint numConnections = DBConfig::get().mysql.numConnections();

	INFO_MSG( "ConsolidateDBsApp::consolidateSecondaryDBs: "
			"Number of connections = %d.\n", 
		numConnections );

	PrimaryDatabaseUpdateQueue primaryDBQueue( numConnections );
	
	if (!primaryDBQueue.init( connectionInfo_, this->entityDefs() ))
	{
		ERROR_MSG( "ConsolidateDBsApp::consolidateSecondaryDBs: "
				"Failed to initialize primaryDBQueue\n" );
		return false;
	}

	ConsolidationProgressReporter progressReporter( *this, filePaths.size() );

	for (FileNames::const_iterator iFilePath = filePaths.begin();
			iFilePath != filePaths.end(); 
			++iFilePath)
	{
		if (!this->consolidateSecondaryDB( *iFilePath, primaryDBQueue,
				progressReporter ))
		{
			if (shouldAbort_)
			{
				WARNING_MSG( "ConsolidateDBsApp::consolidateSecondaryDBs: "
						"Data consolidation was aborted\n" );
			}
			else
			{
				WARNING_MSG( "ConsolidateDBsApp::consolidateSecondaryDBs: "
						"Some entities were not consolidated. Data "
						"consolidation must be re-run after errors have been "
						"corrected.\n" );
			}
			return false;
		}
	}

	return true;
}


/**
 * 	Get list of secondary DBs from the primary DB.
 */
bool ConsolidateDBsApp::getSecondaryDBInfos( 
		SecondaryDBInfos & secondaryDBInfos )
{
	bool isOK = true;
	try
	{
		ResultSet resultSet;
		Query query( "SELECT ip, location FROM bigworldSecondaryDatabases" );

		query.execute( this->connection(), &resultSet );

		uint32					ip;
		BW::string				location;

		while (resultSet.getResult( ip, location ))
		{
			secondaryDBInfos.push_back( SecondaryDBInfo( htonl( ip ),
					location ) );
		}
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "ConsolidateDBsApp::getSecondaryDBInfos: Failed to get "
				"secondary DB information from primary database: %s\n",
			e.what() );
		isOK = false;
	}

	return isOK;
}


/**
 *	Consolidates the secondary database pointed to by filePath into the
 * 	primary database.
 */
bool ConsolidateDBsApp::consolidateSecondaryDB(
		const BW::string & filePath,
		PrimaryDatabaseUpdateQueue & primaryDBQueue,
		ConsolidationProgressReporter & progressReporter )
{
	SecondaryDatabase secondaryDB;

	if (!secondaryDB.init( filePath ))
	{
		return false;
	}

	INFO_MSG( "ConsolidateDBsApp::consolidateSecondaryDB: "
			"Consolidating '%s'\n",
		filePath.c_str() );

	BW::string secondaryDBDigest;
	if (!secondaryDB.getChecksumDigest( secondaryDBDigest ))
	{
		return false;
	}

	if (!this->checkEntityDefsDigestMatch( secondaryDBDigest ))
	{
		ERROR_MSG( "ConsolidateDBsApp::consolidateSecondaryDB: "
				"%s failed entity digest check\n", 
			filePath.c_str() );
		return false;
	}

	if (!secondaryDB.consolidate( primaryDBQueue, progressReporter, 
			!shouldStopOnError_, shouldAbort_ ))
	{
		consolidationErrors_.addSecondaryDB( filePath );
		return false;
	}

	return true;
}


/**
 *	Returns true if the given quoted MD5 digest matches the entity definition
 * 	digest that we've currently loaded
 */
bool ConsolidateDBsApp::checkEntityDefsDigestMatch(
		const BW::string& quotedDigest )
{
	MD5::Digest	digest;
	if (!digest.unquote( quotedDigest ))
	{
		ERROR_MSG( "ConsolidateDBsApp::checkEntityDefsDigestMatch: "
				"Not a valid MD5 digest\n" );
		return false;
	}

	return this->entityDefs().getPersistentPropertiesDigest() == digest;
}


/**
 *	This method is used to report a string description of our progress back to
 *	the DBApp via a watcher.
 */
void ConsolidateDBsApp::onStatus( const BW::string & status )
{
	if (pDBApp_.get() != NULL)
	{
		pDBApp_->setStatus( status );
	}
}


/**
 *	Handle a received signal.
 */
void ConsolidateDBsApp::onSignalled( int sigNum )
{
	switch (sigNum)
	{
		case SIGINT:
		case SIGHUP:
		{
			INFO_MSG( "ConsolidateDBsApp::onSignalled: "
				"aborting consolidation\n" );
			this->abort();
			break;
		}

		default:
			break;
	}
}


/**
 * 	This method returns true if our entity definitions matches the ones used by
 *	the primary database when the system was last started.
 */
bool ConsolidateDBsApp::checkPrimaryDBEntityDefsMatch()
{
	if (!this->checkPrimaryDBEntityDefsMatchInternal())
	{
		ERROR_MSG( "ConsolidateDBsApp::init: Our entity definitions do not "
				"match the ones used by the primary database\n"
				"Database consolidation should be run before making changes to "
				"entity definitions. Changing entity definitions potentially "
				"invalidates unconsolidated data.\n"
				"Run \"consolidate_dbs --clear\" to allow the server to "
				"run without doing data consolidation. Unconsolidated data "
				"will be lost.\n" );
		return false;
	}

	return true;
}


/**
 * 	This method returns true if our entity definitions matches the ones used by
 *  the primary database when the system was last started.
 */
bool ConsolidateDBsApp::checkPrimaryDBEntityDefsMatchInternal()
{
	try
	{
		MySql & connection = this->connection();

		ResultSet resultSet;
		Query query( "SELECT checksum FROM bigworldEntityDefsChecksum" );

		query.execute( connection, &resultSet );

		BW::string checkSum;

		if (resultSet.getResult( checkSum ))
		{
			return this->checkEntityDefsDigestMatch( checkSum );
		}
		else
		{
			ERROR_MSG( "ConsolidateDBsApp::checkEntityDefsMatch: "
					"Checksum table is empty\n" );
		}
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "ConsolidateDBsApp::checkEntityDefsMatch: "
				"Failed to retrieve the primary database "
				"entity definition checksum: %s\n",
			e.what() );
	}

	return false;
}


/**
 *	Aborts the consolidation process.
 */
void ConsolidateDBsApp::abort()
{
	this->dispatcher().breakProcessing();
	shouldAbort_ = true;
}


/**
 * 	This method clears the secondary DB entries from the primary
 * 	database.
 */
bool ConsolidateDBsApp::clearSecondaryDBEntries( uint * pNumEntriesCleared )
{
	try
	{
		MySql & connection = this->connection();
		MySqlTransaction transaction( connection );

		connection.execute( "DELETE FROM bigworldSecondaryDatabases" );

		uint numEntriesCleared = connection.affectedRows();

		if (pNumEntriesCleared)
		{
			*pNumEntriesCleared = numEntriesCleared;
		}

		transaction.commit();

		INFO_MSG( "ConsolidateDBsApp::clearSecondaryDBEntries: "
				"Cleared %u entries from %s:%d (%s)\n",
			numEntriesCleared,
			connectionInfo_.host.c_str(),
			connectionInfo_.port,
			connectionInfo_.database.c_str() );
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "ConsolidateDBsApp::clearSecondaryDBEntries: %s", e.what() );
		return false;
	}

	return true;
}


/**
 *	This method logs the secondary databases that need to be consolidated.
 */
bool ConsolidateDBsApp::printDatabases()
{
	SecondaryDBInfos infos;

	if (!this->getSecondaryDBInfos( infos ))
	{
		return false;
	}

	RESPONSE_INFO_MSG( "\n" );
	RESPONSE_INFO_MSG( "Secondary databases to consolidate = %" PRIzu "\n",
			infos.size() );

	SecondaryDBInfos::iterator iter = infos.begin();

	while (iter != infos.end())
	{
		Mercury::Address addr( iter->hostIP, 0 );

		RESPONSE_INFO_MSG( "%15s %s\n",
					addr.ipAsString(), iter->location.c_str() );

		++iter;
	}

	return true;
}

BW_END_NAMESPACE

// db_consolidator.cpp
