#ifndef BILLING_SYSTEM_HPP
#define BILLING_SYSTEM_HPP

#include "entity_key.hpp"

#include "connection/log_on_status.hpp"


BW_BEGIN_NAMESPACE

class EntityDefs;

/**
 *	This is the callback interface used by getEntityKeyForAccount(). One of its
 *	virtual methods is called when 
 */
class IGetEntityKeyForAccountHandler
{
public:
	/**
	 *	This method is called when getEntityKeyForAccount() completes
	 *	successfully.
	 *
	 *	@param ekey			The entity key for the account.
	 *	@param dataForClient	(optional) A string to send to the client
	 *	@param dataForBaseEntity	(optional) A string to provide to the entity
	 */
	virtual void onGetEntityKeyForAccountSuccess(
		const EntityKey & ekey,
		const BW::string & dataForClient = BW::string(),
		const BW::string & dataForBaseEntity = BW::string() ) = 0;

	/**
	 *	This method is called when getEntityKeyForAccount() completes and it
	 *	wants to load the entity with the given username.
	 *
	 *	@param entityType	The type of Entity to load
	 *	@param username		The identifier of the Entity to load
	 *	@param shouldCreateNewOnLoadFailure	Whether to create a new Entity if no
	 *						entity matches entityType and username
	 *	@param shouldRemeberNewOnLoadFailure	Whether to remember the created
	 *						entity if no entity matches entityType and username
	 *	@param dataForClient	(optional) A string to send to the client
	 *	@param dataForBaseEntity	(optional) A string to provide to the entity
	 */
	virtual void onGetEntityKeyForAccountLoadFromUsername(
		EntityTypeID entityType, const BW::string & username,
		bool shouldCreateNewOnLoadFailure,
		bool shouldRememberNewOnLoadFailure = false,
		const BW::string & dataForClient = BW::string(),
		const BW::string & dataForBaseEntity = BW::string() ) = 0;

	/**
	 *	This method is called when getEntityKeyForAccount wants to create a new
	 *	entity for the account.
	 *
	 *	@param entityType	The type of Entity to load
	 *	@param shouldRemeber	Whether to remember the created entity
	 *	@param dataForClient	(optional) A string to send to the client
	 *	@param dataForBaseEntity	(optional) A string to provide to the entity
	 */
	virtual void onGetEntityKeyForAccountCreateNew(
		EntityTypeID entityType,
		bool shouldRemember,
		const BW::string & dataForClient = BW::string(),
		const BW::string & dataForBaseEntity = BW::string() ) = 0;

	/**
	 *	This method is called when getEntityKeyForAccount rejects the account.
	 *
	 *	@param	status		The LogOnStatus for the failure
	 *	@param	errorMsg	(optional) A string of data describing the failure
	 */
	virtual void onGetEntityKeyForAccountFailure( LogOnStatus status,
		const BW::string & errorMsg = BW::string() ) = 0;
};

class ISetEntityKeyForAccountHandler
{
public:
	/**
	 *	This method is called when setEntityKeyForAccount() completes
	 *	successfully.
	 */
	virtual void onSetEntityKeyForAccountSuccess( const EntityKey & ekey ) = 0;

	/**
	 *	This method is called when setEntityKeyForAccount() fails.
	 */
	virtual void onSetEntityKeyForAccountFailure( const EntityKey & ekey ) = 0;
};


/**
 *	This class defines the interface for integrating with a billing system.
 */
class BillingSystem
{
public:
	BillingSystem( const EntityDefs & entityDefs );
	virtual ~BillingSystem() {}

	/**
	 *	This function turns user/pass login credentials into the EntityKey
	 *	associated with them.
	 *
	 *	This method provides the result of the operation by calling
	 *	one of the methods of the provided handler.
	 */
	virtual void getEntityKeyForAccount(
		const BW::string & username, const BW::string & password,
		const Mercury::Address & clientAddr,
		IGetEntityKeyForAccountHandler & handler ) = 0;

	/**
	 *	This function sets the mapping between user/pass to an entity.
	 *
	 *	This method provides the result of the operation by calling
	 *	one of the methods of the provided handler.
	 */
	virtual void setEntityKeyForAccount( const BW::string & username,
		const BW::string & password, const EntityKey & ekey,
		ISetEntityKeyForAccountHandler & handler ) = 0;

	virtual bool isOkay() const
	{
		return (entityTypeIDForUnknownUsers_ != INVALID_ENTITY_TYPE_ID) ||
			(!shouldAcceptUnknownUsers_ && !authenticateViaBaseEntity_);
	}

protected:
	// Configuration from bw.xml
	bool shouldAcceptUnknownUsers_;
	bool shouldRememberUnknownUsers_;
	bool authenticateViaBaseEntity_;
	EntityTypeID entityTypeIDForUnknownUsers_;
	BW::string entityTypeForUnknownUsers_;
};

BW_END_NAMESPACE

#endif // BILLING_SYSTEM_HPP
