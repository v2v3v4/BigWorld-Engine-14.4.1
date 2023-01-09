#include "database_exception.hpp"

#include <mysql/mysqld_error.h>
#include <mysql/errmsg.h>


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: class DatabaseException
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
DatabaseException::DatabaseException( MYSQL * pConnection ) :
			errStr_( mysql_error( pConnection ) ),
			errNum_( mysql_errno( pConnection ) )
{
}


/**
 *	Destructor.
 */
DatabaseException::~DatabaseException() throw()
{
}


/**
 *	This method returns true if the query that caused this exception should be
 *	retried.
 */
bool DatabaseException::shouldRetry() const
{
	return (errNum_== ER_LOCK_DEADLOCK) ||
			(errNum_ == ER_LOCK_WAIT_TIMEOUT);
}


/**
 *	This method returns whether the exception is fatal. That is, whether the
 *	connection to the server has been lost.
 */
bool DatabaseException::isLostConnection() const
{
	return (errNum_ == CR_SERVER_GONE_ERROR) ||
			(errNum_ == CR_SERVER_LOST) ||
			(errNum_ == CR_CONNECTION_ERROR) ||
			(errNum_ == CR_CONN_HOST_ERROR);
}

BW_END_NAMESPACE

// database_exception.cpp
