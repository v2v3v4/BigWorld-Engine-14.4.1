#include "pch.hpp"

#include "condemned_interfaces.hpp"

#include "network/network_interface.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
CondemnedInterfaces::CondemnedInterfaces():
	container_(),
	dispatcher_()
{
}


/**
 *	Destructor.
 */
CondemnedInterfaces::~CondemnedInterfaces()
{
	this->removeAll();
}


/**
 *	This method adds a condemned network interface.
 *
 *	@param pInterface 	The condemned network interface to add.
 */
void CondemnedInterfaces::add( Mercury::NetworkInterface * pInterface )
{
	pInterface->attach( dispatcher_ );

	container_.push_back( CondemnedInterfaceRemovalRecord( *pInterface, 0 ) );
}


/**
 *	This method registers a condemned network interface to be deleted when
 *	either the interface no longer has unacked packets, or the specified
 *	timeout period.
 *
 *	@param pInterface	The condemned network interface.
 *	@param timeout 		The maximum amount of time (in seconds) to wait for the
 *						network interface's channels to be cleared.
 */
void CondemnedInterfaces::removeWhenAcked( 
		Mercury::NetworkInterface * pInterface, float timeout )
{
	pInterface->attach( dispatcher_ );

	uint64 timeoutStamps = timestamp() +
		uint64( timeout * stampsPerSecond() );

	container_.push_back( CondemnedInterfaceRemovalRecord( *pInterface, 
		timeoutStamps ) );
}


/**
 *	This method clears all registered condemned network interfaces, including
 *	those that are still have unacked packets.
 */
void CondemnedInterfaces::removeAll()
{
	Container::iterator iRecord = container_.begin();
	while (iRecord != container_.end())
	{
		iRecord->deleteInterface();
		++iRecord;
	}

	container_.clear();
}


/**
 *	This method process once and removes all eligible condemned network
 *	interfaces.
 */
void CondemnedInterfaces::processOnce()
{
	dispatcher_.processOnce();

	Container::iterator iRecord = container_.begin();
	while (iRecord != container_.end())
	{
		Container::iterator iThisRecord = iRecord;
		++iRecord;

		if (!iThisRecord->interfaceHasUnackedPackets() || 
				iThisRecord->hasTimedOut())
		{
			iThisRecord->deleteInterface();
			container_.erase( iThisRecord );
		}
	}
}


/**
 *	This method returns the interface.
 */
Mercury::NetworkInterface * CondemnedInterfaceRemovalRecord::pInterface() const
{
	return pInterface_;
}


/**
 *	This method returns whether the record's interface has any outstanding
 *	unacked packets.
 */
bool CondemnedInterfaceRemovalRecord::interfaceHasUnackedPackets() const
{
	return pInterface_->hasUnackedPackets();
}


/**
 *	This method returns whether the unacked wait timeout period has been
 *	reached.
 */
bool CondemnedInterfaceRemovalRecord::hasTimedOut() const
{
	return (unackedCheckTimeout_ == 0) ||
		(timestamp() > unackedCheckTimeout_);
}


/**
 *	This method deletes the associated condemned network interface.
 */
void CondemnedInterfaceRemovalRecord::deleteInterface()
{
	bw_safe_delete( pInterface_ );
}

BW_END_NAMESPACE

// condemned_interface.cpp

