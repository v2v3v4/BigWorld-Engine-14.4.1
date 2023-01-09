#ifndef MYSQL_QUERY_RUNNER_HPP
#define MYSQL_QUERY_RUNNER_HPP

#include "cstdmf/bw_namespace.hpp"

//#include "query.hpp"
#include "string_conv.hpp"

#include <string.h>

#include <sstream>


BW_BEGIN_NAMESPACE

class MySql;
class Query;
class ResultSet;


/**
 *
 */
class QueryRunner
{
public:
	QueryRunner( MySql & conn, const Query & query );

	template <class ARG>
	void pushArg( const ARG & arg );

	void pushArg( const char * arg, int length );

	void pushArg( const BW::string & arg )
	{
		this->pushArg( arg.c_str(), arg.size() );
	}

	void pushArg( const char * arg )
	{
		this->pushArg( arg, strlen( arg ) );
	}

	void execute( ResultSet * pResults ) const;

	MySql & connection() const		{ return conn_; }

private:
	const Query & query_;
	BW::ostringstream stream_;
	int numArgsAdded_;
	MySql & conn_;
};


template <class ARG>
void QueryRunner::pushArg( const ARG & arg )
{
	StringConv::addToStream( stream_, arg );
	stream_ << query_.getPart( ++numArgsAdded_ );
}

BW_END_NAMESPACE

#endif // MYSQL_QUERY_RUNNER_HPP
