#include "script/first_include.hpp"

#include "login_handler.hpp"

#include "pending_logins.hpp"
#include "proxy.hpp"

#include "cstdmf/binary_stream.hpp"

#include "network/unpacked_message_header.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Constructor
 */
LoginHandler::LoginHandler() :
	pPendingLogins_( new PendingLogins ),
	numLogins_( 0 ),
	numLoginsAddrNAT_( 0 ),
	numLoginsPortNAT_( 0 ),
	numLoginsMultiAttempts_( 0 ),
	maxLoginAttempts_( 0 ),
	numLoginCollisions_( 0 )
{
}


/**
 *	Destructor
 */
LoginHandler::~LoginHandler()
{
	delete pPendingLogins_;
	pPendingLogins_ = NULL;
}


/**
 *	This method adds a new entry when a player has logged in via the LoginApp
 *	but has yet to login via the BaseApp.
 */
SessionKey LoginHandler::add( Proxy * pProxy,
		const Mercury::Address & loginAppAddr )
{
	return pPendingLogins_->add( pProxy, loginAppAddr );
}


/**
 *	This method is called periodically to check whether any pending logins have
 *	timed out.
 */
void LoginHandler::tick()
{
	pPendingLogins_->tick();
}


/**
 *	This method updates the statistics associated with a successful login.
 */
void LoginHandler::updateStatistics( const Mercury::Address & addr,
		const Mercury::Address & expectedAddr,
		uint32 attempt )
{
	if (addr != expectedAddr)
	{
		if (addr.ip != expectedAddr.ip)
		{
			++numLoginsAddrNAT_;
			WARNING_MSG( "LoginHandler::updateStatistics: "
					"Addresses are different (due to NAT?). Was %s. Now %s\n",
				 expectedAddr.c_str(), addr.c_str() );
		}
		else if (expectedAddr.port != 0)
		{
			// BaseApp switches are expected to have a different port than when
			// the pending login was registered, this is signified by setting the
			// port to 0.

			++numLoginsPortNAT_;
			INFO_MSG( "LoginHandler::updateStatistics: "
					"Ports are different (due to NAT). Was %s. Now %s\n",
				expectedAddr.c_str(), addr.c_str() );
		}
	}

	++numLogins_;

	if (attempt != 0)
	{
		++numLoginsMultiAttempts_;
	}

	maxLoginAttempts_ = std::max( maxLoginAttempts_, attempt );
}


/**
 *	This method is called by a client to make initial contact with the BaseApp.
 *	It should be called after the client has logged in via the LoginApp.
 */
void LoginHandler::login( Mercury::NetworkInterface & networkInterface,
			const Mercury::Address& srcAddr,
			const Mercury::UnpackedMessageHeader& header,
			const BaseAppExtInterface::baseAppLoginArgs & args )
{
	PendingLogins::iterator pendingIter = pPendingLogins_->find( args.key );

	if (pendingIter == pPendingLogins_->end())
	{
		INFO_MSG( "LoginHandler::login(%s): "
				"No pending login for loginKey %u. Attempt = %u\n",
			srcAddr.c_str(), args.key, args.numAttempts );
		// Bad bundle so break out of dispatching the rest.
		header.breakBundleLoop();

		return;
	}

	const PendingLogin & pending = pendingIter->second;

	SmartPointer<Proxy> pProxy = pending.pProxy();

	if (pProxy->isDestroyed())
	{
		return;
	}

	if (networkInterface.findChannel( srcAddr ) != NULL)
	{
		++numLoginCollisions_;

		INFO_MSG( "LoginHandler::login(%s): "
				"%u collided with an existing channel. Attempt = %u\n",
			srcAddr.c_str(), pProxy->id(), args.numAttempts );

		return;
	}

	this->updateStatistics( srcAddr, pending.addrFromLoginApp(), args.numAttempts );

	pPendingLogins_->erase( pendingIter );

	if (pProxy->attachToClient( srcAddr, header.replyID,
			header.pChannel.get() ))
	{
		INFO_MSG( "LoginHandler::login: "
			"%u attached from %s. Attempt %u\n",
			pProxy->id(), srcAddr.c_str(), args.numAttempts );
	}
}


/**
 *	This method creates a watcher that can inspect instances of this object.
 */
WatcherPtr LoginHandler::pWatcher()
{
	DirectoryWatcherPtr pWatcher = new DirectoryWatcher();

	// Leaving these writable in case they want to be reset.
	pWatcher->addChild( "total",            makeWatcher( &LoginHandler::numLogins_ ) );
	pWatcher->addChild( "numAddrNAT",       makeWatcher( &LoginHandler::numLoginsAddrNAT_ ) );
	pWatcher->addChild( "numPortNAT",       makeWatcher( &LoginHandler::numLoginsPortNAT_ ) );
	pWatcher->addChild( "numMultiAttempts", makeWatcher( &LoginHandler::numLoginsMultiAttempts_ ) );
	pWatcher->addChild( "maxAttempts",      makeWatcher( &LoginHandler::maxLoginAttempts_ ) );
	pWatcher->addChild( "numCollisions",    makeWatcher( &LoginHandler::numLoginCollisions_ ) );

	return pWatcher;
}

BW_END_NAMESPACE

// login_handler.cpp
