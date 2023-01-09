#ifndef CONSTANTS_MONGODB_HPP
#define CONSTANTS_MONGODB_HPP

#include "types.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

namespace MongoDB
{
static const char ADMIN_DB_NAME[] = "admin";
static const char BW_COMMON_DB_NAME[] = "bw_ml_common";

// user related database and collections
static const BW::string USER_DB_PREFIX = "bw_ml_user_";
static const char USER_COLL_ENTRIES_PREFIX[] = "entries_";
// key names in entries collection
static const char ENTRIES_KEY_TIMESTAMP[] = "ts";
static const char ENTRIES_KEY_COUNTER[] = "cnt";
static const char ENTRIES_KEY_COMPONENT[] = "cpt";
static const char ENTRIES_KEY_APP_ID[] = "aid";
static const char ENTRIES_KEY_PID[] = "pid";
static const char ENTRIES_KEY_CATEGORY[] = "ctg";
static const char ENTRIES_KEY_SOURCE[] = "src";
static const char ENTRIES_KEY_SEVERITY[] = "svt";
static const char ENTRIES_KEY_HOST[] = "host";
static const char ENTRIES_KEY_FMT_STR[] = "fmt";
static const char ENTRIES_KEY_MESSAGE[] = "msg";
static const char ENTRIES_KEY_META_DATA[] = "md";
static const char ENTRIES_KEY_SHARD_KEY[] = "sk";

// uid collection
static const char USER_COLL_UID[] = "uid";
static const char USER_UID_KEY[] = "uid";

// server start ups collection
static const char SERVER_STARTS_COLL_NAME[] = "server_start_ups";
static const char SERVER_STARTS_KEY_TIME[] = "time";
static const uint32 SERVER_STARTS_COUNT_LIMIT = 50;
static const uint32 SERVER_STARTS_SIZE_LIMIT = 5000;

// appid maps collection
static const char COMPONENTS_COLL_NAME[] = "components";
static const char COMPONENTS_KEY_HOST[] = "host";
static const char COMPONENTS_KEY_APPID[] = "aid";
static const char COMPONENTS_KEY_PID[] = "pid";
static const char COMPONENTS_KEY_COMPONENT[] = "cpt";
static const char COMPONENTS_UID[] = "uid";
static const char COMPONENTS_USERNAME[] = "username";

// common database and collections

// hostnames collection
static const char HOST_NAMES_COLL[] = "hosts";
static const char HOST_NAMES_KEY_IP[] = "ip";
static const char HOST_NAMES_KEY_HOSTNAME[] = "hostname";

// format string collection
static const char FMT_STRS_COLL[] = "format_strings";
static const char FMT_STRS_KEY_ID[] = "id";
static const char FMT_STRS_KEY_FMT_STR[] = "fmt_str";

// categories collection
static const char CATEGORIES_COLL[] = "categories";
static const char CATEGORIES_KEY_ID[] = "id";
static const char CATEGORIES_KEY_NAME[] = "name";

// components collection
static const char COMPONENTS_COLL[] = "components";
static const char COMPONENTS_KEY_ID[] = "id";
static const char COMPONENTS_KEY_NAME[] = "name";

// sources collection
static const char SOURCES_COLL[] = "sources";
static const char SOURCES_KEY_ID[] = "id";
static const char SOURCES_KEY_NAME[] = "name";

// severities collection
static const char SEVERITIES_COLL[] = "severities";
static const char SEVERITIES_KEY_LEVEL[] = "level";
static const char SEVERITIES_KEY_NAME[] = "name";

// version collection
static const char VERSION_COLL[] = "schema_version";
static const char VERSION_KEY_VERSION[] = "version";

// The version of database schema of Message Logger
// Version 1: Initial implementation, BW2.9
// Version 2: Split collections, multiple Message Loggers with one MongoDB, BW2.10
static const uint32 SCHEMA_VERSION = 2;


// DB connection and communication defaults

// Default write/read operation time out
static const uint32 TCP_TIME_OUT = 5;

// Number of seconds to wait before reattempting connection when autoreconnect
// has failed.
static const uint16 WAIT_BETWEEN_RECONNECTS = 1;

// Default flush the logs to database if this size limit is reached
// This is also the default vector size for a log buffer, to stop many
// reallocations when many thousands of log messages are added per second in a
// production environment.
static const uint32 MAX_LOG_BUFFER_SIZE = 1024;

// The amount of memory to reserve for each log buffer. This reduces vector
// reallocations. In an environment with a lot of logs (such as production) this
// becomes most effective. In an environment with few logs (such as dev) then a
// little extra memory usage should not hurt when there are so few logs anyway.
static const uint32 RESERVE_LOG_BUFFER_SIZE = 128;

// Default flush interval, unit: milliseconds
static const uint32 LOG_FLUSH_INTERVAL = 500; /* 1/2 sec */

//How many days logs could be stored in database before being purged
static const uint32 EXPIRE_LOGS_DAYS = 7;


/********************************* misc ***************************************/

// The separator used in database name to separate loggerID from other part.
// This must be a character valid for MongoDB database name but not valid for
// a user name.
static const BW::string LOGGER_ID_SEPARATOR = "#";

// The loggerID to use when no loggerID is specified
static const BW::string LOGGER_ID_NA = "";

// Config file section name
static const char CONFIG_NAME_MONGO[] = "mongodb";

} // namespace MongoDB

BW_END_NAMESPACE

#endif // CONSTANTS_MONGODB_HPP
