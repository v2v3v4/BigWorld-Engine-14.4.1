#ifndef MYSQL_LOCKED_CONNECTION_HPP
#define MYSQL_LOCKED_CONNECTION_HPP

#include "db_config.hpp"
#include "named_lock.hpp"
#include "wrapper.hpp"

#include <mysql/mysql.h>


BW_BEGIN_NAMESPACE

/**
 * This class wraps a MySql connection and a NamedLock. It is intended to
 * be used by the top level of an application to ensure no other processes
 * are using the same database prior to operating on it.
 *
 * This class does not throw any exceptions.
 */
class MySqlLockedConnection
{
public:
	MySqlLockedConnection( const DBConfig::ConnectionInfo & connectionInfo );
	virtual ~MySqlLockedConnection();

	bool connect( bool shouldLock );
	bool connectAndLock() { return this->connect( true ); }
	bool connectAndLockWithRetry( uint8 numRetries );

	// TODO: This should be removed when the cleanup in mysql_database is done.
	bool reconnectTo( const DBConfig::ConnectionInfo & connectionInfo )
	{
		return pConnection_ && pConnection_->reconnectTo( connectionInfo );
	}

	bool lock();
	bool unlock();

	MySql * connection() { return pConnection_; }

protected:
	virtual MySql * createMysqlWrapper() const;
	DBConfig::ConnectionInfo connectionInfo_;

private:
	MySql * pConnection_;
	MySQL::NamedLock dbLock_;
};

BW_END_NAMESPACE

#endif // MYSQL_LOCKED_CONNECTION_HPP
