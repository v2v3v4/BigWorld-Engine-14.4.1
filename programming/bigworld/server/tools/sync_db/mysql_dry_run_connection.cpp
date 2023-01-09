#include "mysql_dry_run_connection.hpp"

#include "mysql_dry_run_wrapper.hpp"

BW_BEGIN_NAMESPACE

/**
 * Constructor.
 */
MySqlDryRunConnection::MySqlDryRunConnection(
		const DBConfig::ConnectionInfo & connectionInfo ) :
	MySqlLockedConnection( connectionInfo )
{
}

MySql * MySqlDryRunConnection::createMysqlWrapper() const
{
	return new MySqlDryRunWrapper( connectionInfo_ );
}

BW_END_NAMESPACE

// mysql_dry_run_connection.cpp
