#include <Python.h>
#include <sys/types.h>

#include "log_reader.hpp"
#include "../mlutil.hpp"


BW_BEGIN_NAMESPACE

namespace
{
	class MaxHostnameVisitor : public UserComponentVisitor
	{
	public:
		MaxHostnameVisitor( LogCommonMLDB *pBWLog ) :
			pBWLog_( pBWLog ),
			maxHostnameLen_ ( 8 )
		{ }
		virtual bool onComponent( const LoggingComponent &component )
		{
			maxHostnameLen_ = std::max(maxHostnameLen_,
				pBWLog_->getHostByAddr( component.getAddress().ip ).length() );
			return true;
		}

		int maxHostnameLen() const
		{
			return maxHostnameLen_;
		}
	private:
		LogCommonMLDB *pBWLog_;
		size_t maxHostnameLen_;
	};
} // anonymous namespace


// ----------------------------------------------------------------------------
// Section: LogReaderMLDB
// ----------------------------------------------------------------------------

/**
 * Destructor.
 */
LogReaderMLDB::~LogReaderMLDB()
{
	userLogs_.clear();
}


/**
 * Callback method invoked from LogCommonMLDB during initUserLogs().
 *
 * This method updates the map of UID -> username with the current entry.
 */
bool LogReaderMLDB::onUserLogInit( uint16 uid, const BW::string &username )
{
	// In read mode we just make a record of the uid -> username mapping
	usernames_[ uid ] = username;

	return true;
}


/**
 * Initialises the LogReaderMLDB instance for use.
 *
 * @returns true on successful initialisation, false on error.
 */
bool LogReaderMLDB::init( const char *root )
{
	static const char *mode = "r";

	if (root == NULL)
	{
		ERROR_MSG( "LogReaderMLDB::init: No root log path specified.\n" );
		return false;
	}
	this->initRootLogPath( root );

	// Make sure the root directory has the access we want
	if (!MLUtil::isPathAccessible( rootLogPath_.c_str() ))
	{
		ERROR_MSG( "LogReaderMLDB::init: Root logdir (%s) not accessible in "
			"read mode.\n", rootLogPath_.c_str() );
		return false;
	}

	// Call the parent class common initialisation
	if (!this->initCommonFiles( mode ))
	{
		return false;
	}

	if (!this->initUserLogs( mode ))
	{
		return false;
	}

	return true;
}


/**
 * Returns the full path to the logs being reference by this LogReaderMLDB.
 *
 * @returns Absolute path of the log directory being used.
 */
const char *LogReaderMLDB::getLogDirectory() const
{
	return rootLogPath_.c_str();
}


/**
 * Retrieves the category names of all registered categories calling visitor
 * for each category.
 *
 * @returns true on success, false on error.
 */
bool LogReaderMLDB::getCategoryNames( CategoriesVisitor & visitor ) const
{
	return categories_.visitAllWith( visitor );
}


/**
 * Retrieves the component names of all logging components calling visitor
 * for each component.
 *
 * @returns true on success, false on error.
 */
bool LogReaderMLDB::getComponentNames( LogComponentsVisitor &visitor ) const
{
	return componentNames_.visitAllWith( visitor );
}


/**
 * Retrieves the network address and hostname of each known machine in the
 * logs, calling visitor for each known host.
 *
 * @returns true on success, false on error.
 */
bool LogReaderMLDB::getHostnames( HostnameVisitor &visitor ) const
{
	return hostnames_.visitAllWith( visitor );
}


/**
 * Returns a list of all known format strings in the log directory.
 *
 * @returns An BW::vector of BW::strings.
 */
FormatStringList LogReaderMLDB::getFormatStrings() const
{
	return formatStrings_.getFormatStrings();
}


/**
 * Obtains a dictionary of usernames and UIDs of all users seen in the logs.
 *
 * @returns BW::map of unix UID as uint16 and unix username as an BW::string.
 */
const UsernamesMap & LogReaderMLDB::getUsernames() const
{
	return usernames_;
}


/**
 *	This method obtains the ID associated with a categoryName (if one exists).
 *
 *	@param categoryName  The category name to find an ID mapping for.
 *	@param categoryID    The category ID that maps to the provided name. Only
 *	                     modified if this method returns true.
 *
 *	@return true if a category name to ID mapping exists, false otherwise.
 */
bool LogReaderMLDB::getCategoryIDByName( const BW::string & categoryName,
		MessageLogger::CategoryID & categoryID ) const
{
	return categories_.resolveNameToID( categoryName, categoryID );
}


/**
 * Obtains a UserLog for the specified unix UID.
 *
 * If a UserLog already exists for this user it is returned, otherwise it will
 * be created and loaded.
 *
 * @returns A SmartPointer to a UserLogReader on success, NULL on error.
 */
UserLogReaderPtr LogReaderMLDB::getUserLog( uint16 uid )
{
	UserLogs::iterator it = userLogs_.find( uid );
	if (it != userLogs_.end())
	{
		return it->second;
	}

	UsernamesMap::iterator usernameIter = usernames_.find( uid );
	if (usernameIter == usernames_.end())
	{
		// NB: This has been made an error message to assist in track down
		//     potential issues. Can be disabled if it is seen in syslog a lot.
		ERROR_MSG( "LogReaderMLDB::getUserLog: Unable to find a UserLog for "
			"UID %hu.\n", uid );
		return NULL;
	}

	// If we've found the UID in our list of known user logs, load the
	// UserLog for use.
	UserLogReaderPtr pUserLog( new UserLogReader( uid, usernameIter->second ),
						UserLogReaderPtr::NEW_REFERENCE );

	if (!pUserLog->init( rootLogPath_ ) || !pUserLog->isGood())
	{
		ERROR_MSG( "LogReaderMLDB::getUserLog: Failed to create a UserLog "
			"for %s.\n", usernameIter->second.c_str() );
		return NULL;
	}

	userLogs_[ uid ] = pUserLog;

	MaxHostnameVisitor maxHostVisitor( this );

	pUserLog->getUserComponents( maxHostVisitor );

	pUserLog->maxHostnameLen( maxHostVisitor.maxHostnameLen() );

	return pUserLog;
}


/**
 * Obtains the IP address associated with the provided hostname.
 *
 * @returns The IP address of the requested host on success, 0 on error.
 */
uint32 LogReaderMLDB::getAddressFromHost( const char *hostname ) const
{
	return hostnames_.getHostByName( hostname );
}


/**
 * Obtains a handler for the requested log entry, to allow presentation.
 *
 * @returns A pointer to a log handler on success, NULL on error.
 */
const LogStringInterpolator *LogReaderMLDB::getHandlerForLogEntry(
	const LogEntry &entry )
{
	return formatStrings_.getHandlerForLogEntry( entry );
}


/**
 * Refreshes the main log files if they are being written to and have been
 * modified since we last looked at them.
 *
 * @returns true on success, false on failure.
 */
bool LogReaderMLDB::refreshFileMaps()
{
	if (formatStrings_.isDirty() && !formatStrings_.refresh())
	{
		return false;
	}

	if (!hostnames_.reopenIfChanged())
	{
		return false;
	}

	if (hostnames_.isDirty() && !hostnames_.refresh())
	{
		return false;
	}

	if (componentNames_.isDirty() && !componentNames_.refresh())
	{
		return false;
	}

	if (categories_.isDirty() && !categories_.refresh())
	{
		return false;
	}
	
	if (!this->initUserLogs( "r" ))
	{
		return false;
	}

	UserLogs::iterator iter = userLogs_.begin();
	while (iter != userLogs_.end())
	{
		UserLogReaderPtr pUserLog = iter->second;
		pUserLog->reloadFiles();
		++iter;
	}
	
	return true;
}

BW_END_NAMESPACE

// log_reader.cpp
