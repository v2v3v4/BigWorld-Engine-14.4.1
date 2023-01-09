#include "user_log_writer.hpp"

#include "mlutil.hpp"
#include "user_segment_writer.hpp"
#include "log_string_interpolator.hpp"
#include "query_result.hpp"
#include "mldb/log_storage.hpp"


BW_BEGIN_NAMESPACE

/**
 * Constructor.
 */
UserLogWriter::UserLogWriter( uint16 uid, const BW::string &username ) :
 UserLog( uid, username )
{ }


bool UserLogWriter::init( const BW::string rootPath )
{
	if (!UserLog::init( rootPath ))
	{
		return false;
	}

	static const char *mode = "a+";

	if (!MLUtil::softMkDir( path_.c_str() ))
	{
		ERROR_MSG( "UserLogWriter::init: User log directory is not "
			"accessible in write mode: %s\n", path_.c_str() );
		return false;
	}

	if (!uidFile_.init( uidFile_.join( path_.c_str(), "uid" ), mode, uid_ ))
	{
		ERROR_MSG( "UserLogWriter::UserLogWriter: "
			"Failed to init uid file in %s\n", path_.c_str() );
		return false;
	}

	if (!userComponents_.init( path_.c_str(), mode ))
	{
		ERROR_MSG( "UserLogWriter::UserLogWriter: "
			"Failed to read components mapping from %s\n",
			userComponents_.filename() );
		return false;
	}

	isGood_ = true;
	return isGood_;
}


LoggingComponent * UserLogWriter::findLoggingComponent(
	const Mercury::Address &addr )
{
	return userComponents_.getComponentByAddr( addr );
}


LoggingComponent * UserLogWriter::findLoggingComponent(
	const LoggerComponentMessage &msg, const Mercury::Address &addr,
	LogComponentNamesMLDB &logComponents )
{
	return userComponents_.getComponentFromMessage( msg, addr, logComponents );
}


/**
 * Removes the current active segment which will ensure a new segment file
 * is created when the next log message comes in.
 */
void UserLogWriter::rollActiveSegment()
{
	if (!this->hasActiveSegments())
	{
		return;
	}

	UserSegment *pUserSegment = userSegments_.back();
	delete pUserSegment;

	// Why is this being done? This was copied from BWLog::roll, but
	// seems strange to do..
	userSegments_.pop_back();
}


/**
 * Adds a LogEntry to the end of the UserSegment file.
 */
bool UserLogWriter::addEntry( LoggingComponent * component, LogEntry & entry,
		LogStringInterpolator & handler, MemoryIStream & inputStream,
		LogStorageMLDB * pLogStorage, MessageLogger::NetworkVersion version )
{
	// Make sure segment is ready to be written to
	if (userSegments_.empty() || this->getLastSegment()->isFull( pLogStorage ))
	{
		// If userSegments_ is empty, there's a potential race condition here.
		// For the time between the call to 'new Segment()' and the call to
		// 'activeFiles_.write()', the newly created segment could be
		// accidentally rolled by mltar.  To avoid this, we blow away
		// active_files so that mltar knows not to roll at the moment.  That's
		// OK because activeFiles_.write() regenerates the whole file anyway.
		if (userSegments_.empty())
		{
			pLogStorage->deleteActiveFiles();
		}

		UserSegmentWriter *pSegment = new UserSegmentWriter( path_, NULL );
		pSegment->init();

		if (pSegment->isGood())
		{
			// Drop full segments as we don't need em around anymore and they're
			// just eating up memory and file handles
			while (!userSegments_.empty())
			{
				delete userSegments_.back();
				userSegments_.pop_back();
			}

			userSegments_.push_back( pSegment );

			// Update active_files
			if (!pLogStorage->updateActiveFiles())
			{
				ERROR_MSG( "UserLog::addEntry: "
					"Unable to update active_files\n" );
				return false;
			}
		}
		else
		{
			ERROR_MSG( "UserLog::addEntry: "
				"Couldn't create new segment %s; dropping msg with fmt '%s'\n",
				pSegment->getSuffix().c_str(), handler.formatString().c_str() );
			delete pSegment;
			return false;
		}
	}

	MF_ASSERT( userSegments_.size() == 1 );

	UserSegmentWriter *pSegment = this->getLastSegment();
	return pSegment->addEntry( component, this, entry, handler,
			inputStream, version );
}


UserSegmentWriter * UserLogWriter::getLastSegment()
{
	return static_cast< UserSegmentWriter * >( userSegments_.back() );
}


bool UserLogWriter::updateComponent( LoggingComponent *component )
{
	MF_ASSERT( component != NULL );
	return userComponents_.write( component );
}


bool UserLogWriter::removeUserComponent( const Mercury::Address &addr )
{
	return userComponents_.erase( addr );
}


const char * UserLogWriter::logEntryToString( const LogEntry & entry,
		LogCommonMLDB * pBWLog, const LoggingComponent * component,
		LogStringInterpolator & handler, MemoryIStream & is,
		MessageLogger::NetworkVersion version ) const
{
	BW::string msg;
	MemoryIStream args( is.data(), is.remainingLength() );

	handler.streamToString( args, msg, version );
	QueryResult result( entry, pBWLog, this, component, msg, BW::string() );

	// This dodgy return is OK because the pointer is to a static buffer
	const char *text = result.format();
	return text;
}


/**
 * This method validates all user components files to ensure they are not
 * corrupt.
 *
 * First it looks for components files in all subdirectories of the root log
 * path, then attempts to open and read them using the UserComponents class.
 *
 * @returns true on successful validation of all files, false on error
 */
bool UserLogWriter::validateUserComponents( const BW::string & rootPath )
{
	struct dirent *pDirEntry;

	DIR *pRootLogDir = opendir( rootPath.c_str() );
	if (pRootLogDir == NULL)
	{
		ERROR_MSG( "UserLogWriter::validateUserLogs: "
			"Unable to open root log path %s: %s.\n",
			rootPath.c_str(), strerror( errno ) );
		return false;
	}

	bool errorStatus = false;

	// Examine the root log path for all entries, looking for subdirectories
	while ((pDirEntry = readdir( pRootLogDir )) != NULL)
	{
		char fullFilePath[512];

		bw_snprintf( fullFilePath, sizeof( fullFilePath ), "%s/%s",
			rootPath.c_str(), pDirEntry->d_name );

		struct stat subDirStatbuf;

		if (stat( fullFilePath, &subDirStatbuf ) == -1)
		{
			ERROR_MSG( "UserLogWriter::validateUserLogs: "
				"Unable to stat file %s: %s.\n",
				fullFilePath, strerror( errno ) );
			errorStatus = true;
		}

		if (!S_ISDIR( subDirStatbuf.st_mode ))
		{
			continue;
		}

		// Look for a "components" file in the subdirectory
		BW::string componentsPath =
			FileHandler::join( fullFilePath, "components" );

		struct stat componentsStatbuf;

		if (stat( componentsPath.c_str(), &componentsStatbuf ) == -1)
		{
			if (errno == ENOENT)
			{
				// No components file to validate. Skip this directory.
				continue;
			}
			else
			{
				ERROR_MSG( "UserLogWriter::validateUserComponents: "
					"Failed to stat components file %s: %s\n",
					componentsPath.c_str(), strerror( errno ) );
				errorStatus = true;
			}
		}

		UserComponents userComponents;

		if (!userComponents.init( fullFilePath, "r" ))
		{
			ERROR_MSG( "UserLogWriter::validateUserComponents: "
				"Failed to read components mapping from %s\n",
				userComponents.filename() );
			errorStatus = true;
		}
	}

	closedir( pRootLogDir );

	return !errorStatus;
}


BW_END_NAMESPACE

// user_log_writer.cpp
