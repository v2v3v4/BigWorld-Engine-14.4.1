#ifndef MYSQL_CONSTANTS_HPP
#define MYSQL_CONSTANTS_HPP

#include "cstdmf/bw_namespace.hpp"

#include <mysql/mysql.h>

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Useful constants
// -----------------------------------------------------------------------------
// __kyl__ (20/7/2005) For some reason I can't find a #define for these but they
// are well known to MySQL developers.
enum MySQLLimits
{
	MySQLMaxTableNameLen = NAME_LEN,
	MySQLMaxColumnNameLen = NAME_LEN,
	MySQLMaxDbNameLen = NAME_LEN,
	MySQLMaxIndexNameLen = NAME_LEN,
	MySQLMaxInnoDbIndexLen = 767,		// From documentation
	MySQLMaxMyIsamDbIndexLen = 1000		// From documentation
};

enum BWMySQLLimits
{
	BWMySQLMaxTypeNameLen = 64,
	BWMySQLMaxLogOnNameLen = 255,
	BWMySQLMaxLogOnPasswordLen = 255,
	BWMySQLMaxNamePropertyLen = 255
};

#define TABLE_NAME_PREFIX				"tbl"
#define	DEFAULT_SEQUENCE_COLUMN_NAME	"value"
#define DEFAULT_SEQUENCE_TABLE_NAME		"values"
#define ID_COLUMN_NAME					"id"
#define PARENTID_COLUMN_NAME			"parentID"
#define GAME_TIME_COLUMN_NAME			"gameTime"
#define TIMESTAMP_COLUMN_NAME			"timestamp"

const BW::string ID_COLUMN_NAME_STR( ID_COLUMN_NAME );
const BW::string PARENTID_COLUMN_NAME_STR( PARENTID_COLUMN_NAME );
const BW::string TIMESTAMP_COLUMN_NAME_STR( TIMESTAMP_COLUMN_NAME );

// Arbitrary number used to determine our buffer size.
static const unsigned long MAX_SECONDARY_DB_LOCATION_LENGTH = 4096;

BW_END_NAMESPACE

#endif // MYSQL_CONSTANTS_HPP
