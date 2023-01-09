#include "snapshot.hpp"

#include "transfer_db.hpp"

#include "../snapshot_helper/snapshot_constants.hpp"

#include "db/db_config.hpp"

#include "server/child_process.hpp"

#include "sqlite/sqlite_connection.hpp"

#include <sys/types.h>
#include <sys/wait.h>

#include <string.h>
#include <errno.h>

DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )

BW_BEGIN_NAMESPACE

static const char * SNAPSHOT_HELPER = "snapshot_helper";
static const char * RSYNC = "/usr/bin/rsync";


//============================
// class PrimarySnapshotHelper
//============================

/**
 *	This class handles the termination of a child snapshotting process.
 */
class PrimarySnapshotHelper : public IChildProcess
{
public:
	PrimarySnapshotHelper( Snapshot & snapshotter );

	virtual void onChildComplete( int status, ChildProcess * process );

	virtual void onChildAboutToExec();

	bool wasSuccessful() { return snapshotStatus_; }

private:
	Snapshot & snapshotter_;

	bool snapshotStatus_;
};


/**
 *	Constructor.
 */
PrimarySnapshotHelper::PrimarySnapshotHelper( Snapshot & snapshotter ) :
	snapshotter_( snapshotter ),
	snapshotStatus_( false )
{
}


/*
 *	Override from IChildProcess.
 */
void PrimarySnapshotHelper::onChildComplete( int status,
	ChildProcess * pProcess )
{
	snapshotStatus_ = (WIFEXITED( status ) &&
		(WEXITSTATUS( status ) == SnapshotHelper::SUCCESS));
}


/*
 * Override from IChildProcess.
 */
void PrimarySnapshotHelper::onChildAboutToExec()
{
	snapshotter_.onChildAboutToExec();
}


//==============================
// class RSyncChildProcessHelper
//==============================

/**
 *	This class handles the termination of a child snapshotting process.
 */
class RSyncChildProcessHelper : public IChildProcess
{
public:
	RSyncChildProcessHelper( Snapshot & snapshotter );

	virtual void onChildComplete( int status, ChildProcess * process );

	virtual void onChildAboutToExec();

	bool wasSuccessful() { return childSucceeded_; }

private:
	Snapshot & snapshotter_;

	bool childSucceeded_;
};


/**
 *	Constructor.
 */
RSyncChildProcessHelper::RSyncChildProcessHelper( Snapshot & snapshotter ) :
	snapshotter_( snapshotter ),
	childSucceeded_( false )
{
}


// TODO: Can combine RSyncChildProcessHelper with PrimarySnapshotHelper just
// need different methods so the onChildComplete to check for the correct
// return value.
/*
 *	Override from IChildProcess.
 */
void RSyncChildProcessHelper::onChildComplete( int status,
	ChildProcess * pProcess )
{
	childSucceeded_ = (WIFEXITED( status ) &&
		(WEXITSTATUS( status ) == EXIT_SUCCESS));
}


void RSyncChildProcessHelper::onChildAboutToExec()
{
	snapshotter_.onChildAboutToExec();
}




//===========================
// class Snapshot
//===========================


/**
 *	Constructor.
 */
Snapshot::Snapshot() :
	pSignalProcessor_( 0 )
{
}


/**
 *
 */
bool Snapshot::init( BW::string destinationIP, BW::string destinationPath,
	BW::string limitKbps )
{
	// The SignalProcessor is an auto_ptr so that it can be destroyed in the
	// child process to avoid any issues with it still referring to the event
	// dispatcher.
	pSignalProcessor_.reset( new SignalProcessor(
		TransferDB::instance().dispatcher() ) );

	// TODO:
	// validate rsync exists and is runnable / discoverable

	destinationIP_ = destinationIP;
	destinationPath_ = destinationPath;
	limitKbps_ = limitKbps;

	return true;
}


/**
 *	This method tests whether the snapshot_helper process is able to be
 *	successfully executed. 
 */
bool Snapshot::isSnapshotHelperSetUidRoot() const
{
	PrimarySnapshotHelper snapshotHelper( const_cast< Snapshot & >( *this ) );

	Mercury::EventDispatcher & eventDispatcher =
			TransferDB::instance().dispatcher();

	// Run snapshot_helper without any arguments to test it is setuid root
	// capable.
	BW::string pathToSnapshotHelper = "_helpers/";
	pathToSnapshotHelper += SNAPSHOT_HELPER;

	ChildProcess child( eventDispatcher, &snapshotHelper,
		pathToSnapshotHelper.c_str() );

	child.startProcessWithPipe( /*pipeStdOut:*/ true, /*pipeStdErr:*/ true );

	while (!child.isFinished())
	{
		eventDispatcher.processOnce( /*shouldIdle*/ true );
	}

	if (!snapshotHelper.wasSuccessful())
	{
		ERROR_MSG( "Snapshot::isSnapshotHelperSetUidRoot: "
			"snapshot_helper failed to run.\n" );
		ERROR_MSG( "snapshot_helper stdout:\n%s\n", child.stdOut().c_str() );
		ERROR_MSG( "snapshot_helper stderr:\n%s\n", child.stdErr().c_str() );
		return false;
	}

	return true;
}


/**
 *	This methods is called in a child process to close all file descriptors and
 *	sockets that were opened by the parent process.
 */
void Snapshot::onChildAboutToExec()
{
	pSignalProcessor_.reset();
	TransferDB::instance().onChildAboutToExec();
}


/**
 *	This method uses the snapshot_helper program to take a snapshot of the
 *	LVM volume that contains the MySQL database.
 */
bool Snapshot::takePrimarySnapshot( BW::string & pathToSnapshot )
{
	const DBConfig::Config & config = DBConfig::get();
	const BW::string & dbUsername = config.mysql.username();
	const BW::string & dbPassword = config.mysql.password();

	if ((dbUsername.size() == 0) || (dbPassword.size() == 0))
	{
		ERROR_MSG( "Snapshot::takePrimarySnapshot: "
			"dbapp username / password empty.\n" );
		return false;
	}

	PrimarySnapshotHelper snapshotHelper( const_cast< Snapshot & >( *this ) );

	Mercury::EventDispatcher & eventDispatcher =
			TransferDB::instance().dispatcher();

	// Snapshot helper command:
	// snapshot_helper  acquire-snapshot  <db username>  <db password>
	BW::string pathToSnapshotHelper = "_helpers/";
	pathToSnapshotHelper += SNAPSHOT_HELPER;

	ChildProcess child( eventDispatcher, &snapshotHelper,
		pathToSnapshotHelper.c_str() );
	child.addArg( "acquire-snapshot" );
	child.addArg( dbUsername );
	child.addArg( dbPassword );

	child.startProcessWithPipe( /*pipeStdOut:*/ true, /*pipeStdErr:*/ true );

	// isFinished() will only be true after the
	// PrimarySnapshotHelper::onChildComplete() method has been called.
	while (!child.isFinished())
	{
		eventDispatcher.processOnce( /*shouldIdle*/ true );
	}

	if (!snapshotHelper.wasSuccessful())
	{
		ERROR_MSG( "Snapshot::takePrimarySnapshot: "
			"snapshot_helper failed to acquire a snapshot.\n" );
		ERROR_MSG( "snapshot_helper stdout:\n%s\n", child.stdOut().c_str() );
		ERROR_MSG( "snapshot_helper stderr:\n%s\n", child.stdErr().c_str() );
		return false;
	}

	// The last line of the stdout stream should contain the path of the
	// snapshot.
	BW::string snapshotOutput = child.stdOut();
	size_t eolPos = snapshotOutput.rfind( '\n' );
	if (eolPos == BW::string::npos)
	{
		ERROR_MSG( "Snapshot::takePrimarySnapshot: "
			"Unable to find snapshot path from snapshot_helper output.\n" );
		ERROR_MSG( "%s\n", snapshotOutput.c_str() );
		return false;
	}

	size_t startPathPos = snapshotOutput.rfind( '\n', eolPos-1 );
	if (startPathPos == BW::string::npos)
	{
		ERROR_MSG( "Snapshot::takePrimarySnapshot: "
			"Unable to find start of snapshot path from snapshot_helper "
			"output.\n" );
		return false;
	}

	// Move the start position right so we don't include the \n
	startPathPos += 1;
	pathToSnapshot = snapshotOutput.substr( startPathPos,
									(eolPos - startPathPos) );

	// TODO: test that pathToSnapshot exists as a final verification

	return true;
}


/**
 *	This method uses the snapshot_helper program to remove a snapshot that
 *	has been previously taken and transferred back to the requesting host.
 */
bool Snapshot::removePrimarySnapshot()
{
	PrimarySnapshotHelper snapshotHelper( const_cast< Snapshot & >( *this ) );

	Mercury::EventDispatcher & eventDispatcher =
			TransferDB::instance().dispatcher();

	// Snapshot helper command:
	// snapshot_helper  release-snapshot
	BW::string pathToSnapshotHelper = "_helpers/";
	pathToSnapshotHelper += SNAPSHOT_HELPER;

	ChildProcess child( eventDispatcher, &snapshotHelper,
		pathToSnapshotHelper.c_str() );
	child.addArg( "release-snapshot" );

	child.startProcessWithPipe( /*pipeStdOut:*/ true, /*pipeStdErr:*/ true );

	while (!child.isFinished())
	{
		eventDispatcher.processOnce( /*shouldIdle*/ true );
	}

	if (!snapshotHelper.wasSuccessful())
	{
		ERROR_MSG( "Snapshot::removePrimarySnapshot: "
			"Failed to release snapshot\n" );
		ERROR_MSG( "snapshot_helper stdout:\n%s\n", child.stdOut().c_str() );
		ERROR_MSG( "snapshot_helper stderr:\n%s\n", child.stdErr().c_str() );
		return false;
	}

	return true;
}


/**
 *	This method provides a convenient interface for the merged form of the
 *	IP and path that Rsync will use for copying the file.
 */
BW::string Snapshot::destination() const
{
	BW::stringstream tmpDestination;
	tmpDestination << destinationIP_ << ":" << destinationPath_;
	return tmpDestination.str();
}


/**
 *	This method
 */
bool Snapshot::sendSnapshot( BW::string srcPath )
{
	// Send via RSync
	RSyncChildProcessHelper rsyncHelper( const_cast< Snapshot & >( *this ) );

	Mercury::EventDispatcher & eventDispatcher =
			TransferDB::instance().dispatcher();

	ChildProcess child( eventDispatcher, &rsyncHelper, RSYNC );

	child.addArg( "--archive" );
	child.addArg( "--no-specials" );
	child.addArg( "--no-devices" );
	child.addArg( srcPath );
	child.addArg( this->destination() );

//TODO: change atoi to strtoul
	if (atoi( limitKbps_.c_str() ) != 0)
	{
		BW::stringstream ss;

		ss << "--bwlimit=" << limitKbps_;
		child.addArg( ss.str() );
	}

	child.startProcessWithPipe( /*pipeStdOut:*/ false, /*pipeStdErr:*/ true );
	while (!child.isFinished())
	{
		eventDispatcher.processOnce( /*shouldIdle*/ true );
	}

	if (!rsyncHelper.wasSuccessful())
	{
		ERROR_MSG( "Snapshot::sendSnapshot: "
			"rsync failed to transfer snapshot.\n" );
		ERROR_MSG( "rsync stderr:\n%s\n", child.stdErr().c_str() );
		return false;
	}

	return true;
}


/**
 *	This method
 */
bool Snapshot::sendCompletionFile( BW::string srcPath )
{
	// Now the Rsync has completed successfully, we need to transfer a single
	// file which will notify the receiving end that we have finished.

	BW::stringstream srcBasePath;
	srcBasePath << "/tmp/" << basename( srcPath.c_str() ) << ".complete";

	int completionFD = open( srcBasePath.str().c_str(),
								O_WRONLY | O_CREAT, 0644 );
	if (completionFD == -1)
	{
		ERROR_MSG( "Snapshot::sendCompletionFile: "
			"Unable to create completion file '%s': %s\n",
			srcBasePath.str().c_str(), strerror( errno ) );
		return false;
	}
	close( completionFD );

	RSyncChildProcessHelper rsyncHelper( *this );

	Mercury::EventDispatcher & eventDispatcher =
			TransferDB::instance().dispatcher();

	ChildProcess child( eventDispatcher, &rsyncHelper, RSYNC );

	child.addArg( srcBasePath.str() );
	child.addArg( this->destination() );

	child.startProcessWithPipe( /*pipeStdOut:*/ false, /*pipeStdErr:*/ true );
	while (!child.isFinished())
	{
		eventDispatcher.processOnce( /*shouldIdle*/ true );
	}

	if (!rsyncHelper.wasSuccessful())
	{
		ERROR_MSG( "Snapshot::sendCompletionFile: "
			"rsync failed to transfer snapshot. stderr follows.\n" );
		ERROR_MSG( "%s\n", child.stdErr().c_str() );
		// Purposefully not returning false here to allow unlinking
	}

	if (unlink( srcBasePath.str().c_str() ) == -1)
	{
		ERROR_MSG( "Snapshot::sendCompletionFile: "
			"Unable to unlink completion file '%s': %s\n",
			srcBasePath.str().c_str(), strerror( errno ) );
		return false;
	}

	return rsyncHelper.wasSuccessful();
}


/**
 *	This method transfers a snapshot that has already been taken back to the
 *	requesting machine using the program rsync.
 */
bool Snapshot::transferSnapshotViaRsync( BW::string srcPath )
{
	return (this->sendSnapshot( srcPath ) &&
				this->sendCompletionFile( srcPath ));
}


/**
 *	This method initiates the process of snapshotting the primary database.
 */
bool Snapshot::transferPrimary()
{
	INFO_MSG( "Snapshot::transferPrimary: "
		"Starting primary DB snapshot and transfer.\n" );

	// Test that the snapshot_helper process can be run before using it.
	NOTICE_MSG( "Snapshot::transferPrimary: "
		"Testing snapshot_helper is installed with setuid root privileges.\n" );

	if (!this->isSnapshotHelperSetUidRoot())
	{
		ERROR_MSG( "Snapshot::transferPrimary: "
			"snapshot_helper binary appears to be incorrectly installed.\n" );
		return false;
	}

	NOTICE_MSG( "Snapshot::transferPrimary: "
		"Attempting to run snapshot_helper to create image of primary DB.\n" );

	// Take the snapshot using snapshot_helper
	BW::string pathToSnapshot;
	if (!this->takePrimarySnapshot( pathToSnapshot ))
	{
		ERROR_MSG( "Snapshot::transferPrimary: "
			"Failed to take primary snapshot.\n" );
		return false;
	}

	// Once the snapshot has been successfully taken it must be sent back to
	// the machine that initiated the snapshotting procedure.
	NOTICE_MSG( "Snapshot::transferPrimary: "
		"Attempting to send new snapshot back to %s using rsync.\n",
		destinationIP_.c_str() );

	bool wasTransferred = this->transferSnapshotViaRsync( pathToSnapshot );
	if (!wasTransferred)
	{
		ERROR_MSG( "Snapshot::transferPrimary: "
			"Failed to transfer successful snapshot back to the requesting "
			"host.\n" );
	}


	// Now that the snapshot has been transferred back to the requesting machine
	// we remove the snapshot directory using snapshot_helper.
	NOTICE_MSG( "Snapshot::transferPrimary: "
		"Attempting to remove successfully transferred snapshot.\n" );
	bool wasSnapshotRemoved = this->removePrimarySnapshot();
	if (!wasSnapshotRemoved)
	{
		ERROR_MSG( "Snapshot::transferPrimary: "
			"Failed to release snapshot of primary database.\n" );
	}

	return (wasTransferred && wasSnapshotRemoved);
}


/**
 *	This method opens both the source and destination sqlite databases and
 *	performs a safe backup from the source into the destination.
 *
 *	@param srcPath  The location of the source sqlite DB to backup.
 *	@param destPath The location where the backup should be placed.
 *
 *	@returns true on success, false on error.
 */
bool Snapshot::backupSqliteDatabase( const BW::string & srcPath,
	const BW::string & destPath )
{
	std::auto_ptr< SqliteConnection > pSrcDB( new SqliteConnection );
	if (!pSrcDB->open( srcPath.c_str() ))
	{
		ERROR_MSG( "Snapshot::backupSqliteDatabase: "
			"Source database inaccessible\n" );
		return false;
	}

	std::auto_ptr< SqliteConnection > pDestDB( new SqliteConnection );
	if (!pDestDB->open( destPath.c_str() ))
	{
		ERROR_MSG( "Snapshot::backupSqliteDatabase: "
			"Destination database inaccessible\n" );
		return false;
	}

	// NB: This doesn't appear to be defined anywhere else, it should possibly
	//     be made into a constant or a define somewhere.
	BW::string sqliteSecondaryDatabaseName = "main";
	return pDestDB->backup( pSrcDB.get(), sqliteSecondaryDatabaseName );
}


/**
 *	This method makes a safe backup of the sqlite secondary database
 *	and uses rsync to transfer it back to the requesting host.
 *
 *	@param sqlitePath  The location of the sqlite database on the local host.
 *
 *	@returns true on success, false on error.
 */
bool Snapshot::transferSecondary( const BW::string & sqlitePath )
{
	INFO_MSG( "Snapshot::transferSecondary: "
		"Starting secondary DB snapshot and transfer.\n" );

	BW::string sqliteFilename = basename( sqlitePath.c_str() );

	// This variable is used by mkdtemp() as the template name for the
	// temporary directory creation. The last 6 characters must be XXXXXX. The
	// variable will be modified by the call to mkdtemp().
	char tempDirname[] = "/tmp/transferdb-XXXXXX";

	if (mkdtemp( tempDirname ) == NULL)
	{
		ERROR_MSG( "Snapshot::transferSecondary: "
			"Failed to create temporary directory: %s", strerror( errno ) );
		return false;
	}

	BW::stringstream snapshotPath;
	snapshotPath << tempDirname << "/" << sqliteFilename;

	// For details on the sqlite backup functionality see
	//  http://www.sqlite.org/backup.html
	bool wasCopied = this->backupSqliteDatabase( sqlitePath,
							snapshotPath.str() );
	if (!wasCopied)
	{
		ERROR_MSG( "Snapshot::transferSecondary: "
			"Failed to backup the sqlite db '%s'.\n", sqlitePath.c_str() );
	}

	bool wasTransferred = false;
	if (wasCopied)
	{
		// Perform the rsync of the backup now
		wasTransferred = this->transferSnapshotViaRsync( snapshotPath.str() );
		if (!wasTransferred)
		{
			ERROR_MSG( "Snapshot::transferSecondary: "
				"Failed to transfer successful secondary snapshot back "
				"to the requesting host.\n" );
		}
	}

	if (unlink( snapshotPath.str().c_str() ) == -1)
	{
		ERROR_MSG( "Snapshot::transferSecondary: "
			"Unable to remove secondary db snapshot '%s': %s\n",
			snapshotPath.str().c_str(), strerror( errno ) );
		// This shouldn't be considered a failure case as the temporary
		// directory won't hinder any operational behaviour.
		//return false;
	}

	if (rmdir( tempDirname ) == -1)
	{
		ERROR_MSG( "Snapshot::transferSecondary: "
			"Unable to remove temporary directory '%s': %s\n",
			tempDirname, strerror( errno ) );
		// This shouldn't be considered a failure case as the temporary
		// directory won't hinder any operational behaviour.
		//return false;
	}

	return (wasCopied && wasTransferred);
}

BW_END_NAMESPACE

// snapshot.cpp
