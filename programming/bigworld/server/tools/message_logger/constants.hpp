#ifndef MESSAGE_LOGGER_CONSTANTS_HPP
#define MESSAGE_LOGGER_CONSTANTS_HPP

#include "network/logger_message_forwarder.hpp"
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

// This constant represents the format version of the log directory and
// contained files. It is independant of MESSAGE_LOGGER_VERSION as defined
// in 'src/lib/network/logger_message_forwarder.hpp' which represents the
// protocol version of messages sent between components and the logger.
// Version 6: * LogTime seconds changed from time_t to int64 to prevent
//              architecture differences.
// Version 7: * User segment entries now contain an extra field to represent
//              DebugMessageSource and a new categories file
// Version 8: * User segment entries now contain an extra field to represent
//              length and offset of an entry in a new metadata file
static const int LOG_FORMAT_VERSION = 8;

static const double LOG_BEGIN = 0;
static const double LOG_END = -1;

static const int DEFAULT_SEGMENT_SIZE_MB = 100;

const BW::string BW_TOOLS_COMPONENT_NAME = "Tools";
const BW::string BW_TOOLS_SERVER_START_MSG = "Starting server";

namespace // anonymous
{

const MessageLogger::NetworkVersion LOG_METADATA_VERSION = 9;
const MessageLogger::NetworkVersion LOG_METADATA_STREAM_VERSION = 10;

} // end anonymous namespace



enum DisplayFlags
{
	SHOW_DATE = 1 << 0,
	SHOW_TIME = 1 << 1,
	SHOW_HOST = 1 << 2,
	SHOW_USER = 1 << 3,
	SHOW_PID = 1 << 4,
	SHOW_APPID = 1 << 5,
	SHOW_PROCS = 1 << 6,
	SHOW_SEVERITY = 1 << 7,
	SHOW_MESSAGE = 1 << 8,
	SHOW_MESSAGE_SOURCE_TYPE = 1 << 9,
	SHOW_CATEGORY = 1 << 10,
	SHOW_ALL = ((1 << 11) - 1)
};


enum SearchDirection
{
	QUERY_FORWARDS = 1,
	QUERY_BACKWARDS = -1
};


enum InterpolateFlags
{
	DONT_INTERPOLATE = 0,
	POST_INTERPOLATE = 1,
	PRE_INTERPOLATE = 2
};

BW_END_NAMESPACE

#endif // MESSAGE_LOGGER_CONSTANTS_HPP
