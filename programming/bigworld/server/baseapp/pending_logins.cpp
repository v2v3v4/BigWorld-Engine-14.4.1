#include "script/first_include.hpp"

#include "pending_logins.hpp"

#include "baseapp.hpp"
#include "baseapp_config.hpp"
#include "proxy.hpp"

#include "server/nat_config.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PendingLogins
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PendingLogins::PendingLogins() :
	container_(),
	queue_(),
	numInternalLogins_( 0 ),
	numExternalLogins_( 0 )
{
	NATConfig::postInit();
}


/**
 *	This method returns the pending login associated with the input key. Like
 *	standard STL containers, it returns the end iterator when the item cannot be
 *	found.
 */
PendingLogins::iterator PendingLogins::find( SessionKey key )
{
	return container_.find( key );
}

/**
 *	This method returns the end iterator associated with PendingLogins like
 *	standard STL containers.
 */
PendingLogins::iterator PendingLogins::end()
{
	return container_.end();
}


/**
 *	This method removes the element pointed to by the input iterator.
 */
void PendingLogins::erase( PendingLogins::iterator iter )
{
	if (NATConfig::isInternalIP( iter->second.addrFromLoginApp().ip ))
	{
		++numInternalLogins_;
	}
	else
	{
		++numExternalLogins_;
	}

	container_.erase( iter );
}


/**
 *	This method adds a proxy to the set of pending logins. A baseAppLogin
 *	request is now expected from the client to take it out of this set.
 */
SessionKey PendingLogins::add( Proxy * pProxy,
		const Mercury::Address & loginAppAddr )
{
	SessionKey loginKey = pProxy->sessionKey();
	pProxy->regenerateSessionKey();

	// Make sure proxy is only in the pending list once.
	// Note: Brute-force but not too much of an issue here.
	for (iterator iter = container_.begin(); iter != container_.end(); ++iter)
	{
		if (iter->second.pProxy() == pProxy)
		{
			container_.erase( iter );
			break;
		}
	}

	container_.insert( Container::value_type( loginKey,
		PendingLogin( pProxy, loginAppAddr ) ) );

	// Could make this configurable.
	const int PENDING_LOGINS_TIMEOUT = 30; // 30 seconds

	queue_.push_back( QueueElement(
			BaseApp::instance().time() +
				PENDING_LOGINS_TIMEOUT * BaseAppConfig::updateHertz(),
			pProxy->id(), loginKey ) );

	return loginKey;
}


/**
 *	This method checks whether any pending logins have timed out.
 */
void PendingLogins::tick()
{
	BaseApp & baseApp = BaseApp::instance();

	while (!queue_.empty() && queue_.front().hasExpired( baseApp.time() ))
	{
		Container::iterator iter = container_.find( queue_.front().loginKey() );

		// If the entity is still in the pending list (i.e. not yet logged in)
		if (iter != container_.end())
		{
			const Mercury::Address & clientAddr =
				iter->second.addrFromLoginApp();

			bool isFirstLogin =
					(numInternalLogins_ == 0) && (numExternalLogins_ == 0);

			bool isFirstExternalLogin = (numExternalLogins_ == 0) &&
				NATConfig::isExternalIP( clientAddr.ip );

			queue_.front().expire( clientAddr,
					isFirstLogin, isFirstExternalLogin );

			container_.erase( iter );
		}

		queue_.pop_front();
	}
}


/**
 *	This method is called when a pending login expires.
 */
void PendingLogins::QueueElement::expire( const Mercury::Address & clientAddr,
		bool isFirstLogin, bool isFirstExternalLogin )
{
	Base * pBase = BaseApp::instance().bases().findEntity( this->proxyID() );

	if (!pBase)
	{
		INFO_MSG( "PendingLogins::tick: %u expired. Base not found\n",
				this->proxyID() );
		return;
	}

	MF_ASSERT( pBase->isProxy() );

	WARNING_MSG( "PendingLogins::tick: "
			"Entity %u did not hear from %s. Calling onClientDeath\n",
		this->proxyID(), clientAddr.c_str() );

	if (isFirstLogin || isFirstExternalLogin)
	{
		WARNING_MSG( "PendingLogins::tick: The client at %s did not contact "
				"the BaseApp after logging in.\n",
			clientAddr.c_str() );
	}

	const Mercury::Address & baseAppAddr =
		BaseApp::instance().extInterface().address();

	if (isFirstLogin)
	{
		WARNING_MSG( "PendingLogins::tick: This may be caused by a "
			"mis-configured server. Check that there is not a firewall\n"
			"blocking access to the BaseApp's external port (%s) and that "
			"the client (%s) can route to this address.\n",
			baseAppAddr.c_str(), clientAddr.c_str() );
	}

	if (isFirstExternalLogin)
	{
		uint32 natIP = NATConfig::externalIPFor( baseAppAddr.ip );
		Mercury::Address natAddr( natIP, baseAppAddr.port );

		WARNING_MSG( "PendingLogins::tick: This is a login attempt from "
				"behind a NAT. This may be caused by a mis-configured NAT.\n"
				"Check that the NAT is forwarding %s to %s.\n",
			natAddr.c_str(), baseAppAddr.c_str() );
	}

	static_cast<Proxy *>( pBase )->onClientDeath( CLIENT_DISCONNECT_TIMEOUT,
		/* shouldExpectClient */ false );
}

BW_END_NAMESPACE

// pending_logins.cpp
