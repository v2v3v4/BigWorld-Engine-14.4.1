#include "log_storage.hpp"
#include "../constants.hpp"
#include "../user_log_writer.hpp"
#include "../mlutil.hpp"
#include "../log_entry.hpp"

#include <libgen.h>


BW_BEGIN_NAMESPACE

namespace  // anonymous
{
const char * LOCK_FILE_NAME = "pid_lock";
}


/**
 * Constructor.
 */
LogStorageMLDB::LogStorageMLDB( Logger & logger ) :
	LogStorage( logger ),
	maxSegmentSize_( DEFAULT_SEGMENT_SIZE_MB << 20 ),
	pHostnamesValidator_( NULL )
{ }


/**
 * Destructor.
 */
LogStorageMLDB::~LogStorageMLDB()
{
	DEBUG_MSG( "LogStorageMLDB::~LogStorageMLDB(): shutting down\n" );

	// Ensure the reference counted objects are destroyed.
	userLogs_.clear();

	// If the current lock file is the same as our process, get rid of the
	// files we've created.
	if (lock_.getValue() == mf_getpid())
	{
		// Clean up the lock file so someone else can write here
		if (unlink( lock_.filename() ))
		{
			ERROR_MSG( "LogStorageMLDB::~LogStorageMLDB(): "
				"Unable to remove lock file '%s': %s\n",
				lock_.filename(), strerror( errno ) );
		}

		// Clean up the active_files
		activeFiles_.deleteFile();
	}

	// cleanup the validation Hostnames file if it exists
	this->cleanupValidatedHostnames();
}


/**
 * Initialises the LogStorageMLDB instance with any available config options.
 *
 * @returns true on success, false on error.
 */
bool LogStorageMLDB::initFromConfig( const ConfigReader &config )
{
	bool isConfigOK = true;

	BW::string tmpString;
	if (config.getValue( "mldb", "segment_size", tmpString ))
	{
		if (sscanf( tmpString.c_str(), "%d", &maxSegmentSize_ ) != 1)
		{
			ERROR_MSG( "LogStorageMLDB::initFromConfig: Failed to convert "
				"'segment_size' to an integer.\n" );
			isConfigOK = false;
		}
	}
	else
	{
		ERROR_MSG( "LogStorageMLDB::initFromConfig: Failed to get "
			"segment_size from config.\n" );
		isConfigOK = false;
	}

	if (config.getValue( "mldb", "logdir", logDir_))
	{

		// If logdir begins with a slash, it is absolute, otherwise it is
		// relative to the directory the config file resides in
		if (logDir_.c_str()[0] != '/')
		{
			BW::string newLogDir;

			char cwd[ 512 ], confDirBuf[ 512 ];
			char *confDir;

			memset( cwd, 0, sizeof( cwd ) );

			const char *confFilename = config.filename();
			// If the path to the config file isn't absolute...
			if (confFilename[0] != '/')
			{
				if (getcwd( cwd, sizeof( cwd ) ) == NULL)
				{
					ERROR_MSG( "LogStorageMLDB::initFromConfig: Failed to "
						"getcwd(). %s\n", strerror( errno ) );
					isConfigOK = false;
				}
			}
			strcpy( confDirBuf, confFilename );
			confDir = dirname( confDirBuf );

			if (cwd[0])
			{
				newLogDir = cwd;
				newLogDir.push_back( '/' );
			}
			newLogDir += confDir;
			newLogDir .push_back( '/' );
			newLogDir += logDir_.c_str();

			logDir_ = newLogDir;
		}
	}
	else
	{
		ERROR_MSG( "LogStorageMLDB::initFromConfig: Failed to get logdir from "
			"config.\n" );
		isConfigOK = false;
	}

	return isConfigOK;
}


/**
 * Callback method invoked from LogCommonMLDB during initUserLogs().
 *
 * This method creates a unique UserLog instance for the newly discovered
 * user.
 */
bool LogStorageMLDB::onUserLogInit( uint16 uid, const BW::string &username )
{
	// In write mode we actually load up the UserLog, since write mode
	// UserLogs only keep a pair of file handles open.
	return (this->createUserLog( uid, username ) != NULL);
}


/**
 * Initialises the LogStorageMLDB instance for use.
 *
 * @returns true on successful initialisation, false on error.
 */
bool LogStorageMLDB::init( const ConfigReader &config, const char *root )
{
	static const char *mode = "a+";

	// Read config in append mode only, since the Python will always pass a
	// 'root' parameter in read mode
	if (!this->initFromConfig( config ))
	{
		ERROR_MSG( "LogStorageMLDB::init: Failed to read config file\n" );
		return false;
	}

	// Only use logdir from the config file if none has been provided
	if (root == NULL || root[0] == '\0')
	{
		root = logDir_.c_str();
	}

	if (!this->initRootLogPath( root ))
	{
		ERROR_MSG( "LogStorageMLDB::init: Failed to initialise root log path.\n" );
		return false;
	}

	// Make sure the root directory has the access we want
	if (!MLUtil::softMkDir( rootLogPath_.c_str() ))
	{
		ERROR_MSG( "LogStorageMLDB::init: Root logdir (%s) not accessible in "
			"write mode.\n", rootLogPath_.c_str() );
		return false;
	}

	// Make sure another logger isn't already logging to this directory
	if (!lock_.init( lock_.join( root, LOCK_FILE_NAME ), mode, mf_getpid() ))
	{
		ERROR_MSG( "LogStorageMLDB::init: Another logger seems to be writing "
			"to %s\n", root );
		return false;
	}

	// Call the parent class common initialisation
	if (!this->initCommonFiles( mode ))
	{
		ERROR_MSG( "LogStorageMLDB::init: Failed to initialise common files.\n" );
		return false;
	}

	if (!UserLogWriter::validateUserComponents( rootLogPath_ ))
	{
		ERROR_MSG( "LogStorageMLDB::init: Failed to validate user components "
			"files.\n" );
		return false;
	}

	if (!this->initUserLogs( mode ))
	{
		ERROR_MSG( "LogStorageMLDB::init: Failed to initialise user logs.\n" );
		return false;
	}

	if (!activeFiles_.init( rootLogPath_, &userLogs_ ))
	{
		ERROR_MSG( "LogStorageMLDB::init: Failed to init 'active_files'\n" );
		return false;
	}

	// Now all the UserLogs have been opened, update the 'active_files'
	if (!activeFiles_.update())
	{
		ERROR_MSG( "LogStorageMLDB::init: Failed to touch 'active_files'\n" );
		return false;
	}

	return true;
}


/**
 * Terminates all current log segments.
 *
 * @returns true on success, false on error.
 */
bool LogStorageMLDB::roll()
{
	INFO_MSG( "Rolling logs\n" );

	UserLogs::iterator iter = userLogs_.begin();

	while (iter != userLogs_.end())
	{
		UserLogWriterPtr pUserLog = iter->second;

		pUserLog->rollActiveSegment();

		// Since the userlog now has no active segments, drop the object
		// entirely since it's likely we will be mlrm'ing around this time which
		// could blow away this user's directory.  If that happens, then a new
		// UserLog object must be created when the next log message arrives.
		UserLogs::iterator oldIter = iter++;
		userLogs_.erase( oldIter );
	}

	return activeFiles_.update();
}


/**
 * Finds the component associated with the provided address and sets the
 * instance ID of that component.
 *
 * This is used to update a component with information from bwmachined so we
 * know that we know which process (eg: cellapp01 or cellapp02) we are
 * are referring to.
 *
 * @returns true on success, false on error.
 */
bool LogStorageMLDB::setAppInstanceID( const Mercury::Address & addr,
	ServerAppInstanceID appInstanceID )
{
	LoggingComponent *pComponent = this->findLoggingComponent( addr );

	if (pComponent == NULL)
	{
		ERROR_MSG( "LogStorageMLDB::setAppInstanceID: "
			"Can't set app ID for unknown address %s\n", addr.c_str() );
		return false;
	}

	return pComponent->setAppInstanceID( appInstanceID );
}


/**
 * This method removes a process from the list of currently logging user
 * processes.
 *
 * @returns true on success, false on failure.
 */
bool LogStorageMLDB::stopLoggingFromComponent( const Mercury::Address &addr )
{
	// Search through all the UserLogs and remove that component
	UserLogs::iterator it = userLogs_.begin();
	while (it != userLogs_.end())
	{
		UserLogWriterPtr pUserLog = it->second;
		if (pUserLog->removeUserComponent( addr ))
		{
			return true;
		}

		++it;
	}

	return false;
}


/**
 * This method write an incoming log to the db.
 *
 * It is responsible for determining which user the log message belongs to,
 * and handing off the message to the appropriate UserLog to be written.
 *
 * @returns enum type AddLogMessageResult
 */
LogStorageMLDB::AddLogMessageResult LogStorageMLDB::writeLogToDB(
		const LoggerComponentMessage & componentMessage,
	 	const Mercury::Address & address, MemoryIStream & inputStream,
	 	const LoggerMessageHeader & header, LogStringInterpolator *pHandler,
	 	MessageLogger::CategoryID categoryID )
{
	uint16 uid = componentMessage.uid_;

	// Get the user log segment
	UserLogWriterPtr pUserLog = this->getUserLog( uid );
	if (pUserLog == NULL)
	{
		BW::string username;
		Mercury::Reason reason = this->resolveUID( uid, address.ip, username );

		if (reason == Mercury::REASON_SUCCESS)
		{
			pUserLog = this->createUserLog( uid, username );
			if (pUserLog == NULL)
			{
				ERROR_MSG( "LogStorageMLDB::writeLogToDB: Failed to create a "
							"UserLog for UID %hu\n", uid );

				return LOG_ADDITION_FAILED;
			}
		}
		else
		{
			ERROR_MSG( "LogStorageMLDB::writeLogToDB: Couldn't resolve uid %d "
				"(%s). UserLog not started.\n",
				uid, Mercury::reasonToString( reason ) );
			return LOG_ADDITION_FAILED;
		}
	}

	MF_ASSERT( pUserLog != NULL );

	LoggingComponent *component = pUserLog->findLoggingComponent(
										componentMessage, address,
										componentNames_ );

	// We must have a component now, if it didn't exist, it should have been
	// created
	MF_ASSERT( component != NULL );

	struct timeval tv;
	gettimeofday( &tv, NULL );

	LogEntry entry( tv, component->getUserComponentID(),
			header.messagePriority_, pHandler->fileOffset(),
			categoryID, header.messageSource_ );


	// TODO: the write to STDOUT functionality may be better placed inside
	//       UserLogWriter. All the same args are being passed in?
	// Dump output to stdout if required
	if (writeToStdout_)
	{
		std::cout << pUserLog->logEntryToString( entry,
										static_cast< LogCommonMLDB *>( this ),
										component, *pHandler, inputStream,
										componentMessage.version_ );
	}

	if (!pUserLog->addEntry( component, entry, *pHandler, inputStream,
			this, componentMessage.version_ ))
	{
		ERROR_MSG( "LogStorageMLDB::writeLogToDB: "
			"Failed to add entry to user log\n" );
		return LOG_ADDITION_FAILED;
	}

	return LOG_ADDITION_SUCCESS;
}


/**
 * This method forces the active files to update in case any new files have
 * been created by a UserLog.
 *
 * @returns true on success, false on error.
 */
bool LogStorageMLDB::updateActiveFiles()
{
	return activeFiles_.update();
}


/**
 * This method forces the deletion of the active segments file.
 *
 * It is used only to avoid a potential race condition when adding an entry to
 * the UserLogs.
 */
void LogStorageMLDB::deleteActiveFiles()
{
	activeFiles_.deleteFile();
}


/**
 * This method returns the maximum allowable size that UserSegments should
 * consume before rolling into a new segment file.
 *
 * @returns Maximum segment size allowable (in bytes).
 */
int LogStorageMLDB::getMaxSegmentSize() const
{
	return maxSegmentSize_;
}


/**
 * Creates a new UserLogWriter and adds it to the list of UserLogs.
 *
 * @returns A SmartPointer to a UserLogWriter on success, NULL on error.
 */
UserLogWriterPtr LogStorageMLDB::createUserLog( uint16 uid,
	const BW::string &username )
{
	UserLogWriterPtr pUserLog( new UserLogWriter( uid, username ),
							UserLogWriterPtr::NEW_REFERENCE );

	if (!pUserLog->init( rootLogPath_ ) || !pUserLog->isGood())
	{
		ERROR_MSG( "LogStorageMLDB::createUserLog: Failed to create a UserLog "
			"for %s.\n", username.c_str() );
		return NULL;
	}

	userLogs_[ uid ] = pUserLog;

	return pUserLog;
}


/**
 * Returns the UserLog object for the requested UID.
 *
 * UserLogs should exist for all users necessary by the time this method is
 * invoked.
 *
 * @returns A SmartPointer to a UserLogWriter on success, NULL on error.
 */
UserLogWriterPtr LogStorageMLDB::getUserLog( uint16 uid )
{
	UserLogs::iterator it = userLogs_.find( uid );
	if (it != userLogs_.end())
	{
		return it->second;
	}

	return NULL;
}


/**
 * Initialises hostnames validation process using HostnamesValidatorMLDB class.
 *
 * @returns true on success, false on error.
 */
bool LogStorageMLDB::initValidatedHostnames()
{
	if (pHostnamesValidator_)
	{
		// already in validation mode
		return false;
	}

	pHostnamesValidator_ = new HostnamesValidatorMLDB();

	if (!pHostnamesValidator_->init( rootLogPath_.c_str() ))
	{
		ERROR_MSG( "LogStorageMLDB::initValidatedhostnames: "
			"Unable to initialise validator.\n" );
		return false;
	}

	// Copy the full hostname map from hostnames_ to pHostnamesValidator_
	HostnameCopier hostnameCopier( pHostnamesValidator_ );
	hostnames_.visitAllWith( hostnameCopier );

	INFO_MSG( "LogStorageMLDB::initValidatedHostnames: "
		"Hostname validation process started.\n" );

	return true;
}


/**
 * This method deletes the temporary hostnames file and directory.
 *
 * @param pTempFilename		Temporary filename to delete.
 *
 * @returns true on success, false on error.
 */
bool LogStorageMLDB::cleanupTempHostnames( const char *pTempFilename ) const
{
	if (!pTempFilename)
	{
		ERROR_MSG( "LogStorageMLDB::cleanupTempHostnames: "
			"Invalid argument: pTempFilename can not be NULL.\n" );
		return false;
	}

	if (bw_unlink( pTempFilename ) == -1)
	{
		ERROR_MSG( "LogStorageMLDB::cleanupTempHostnames: "
			"Unable to unlink backup hostnames file '%s': %s\n",
			pTempFilename, strerror( errno ) );
		return false;
	}

	return true;
}


/**
 * Restores a backed-up hostnames file.
 *
 * @param pBackupFilename	The name of the backup file.
 * @param restoreToFilename	The filename to restore to.
 *
 * @returns true on success, false on error.
 */
bool LogStorageMLDB::restoreBackupHostnames( const char *pBackupFilename,
	const char *restoreToFilename ) const
{
	// Rollback (ie. relink the backup hostnames to the original filename to
	// prevent ending up with no hostnames file at all!)
	if (link( pBackupFilename, restoreToFilename ) < 0)
	{
		// Unrecoverable error, we now may be missing a hostnames file!
		CRITICAL_MSG( "LogStorageMLDB::restoreBackupHostnames: "
			"Unable to restore hostnames file '%s': %s. Hostnames file may be "
			"missing.\n",
			restoreToFilename, strerror( errno ) );
		return false;
	}

	return true;
}


BW::string LogStorageMLDB::getBackupHostnamesFile() const
{
	// Generate a temp filename to use in the root log directory
	time_t now = time( NULL );
	struct tm *pTime = localtime( &now );
	MF_ASSERT( pTime != NULL);

	char dateTime[15];
	if (strftime( dateTime, sizeof( dateTime ), "%Y%m%d%H%M%S", pTime ) == 0)
	{
		ERROR_MSG( "LogStorageMLDB:getBackupHostnamesFile: "
			"An error occurred converting time to string (strftime).\n" );
		return BW::string();
	}

	BW::string newFilename = HostnamesMLDB::join( rootLogPath_.c_str(),
		HostnamesMLDB::getHostnamesFilename() );
	newFilename += ".bak.";
	newFilename += dateTime;
	const char *pNewFilename = newFilename.c_str();

	struct stat buf;

	if (stat( pNewFilename, &buf ) == -1)
	{
		if (errno != ENOENT)
		{
			// System error
			ERROR_MSG( "LogStorageMLDB::getBackupHostnamesFile: "
				"Unable to stat file '%s': %s\n",
				pNewFilename, strerror( errno ) );
			return BW::string();
		}
	}
	else
	{
		ERROR_MSG( "LogStorageMLDB::getBackupHostnamesFile: "
			"File '%s' exists.", pNewFilename );
		return BW::string();
	}

	return newFilename;
}


/**
 * Replaces the hostnames file with a newly validated hostnames. Includes a
 * file backup and recovery process in case of link failure.
 *
 * @returns true on success, false on error.
 */
bool LogStorageMLDB::relinkValidatedHostnames()
{
	if ( !pHostnamesValidator_ )
	{
		ERROR_MSG( "LogStorageMLDB::relinkValidatedHostnames: "
			"Unable to unlink original hostnames file '%s': "
			"pHostnamesValidator_ is NULL.\n",
			hostnames_.filename() );
		return false;
	}

	if ( pHostnamesValidator_->isOpened() )
	{
		pHostnamesValidator_->close();
	}


	// Backup the hostnames file so that if the relink process below fails we
	// can try to recover the link to the original
	BW::string tempFilename = this->getBackupHostnamesFile();

	if (tempFilename.length() == 0)
	{
		ERROR_MSG( "LogStorageMLDB::relinkValidatedHostnames: "
			"Unable to get a temporary filename for backup.\n" );
		return false;
	}

	const char *pBackupFilename = tempFilename.c_str();

	if (link( hostnames_.filename(), pBackupFilename ) == -1)
	{
		ERROR_MSG( "LogStorageMLDB::relinkValidatedHostnames: "
			"Unable to link original hostnames file '%s' "
			"to backup filename '%s': %s\n",
			hostnames_.filename(), pBackupFilename, strerror( errno ) );
		return false;
	}


	// Now perform the actual unlink/relink.
	if (bw_unlink( hostnames_.filename() ) == -1)
	{
		ERROR_MSG( "LogStorageMLDB::relinkValidatedHostnames: "
			"Unable to unlink original hostnames file '%s': %s\n",
			hostnames_.filename(), strerror( errno ) );
		this->cleanupTempHostnames( pBackupFilename );
		return false;
	}

	if (link( pHostnamesValidator_->filename(), hostnames_.filename() ) == -1)
	{
		ERROR_MSG( "LogStorageMLDB::relinkValidatedHostnames: "
			"Unable to link new hostnames file '%s' to original filename '%s': "
			"%s\n",
			pHostnamesValidator_->filename(), hostnames_.filename(),
			strerror( errno ) );

		if (!this->restoreBackupHostnames( pBackupFilename, hostnames_.filename() ))
		{
			CRITICAL_MSG( "LogStorageMLDB::relinkValidatedHostnames: "
				"Unable to restore backup hostnames.\n" );
			// Do not clean up temp hostnames, as it may be required by the
			// administrator for a manual restore
		}
		else
		{
			this->cleanupTempHostnames( pBackupFilename );
		}
		return false;
	}

	if (!hostnames_.reopenIfChanged())
	{
		ERROR_MSG( "LogStorageMLDB::relinkValidatedHostnames: "
			"Unable to reopen hostnames_.\n" );

		if (!this->restoreBackupHostnames( pBackupFilename, hostnames_.filename() ))
		{
			CRITICAL_MSG( "LogStorageMLDB::relinkValidatedHostnames: "
				"Unable to restore backup hostnames.\n" );
			// Do not clean up temp hostnames, as it may be required by the
			// administrator for a manual restore
		}
		else
		{
			this->cleanupTempHostnames( pBackupFilename );
		}
		return false;
	}

	this->cleanupTempHostnames( pBackupFilename );
	this->cleanupValidatedHostnames();

	return true;
}


/**
 * This method is used to finalise a hostnames validation process.
 *
 * The destruction of pHostnamesValidator_ will clean up resources, and setting
 * it to NULL will indicate that we are no longer in the validation process.
 */
void LogStorageMLDB::cleanupValidatedHostnames()
{
	if (pHostnamesValidator_)
	{
		delete pHostnamesValidator_;
		pHostnamesValidator_ = NULL;
	}
}


/**
 * Initialises the validation process (if it is not already) and then checks
 * the next unvalidated hostname in the list
 *
 * @returns true on continue processing, false on do not continue (ie.
 * error/finished)
 */
HostnamesValidatorProcessStatus LogStorageMLDB::validateNextHostname()
{
	// Start validation process if it is not already started
	if (!pHostnamesValidator_ && !this->initValidatedHostnames())
	{
		ERROR_MSG( "LogStorageMLDB::validateNextHostname: "
			"Unable to initialise hostnames validation process.\n" );
		return BW_VALIDATE_HOSTNAMES_FAILED;
	}

	HostnamesValidatorProcessStatus status =
		pHostnamesValidator_->validateNextHostname();

	if (status == BW_VALIDATE_HOSTNAMES_FINISHED)
	{
		// Successful completion
		if (pHostnamesValidator_->writeHostnamesToDB())
		{
			if (!relinkValidatedHostnames())
			{
				ERROR_MSG( "LogStorageMLDB::validateNextHostname: "
					"Unable to relink validated hostnames.\n" );
				// Force failure status and let it flow through to cleanup
				status = BW_VALIDATE_HOSTNAMES_FAILED;
			}
			else
			{
				INFO_MSG( "Logger::handleMessages: "
					"Hostname validation process completed successfully.\n" );
			}
		}
	}

	if (status != BW_VALIDATE_HOSTNAMES_CONTINUE)
	{
		// End of process - always ensure the validator file is cleaned up
		// regardless of what happened (eg. an error may have occurred during
		// the validation or relink processes).
		this->cleanupValidatedHostnames();
	}

	return status;
}


/**
 * This method retrieves the current logging component associated with the
 * specified address.
 *
 * @returns A pointer to a LoggingComponent on success, NULL if no
 *          LoggingComponent matches the specified address.
 */
LoggingComponent * LogStorageMLDB::findLoggingComponent(
	const Mercury::Address &addr )
{
	LoggingComponent *pComponent = NULL;

	UserLogs::iterator it = userLogs_.begin();
	while (it != userLogs_.end())
	{
		UserLogWriterPtr pUserLog = it->second;
		pComponent = pUserLog->findLoggingComponent( addr );

		if (pComponent != NULL)
		{
			return pComponent;
		}

		++it;
	}

	return NULL;
}

BW_END_NAMESPACE

// log_storage.cpp
