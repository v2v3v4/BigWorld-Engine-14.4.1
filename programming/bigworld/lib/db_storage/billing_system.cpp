#include "script/first_include.hpp"

#include "billing_system.hpp"

#include "db_entitydefs.hpp"

#include "cstdmf/watcher.hpp"
#include "server/bwconfig.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
BillingSystem::BillingSystem( const EntityDefs & entityDefs ) :
	shouldAcceptUnknownUsers_(
		BWConfig::get( "billingSystem/shouldAcceptUnknownUsers", false ) ),
	shouldRememberUnknownUsers_(
		BWConfig::get( "billingSystem/shouldRememberUnknownUsers", false ) ),
	authenticateViaBaseEntity_(
			BWConfig::get( "billingSystem/authenticateViaBaseEntity", false ) ),
	entityTypeIDForUnknownUsers_( INVALID_ENTITY_TYPE_ID ),
	entityTypeForUnknownUsers_(
			BWConfig::get( "billingSystem/entityTypeForUnknownUsers", "" ) )
{
	INFO_MSG( "\tshouldAcceptUnknownUsers = %s\n",
							shouldAcceptUnknownUsers_ ? "True" : "False" );
	INFO_MSG( "\tshouldRememberUnknownUsers = %s\n",
							shouldRememberUnknownUsers_ ? "True" : "False" );
	INFO_MSG( "\tauthenticateViaBaseEntity = %s\n",
							authenticateViaBaseEntity_ ? "True" : "False" );
	INFO_MSG( "\tentityTypeForUnknownUsers = %s\n",
							entityTypeForUnknownUsers_.c_str() );

	entityTypeIDForUnknownUsers_ =
			entityDefs.getEntityType( entityTypeForUnknownUsers_ );

	if (shouldAcceptUnknownUsers_ || authenticateViaBaseEntity_)
	{
		if (entityTypeIDForUnknownUsers_ == INVALID_ENTITY_TYPE_ID)
		{
			ERROR_MSG( "BillingSystem::BillingSystem: "
						"Invalid entity type for unknown users = '%s'\n",
					entityTypeForUnknownUsers_.c_str() );
		}
	}
}

BW_END_NAMESPACE

// billing_system.cpp
