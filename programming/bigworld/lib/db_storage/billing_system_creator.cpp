#include "billing_system_creator.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/bw_vector.hpp"

#include "server/bwconfig.hpp"
#include "server/server_app.hpp"

DECLARE_DEBUG_COMPONENT2( "Billing", 0 )

BW_BEGIN_NAMESPACE

namespace // (anonymous)
{

typedef BW::vector< BillingSystemCreator * > BillingSystems;

// This global is used by the IntrusiveObject of the BillingSystemCreators
BillingSystems * g_pBillingSystemCollection = NULL;

} // end namespace (anonymous)


// --------------------------------------------------------------------------
// Section: BillingSystemCreator
// --------------------------------------------------------------------------

/**
 *	Constructor.
 */
BillingSystemCreator::BillingSystemCreator() :
	IntrusiveObject< BillingSystemCreator >( g_pBillingSystemCollection )
{ }


/**
 *	This method creates a BillingSystem instance 
 */
/* static */ BillingSystem * BillingSystemCreator::createFromConfig(
	const EntityDefs & entityDefs, ServerApp & app )
{
	BW::string type = BWConfig::get( "billingSystem/type", "standard" );

	if (g_pBillingSystemCollection == NULL)
	{
		WARNING_MSG( "BillingSystemCreator::createFromConfig: "
			"No billing systems registered.\n" );
		return NULL;
	}

	BillingSystems::iterator iter = g_pBillingSystemCollection->begin();
	while (iter != g_pBillingSystemCollection->end())
	{
		if ((*iter)->typeName() == type)
		{
			BillingSystem * pBillingSystem = (*iter)->create( entityDefs, app );
			if (pBillingSystem)
			{
				if (!pBillingSystem->isOkay())
				{
					ERROR_MSG( "BillingSystemCreator::createFromConfig: "
						"Billing system is not configured correctly\n" );
					delete pBillingSystem;
					pBillingSystem = NULL;
				}
				else
				{
					INFO_MSG( "Using billing system: %s\n", type.c_str() );
				}
			}

			return pBillingSystem;
		}

		++iter;
	}

	return NULL;
}

BW_END_NAMESPACE

// billing_system_creator.cpp
