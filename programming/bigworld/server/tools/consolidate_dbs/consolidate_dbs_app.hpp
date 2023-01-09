#ifndef CONSOLIDATE_DBS_APP_HPP
#define CONSOLIDATE_DBS_APP_HPP

#include "dbapp.hpp"
#include "dbapp_status_reporter.hpp"
#include "db_consolidator_errors.hpp"
#include "file_receiver_mgr.hpp"

#include "db_storage/entity_key.hpp"

#include "db_storage_mysql/connection_info.hpp"
#include "db_storage_mysql/database_tool_app.hpp"

#include "cstdmf/singleton.hpp"


BW_BEGIN_NAMESPACE

class ConsolidationProgressReporter;
class PrimaryDatabaseUpdateQueue;

/**
 * 	Consolidates data from remote secondary databases.
 */
class ConsolidateDBsApp : public DatabaseToolApp,
		public Singleton< ConsolidateDBsApp >,
		public DBAppStatusReporter
{
public:
	ConsolidateDBsApp( bool shouldStopOnError );
	virtual ~ConsolidateDBsApp();

	bool initLogger();

	bool init( bool isVerbose, bool shouldReportToDBApp, bool shouldLock,
		const DBConfig::ConnectionInfo & primaryDBConnectionInfo );

	bool checkPrimaryDBEntityDefsMatch();

	bool transferAndConsolidate();
	bool consolidateSecondaryDBs( const FileNames & filePaths );
	bool clearSecondaryDBEntries( uint * pNumEntriesCleared = NULL );

	bool printDatabases();

	void abort();

private:
	// From DBAppStatusReporter
	virtual void onStatus( const BW::string & status );

	// From DatabaseToolApp
	virtual void onSignalled( int sigNum );

	bool checkPrimaryDBEntityDefsMatchInternal();

	bool getSecondaryDBInfos( SecondaryDBInfos & secondaryDBInfos );
	bool transferSecondaryDBs( const SecondaryDBInfos & secondaryDBInfos,
		FileReceiverMgr & fileReceiverMgr );

	bool consolidateSecondaryDB( const BW::string & filePath,
		PrimaryDatabaseUpdateQueue & primaryDBQueue,
		ConsolidationProgressReporter & progressReporter );

	bool checkEntityDefsDigestMatch( const BW::string & quotedDigest );


	// Member data

	uint32						internalIP_;

	std::auto_ptr< DBApp >		pDBApp_;

	DBConfig::ConnectionInfo 	connectionInfo_;

	BW::string					consolidationDir_;

	DBConsolidatorErrors		consolidationErrors_;
	bool						shouldStopOnError_;

	// Flag for aborting our wait loop.
	bool						shouldAbort_;
};

BW_END_NAMESPACE

#endif // CONSOLIDATE_DBS_APP_HPP
