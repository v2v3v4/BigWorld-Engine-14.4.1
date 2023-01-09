//#include "query_runner.hpp"
// query.hpp includes query_runner.hpp in the middle to avoid
// incomplete type usage between the methods of Query and QueryRunner.
#include "query.hpp"

#include "wrapper.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: QueryRunner
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
QueryRunner::QueryRunner( MySql & conn, const Query & query ) :
	query_( query ),
	stream_(),
	numArgsAdded_( 0 ),
	conn_( conn )
{
	stream_ << query.getPart( 0 );
}


/**
 *	This method executes the query.
 *
 *	@param pResults If not NULL, the results from the query are placed into this
 *		object.
 *
 *	@note This method will raise a DatabaseException if the query fails.
 */
void QueryRunner::execute( ResultSet * pResults ) const
{
	MF_ASSERT( numArgsAdded_ == query_.numArgs() );

	BW::string queryStr = stream_.str();
	// DEBUG_MSG( "QueryRunner::execute: '%s'\n", queryStr.c_str() );

	conn_.execute( queryStr, pResults );
}


/**
 *
 */
void QueryRunner::pushArg( const char * arg, int length )
{
	if (length < 1024)
	{
		char buffer[ 2048 ];
		mysql_real_escape_string( conn_.get(), buffer, arg, length );
		stream_ << '\'' << buffer << '\'';
	}
	else
	{
		char * buffer = new char[ 1 + 2*length ];
		mysql_real_escape_string( conn_.get(), buffer, arg, length );
		stream_ << '\'' << buffer << '\'';
		delete [] buffer;
	}

	stream_ << query_.getPart( ++numArgsAdded_ );
}

BW_END_NAMESPACE

// query_runner.cpp
