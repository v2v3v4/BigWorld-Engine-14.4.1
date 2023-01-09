#include "pch.hpp"
#include <fstream>
#include <sstream>
#include <time.h>
#include "debug_filter.hpp"
#include "debug_message_file_logger.hpp"
#include "log_msg.hpp"
#include "bwversion.hpp"
#include "string_utils.hpp"
#include "config.hpp"
#include "bw_util.hpp"
#include "string_builder.hpp"

namespace {// anonymous
const char * strSeverities[] = {
	"TRACE",
	"DEBUG",
	"INFO",
	"NOTICE",
	"WARNING",
	"ERROR",
	"CRITICAL",
	"HACK",
	"ASSET"
};

const char * strSources[] = {
	"CPP",
	"SCRIPT"
};


/**
* This method convert an array of string values into bits basedon the table 
* provided by availableStrings.
*
* @param stringArrayValue		The string array that's need be converted.
* @param stringsTable			An array of strings where the index is 
* @param lenStringsTable		The length of stringsTable
*								associated to the bits, ie. stringTable[1]
*								associated to 1<<1, stringTable[2] associated to
*								1<<2.
* @return						The bits result converted from the string array.
*
*/
template<typename T>
T stringArrayToBits( const BW::vector<BW::string> & stringArrayValue, 
					const char ** stringsTable, size_t lenStringsTable,
					BW::StringBuilder& errorMsg )
{
	MF_ASSERT( lenStringsTable < sizeof( T ) * 8 );
	T retBits = 0;
	BW::vector<BW::string>::const_iterator sectIt = stringArrayValue.begin();
	BW::vector<BW::string>::const_iterator sectEnd = stringArrayValue.end();
	for (; sectIt < sectEnd; ++sectIt)
	{
		bool found = false;
		for (size_t i = 0; i < lenStringsTable; ++i)
		{
			if (bw_stricmp((*sectIt).c_str(), stringsTable[ i ]) == 0)
			{
				retBits |= (1 << i);
				found = true;
				break;
			}
		}
		if (!found)
		{
			errorMsg.appendf(
				"stringArrayToBits: '%s' is not valid, will be ignored.\n",
				(*sectIt).c_str() );
		}
	}
	return retBits;
}


/**
* This method convert bits value into an array of string based on the table 
* provided by availableStrings.
*
* @param bitsValue				The bits value that's need be converted.
* @param stringsTable			An array of strings where the index is 
*								associated to the bits, ie. stringTable[1]
*								associated to 1<<1, stringTable[2] associated to
*								1<<2.
* @param lenStringsTable			The length of stringsTable
* @param retString				Out, the result string.
* @param maxRetString			The max length of retString.
*
*/
template<typename T>
void bitsToString( T bitsValue, const char ** stringsTable,
			size_t lenStringsTable, char* retString, size_t maxRetString )
{
	MF_ASSERT( lenStringsTable < sizeof( T ) * 8 );
	BW::StringBuilder builder( retString, maxRetString );
	for (size_t i = 0; i < lenStringsTable; ++i)
	{
		if (bitsValue & ( 1 << i ))
		{
			if (builder.length() != 0)
			{
				builder.append( ';' );
			}
			builder.append( stringsTable[ i ] );
		}
	}
	builder.string();
}

} // anonymous namespace

BW_BEGIN_NAMESPACE


/**
 *	Default Constructor.
 */
DebugMessageFileLogger::DebugMessageFileLogger():
	enable_( false ),
	hasWrittenBanner_( false ),
	pOutFile_( NULL ),
	severities_( SERVERITY_ALL ),
	sources_( SOURCE_ALL ),
	category_( "" )
{
	BW_GUARD;
	DebugFilter::instance().addMessageCallback( this );
}


/**
 *	Destructor.
 */
DebugMessageFileLogger::~DebugMessageFileLogger()
{
	BW_GUARD;

	DebugFilter::instance().deleteMessageCallback( this );
	bw_safe_delete( pOutFile_ );
	this->onLogFileChanged( NULL );
}


/**
 *	This method returns the filename that will/is being logged to.
 *
 *	@return The filename the file logger will log to.
 */
BW::string DebugMessageFileLogger::fileName() const
{
	BW_GUARD;
	return fileName_;
}


/**
 *	This method returns the file mode (write / append) to be used when logging.
 *
 *
 *	@return The file mode being used (a = Append, w = Write).
 */
BW::string DebugMessageFileLogger::openMode() const
{
	BW_GUARD;
	return (openMode_ == APPEND) ? "a" : "w";
}


/**
 *
 */
BW::string DebugMessageFileLogger::severities() const
{
	BW_GUARD;
	char retString[ 1024 ] = "";
	bitsToString<uint16>( severities_, strSeverities, 
								ARRAY_SIZE( strSeverities ), retString, 1024 );
	return retString;
}


/**
 *
 */
BW::string DebugMessageFileLogger::sources() const
{
	BW_GUARD;
	char retString[ 1024 ] = "";
	bitsToString<uint8>( sources_, strSources, 
								ARRAY_SIZE(strSources), retString, 1024 );
	return retString;
}


/**
 *	This method returns whether the file logger is enabled or not.
 *
 *	@return true if enabled, false otherwise.
 */
bool DebugMessageFileLogger::enable() const 
{
	BW_GUARD;
#if FORCE_ENABLE_DEBUG_MESSAGE_FILE_LOG
	return true;
#endif//FORCE_ENABLE_DEBUG_MESSAGE_FILE_LOG
	return enable_; 
}


/**
 *	This method convert an array of source string into unit8
 */
/*static*/ uint8 DebugMessageFileLogger::convertSources( 
								const BW::vector<BW::string> & sourceArray, 
								StringBuilder& errorMsg )
{
	BW_GUARD;
	return stringArrayToBits<uint8>( sourceArray, strSources,
									ARRAY_SIZE( strSources ), errorMsg );
}


/**
 *	This method convert an array of severity string into unit16
 */
/*static*/ uint16 DebugMessageFileLogger::convertSeverities( 
								const BW::vector<BW::string> & serverityArray,
								StringBuilder& errorMsg )
{
	BW_GUARD;
		return stringArrayToBits<uint16>( serverityArray, strSeverities,
									ARRAY_SIZE( strSeverities ), errorMsg );
}


/**
 *	This method convert string openMode into enum FileOpenMode
 */
/*static*/ DebugMessageFileLogger::FileOpenMode 
		DebugMessageFileLogger::convertOpenMode( const BW::string& openMode,
													StringBuilder& errorMsg )
{
	if (bw_str_equal( openMode.c_str(), "w", 1 ))
	{
		return DebugMessageFileLogger::OVERWRITE;
	}
	else if (bw_str_equal( openMode.c_str(), "a", 1 ))
	{
		return DebugMessageFileLogger::APPEND;
	}
	else
	{
		errorMsg.appendf( "DebugMessageFileLogger::convertOpenMode:"
			"The openMode '%s' is not valid, will use default 'a' instead.\n",
			openMode.c_str());

		return DebugMessageFileLogger::APPEND;
	}
}


/**
 * This method set configuration for the logging, 
 *
 * @param logFileName		The file to save the log,if "", then 
 *							executable_file_name.log will be used.
 * @param severities		combination of severities.
 * @param sources			combination of source.
 * @param category			category, if it's "",means all categories are included.
 * @param openMode			file open mode.
 * @param enableLog			if start to write into disk.
*/
void DebugMessageFileLogger::config( const BW::string & logFileName /*= BW::string()*/,
									uint16 severities /*= SERVERITY_ALL*/,
									const BW::string & category /*= BW::string()*/,
									uint8 sources /*= SOURCE_ALL*/,
									FileOpenMode openMode /*=  APPEND*/,
									bool enableLog /*= false*/ )
{
	BW_GUARD;
	BW::StringBuilder fileNameSB( fileName_, MAX_LOG_NAME );
	fileNameSB.clear();
	if (logFileName.size() == 0)// Use the executable_file_name.log.
	{
		fileNameSB.append( BWUtil::executableBasename() );
		fileNameSB.append( ".log" );
	}
	else
	{
		fileNameSB.append( logFileName );
	}
	fileNameSB.string();

	SimpleMutexHolder smh( mutex_ );

	// Always close the old file and open again, it won't hurt even though it's 
	// same file, cause openMode might has been changed.
	bw_safe_delete( pOutFile_ );
	pOutFile_ = new std::ofstream( fileName_,
			(openMode == APPEND)? std::ios::app :  std::ios::trunc );

	if (pOutFile_->bad())
	{
		ERROR_MSG("DebugMessageFileLogger::config: Can not open file %s\n",
			fileName_ );
		bw_safe_delete( pOutFile_ );
	}

	this->onLogFileChanged( pOutFile_ );

	severities_ = severities;
	category_ = category;
	std::transform( category_.begin(), category_.end(), 
										category_.begin(), tolower );
	sources_ = sources;
	openMode_ = openMode;
	this->enable( enableLog );
	hasWrittenBanner_ = false;
}


/**
 *	This method determine if priority match our severities_
 */
bool DebugMessageFileLogger::matchSeverity( DebugMessagePriority priority ) const
{
	BW_GUARD;
	return ((priority == MESSAGE_PRIORITY_TRACE) && (severities_ & SERVERITY_TRACE))||
		((priority == MESSAGE_PRIORITY_DEBUG) && (severities_ & SERVERITY_DEBUG))||
		((priority == MESSAGE_PRIORITY_INFO) && (severities_ & SERVERITY_INFO))||
		((priority == MESSAGE_PRIORITY_NOTICE) && (severities_ & SERVERITY_NOTICE))||
		((priority == MESSAGE_PRIORITY_WARNING) && (severities_ & SERVERITY_WARNING))||
		((priority == MESSAGE_PRIORITY_ERROR) && (severities_ & SERVERITY_ERROR))||
		((priority == MESSAGE_PRIORITY_CRITICAL) && (severities_ & SERVERITY_CRITICAL))||
		((priority == MESSAGE_PRIORITY_HACK) && (severities_ & SERVERITY_HACK))||
		((priority == MESSAGE_PRIORITY_ASSET) && (severities_ & SERVERITY_ASSET));
}


/**
 *	This method determine if category match our category_
 */
bool DebugMessageFileLogger::matchCategory( const char * pCategory ) const
{
	BW_GUARD;

	if (category_.size() == 0)//category_ == ""
	{
		return true;
	}

	if ((pCategory == NULL) || (pCategory[0] == '\0'))
	{
		return false;
	}

	return (bw_stricmp( category_.c_str(), pCategory ) == 0);
}


/**
 *	This method determine if messageSource match our sources_
 */
bool DebugMessageFileLogger::matchSource( DebugMessageSource source ) const
{
	BW_GUARD;
	return ((source == MESSAGE_SOURCE_CPP) && (sources_ & SOURCE_CPP))||
		((source == MESSAGE_SOURCE_SCRIPT) && (sources_ & SOURCE_SCRIPT));
}


/*
 *	Override from DebugMessageCallback.
 */
bool DebugMessageFileLogger::handleMessage(
	DebugMessagePriority messagePriority, const char * pCategory,
	DebugMessageSource messageSource, const LogMetaData & /*metaData*/,
	const char *pFormat, va_list argPtr )
{
	BW_GUARD;
	SimpleMutexHolder smh( mutex_ );

	// Try to log the message into file.
	if (pOutFile_ == NULL||
		!this->enable() ||
		!this->matchSeverity( messagePriority ) ||
		!this->matchSource( messageSource ) ||
		!this->matchCategory( pCategory ))
	{
		return false;
	}

	// TODO: implement metadata logging to DebugMessageFileLogger

	const char * priorityStr = messagePrefix( messagePriority );

	static const int MAX_MESSAGE_BUFFER = 8192;
	static char buffer[ MAX_MESSAGE_BUFFER ];
	LogMsg::formatMessage( buffer, MAX_MESSAGE_BUFFER, pFormat, argPtr );
	*pOutFile_ << priorityStr << ": " << buffer << std::endl;

	return false;
}


/**
 *
 */
void DebugMessageFileLogger::enable( bool enable ) 
{ 
	BW_GUARD;
	enable_ = enable; 
	if (enable_)
	{
		this->addLogHeader();
	}
}


/*
 *	This method add the welcome header when starting to log
 */
void DebugMessageFileLogger::addLogHeader()
{
	BW_GUARD;

	SimpleMutexHolder smh( mutex_ );

	if (hasWrittenBanner_ || pOutFile_ == NULL || !this->enable())
	{
		return;
	}

	char executeTime [64];
	time_t curTime = time(NULL);
	strftime (executeTime, 64, "%c", localtime(&curTime));

	const char * compileTime = __TIME__ " " __DATE__;
	*pOutFile_ << "\n/-----------------------------------------------"
		"-------------------------------------------\\\n";

	BW::string exe = BWUtil::executableBasename();
	*pOutFile_ << "BigWorld "<< exe.c_str()
		<< " " << BWVersion::versionString()
		<< " (compiled at " << compileTime << ") starting on "
		<< executeTime << "\n\n";

	pOutFile_->flush();

	hasWrittenBanner_ = true;
}


BW_END_NAMESPACE

