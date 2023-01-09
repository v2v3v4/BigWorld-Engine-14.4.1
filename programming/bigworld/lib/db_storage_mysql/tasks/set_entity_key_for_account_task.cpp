#include "set_entity_key_for_account_task.hpp"

#include "../database_exception.hpp"
#include "../query.hpp"

#include "db_storage/billing_system.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: SetEntityKeyForAccountTask
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
SetEntityKeyForAccountTask::SetEntityKeyForAccountTask( 
			const BW::string & username,
			const BW::string & password,
			const EntityKey & ekey,
			ISetEntityKeyForAccountHandler & handler ) :
		MySqlBackgroundTask( "SetEntityKeyForAccountTask" ),
		username_( username ),
		password_( password ),
		entityKey_( ekey ),
		handler_( handler )
{
}


/**
 *	This method writes the log on mapping into the table.
 */
void SetEntityKeyForAccountTask::performBackgroundTask( MySql & conn )
{
	static const Query query(
		"REPLACE INTO bigworldLogOnMapping "
				"(logOnName, password, entityType, entityID) "
			"VALUES (?, "
				"IF( (SELECT isPasswordHashed FROM bigworldInfo), "
						"MD5( CONCAT( ?, logOnName ) ), "
						"? ), "
				"(SELECT typeID FROM bigworldEntityTypes WHERE bigworldID=?), "
				"?)" );

	query.execute( conn,
		username_, password_, password_, entityKey_.typeID, entityKey_.dbID,
		/* pResultSet:*/ NULL );
}


/**
 *	This method is called in the main thread after run() is complete.
 */
void SetEntityKeyForAccountTask::performMainThreadTask( bool succeeded )
{
	handler_.onSetEntityKeyForAccountSuccess( entityKey_ );
}


/**
 *	Override from MySqlBackgroundTask.
 */
void SetEntityKeyForAccountTask::onException( const DatabaseException & e )
{
	ERROR_MSG( "SetEntityKeyForAccountTask::onException: "
			"Could not set entity key for account: %s\n", e.what() );

	handler_.onSetEntityKeyForAccountFailure( entityKey_ );
}

BW_END_NAMESPACE

// set_entity_key_for_account.cpp
