#ifndef BWAUTH_BILLING_SYSTEM_HPP
#define BWAUTH_BILLING_SYSTEM_HPP

#include "db_storage/billing_system.hpp"


BW_BEGIN_NAMESPACE

class EntityDefs;


/**
 *	This class is used to integrate with BigWorld Authentication's RESTful API.
 */
class BWAuthBillingSystem : public BillingSystem
{
public:
	BWAuthBillingSystem( const EntityDefs & entityDefs );
	~BWAuthBillingSystem();

	virtual void getEntityKeyForAccount(
		const BW::string & username, const BW::string & password,
		const Mercury::Address & clientAddr,
		IGetEntityKeyForAccountHandler & handler );

	virtual void setEntityKeyForAccount( const BW::string & username,
		const BW::string & password, const EntityKey & ekey,
		ISetEntityKeyForAccountHandler & handler );

	virtual bool isOkay() const;

	class Impl;

private:
	Impl *pImpl_;
};

BW_END_NAMESPACE

#endif // BWAUTH_BILLING_SYSTEM_HPP
