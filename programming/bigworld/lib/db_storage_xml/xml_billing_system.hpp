#ifndef XML_BILLING_SYSTEM_HPP
#define XML_BILLING_SYSTEM_HPP

#include "db_storage/billing_system.hpp"
#include "resmgr/datasection.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class EntityDefs;
class XMLDatabase;

/**
 *	This class implements the XML database functionality.
 */
class XMLBillingSystem : public BillingSystem
{
public:
	XMLBillingSystem( DataSectionPtr pLogonMapSection,
			const EntityDefs & entityDefs,
			XMLDatabase * pDatabase );

	virtual void getEntityKeyForAccount(
		const BW::string & logOnName, const BW::string & password,
		const Mercury::Address & clientAddr,
		IGetEntityKeyForAccountHandler & handler );
	virtual void setEntityKeyForAccount( const BW::string & username,
		const BW::string & password, const EntityKey & ekey,
		ISetEntityKeyForAccountHandler & handler );

private:
	void initLogOnMapping( XMLDatabase * pDatabase );

	// Equivalent of bigworldLogOnMapping table in MySQL.
	class LogOnMapping
	{
	public:
		LogOnMapping( const EntityKey & key, const BW::string & password ) :
			entityKey_( key ),
			password_( password )
		{}
		LogOnMapping() : entityKey_( 0, 0 ), password_( "" ) {}

		bool matchesPassword( const BW::string & password ) const 
		{
			return password == password_;
		}

		const EntityKey & entityKey() const	{ return entityKey_; }

	private:
		EntityKey		entityKey_;
		BW::string		password_;
	};

	// The key is the "logOnName" column.
	typedef BW::map< BW::string, LogOnMapping > LogonMap;
	LogonMap		logonMap_;
	DataSectionPtr	pLogonMapSection_;

	const EntityDefs & entityDefs_;
};

BW_END_NAMESPACE

#endif // XML_BILLING_SYSTEM_HPP
