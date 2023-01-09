#include "script/first_include.hpp"

#include "bwauth_billing_system.hpp"

#include "dbapp.hpp"

#include "db_storage/billing_system_creator.hpp"
#include "db_storage/db_entitydefs.hpp"

#include "network/address_resolver.hpp"
#include "network/event_dispatcher.hpp"
#include "network/frequent_tasks.hpp"

#include "resmgr/bwresource.hpp"

#include "urlrequest/manager.hpp"
#include "urlrequest/response.hpp"
#include "urlrequest/string_list.hpp"

#include "cstdmf/bw_string.hpp"

#include "json/json.h"

#include <sstream>
#include <utility>

DECLARE_DEBUG_COMPONENT2( "Billing", 0 )

BW_BEGIN_NAMESPACE

// ---------------------------------------------------------------------------
// Section: BWAuthBillingSystemCreator
// ---------------------------------------------------------------------------

/**
 *
 */
class BWAuthBillingSystemCreator : public BillingSystemCreator
{
public:
	virtual BW::string typeName()
	{
		return "bwauth";
	}

	BillingSystem * create( const EntityDefs & entityDefs,
		ServerApp & app ) const
	{
		return new BWAuthBillingSystem( entityDefs );
	}
};

namespace // (anonymous)
{

BWAuthBillingSystemCreator staticInitialiser;

} // end namespace (anonymous)


// ---------------------------------------------------------------------------
// Section: BWAuthBillingSystem::Impl declaration
// ---------------------------------------------------------------------------

namespace // (anonymous)
{

const char BILLING_SYSTEM_CONFIG[] = "server/remote_auth_key.xml";

const char INDIE_BILLING_HOST[] = "https://games.bigworldtech.com";

const float DEFAULT_REQUEST_TIMEOUT_SECONDS = 5.f;

} // end namespace (anonymous)

/**
 *	Implementation of the Indie BWAuth billing system. 
 */
class BWAuthBillingSystem::Impl
{
public:
	Impl( const EntityDefs & entityDefs, EntityTypeID entityTypeIDForUnknown );
	~Impl();

	int publicKey() const
		{ return publicKey_; }

	const BW::string & privateKeyBase64() const
		{ return privateKeyBase64_; }

	std::ostringstream & streamSigninURLPrefix( 
			std::ostringstream & oss ) const;

	URL::Manager & requestManager()
		{ return requestManager_; }

	EntityTypeID lookUpEntityType( const BW::string & entityTypeName ) const
	{
		return (entityTypeName == "") ?
					entityTypeID_ : entityDefs_.getEntityType( entityTypeName );
	}

	const BW::string & entityTypeName( EntityTypeID entityTypeID ) const
		{ return entityDefs_.getEntityDescription( entityTypeID ).name(); }

	void getEntityKeyForAccount(
		const BW::string & username, const BW::string & password,
		const Mercury::Address & clientAddr,
		IGetEntityKeyForAccountHandler & handler );

	void setEntityKeyForAccount( const BW::string & username,
		const BW::string & password, const EntityKey & ekey,
		ISetEntityKeyForAccountHandler & handler );

	bool isOkay() const;

private:

	const EntityDefs & entityDefs_;

	int publicKey_;
	BW::string privateKeyBase64_;
	BW::string hostURL_;

	EntityTypeID entityTypeID_;

	URL::Manager requestManager_;
};


// ---------------------------------------------------------------------------
// Section: BWAuthRequest and subclasses
// ---------------------------------------------------------------------------

namespace // (anonmyous)
{

/**
 *	This abstract class is the base class for requests to the BWAuth system.
 */
class BWAuthRequest : public URL::SimpleResponse
{
public:
	BWAuthRequest( BWAuthBillingSystem::Impl & billingSystem );
	virtual ~BWAuthRequest() {}

protected:
	bool send( const BW::string & url, URL::Method method = URL::METHOD_GET );

	BWAuthBillingSystem::Impl & billingSystem_;
};


/**
 *	Constructor.
 *
 *	@param billingSystem 	The billing system implementation.
 */
BWAuthRequest::BWAuthRequest( BWAuthBillingSystem::Impl & billingSystem ) :
		billingSystem_( billingSystem )
{
}


/**
 *	This method starts the request.
 */
bool BWAuthRequest::send( const BW::string & url, URL::Method method )
{
	std::ostringstream authHeader;
	authHeader << "Authorization: " << billingSystem_.privateKeyBase64();

	URL::Headers headers;
	headers.push_back( authHeader.str() );

	billingSystem_.requestManager().fetchURL( url, this, &headers, method );

	return true;
}


/**
 *	This class is used to retrieve an entity key from the BWAuth system.
 */
class GetEntityKeyForAccountRequest : public BWAuthRequest
{
public:
	GetEntityKeyForAccountRequest( BWAuthBillingSystem::Impl & billingSystem,
		IGetEntityKeyForAccountHandler & handler );

	bool send( const BW::string & username, const BW::string & signinKey,
		const Mercury::Address & clientAddr );

	virtual ~GetEntityKeyForAccountRequest() {}

	// Overrides from BWAuthRequest / URL::Response
	virtual void onDone( int responseCode, const char * error );

private:
	IGetEntityKeyForAccountHandler & handler_;

	BW::string username_;
	Mercury::Address clientAddr_;
};


/**
 *	Constructor.
 *
 *	@param billingSystem	The billing system implementation.
 *	@param handler 			The handler for this request.
 */
GetEntityKeyForAccountRequest::GetEntityKeyForAccountRequest( 
		BWAuthBillingSystem::Impl & billingSystem,
		IGetEntityKeyForAccountHandler & handler ) :
	BWAuthRequest( billingSystem ),
	handler_( handler ),
	username_(),
	clientAddr_()
{}


/**
 *	Initialiser.
 *
 *	@param username			The user name of the remote account to get the
 *							entity key for.
 *	@param signinKey 		The signin key for the remote account.
 *	@param clientAddr		The address of the requesting client.
 */
bool GetEntityKeyForAccountRequest::send( const BW::string & username, 
		const BW::string & signinKey,
		const Mercury::Address & clientAddr )
{
	username_ = username;
	clientAddr_ = clientAddr;

	std::ostringstream url;
	billingSystem_.streamSigninURLPrefix( url ) << 
		URL::escape( username ) << "?signin_key=" << 
		URL::escape( signinKey ) << "&client_addr=" <<
		URL::escape( clientAddr.c_str() );

	return this->BWAuthRequest::send( url.str() );
}


/**
 *	Override from URL::Response to handle a request completion.
 */
void GetEntityKeyForAccountRequest::onDone( int responseCode,
		const char * error )
{
	Json::Value root;

	Json::Reader reader;

	if (responseCode < 200 || responseCode >= 300)
	{
		ERROR_MSG( "GetEntityKeyForAccountRequest::onDone: "
				"Failed getting key for user %s logging in from %s. %d %s\n",
			username_.c_str(), clientAddr_.c_str(), responseCode,
			error ? error : "" );

		handler_.onGetEntityKeyForAccountFailure( 
				(responseCode == -1) ?
					LogOnStatus::LOGIN_REJECTED_AUTH_SERVICE_UNREACHABLE :
					LogOnStatus::LOGIN_REJECTED_AUTH_SERVICE_GENERAL_FAILURE );
	}
	else if (reader.parse( body_.str(), root ))
	{
		const BW::string response = root["response"].asString();

		if (response == "loadEntity")
		{
			const DatabaseID databaseID = root["entityID"].asInt();
			const BW::string entityTypeName = root["entityType"].asString();

			const EntityTypeID entityTypeID = 
					billingSystem_.lookUpEntityType( entityTypeName );

			EntityKey ekey( entityTypeID, databaseID );
			handler_.onGetEntityKeyForAccountSuccess( ekey );
		}
		else if (response == "createNewEntity")
		{
			const BW::string entityTypeName = root["entityType"].asString();

			const EntityTypeID entityTypeID = 
				billingSystem_.lookUpEntityType( entityTypeName );

			handler_.onGetEntityKeyForAccountCreateNew( entityTypeID, 
				true );
		}
		else if (response == "failureInvalidCredentials")
		{
			handler_.onGetEntityKeyForAccountFailure( 
				LogOnStatus::LOGIN_REJECTED_INVALID_PASSWORD,
				"Invalid credentials" );
		}
		else if (response == "failureUserIsBlocked")
		{
			handler_.onGetEntityKeyForAccountFailure(
				LogOnStatus::LOGIN_REJECTED_AUTH_SERVICE_LOGIN_DISALLOWED,
				"Account is blocked" );
		}
		else if (response == "failureGameIsLocked")
		{
			handler_.onGetEntityKeyForAccountFailure(
				LogOnStatus::LOGIN_REJECTED_AUTH_SERVICE_LOGIN_DISALLOWED,
				"Game is locked" );
		}
		else
		{
			ERROR_MSG( "GetEntityKeyForAccountRequest::onURLRequestFinished: "
					"Got unknown response from server: \"%s\"\n", 
				response.c_str() );

			handler_.onGetEntityKeyForAccountFailure( 
				LogOnStatus::LOGIN_REJECTED_AUTH_SERVICE_INVALID_RESPONSE,
				"Unknown response from BWAuth server." );
		}
	}
	else
	{
		ERROR_MSG( "GetEntityKeyForAccountRequest::onURLRequestFinished: "
				"Got invalid response from server (got %zu bytes)\n", 
			body_.str().size() );

		handler_.onGetEntityKeyForAccountFailure( 
			LogOnStatus::LOGIN_REJECTED_AUTH_SERVICE_INVALID_RESPONSE,
			"Bad response from BWAuth server." );
	}

	delete this;
}


/**
 *	This class is used to set the entity key for an account in the BWAuth
 *	system.
 */
class SetEntityKeyForAccountRequest : public BWAuthRequest
{
public:
	SetEntityKeyForAccountRequest( BWAuthBillingSystem::Impl & billingSystem,
		ISetEntityKeyForAccountHandler & handler );

	bool send( const BW::string & username, const BW::string & signinKey,
		const EntityKey & entityKey );

	/** Destructor. */
	virtual ~SetEntityKeyForAccountRequest() {}

	// Overrides from BWAuthRequest / URL::Response
	virtual void onDone( int responseCode, const char * error );

private:
	ISetEntityKeyForAccountHandler & handler_;
	BW::string username_;
	EntityKey entityKey_;
};


/**
 *	Constructor.
 *
 *	@param billingSystem 		The billing system implementation.
 */
SetEntityKeyForAccountRequest::SetEntityKeyForAccountRequest( 
		BWAuthBillingSystem::Impl & billingSystem,
		ISetEntityKeyForAccountHandler & handler ) :
	BWAuthRequest( billingSystem ),
	handler_( handler ),
	username_(),
	entityKey_( 0, 0 )
{}


/**
 *	This method sends this request.
 *
 *	@param username		The name of the account to set the entity key for.
 *	@param signinKey	The signin key of the account to set the entity key
 *						for.
 *	@param entityKey	The entity key to set for the account.
 */
bool SetEntityKeyForAccountRequest::send( const BW::string & username,
		const BW::string & signinKey,
		const EntityKey & entityKey )
{
	username_ = username;
	entityKey_ = entityKey;

	std::ostringstream url;

	const BW::string & entityTypeName = 
		billingSystem_.entityTypeName( entityKey_.typeID );

	billingSystem_.streamSigninURLPrefix( url ) << 
		URL::escape( username_ ) << 
		"?entityType=" << entityTypeName <<
		"&entityID=" << entityKey_.dbID;

	return this->BWAuthRequest::send( url.str(), URL::METHOD_PUT );
}


/**
 *	Override from URLRequest to handle request failure.
 */
void SetEntityKeyForAccountRequest::onDone( int responseCode,
		const char * error )
{
	if (error)
	{
		// Old onURLRequestFailed
		ERROR_MSG( "SetEntityKeyForAccountRequest::onURLRequestFailed: "
				"Failed to set the entity key for %s entity with "
				"dbID %" FMT_DBID "\n",
			billingSystem_.entityTypeName( entityKey_.typeID ).c_str(),
			entityKey_.dbID );

		delete this;
		return;
	}

	Json::Value root;

	Json::Reader reader;

	bool success = false;

	BW::string data = body_.str();

	if (reader.parse( data, root ))
	{
		const Json::Value & idValue = root["id" ];
		if (idValue.isInt())
		{
			int bwAuthID = root["id"].asInt();

			INFO_MSG( "SetEntityKeyForAccountRequest::onURLRequestFinished: "
					"%s entity with dbID %" FMT_DBID " saved as id = %d\n", 
				billingSystem_.entityTypeName( entityKey_.typeID ).c_str(),
				entityKey_.dbID,
				bwAuthID );
			handler_.onSetEntityKeyForAccountSuccess( entityKey_ );
			success = true;
		}
	}

	if (!success)
	{
		ERROR_MSG( "SetEntityKeyForAccountRequest::onURLRequestFinished: "
				"could not update external authentication service for "
				"%s entity with dbID %" FMT_DBID ": "
				"bad response received (%zu bytes)\n",
			billingSystem_.entityTypeName( entityKey_.typeID ).c_str(),
			entityKey_.dbID, data.size() );

		handler_.onSetEntityKeyForAccountFailure( entityKey_ );
	}

	delete this;
}


} // end namespace (anonymous)

// ---------------------------------------------------------------------------
// Section: BWAuthBillingSystem::Impl implementation
// ---------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param entityDefs 	Entity definitions.
 */
BWAuthBillingSystem::Impl::Impl( const EntityDefs & entityDefs,
			EntityTypeID entityTypeIDForUnknown ) :
		entityDefs_( entityDefs ),
		publicKey_( -1 ),
		privateKeyBase64_(),
		hostURL_(),
		entityTypeID_( entityTypeIDForUnknown ),
		requestManager_( DBApp::instance().mainDispatcher() )
{
	DataSectionPtr pConfig = BWResource::openSection( BILLING_SYSTEM_CONFIG );

	if (!pConfig)
	{
		ERROR_MSG( "BWAuthBillingSystem: "
			"Unable to read %s\n", BILLING_SYSTEM_CONFIG );
		return;
	}

	publicKey_ = pConfig->readInt( "publicKey", -1 );	
	privateKeyBase64_ = pConfig->readString( "privateKey", "", 
		DS_TrimWhitespace );
	hostURL_ = pConfig->readString( "host", "", DS_TrimWhitespace );

	BW::string::size_type hostNameStart = hostURL_.find( "://" ) + 3;
	if (hostNameStart == BW::string::npos)
	{
		ERROR_MSG( "BWAuthBillingSystem: "
				"Invalid host specification: \"%s\"\n",
			hostURL_.c_str() );
		hostURL_.clear();
		return;
	}
	else
	{
		BW::string::size_type hostNameEnd = hostURL_.find_first_of( ":/", 
			hostNameStart );

		if (hostNameEnd == BW::string::npos)
		{
			hostNameEnd = hostURL_.length();
		}

		BW::string hostName( hostURL_, hostNameStart, 
				hostNameEnd - hostNameStart );

		// We resolve it here so as to save the blocking look-up on each
		// request.
		BW::string hostIP;
		if (!Mercury::AddressResolver::resolve( hostName, hostIP ))
		{
			ERROR_MSG( "BWAuthBillingSystem: "
					"Could not resolve host address %s\n",
				hostName.c_str() );
			hostURL_.clear();
			return;
		}

		hostURL_ = hostURL_.replace( hostNameStart, 
			hostNameEnd - hostNameStart, hostIP );
	}

	INFO_MSG( "BWAuthBillingSystem: host = %s, publicKey = %d\n",
		hostURL_.c_str(), publicKey_ );
}


/**
 *	Destructor.
 */
BWAuthBillingSystem::Impl::~Impl()
{
}


/**
 *	This method is a convenience function for adding the URL prefix for game
 *	sign-in requests to a output string stream.
 *
 *	@param oss 		The output string stream to add the URL prefix to.
 *
 *	@return			A reference to the output string stream.
 */
std::ostringstream &  BWAuthBillingSystem::Impl::streamSigninURLPrefix( 
		std::ostringstream & oss ) const
{
	oss << hostURL_ << "/api/games/" << publicKey_ << "/signins/";
	return oss;
}


/**
 *	Override from BillingSystem to retrieve the entity key for the given login
 *	credentials.
 *
 *	@param username		The username of the account.
 *	@param password		The signin key of the account.
 *	@param clientAddr 	The requesting client's address.
 *	@param handler		A handler for this request.
 */
void BWAuthBillingSystem::Impl::getEntityKeyForAccount( 
		const BW::string & username, const BW::string & password,
		const Mercury::Address & clientAddr,
		IGetEntityKeyForAccountHandler & handler )
{
	GetEntityKeyForAccountRequest * pRequest = 
		new GetEntityKeyForAccountRequest( *this, handler );

	if (!pRequest->send( username, password, clientAddr ))
	{
		handler.onGetEntityKeyForAccountFailure( 
			LogOnStatus::LOGIN_MALFORMED_REQUEST,
			"Could not create valid getEntityKeyForAccount request." );
		delete pRequest;
	}
}


/**
 *	Override from BillingSystem to set the entity key for the given login
 *	credentials.
 */
void BWAuthBillingSystem::Impl::setEntityKeyForAccount(
		const BW::string & username, const BW::string & password,
		const EntityKey & ekey,
		ISetEntityKeyForAccountHandler & handler )
{
	SetEntityKeyForAccountRequest * pRequest = 
		new SetEntityKeyForAccountRequest( *this, handler );

	if (!pRequest->send( username, password, ekey ))
	{
		ERROR_MSG( "BWAuthBillingSystem::setEntityKeyForAccount: "
				"Failed to create a valid setEntityKeyForAccount request." );
		handler.onSetEntityKeyForAccountFailure( ekey );
		delete pRequest;
	}
}


/**
 *	Override from BillingSystem. 
 */
bool BWAuthBillingSystem::Impl::isOkay() const
{
	return !hostURL_.empty() && publicKey_ != -1 && 
		entityTypeID_ != INVALID_ENTITY_TYPE_ID;
}


// ----------------------------------------------------------------------------
// Section: BWAuthBillingSystem passthroughs to BWAuthBillingSystem::Impl
// ----------------------------------------------------------------------------


/**
 *	Constructor.
 */
BWAuthBillingSystem::BWAuthBillingSystem( 
		const EntityDefs & entityDefs ) :
	BillingSystem( entityDefs ),
	pImpl_( new Impl( entityDefs, entityTypeIDForUnknownUsers_ ) )
{
}


/**
 *	Destructor.
 */
BWAuthBillingSystem::~BWAuthBillingSystem()
{
	delete pImpl_;
	pImpl_ = NULL;
}


/**
 *	This method validates a login attempt and returns the details of the entity
 *	to use via the handler.
 */
void BWAuthBillingSystem::getEntityKeyForAccount(
	const BW::string & username, const BW::string & password,
	const Mercury::Address & clientAddr,
	IGetEntityKeyForAccountHandler & handler )
{
	pImpl_->getEntityKeyForAccount( username, password, clientAddr, handler );
}


/**
 *	This method is used to inform the billing system that a new entity has been
 *	associated with a login.
 */
void BWAuthBillingSystem::setEntityKeyForAccount( const BW::string & username,
		const BW::string & password, const EntityKey & ekey,
		ISetEntityKeyForAccountHandler & handler )
{
	// TODO: Code to store the new account in the billing system. This is
	// required is shouldRemember is set to true in
	// onGetEntityKeyForAccountCreateNew.
	pImpl_->setEntityKeyForAccount( username, password, ekey, handler );
}


/**
 *	This method returns whether the billing system has been set up correctly.
 */
bool BWAuthBillingSystem::isOkay() const
{
	return pImpl_->isOkay();
}

BW_END_NAMESPACE

// bwauth_billing_system.cpp
