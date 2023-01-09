#include "get_entity_key_for_account_task.hpp"

#include "../query.hpp"
#include "../result_set.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: class GetEntityKeyForAccountTask
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
GetEntityKeyForAccountTask::GetEntityKeyForAccountTask(
		const BW::string & logOnName,
		const BW::string & password,
		bool shouldAcceptUnknownUsers,
		bool shouldRememberUnknownUsers,
		EntityTypeID entityTypeIDForUnknownUsers,
		IGetEntityKeyForAccountHandler & handler ) :
	MySqlBackgroundTask( "GetEntityKeyForAccountTask" ),
	logOnName_( logOnName ),
	password_( password ),
	loginStatus_( LogOnStatus::LOGGED_ON ),
	entityTypeID_( entityTypeIDForUnknownUsers ),
	defaultEntityTypeID_( entityTypeIDForUnknownUsers ),
	databaseID_( 0 ),
	shouldAcceptUnknownUsers_( shouldAcceptUnknownUsers ),
	shouldRememberUnknownUsers_( shouldRememberUnknownUsers ),
	handler_( handler )
{
}


/**
 *	This method writes the new default entity into the database.
 *	May be run in another thread.
 */
void GetEntityKeyForAccountTask::performBackgroundTask( MySql & conn )
{
	static const Query query(
		"SELECT (i.isPasswordHashed AND "
						" m.password = MD5( CONCAT( ?, m.logOnName ) )) OR "
					"(NOT i.isPasswordHashed AND m.password = ?), "
				"t.bigworldID, m.entityID "
			"FROM bigworldLogOnMapping m "
			"LEFT JOIN bigworldEntityTypes t ON m.entityType = t.typeID "
			"JOIN bigworldInfo i "
			"WHERE m.logOnName = ?" );

	ResultSet results;
	query.execute( conn, password_, password_, logOnName_, &results );

	bool isOkay = true;

	if (!results.getResult( isOkay, entityTypeID_, databaseID_ ))
	{
		loginStatus_ = LogOnStatus::LOGIN_REJECTED_NO_SUCH_USER;
	}
	else if (!isOkay)
	{
		loginStatus_ = LogOnStatus::LOGIN_REJECTED_INVALID_PASSWORD;
	}
	else
	{
		loginStatus_ = LogOnStatus::LOGGED_ON;
	}
}


/*
 *	This method overrides the virtual method to handle the exception case.
 */
void GetEntityKeyForAccountTask::onException( const DatabaseException & e )
{
	loginStatus_ = LogOnStatus::LOGIN_REJECTED_DB_GENERAL_FAILURE;
}


/**
 *	This method is called in the main thread after run() is complete.
 */
void GetEntityKeyForAccountTask::performMainThreadTask( bool succeeded )
{
	const EntityTypeID * pEntityTypeID = entityTypeID_.get();

	if (pEntityTypeID == NULL)
	{
		pEntityTypeID = &defaultEntityTypeID_;
	}

	const DatabaseID * pDBID = databaseID_.get();

	if (loginStatus_ == LogOnStatus::LOGGED_ON)
	{
		if (pDBID)
		{
			handler_.onGetEntityKeyForAccountSuccess(
					EntityKey( *pEntityTypeID, *pDBID ) );
		}
		else
		{
			handler_.onGetEntityKeyForAccountCreateNew( *pEntityTypeID, true );
		}
	}
	else if ((loginStatus_ == LogOnStatus::LOGIN_REJECTED_NO_SUCH_USER) &&
		shouldAcceptUnknownUsers_)
	{
		handler_.onGetEntityKeyForAccountCreateNew( *pEntityTypeID,
				shouldRememberUnknownUsers_ );
	}
	else
	{
		handler_.onGetEntityKeyForAccountFailure( loginStatus_ );
	}
}

BW_END_NAMESPACE

// get_entity_key_for_account.cpp
