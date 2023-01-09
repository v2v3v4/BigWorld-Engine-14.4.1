#include "script/first_include.hpp"

#include "query_params.hpp"
#include "constants.hpp"
#include "user_log_reader.hpp"
#include "log_entry.hpp"
#include "types.hpp"
#include "mldb/log_reader.hpp"

#include <sys/types.h>
#include <regex.h>


BW_BEGIN_NAMESPACE

// Helper function for compiling regexs
static regex_t *reCompile( const char *pattern, bool casesens )
{
	regex_t *re = new regex_t();
	int reFlags = REG_NEWLINE | REG_EXTENDED | REG_NOSUB | (casesens ? 0 : REG_ICASE);

	int reError = regcomp( re, pattern, reFlags );

	if (reError != 0)
	{
		char reErrorBuf[ 256 ];
		regerror( reError, re, reErrorBuf, sizeof( reErrorBuf ) );

		PyErr_Format( PyExc_SyntaxError,
			"Failed to compile regex '%s': %s\n",
			pattern, reErrorBuf );

		delete re;
		return NULL;
	}

	return re;
}


/**
 * The only mandatory argument to this method is the uid.  Everything else has
 * (reasonably) sensible defaults.
 *
 * Also, be aware that if you are searching backwards (i.e. end time > start
 * time) then your results will be in reverse order.  This is because results
 * are generated on demand, not in advance.
 *
 * This class now automatically swaps start/end and startaddr/endaddr if they
 * are passed in reverse order.  This is so we can avoid repeating all the
 * reordering logic in higher level apps.
 *
 * @param addr	IP Address of the records to find (0 for all records)
 */
QueryParams::QueryParams() :
	uid_( 0 ),
	start_( LOG_BEGIN ),
	end_( LOG_END ),
	ipAddress_( 0 ),
	pid_( 0 ),
	appid_( 0 ),
	procs_( -1 ),
	severities_( -1 ),
	logSourceTypeFlags_( (1 << NUM_MESSAGE_SOURCE) - 1 ),
	pInclude_( NULL ),
	pExclude_( NULL ),
	interpolate_( PRE_INTERPOLATE ),
	casesens_( true ),
	direction_( QUERY_FORWARDS ),
	numLinesOfContext_( 0 ),
	negateHost_( false ),
	negatePID_( false ),
	negateAppID_( false ),
	isGood_( false )
{ }


bool QueryParams::initHost( const char *host, const LogReaderMLDB *pLogReader )
{
	if (*host)
	{
		ipAddress_ = pLogReader->getAddressFromHost( host );
	}
	else
	{
		ipAddress_ = 0;
	}

	if (*host && (ipAddress_ == 0))
	{
		PyErr_Format( PyExc_LookupError,
			"Queried host '%s' was not known in the logs", host );
		return false;
	}

	return true;
}


bool QueryParams::initRegex( const char *include, const char *exclude )
{
	// PyErr messages are set in reCompile on failure.

	// Regex compilation
	if (*include)
	{
		pInclude_ = reCompile( include, casesens_ );
		if (pInclude_ == NULL)
		{
			return false;
		}
	}

	if (*exclude)
	{
		pExclude_ = reCompile( exclude, casesens_ );
		if (pExclude_ == NULL)
		{
			return false;
		}
	}

	return true;
}


/**
 *	This method initialises a comma separated list of categories to use for
 *	filtering the query.
 *
 *	@param pCategories A Python sequence containing categories to display.
 *	@param pLogReader The log reader that will be used for reading logs from.
 *
 *	@return true if initialising succeeded, false otherwise.
 */
bool QueryParams::initCategories( PyObject * pCategories,
	const LogReaderMLDB * pLogReader )
{
	if (pCategories == NULL)
	{
		return true;
	}

	if (!PySequence_Check( pCategories ))
	{
		return false;
	}

	// Note: We can't use the correct Py_ssize_t return type here because
	//       CentOS 5 is still Python 2.4
	int tupleSize = PySequence_Size( pCategories );

	for (int i = 0; i < tupleSize; ++i)
	{
		PyObject * pCategoryString = PySequence_GetItem( pCategories, i );

		if (!PyString_Check( pCategoryString ))
		{
			PyErr_Format( PyExc_LookupError,
				"Category at element %d was not a string.\n", i );
			Py_XDECREF( pCategoryString );
			return false;
		}

		BW::string categoryName = PyString_AsString( pCategoryString );

		Py_DECREF( pCategoryString );

		MessageLogger::CategoryID categoryID;
		bool hasCategoryID = pLogReader->getCategoryIDByName( categoryName,
								categoryID );
		if (hasCategoryID)
		{
			categories_.insert( categoryID );
		}
	}

	return true;
}


bool QueryParams::initStartPeriod( PyObject *startAddr, double startTime,
	UserLogReaderPtr pUserLog, const BW::string &period )
{
	// Start address take precedences over start time if both are specified.
	if (startAddr != NULL)
	{
		if (!startAddress_.fromPyTuple( startAddr ))
		{
			return false;
		}

		// Obtain the start time here to ensure that inference of direction
		// based on start and end time is always correct.
		LogEntry entry;

		if (!pUserLog->getEntry( startAddress_, entry ))
		{
			PyErr_Format( PyExc_RuntimeError,
				"Couldn't determine time for %s's entry address %s:%d",
				pUserLog->getUsername().c_str(),
				startAddress_.getSuffix(),
				startAddress_.getIndex() );
			return false;
		}

		start_ = entry.time();
	}
	else
	{
		// This code path should be used only if searching relative to a time
		// period, such as from beginning of log forward 3 seconds.
		//
		// We don't support searching backwards from the end of log, so that
		// is explicitly caught and failed.

		start_ = startTime;

		// If start time was given as either extremity and we're using a fixed
		// period (i.e. period isn't to beginning or present), we need the
		// actual time for the same reason as above.
		// TODO: hard coded "to beginning" .. etc should be changed to constants
		if (period.size() &&
			period != "to beginning" &&
			period != "to present" &&
			(isEqual( startTime, LOG_BEGIN ) || isEqual( startTime, LOG_END )))
		{
			LogEntry entry;

			if (isEqual( startTime, LOG_BEGIN ) &&
				!pUserLog->getFirstLogEntryFromStartOfLogs( entry ))
			{
				PyErr_Format( PyExc_RuntimeError,
					"Couldn't determine first log entry for user %s",
						pUserLog->getUsername().c_str() );
				return false;
			}

			// This is only used by LiveOutput mode in LogViewer or 'mlcat -f'
			if (isEqual( startTime, LOG_END ) &&
				!pUserLog->getLastLogEntryFromEndOfLogs( entry ))
			{
				PyErr_Format( PyExc_RuntimeError,
					"Couldn't determine last log entry for user %s",
						pUserLog->getUsername().c_str() );
				return false;
			}

			start_ = entry.time();
		}
	}

	return true;
}


bool QueryParams::initEndPeriod( PyObject *endAddr, double endTime,
	UserLogReaderPtr pUserLog, const BW::string &period )
{
	// endAddr takes the highest priority for figuring out the endpoint,
	// followed by period, and then by end time.
	if (endAddr != NULL)
	{
		if (!endAddress_.fromPyTuple( endAddr ))
		{
			return false;
		}

		// Obtain the end time here to ensure that inference of direction
		// based on start and end time is always correct.
		LogEntry entry;

		if (!pUserLog->getEntry( endAddress_, entry ))
		{
			PyErr_Format( PyExc_RuntimeError,
				"Couldn't determine time for %s's entry address %s:%d",
				pUserLog->getUsername().c_str(),
				endAddress_.getSuffix(),
				endAddress_.getIndex() );
			return false;
		}

		end_ = entry.time();
	}
	else if (period.size())
	{
		if (period == "to beginning")
		{
			end_ = 0;
		}
		else if (period == "to present")
		{
			end_ = -1;
		}
		else
		{
			end_ = start_.asDouble() + atof( period.c_str() );

			if ((period[0] != '+') && (period[0] != '-'))
			{
				start_ = start_.asDouble() - atof( period.c_str() );
			}
		}
	}
	else
	{
		end_ = endTime;
	}

	return true;
}


/**
 *	This method initialises the query parameters for use.
 *
 *	@param args The Python arguments to parse.
 *	@param kwargs  The keyword arguments to parse.
 *	@param pLogReader  A pointer to the LogReaderMLDB object used to access logs.
 *
 *	@returns true on success, false if a Python exception is being thrown.
 */
bool QueryParams::init( PyObject * args, PyObject * kwargs,
	const LogReaderMLDB * pLogReader )
{
	static const char *kwlist[] = { "uid", "start", "end", "startaddr",
							"endaddr", "period", "host", "pid", "appid",
							"procs", "severities", "message", "exclude",
							"interpolate", "casesens", "direction", "context",
							"source", "categories", "negate_host",
							"negate_pid", "negate_appid",
							NULL };

	int direction = QUERY_FORWARDS;
	double start = LOG_BEGIN;
	double end = LOG_END;

	const char * host = "";
	const char * include = "";
	const char * exclude = "";
	const char * cperiod = "";

	PyObject * startAddr = NULL;
	PyObject * endAddr = NULL;
	PyObject * pCategories = NULL;

	int negateHost = 0;
	int negatePID = 0;
	int negateAppID = 0;

	if (!PyArg_ParseTupleAndKeywords( args, kwargs, "H|ddO!O!ssHHiissibiiiOiii",
			const_cast<char **>( kwlist ),
			&uid_, &start, &end,
			&PyTuple_Type, &startAddr,
			&PyTuple_Type, &endAddr,
			&cperiod, &host, &pid_, &appid_, &procs_, &severities_,
			&include, &exclude, &interpolate_, &casesens_, &direction,
			&numLinesOfContext_, &logSourceTypeFlags_,
			&pCategories, &negateHost, &negatePID, &negateAppID ))
	{
		// No need to set PyErr here, taken care of by Python
		return false;
	}

	negateHost_  = (negateHost != 0);
	negatePID_   = (negatePID != 0);
	negateAppID_ = (negateAppID != 0);

	if ((direction != QUERY_FORWARDS) && (direction != QUERY_BACKWARDS))
	{
		PyErr_Format( PyExc_ValueError,
			"Invalid direction (%d)", direction );
		return false;
	}
	direction_ = (SearchDirection)direction;

	if (!this->initHost( host, pLogReader ))
	{
		// PyErr is set in initHost
		return false;
	}

	if (!this->initRegex( include, exclude ))
	{
		// PyErr messages are set in reCompile on failure.
		return false;
	}

	if (!this->initCategories( pCategories, pLogReader ))
	{
		return false;
	}

	UserLogReaderPtr pUserLog =
		const_cast< LogReaderMLDB * >( pLogReader )->getUserLog( uid_ );

	if (pUserLog == NULL)
	{
		PyErr_Format( PyExc_LookupError,
			"UID %d doesn't have any entries in this log", uid_ );
		return false;
	}

	// If this user's log has no segments (i.e. they have been rolled away) then
	// bail out now
	if (!pUserLog->hasActiveSegments())
	{
		WARNING_MSG( "QueryParams::init: "
			"%s's log has no segments, they may have been rolled\n",
			pUserLog->getUsername().c_str() );

		isGood_ = true;
		return isGood_;
	}

	BW::string period = cperiod;

	if (!this->initStartPeriod( startAddr, start, pUserLog, period ))
	{
		// PyErr set in initStartPeriod
		return false;
	}

	if (!this->initEndPeriod( endAddr, end, pUserLog, period ))
	{
		// PyErr set in initEndPeriod
		return false;
	}

	// Re-order times if passed in reverse order.  TODO: Actually verify that
	// the segment numbers are in order instead of using lex cmp on suffixes.
	bool hasValidTimes = (end_ < start_);
	bool hasValidAddresses = startAddress_.isValid() &&
						endAddress_.isValid() && endAddress_ < startAddress_;

	if (hasValidTimes || hasValidAddresses)
	{
		LogTime lt = end_;
		end_ = start_;
		start_ = lt;

		LogEntryAddressReader ea = endAddress_;
		endAddress_ = startAddress_;
		startAddress_ = ea;

		// Reverse the direction now
		//direction_ *= -1;
		direction_ = (direction_ == QUERY_FORWARDS) ?
							QUERY_BACKWARDS : QUERY_FORWARDS;
	}

	if (start_ > end_)
	{
		PyErr_Format( PyExc_RuntimeError, "QueryParams::init: "
			"Start time (%ld.%d) is greater than end time (%ld.%d)",
			(LogTime::Seconds)start_.secs_, start_.msecs_,
			(LogTime::Seconds)end_.secs_, end_.msecs_ );
	}
	else
	{
		isGood_ = true;
	}

	return isGood_;
}


/**
 *	Destructor.
 */
QueryParams::~QueryParams()
{
	if (pInclude_ != NULL)
	{
		regfree( pInclude_ );
		delete pInclude_;
	}

	if (pExclude_ != NULL)
	{
		regfree( pExclude_ );
		delete pExclude_;
	}
}


uint16 QueryParams::getUID() const
{
	return uid_;
}


void QueryParams::setUID( uint16 uid )
{
	uid_ = uid;
}


LogTime QueryParams::getStartTime() const
{
	return start_;
}


LogTime QueryParams::getEndTime() const
{
	return end_;
}


LogEntryAddressReader QueryParams::getStartAddress() const
{
	return startAddress_;
}


LogEntryAddressReader QueryParams::getEndAddress() const
{
	return endAddress_;
}


SearchDirection QueryParams::getDirection() const
{
	return direction_;
}


int QueryParams::getNumLinesOfContext() const
{
	return numLinesOfContext_;
}


bool QueryParams::isPreInterpolate() const
{
	return (interpolate_ == PRE_INTERPOLATE);
}


bool QueryParams::isPostInterpolate() const
{
	return (interpolate_ == POST_INTERPOLATE);
}


/**
 * Validates whether we have an address set, and if so, whether the address
 * provided matches.
 *
 * It is valid for there to be no address set in the parameters.
 */
bool QueryParams::validateAddress( MessageLogger::IPAddress ipAddress ) const
{
	// If no address has been specific then we can continue
	if (!ipAddress_)
	{
		return true;
	}

	return negateHost_ ? (ipAddress_ != ipAddress) : (ipAddress_ == ipAddress);
}


bool QueryParams::validatePID( uint16 pid ) const
{
	if (!pid_)
	{
		return true;
	}

	return negatePID_ ? (pid_ != pid) : (pid_ == pid);
}


bool QueryParams::validateAppID( MessageLogger::AppInstanceID appid ) const
{
	if (!appid_)
	{
		return true;
	}

	return negateAppID_ ? (appid_ != appid) : (appid_ == appid);
}


bool QueryParams::validateProcessType(
	MessageLogger::AppTypeID appTypeID ) const
{
	if (procs_ == -1)
	{
		return true;
	}

	return (procs_ & (1 << appTypeID));
}


bool QueryParams::validateMessagePriority( int priority ) const
{
	if (severities_ == -1)
	{
		return true;
	}

	return (severities_ & (1 << priority));
}


bool QueryParams::validateIncludeRegex( const char *searchStr ) const
{
	if (pInclude_ == NULL)
	{
		return true;
	}

	return (regexec( pInclude_, searchStr, 0, NULL, 0 ) == 0);
}


bool QueryParams::validateExcludeRegex( const char *searchStr ) const
{
	if (pExclude_ == NULL)
	{
		return true;
	}

	return (regexec( pExclude_, searchStr, 0, NULL, 0 ) != 0);
}


bool QueryParams::validateLogMessageType(
	DebugMessageSource messageSource ) const
{
	return (logSourceTypeFlags_ & (1 << messageSource));
}


bool QueryParams::validateCategory( MessageLogger::CategoryID categoryID ) const
{
	if (categories_.empty())
	{
		return true;
	}

	return (categories_.find( categoryID ) != categories_.end());
}

BW_END_NAMESPACE

// query_params.cpp
