#include "script/first_include.hpp"

#include "dbapp.hpp"
#include "py_billing_system.hpp"

#include "db_storage/billing_system_creator.hpp"
#include "db_storage/billing_system.hpp"

#include "pyscript/personality.hpp"
#include "pyscript/script.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Billing", 0 )

BW_BEGIN_NAMESPACE

/*
 *	This is the standard billing system used by BigWorld when no alternate
 *	billing system has been implemented. The behaviour of this billing system
 *	is to use Python script to connect to a billing system, or else to fall
 *	back to the current DB type implemented in the db_storage_mysql or
 *	db_storage source via createBillingSystem()
 */

class EntityDefs;

class StandardBillingSystemCreator : public BillingSystemCreator
{
public:
	virtual BW::string typeName()
	{
		return "standard";
	}

	virtual BillingSystem * create( const EntityDefs & entityDefs,
		ServerApp & app ) const;
};


namespace // (anonymous)
{
StandardBillingSystemCreator staticInitialiser;
} // end namespace (anonymous)


BillingSystem * StandardBillingSystemCreator::create(
	const EntityDefs & entityDefs, ServerApp & app ) const
{
	BillingSystem * pBillingSystem = NULL;

	ScriptObject func;

	if (Personality::instance())
	{
		func = Personality::instance().getAttribute( "connectToBillingSystem",
			ScriptErrorClear() );
	}

	if (func)
	{
		ScriptObject billingSystem = func.callFunction( ScriptErrorPrint() );
		if (!billingSystem)
		{
			ERROR_MSG( "StandardBillingSystemCreator::create: "
					"Failed. See script errors\n" );
			return NULL;
		}

		if (!billingSystem.isNone())
		{
			pBillingSystem = new PyBillingSystem( billingSystem.get(), entityDefs );
		}
	}

	if (pBillingSystem == NULL)
	{
		pBillingSystem = DBApp::instance().getIDatabase().createBillingSystem();
	}

	return pBillingSystem;
}

BW_END_NAMESPACE

// standard_billing_system.cpp
