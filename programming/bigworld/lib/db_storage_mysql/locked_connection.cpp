#include "locked_connection.hpp"

#include "wrapper.hpp"

#include "cstdmf/timestamp.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )


BW_BEGIN_NAMESPACE

/**
 * Constructor.
 */
MySqlLockedConnection::MySqlLockedConnection(
		const DBConfig::ConnectionInfo & connectionInfo ) :
	connectionInfo_( connectionInfo ),
	pConnection_( NULL ),
	dbLock_( connectionInfo.generateLockName() )
{
}


/**
 * Destructor.
 */
MySqlLockedConnection::~MySqlLockedConnection()
{
	if (pConnection_)
	{
		if (dbLock_.isLocked())
		{
			dbLock_.unlock();
		}

		pConnection_->close();
		delete pConnection_;
	}
}


/**
 *	Attempt to connect and lock the connection until the timeout expires.
 *
 *	@param numRetries  The number of retries.
 *
 *	@returns true if a locked connection was established, false otherwise.
 */
bool MySqlLockedConnection::connectAndLockWithRetry( uint8 numRetries )
{
	uint8 retry = 0;

	while (!this->connectAndLock())
	{
		if (retry >= numRetries)
		{
			return false;
		}

		++retry;
		sleep( 1 );

		INFO_MSG( "MySqlLockedConnection::connectAndLockWithRetry: "
					"Retry %d/%d.\n", retry, numRetries );
	}

	return true;
}


/**
 * Create a new MySql connection.
 * 
 * @return new created connection instance.
 */
MySql * MySqlLockedConnection::createMysqlWrapper() const
{
	return new MySql( connectionInfo_ );
}


/**
 * This method establishes a connection to the MySql server and acquires the
 * specified named lock.
 *
 * @returns true if both a connection and lock are established, false
 * 			otherwise.
 */
bool MySqlLockedConnection::connect( bool shouldLock )
{
	if (pConnection_)
	{
		ERROR_MSG( "MySqlLockedConnection::connect: "
			"A connection has already been established.\n" );
		return false;
	}

	bool isConnectedAndLocked = true;
	try
	{
		pConnection_ = this->createMysqlWrapper();

		if (shouldLock)
		{
			isConnectedAndLocked = this->lock();
			if (!isConnectedAndLocked)
			{
				WARNING_MSG( "MySqlLockedConnection::connect: "
						"Unable to obtain a named lock on MySql database "
						"%s:%d (%s). It may be in use by another process.\n",
					connectionInfo_.host.c_str(),
					connectionInfo_.port,
					connectionInfo_.database.c_str() );
				delete pConnection_;
				pConnection_ = NULL;
			}
		}
	}
	catch (std::exception & e)
	{
		ERROR_MSG( "MySqlLockedConnection::connect: "
				"Unable to connect to MySql server %s:%d (%s): %s\n",
			connectionInfo_.host.c_str(),
			connectionInfo_.port,
			connectionInfo_.database.c_str(),
			e.what() );
		isConnectedAndLocked = false;
		delete pConnection_;
		pConnection_ = NULL;
	}

	return isConnectedAndLocked;
}


/**
 * This method attempts to locks the connection if it is not already locked.
 *
 * @returns true if the connection was able to be locked, false otherwise.
 */
bool MySqlLockedConnection::lock()
{
	return pConnection_ && dbLock_.lock( *pConnection_ );
}


/**
 * This method attempts to locks the connection if it is not already locked.
 *
 * @returns true if the connection was able to be unlocked, false otherwise.
 */
bool MySqlLockedConnection::unlock()
{
	return pConnection_ && dbLock_.unlock();
}

BW_END_NAMESPACE

// locked_connection.cpp
