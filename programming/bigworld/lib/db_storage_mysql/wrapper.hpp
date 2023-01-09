#ifndef MYSQL_WRAPPER_HPP
#define MYSQL_WRAPPER_HPP

#include "connection_info.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"

#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"

#include <string.h>
#include <stdexcept>


BW_BEGIN_NAMESPACE

// Forward declarations.
class BinaryOStream;
class BinaryIStream;
class ResultSet;

namespace DBConfig
{
	class ConnectionInfo;
}

// Constants
#define MYSQL_ENGINE_TYPE "InnoDB"

time_t convertMySqlTimeToEpoch(  const MYSQL_TIME& mysqlTime );





// represents a MySQL server connection
class MySql
{
public:
	enum EngineSupportState
	{
		DEFAULT,
		SUPPORTED,
		NOT_SUPPORTED
	};


	MySql( const DBConfig::ConnectionInfo & connectInfo );
	virtual ~MySql();

	MYSQL * get() { return sql_; }

	virtual void execute( const BW::string & statement,
		BinaryOStream * pResData = NULL );
	virtual void execute( const BW::string & queryStr, MYSQL_RES *& pMySqlResult );
	virtual void execute( const BW::string & queryStr, ResultSet * pResultSet );
	virtual int query( const BW::string & statement );

	void close();
	bool reconnectTo( const DBConfig::ConnectionInfo & connectInfo );
	bool reconnect()
	{
		return this->reconnectTo( connectInfo_ );
	}

	bool ping()					{ return mysql_ping( sql_ ) == 0; }
	void getTableNames( BW::vector< BW::string > & tableNames,
						const char * pattern );
	EngineSupportState checkEngineSupportState();
	bool checkTableEngines();
	my_ulonglong insertID()		{ return mysql_insert_id( sql_ ); }
	my_ulonglong affectedRows()	{ return mysql_affected_rows( sql_ ); }
	const char* info()			{ return mysql_info( sql_ ); }
	const char* getLastError()	{ return mysql_error( sql_ ); }
	unsigned int getLastErrorNum() { return mysql_errno( sql_ ); }
	bool nextResult( ResultSet * pResultSet );
	uint fieldCount() 			{ return mysql_field_count( sql_ ); }

	void inTransaction( bool value )
	{
		MF_ASSERT( inTransaction_ != value );
		inTransaction_ = value;
	}

	bool hasLostConnection() const		{ return hasLostConnection_; }
	void hasLostConnection( bool v )	{ hasLostConnection_ = v; }

	static uint charsetWidth( unsigned int charsetnr );

private:
	int realQuery( const BW::string & query );
	void storeResultInto( ResultSet * pResultSet );
	void throwError();

	void connect( const DBConfig::ConnectionInfo & connectInfo );

	void queryCharsetLengths();
	typedef BW::map<uint, uint> CollationLengths;

	MySql( const MySql& );
	void operator=( const MySql& );

	MYSQL * sql_;
	bool inTransaction_;
	bool hasLostConnection_;

	DBConfig::ConnectionInfo connectInfo_;

	static uint32 s_connect_timeout_;
	static bool s_hasInitedCollationLengths_;
	static CollationLengths s_collationLengths_;
	static bool s_hasMaxAllowedPacket_;
	static uint32 s_maxAllowedPacket_;
	static uint32 s_maxAllowedPacketWarnLevel_;
};

BW_END_NAMESPACE

#endif // MYSQL_WRAPPER_HPP
