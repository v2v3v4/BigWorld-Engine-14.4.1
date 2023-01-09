#ifndef BILLING_SYSTEM_CREATOR
#define BILLING_SYSTEM_CREATOR

#include "billing_system.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/intrusive_object.hpp"


BW_BEGIN_NAMESPACE

class ServerApp;

/**
 *	This class is the interface that should be implemented in order for
 *	BillingSystems to register with the BillingSystemCollection.
 *
 *	The static method is used by DBApp in order to create an instance of a
 *	registered BillingSystem based on the bw.xml billingSystem/type option.
 */
class BillingSystemCreator : public IntrusiveObject< BillingSystemCreator >
{
public:
	BillingSystemCreator();

	/**
	 *	This abstract method is invoked to determine the type name of a
	 *	concrete BillingSystemCreator.
	 */
	virtual BW::string typeName() = 0;

	/**
	 *	This abstract method is invoked to create a concrete billing system
	 *	instance.
	 */
	virtual BillingSystem * create( const EntityDefs & entityDefs,
		ServerApp & app ) const = 0;

	static BillingSystem * createFromConfig( const EntityDefs & entityDefs, 
		ServerApp & app );
};

BW_END_NAMESPACE

#endif // BILLING_SYSTEM_CREATOR
