#ifndef MYSQL_DRY_RUN_CONNECTION_HPP
#define MYSQL_DRY_RUN_CONNECTION_HPP

#include "db_storage_mysql/locked_connection.hpp"


BW_BEGIN_NAMESPACE

/**
 * This class wraps a customized MySql connection which only allow read only
 * queries to be executed on remote MySql database server, all other SQL
 * statements are just outputed to console.
 *
 * This class does not throw any exceptions.
 */
class MySqlDryRunConnection : public MySqlLockedConnection
{
public:
	MySqlDryRunConnection( const DBConfig::ConnectionInfo & connectionInfo);
	~MySqlDryRunConnection() {}

private:
	MySql * createMysqlWrapper() const; /* override */
};

BW_END_NAMESPACE

#endif // MYSQL_DRY_RUN_CONNECTION_HPP
