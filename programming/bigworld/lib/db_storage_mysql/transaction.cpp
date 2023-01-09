#include "transaction.hpp"

#include "database_exception.hpp"
#include "wrapper.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
MySqlTransaction::MySqlTransaction( MySql& sql ) :
	sql_( sql ),
	committed_( false )
{
	sql_.execute( "START TRANSACTION" );

	// Note: Important that this is last as execute can cause an exception
	sql_.inTransaction( true );
}


/**
 *	Destructor.
 */
MySqlTransaction::~MySqlTransaction()
{
	if (!committed_ && !sql_.hasLostConnection())
	{
		// Can't let exception escape from destructor otherwise terminate()
		// will be called.
		try
		{
			WARNING_MSG( "MySqlTransaction::~MySqlTransaction: "
					"Rolling back\n" );
			sql_.execute( "ROLLBACK" );
		}
		catch (DatabaseException & e)
		{
			if (e.isLostConnection())
			{
				sql_.hasLostConnection( true );
			}
		}
	}

	sql_.inTransaction( false );
}


/**
 *
 */
bool MySqlTransaction::shouldRetry() const
{
	return (sql_.getLastErrorNum() == ER_LOCK_DEADLOCK);
}


/**
 *
 */
void MySqlTransaction::commit()
{
	MF_ASSERT( !committed_ );
	sql_.execute( "COMMIT" );
	committed_ = true;
}

BW_END_NAMESPACE

// transaction.cpp
