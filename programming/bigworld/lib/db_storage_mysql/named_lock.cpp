#include "named_lock.hpp"

#include "query.hpp"
#include "result_set.hpp"
#include "wrapper.hpp"

#include "cstdmf/debug.hpp"

#include <sstream>

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

namespace MySQL
{

bool obtainNamedLock( MySql & connection, const BW::string & lockName );
bool releaseNamedLock( MySql & connection, const BW::string & lockName );


// -----------------------------------------------------------------------------
// Section: Functions
// -----------------------------------------------------------------------------

/**
 * 	This function obtains a named lock. Returns true if it was successful,
 * 	false if it failed i.e. lock already held by someone else.
 */
bool obtainNamedLock( MySql & connection, const BW::string & lockName )
{
	const Query query( "SELECT GET_LOCK( ?, 0 )" );

	bool wasLockObtained = true;

	// We don't want to ever throw an exception back to the application
	// when we are locking.
	try
	{
		ResultSet resultSet;
		query.execute( connection, lockName, &resultSet );

		int result = 0;
		resultSet.getResult( result );

		wasLockObtained = result;
	}
	catch (...)
	{
		wasLockObtained = false;
	}


	return wasLockObtained;
}


/**
 * 	This function releases a named lock (acquired by obtainNamedLock()).
 */
bool releaseNamedLock( MySql & connection, const BW::string & lockName )
{
	bool wasLockReleased = true;
	try
	{
		const Query query( "SELECT RELEASE_LOCK( ? )" );
		query.execute( connection, lockName, NULL );
	}
	catch (...)
	{
		wasLockReleased = false;
	}

	return wasLockReleased;
}


// -----------------------------------------------------------------------------
// Section: NamedLock
// -----------------------------------------------------------------------------
/**
 *	Constructor
 *
 *	@param lockName		The name of the lock to hold.
 */ 		
NamedLock::NamedLock( const BW::string & lockName ) :
	pConnection_( NULL ),
	lockName_( lockName )
{
}


/**
 *	Destructor.
 */
NamedLock::~NamedLock()
{
	this->unlock();
}


/**
 *	This method acquires the named lock.
 *
 *	@returns true if the lock was successfully acquired.
 */
bool NamedLock::lock( MySql & connection )
{
	if (!this->isLocked() &&
			obtainNamedLock( connection, lockName_ ))
	{
		pConnection_ = &connection;
		return true;
	}
	return false;
}


/**
 *	This releases the named lock held by the lock's connection.
 *
 *	@returns true if the lock was previously held and is now released, false otherwise.
 */ 
bool NamedLock::unlock()
{
	if (pConnection_)
	{
		if (!pConnection_->hasLostConnection() &&
			!releaseNamedLock( *pConnection_, lockName_ ))
		{
			WARNING_MSG( "NamedLock::unlock: "
				"Was unable to successfully unlock. "
				"Forgetting connection.\n" );
		}

		pConnection_ = NULL;
		return true;
	}

	return false;
}

}	// namespace MySQL

BW_END_NAMESPACE

// named_lock.cpp
