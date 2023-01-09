#include "mysql_dry_run_wrapper.hpp"

#include "db_storage_mysql/connection_info.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Returns true if the given statement only does read only SELECT query.
 */
bool MySqlDryRunWrapper::isReadOnlyStatement(
	const BW::string & statement ) const
{
	BW::string::size_type pos = statement.find_first_not_of( " \t" );

	if (pos == BW::string::npos)
	{
		ERROR_MSG( "MySqlDryRunWrapper::isReadOnlyStatement: "
			"Invalid SQL statement: '%s'\n", statement.c_str() );

		return false;
	}

	BW::string str;
	str.resize( statement.size() - pos );
	std::transform( statement.begin() + pos, statement.end(), str.begin(),
		::toupper );

	if (str.find( "SHOW" ) == 0)
	{
		return true;
	}

	if (str.find( "SELECT" ) == 0)
	{
		// MySql supports "SELECT ... FOR UPDATE | LOCK IN SHARE MODE",
		// this kind of statements should not be considered as read only.
		return str.find( "FOR UPDATE" ) == BW::string::npos &&
			str.find( "LOCK IN SHARE MODE" ) == BW::string::npos;
	}

	return false;
}


/**
 *	This method executes the given query and adds the result to a stream.
 *
 *	@param statement	The SQL string to execute.
 *	@param resdata If not NULL, the results from this query are put into this.
 */
void MySqlDryRunWrapper::execute( const BW::string & statement,
	BinaryOStream * resdata )
{
	if (!this->isReadOnlyStatement( statement ))
	{
		INFO_MSG( "DRYRUN:\t%s;\n", statement.c_str() );
		return;
	}

	this->MySql::execute( statement, resdata );
}


/**
 *	This method executes the given query, and stores the result in a raw MySQL
 *	result object. This version of execute should only be used in special cases,
 *	as it bypasses the error checking in result_set.hpp: RESULT_SET_GET_RESULT.
 *
 *	@param queryStr		The SQL string to execute
 *	@param pMySqlResult	The results from the query are put into a new object
 *						whose address is stored here.
 */
void MySqlDryRunWrapper::execute( const BW::string & queryStr,
	MYSQL_RES *& pMySqlResult )
{
	if (!this->isReadOnlyStatement( queryStr ))
	{
		INFO_MSG( "DRYRUN:\t%s;\n", queryStr.c_str() );
		return;
	}

	this->MySql::execute( queryStr, pMySqlResult );
}


/**
 *	This method executes the given query.
 *
 *	@param queryStr	The SQL string to execute.
 *	@param pResultSet If not NULL, the results from this query are put into this
 *		object.
 *
 *	@note This method can throw DatabaseException exceptions.
 */
void MySqlDryRunWrapper::execute( const BW::string & queryStr,
	ResultSet * pResultSet )
{
	if (!this->isReadOnlyStatement( queryStr ))
	{
		INFO_MSG( "DRYRUN:\t%s;\n", queryStr.c_str() );
		return;
	}

	this->MySql::execute( queryStr, pResultSet );
}


/**
 *	This is the non-exception version of execute().
 */
int MySqlDryRunWrapper::query( const BW::string & statement )
{
	if (!this->isReadOnlyStatement( statement ))
	{
		INFO_MSG( "DRYRUN:\t%s;\n", statement.c_str() );
		return 0;
	}

	return this->MySql::query( statement );
}


BW_END_NAMESPACE

// mysql_dry_run_wrapper.cpp
