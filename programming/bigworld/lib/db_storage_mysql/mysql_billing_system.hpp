#ifndef MYSQL_BILLING_SYSTEM_HPP
#define MYSQL_BILLING_SYSTEM_HPP

#include "cstdmf/bgtask_manager.hpp"
#include "db_storage/billing_system.hpp"


BW_BEGIN_NAMESPACE

class EntityDefs;

/**
 *	This class implements the default billing system integration when the
 *	MySQL database is used.
 */
class MySqlBillingSystem : public BillingSystem
{
public:
	MySqlBillingSystem( TaskManager & bgTaskManager,
			const EntityDefs & entityDefs );

	virtual void getEntityKeyForAccount(
		const BW::string & logOnName, const BW::string & password,
		const Mercury::Address & clientAddr,
		IGetEntityKeyForAccountHandler & handler );
	virtual void setEntityKeyForAccount( const BW::string & username,
		const BW::string & password, const EntityKey & ekey,
		ISetEntityKeyForAccountHandler & handler );

private:
	TaskManager & bgTaskManager_;
};

BW_END_NAMESPACE

#endif // MYSQL_BILLING_SYSTEM_HPP
