#include "custom_billing_system.hpp"

#include "db_storage/billing_system_creator.hpp"
#include "db_storage/db_entitydefs.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Billing", 0 )


BW_BEGIN_NAMESPACE

// ---------------------------------------------------------------------------
// Section: CustomBillingSystemCreator
// ---------------------------------------------------------------------------

/**
 *
 */
class CustomBillingSystemCreator : public BillingSystemCreator
{
public:
	virtual BW::string typeName()
	{
		return "custom";
	}

	BillingSystem * create( const EntityDefs & entityDefs,
		ServerApp & app ) const
	{
		return new CustomBillingSystem( entityDefs );
	}
};

namespace // (anonymous)
{

CustomBillingSystemCreator staticInitialiser;

} // end namespace (anonymous)

//
// -----------------------------------------------------------------------------
// Section: CustomBillingSystem
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
CustomBillingSystem::CustomBillingSystem( const EntityDefs & entityDefs ) :
	BillingSystem( entityDefs ),
	entityTypeID_( entityDefs.getEntityType( "Account" ) )
{
}


/**
 *	Destructor.
 */
CustomBillingSystem::~CustomBillingSystem()
{
}


/**
 *	This method validates a login attempt and returns the details of the entity
 *	to use via the handler.
 */
void CustomBillingSystem::getEntityKeyForAccount(
	const BW::string & username, const BW::string & password,
	const Mercury::Address & clientAddr,
	IGetEntityKeyForAccountHandler & handler )
{
	// TODO: Look up the entity and call onGetEntityKeyForAccountSuccess.

	// As a fallback, allow the player to log in and create the default entity
	// type. Do not remember the entity since setEntityKeyForAccount is not yet
	// implemented by this class.
	// Since we didn't authenticate the password, give it to the base entity to
	// deal with.
	// This is equivalent to the "authenticateViaBaseEntity" behaviour of the
	// standard billing system.
	handler.onGetEntityKeyForAccountCreateNew( entityTypeID_,
		/* shouldRemember */ false,
		BW::string(), password );
}


/**
 *	This method is used to inform the billing system that a new entity has been
 *	associated with a login.
 */
void CustomBillingSystem::setEntityKeyForAccount( const BW::string & username,
	const BW::string & password, const EntityKey & ekey,
		ISetEntityKeyForAccountHandler & handler )
{
	// TODO: Code to store the new account in the billing system. This is
	// required if shouldRememberNewOnLoadFailure is set to true in
	// onGetEntityKeyForAccountCreateNew.
	handler.onSetEntityKeyForAccountSuccess( ekey );
}


/**
 *	This method returns whether the billing system has been set up correctly.
 */
bool CustomBillingSystem::isOkay() const
{
	return entityTypeID_ != INVALID_ENTITY_TYPE_ID;
}

BW_END_NAMESPACE

// custom_billing_system.cpp
