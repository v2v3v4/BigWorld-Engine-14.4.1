#include "wrapper.hpp"

#include "connection_info.hpp"
#include "database_exception.hpp"
#include "result_set.hpp"
#include "string_conv.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/debug_message_categories.hpp"

#include <mysql/errmsg.h>

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: class MySql
// -----------------------------------------------------------------------------

bool MySql::s_hasInitedCollationLengths_ = false;
MySql::CollationLengths MySql::s_collationLengths_;

uint32 MySql::s_connect_timeout_ = 5;
bool MySql::s_hasMaxAllowedPacket_ = false;
uint32 MySql::s_maxAllowedPacket_ = 1048576; // 1M
uint32 MySql::s_maxAllowedPacketWarnLevel_ = 1048576 / 2; // 50%

/**
 *	Constructor.
 *
 *	@param connectInfo 	The connection parameters.
 */
MySql::MySql( const DBConfig::ConnectionInfo & connectInfo ) :
	// initially set all pointers to 0 so that we can see where we got to
	// should an error occur
	sql_( NULL ),
	inTransaction_( false ),
	hasLostConnection_( false ),
	connectInfo_( connectInfo )
{
	this->connect( connectInfo );
}


/**
 *	Destructor.
 */
MySql::~MySql()
{
	MF_ASSERT( !inTransaction_ );
	this->close();
}


/**
 *	Close the MySQL connection, and clear the connection state.
 */
void MySql::close()
{
	if (sql_)
	{
		mysql_close( sql_ );
		sql_ = NULL;
	}
}


/**
 *	Close any current connection and reconnect to the given server.
 */
bool MySql::reconnectTo( const DBConfig::ConnectionInfo & connectInfo )
{
	this->close();

	try
	{
		this->connect( connectInfo );
	}
	catch (...)
	{
		return false;
	}

	return this->ping();
}


/**
 *	Connect to the given server.
 */
void MySql::connect( const DBConfig::ConnectionInfo & connectInfo )
{
	this->close();
	hasLostConnection_ = false;

	try
	{
		sql_ = mysql_init( 0 );

		if (!sql_)
		{
			this->throwError();
		}

		my_bool secureAuth = connectInfo.secureAuth;

		mysql_options( sql_, MYSQL_SECURE_AUTH, & secureAuth );

		// When server is not available and when timeout is 0, we block
		// here until SIGQUIT. SIGINT is trapped by mysql_real_connect.
		sql_->options.connect_timeout = s_connect_timeout_;

		if (!mysql_real_connect( sql_, connectInfo.host.c_str(),
				connectInfo.username.c_str(), connectInfo.password.c_str(),
				connectInfo.database.c_str(), connectInfo.port, NULL, 
				CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS ))
		{
			this->throwError();
		}

		// Set the client connection to be UTF8
		if (0 != mysql_set_character_set( sql_, "utf8" ))
		{
			ERROR_MSG( "MySql::MySql: "
				"Could not set client connection character set to UTF-8\n" );
			this->throwError();
		}

		if ( !s_hasMaxAllowedPacket_ )
		{
			// Get the max allowed packet length
			const static BW::string QUERY_MAX_PACKET( "SHOW VARIABLES LIKE "
				"'max_allowed_packet'" );
			this->realQuery( QUERY_MAX_PACKET );
			MYSQL_RES * res = mysql_store_result( sql_ );
			MYSQL_ROW row = mysql_fetch_row( res );

			if ( row )
			{
				StringConv::toValue( s_maxAllowedPacket_, row[1] );
				CONFIG_INFO_MSG("MySql::connect: max_allowed_packet = %d\n",
					s_maxAllowedPacket_);
				s_maxAllowedPacketWarnLevel_ = s_maxAllowedPacket_ / 2;
				s_hasMaxAllowedPacket_ = true;
			}

			mysql_free_result( res );
		}

		this->queryCharsetLengths();
	}
	catch (std::exception& e)
	{
		ERROR_MSG( "MySql::MySql: %s\n", e.what() );
		hasLostConnection_ = true;
		throw;
	}
}


/**
 *	Query the character set lengths.
 */
void MySql::queryCharsetLengths()
{
	if (s_hasInitedCollationLengths_)
	{
		return;
	}

	// We only need to collect char set information once over all
	// connections on the assumption that we are only ever connecting to
	// one database, and thus charset information shouldn't be different
	// for different connections.

	// Since character sets are identified in MYSQL_FIELD structures by a
	// collation ID, which uniquely identifies both the collation and the
	// character set used, we map each collation ID with the corresponding
	// character set's maximum length.

	const static BW::string QUERY_COLLATIONS( "SHOW COLLATION" );
	const static BW::string QUERY_CHARSETS( "SHOW CHARACTER SET" );
	const static unsigned long MAXLEN_WIDTH = 1;

	typedef BW::map< BW::string, uint > CharsetLengths;
	CharsetLengths charsetLengths;

	s_collationLengths_.clear();

	// Make a map of the character set names -> character lengths.
	this->realQuery( QUERY_CHARSETS );
	MYSQL_RES * res = mysql_store_result( sql_ );
	MYSQL_ROW row = NULL;
	while ((row = mysql_fetch_row( res )))
	{
		unsigned long * colLens = mysql_fetch_lengths( res );

		// The column order is: charset, description, default collation 
		// and maxlen.
		BW::string charsetName( row[0], colLens[0] );
		if (colLens[3] != MAXLEN_WIDTH)
		{
			CRITICAL_MSG( "MySql::queryCharsetLength: Could not retrieve "
				"maxlen for charset, expecting length 1 column for "
				"maxlen\n" );
		}

		uint maxLen = 0;
		StringConv::toValue( maxLen, row[3] );
		charsetLengths[charsetName] = maxLen;

	}
	mysql_free_result( res );

	// Insert into the collation length map the collation ID and the
	// corresponding character set character length.
	this->realQuery( QUERY_COLLATIONS );
	res = mysql_store_result( sql_ );

	while ((row = mysql_fetch_row( res )))
	{
		unsigned long * colLens = mysql_fetch_lengths( res );
		// The column order is : collation name, charset, ID, default, 
		// compiled, sortlen.
		BW::string charsetName( row[1], colLens[1] );
		uint collationID = 0;
		StringConv::toValue( collationID, row[2] );
		s_collationLengths_[collationID] = charsetLengths[charsetName];
	}
	mysql_free_result( res );

	s_hasInitedCollationLengths_ = true;
}


/**
 *	Return the maximum length of a character as represented in a charset
 *	identified by its charset/collation ID.
 *
 *	@param charsetnr 	The 'charsetnr' field value in the MYSQL_FIELD.
 */
uint MySql::charsetWidth( unsigned int charsetnr )
{
	MF_ASSERT( s_hasInitedCollationLengths_ );
	CollationLengths::const_iterator iLength =
		s_collationLengths_.find( charsetnr );

	if (iLength != s_collationLengths_.end())
	{
		return iLength->second;
	}

	// If unknown (or 0), assume it is 1-byte/character.
	return 1;
}


namespace MySqlUtils
{
	inline unsigned int getErrno( MYSQL* connection )
	{
		return mysql_errno( connection );
	}

	inline unsigned int getErrno( MYSQL_STMT* statement )
	{
		return mysql_stmt_errno( statement );
	}
}


/**
 *	This method throws an exeception based on MySQL's state.
 */
void MySql::throwError()
{
	DatabaseException e( sql_ );

	if (e.isLostConnection())
	{
		this->hasLostConnection( true );
	}

	throw e;
}


/**
 *	This method executes mysql_real_query. If the initial attempt fails due to
 *	a lost connection. The connection is re-established and retried.
 */
int MySql::realQuery( const BW::string & query )
{
	if ( query.length() > s_maxAllowedPacketWarnLevel_ )
	{
		WARNING_MSG( "MySQL::realQuery: SQL query length (%" PRIzu ") is over "
			"50%% of max_allowed_packet (%u), increase this value in my.cnf "
			"to suppress this message\n"
			"First 128 bytes of the Query: %s\n", query.length(),
			s_maxAllowedPacket_, query.substr(0, 128).c_str() );
	}

	int result = mysql_real_query( sql_, query.c_str(), query.length() );

	if (result == 0)
	{
		// Success
		return 0;
	}

	unsigned int errNum = mysql_errno( sql_ );

	if (!inTransaction_ &&
			((errNum == CR_SERVER_LOST) || (errNum == CR_SERVER_GONE_ERROR)))
	{
		INFO_MSG( "MySql::realQuery: "
				"Connection lost. Attempting to reconnect.\n" );

		if (this->reconnect())
		{
			INFO_MSG( "MySql::realQuery: Reconnect succeeded.\n" );
			result = mysql_real_query( sql_, query.c_str(), query.length() );
		}
		else
		{
			WARNING_MSG( "MySql::realQuery: "
					"Reconnect failed when executing %s.\n", query.c_str() );
		}
	}
	else
	{
		WARNING_MSG( "MySql::realQuery: MySQL reported error: %s\n",
			mysql_error( sql_ ) );
	}

	return result;
}


/**
 *	This method executes the given string and adds the result to a stream. This
 *	is used by BigWorld.executeRawDatabaseCommand().
 *
 *	@param statement	The SQL string to execute.
 *	@param resdata If not NULL, the results from this query are put into this.
 */
void MySql::execute( const BW::string & statement, BinaryOStream * resdata )
{
	int result = this->realQuery( statement );

	if (result != 0)
	{
		this->throwError();
		return;
	}

	MYSQL_RES * pResult = mysql_store_result( sql_ );

	if (pResult)
	{
		if (resdata != NULL)
		{
			int nrows = mysql_num_rows( pResult );
			int nfields = mysql_num_fields( pResult );
			(*resdata) << nrows << nfields;
			MYSQL_ROW arow;

			while ((arow = mysql_fetch_row( pResult )) != NULL)
			{
				unsigned long *lengths = mysql_fetch_lengths( pResult );
				for (int i = 0; i < nfields; i++)
				{
					if (arow[i] == NULL)
					{
						(*resdata) << "NULL";
					}
					else
					{
						resdata->appendString(arow[i],lengths[i]);
					}
				}
			}
		}

		mysql_free_result( pResult );
	}
}


/**
 *	This method executes the given string, and stores the result in a raw MySQL
 *	result object. This version of execute should only be used in special cases,
 *	as it bypasses the error checking in result_set.hpp: RESULT_SET_GET_RESULT.
 *
 *	@param queryStr		The SQL string to execute
 *	@param pMySqlResult	The results from the query are put into a new object
 *						whose address is stored here.
 */
void MySql::execute( const BW::string & queryStr, MYSQL_RES *& pMySqlResult )
{
	int result = this->realQuery( queryStr );

	if (result != 0)
	{
		this->throwError();
		return;
	}

	pMySqlResult = mysql_store_result( sql_ );

	if (!pMySqlResult)
	{
		this->throwError();
	}
}


/**
 *	This method executes the given string.
 *
 *	@param queryStr	The SQL string to execute.
 *	@param pResultSet If not NULL, the results from this query are put into this
 *		object.
 *
 *	@note This method can throw DatabaseException exceptions.
 */
void MySql::execute( const BW::string & queryStr, ResultSet * pResultSet )
{
	int result = this->realQuery( queryStr );

	this->storeResultInto( pResultSet );

	if (result != 0)
	{
		const size_t MAX_SIZE = 1000000;

		if (queryStr.length() < MAX_SIZE)
		{
			WARNING_MSG( "MySql::execute: Query failed (%d) '%s'\n",
					result, queryStr.c_str() );
		}
		else
		{
			ERROR_MSG( "MySql::execute: Query failed (%d): "
						"Size of query string is %" PRIzu "\n",
					result, queryStr.length() );
		}
		this->throwError();
	}
}


/**
 *	This is the non-exception version of execute().
 */
int MySql::query( const BW::string & statement )
{
	int errorNum = this->realQuery( statement );

	DatabaseException e( sql_ );

	if (e.isLostConnection())
	{
		this->hasLostConnection( true );
	}

	return errorNum;
}


/**
 * 	This function returns the list of table names that matches the specified
 * 	pattern.
 */
void MySql::getTableNames( BW::vector<BW::string>& tableNames,
							const char * pattern )
{
	tableNames.clear();

	MYSQL_RES * pResult = mysql_list_tables( sql_, pattern );
	if (pResult)
	{
		tableNames.reserve( mysql_num_rows( pResult ) );

		MYSQL_ROW row;
		while ((row = mysql_fetch_row( pResult )) != NULL)
		{
			unsigned long *lengths = mysql_fetch_lengths( pResult );
			tableNames.push_back( BW::string( row[0], lengths[0] ) );
		}
		mysql_free_result( pResult );
	}
}


/**
 *	This method reads the next row from the given result set, and sets engine
 *	and supportState to the values found in the result.
 *
 *	@param pMySqlResult	the result set containing the results from "SHOW TABLE".
 *	@param engine		will be assigned the Engine field of the next result.
 *	@param supportState	will be assigned the SupportState field of the next
 *						result.
 *
 *	@return				true if a new row was found in the result set, false if
 *						fetch row fails to find another entry, if the result
 *						set has an invalid format, or if retrieving individual
 *						entries fails.
 */
bool getEngineSupportState(
		MYSQL_RES * pMySqlResult, BW::string& engine, BW::string& supportState )
{
	const int minFields = 3;
	int numFields = mysql_num_fields( pMySqlResult);

	if (numFields < minFields)
	{
		ERROR_MSG( "Invalid engines table format. Expecting at least %d "
				"fields. Got %d.\n", minFields, numFields );
		return false;
	}

	MYSQL_ROW row = mysql_fetch_row( pMySqlResult );

	if (!row)
	{
		return false;
	}

	unsigned long * lengths = mysql_fetch_lengths( pMySqlResult );

	return getValueFromString( row[0], lengths[0], engine ) &&
		getValueFromString( row[1], lengths[1], supportState );
}


/**
 *	This method queries the server for the support of the required storage
 *	engine.
 */
MySql::EngineSupportState MySql::checkEngineSupportState()
{	
	MySql::EngineSupportState targetSupportState = NOT_SUPPORTED;

	try
	{
		// A raw MySQL result object is used instead of a ResultSet because the
		// resulting table will have a variable number of fields depending on
		// the MySQL server version, rendering us unable to know which version
		// of getResult to call.
		MYSQL_RES * pMySqlResult;

		this->execute( "SHOW ENGINES", pMySqlResult );

		BW::string engine;
		BW::string supportState;

		while ( getEngineSupportState( pMySqlResult, engine, supportState ) )
		{
			if (engine == MYSQL_ENGINE_TYPE)
			{
				if (supportState == "DEFAULT")
				{
					targetSupportState = DEFAULT;
				}
				else if (supportState == "YES")
				{
					targetSupportState = SUPPORTED;
				}
				break;
			}
		}

		mysql_free_result( pMySqlResult );
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "MySql::checkEngineSupportState: An exception occured while "
				"querying MySQL support for the %s engine: %s",
				MYSQL_ENGINE_TYPE, e.what() );
	}

	return targetSupportState;
}


/**
 *	This method checks the storage engine type for each table in the database.
 *	If a table is found to have the wrong engine type, it is altered.
 */
bool MySql::checkTableEngines()
{
	char buffer[ 512 ];
	ResultSet resultSet;

	try
	{
		bw_snprintf( buffer, sizeof( buffer ),
				"SELECT table_name FROM information_schema.tables "
				"WHERE table_schema = '%s' AND engine != '%s'",
				connectInfo_.database.c_str(), MYSQL_ENGINE_TYPE );
		this->execute( buffer, &resultSet );
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "MySql::checkTableEngines(): Failed to get storage engine "
					"information for database %s\n",
				connectInfo_.database.c_str() );
	}

	BW::string tableName;
	bool tablesAreOK = true;

	while (resultSet.getResult( tableName ))
	{
		try
		{
			NOTICE_MSG( "MySql::checkTableEngines: Altering table '%s' to use "
						"the %s storage engine\n",
					tableName.c_str(), MYSQL_ENGINE_TYPE );

			bw_snprintf( buffer, sizeof( buffer ),
					"ALTER TABLE %s ENGINE = '%s'",
					tableName.c_str(), MYSQL_ENGINE_TYPE );
			this->execute( buffer );
		}
		catch (std::exception & e)
		{
			ERROR_MSG( "MySql::checkTableEngines: Unable to alter storage "
					"engine of table %s: %s\n", tableName.c_str(), e.what() );
			tablesAreOK = false;
		}
	}

	return tablesAreOK;
}


/**
 *	This method stores a result from a connection into a result set.
 */
void MySql::storeResultInto( ResultSet * pResultSet )
{
	MYSQL_RES * pMySqlResultSet = mysql_store_result( sql_ );

	if (pMySqlResultSet != NULL && pResultSet == NULL)
	{
		mysql_free_result( pMySqlResultSet );
	}

	if (pResultSet)
	{
		pResultSet->setResults( pMySqlResultSet );
	}
}


/**
 *	This method advances the result set to the next result set. This is used
 *	for statements that give multiple results, for example calling stored
 *	procedures.
 *
 *	@return true if there were more results, false otherwise.
 */
bool MySql::nextResult( ResultSet * pResultSet )
{
	int result = mysql_next_result( sql_ );

	if (result > 0)
	{
		this->throwError();
	}

	this->storeResultInto( pResultSet );

	return (result == 0);
}

BW_END_NAMESPACE

// mysql_wrapper.cpp
