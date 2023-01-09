#include "mysql_billing_system.hpp"

#include "tasks/get_entity_key_for_account_task.hpp"
#include "tasks/set_entity_key_for_account_task.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
MySqlBillingSystem::MySqlBillingSystem( TaskManager & bgTaskManager,
	   const EntityDefs & entityDefs ) :
	BillingSystem( entityDefs ),
	bgTaskManager_( bgTaskManager )
{
}


/**
 *	BillingSystem override
 */
void MySqlBillingSystem::getEntityKeyForAccount( const BW::string & logOnName,
		const BW::string & password,
		const Mercury::Address & clientAddr,
		IGetEntityKeyForAccountHandler & handler )
{
	if (authenticateViaBaseEntity_)
	{
		handler.onGetEntityKeyForAccountLoadFromUsername(
			entityTypeIDForUnknownUsers_, logOnName,
			/* shouldCreateNewOnLoadFailure */ true,
			/* shouldRememberNewOnLoadFailure */ false,
			BW::string(), password );
	}
	else
	{
		bgTaskManager_.addBackgroundTask(
			new GetEntityKeyForAccountTask( logOnName, password,
				shouldAcceptUnknownUsers_,
				shouldRememberUnknownUsers_,
				entityTypeIDForUnknownUsers_,
				handler ) );
	}
}


/**
 *	BillingSystem override
 */
void MySqlBillingSystem::setEntityKeyForAccount( const BW::string & username,
	const BW::string & password, const EntityKey & ekey,
		ISetEntityKeyForAccountHandler & handler )
{
	bgTaskManager_.addBackgroundTask(
		new SetEntityKeyForAccountTask( username, password, ekey, handler ) );
}

BW_END_NAMESPACE

// mysql_billing_system.cpp
