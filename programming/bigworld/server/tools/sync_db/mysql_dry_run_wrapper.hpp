#ifndef MYSQL_DRY_RUN_WRAPPER_HPP
#define MYSQL_DRY_RUN_WRAPPER_HPP

#include "db_storage_mysql/wrapper.hpp"

BW_BEGIN_NAMESPACE

// Represents a MySQL server connection
class MySqlDryRunWrapper : public MySql
{
public:
	MySqlDryRunWrapper( const DBConfig::ConnectionInfo & connectInfo )
			: MySql( connectInfo ) {}
	~MySqlDryRunWrapper() {}

	// Overrides from MySql
	void execute( const BW::string & statement,
		BinaryOStream * pResData = NULL );
	void execute( const BW::string & queryStr, MYSQL_RES *& pMySqlResult );
	void execute( const BW::string & queryStr, ResultSet * pResultSet );
	int query( const BW::string & statement );

private:
	bool isReadOnlyStatement( const BW::string & statement ) const;
};

BW_END_NAMESPACE

#endif // MYSQL_WRAPPER_HPP
