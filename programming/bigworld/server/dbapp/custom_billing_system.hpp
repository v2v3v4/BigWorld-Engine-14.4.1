#ifndef CUSTOM_BILLING_SYSTEM_HPP
#define CUSTOM_BILLING_SYSTEM_HPP

#include "db_storage/billing_system.hpp"


BW_BEGIN_NAMESPACE

class EntityDefs;


/**
 *	This class is used to integrate with a billing system via Python.
 */
class CustomBillingSystem : public BillingSystem
{
public:
	CustomBillingSystem( const EntityDefs & entityDefs );
	~CustomBillingSystem();

	virtual void getEntityKeyForAccount(
		const BW::string & username, const BW::string & password,
		const Mercury::Address & clientAddr,
		IGetEntityKeyForAccountHandler & handler );

	virtual void setEntityKeyForAccount( const BW::string & username,
		const BW::string & password, const EntityKey & ekey,
		ISetEntityKeyForAccountHandler & handler );

	virtual bool isOkay() const;

private:
	EntityTypeID entityTypeID_;
};

BW_END_NAMESPACE

#endif // CUSTOM_BILLING_SYSTEM_HPP
