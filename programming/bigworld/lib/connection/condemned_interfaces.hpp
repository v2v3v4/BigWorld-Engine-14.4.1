#ifndef CONDEMNED_INTERFACES_HPP
#define CONDEMNED_INTERFACES_HPP

#include "bwentity/bwentity_api.hpp"

#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_safe_allocatable.hpp"
#include "cstdmf/timestamp.hpp"

#include "network/event_dispatcher.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{
	class NetworkInterface;
} // end namespace Mercury


/**
 *	This class is used to store a condemned interface, and optionally its
 *	unacked wait timeout period.
 */
class CondemnedInterfaceRemovalRecord
{
public:
	CondemnedInterfaceRemovalRecord( 
			Mercury::NetworkInterface & networkInterface, 
			uint64 unackedCheckTimeout ):
		pInterface_( &networkInterface ),
		unackedCheckTimeout_( unackedCheckTimeout )
	{}

	Mercury::NetworkInterface * pInterface() const;
	bool interfaceHasUnackedPackets() const;
	bool hasTimedOut() const;

	void deleteInterface();

private:
	Mercury::NetworkInterface * pInterface_;
	uint64 unackedCheckTimeout_;
};


/**
 *	This class is used to store condemned network interfaces that have their
 *	deletion deferred until after they are no longer in use.
 */
class CondemnedInterfaces : public SafeAllocatable
{
public:
	BWENTITY_API CondemnedInterfaces();
	BWENTITY_API ~CondemnedInterfaces();

	void add( Mercury::NetworkInterface * pInterface );
	void removeWhenAcked( Mercury::NetworkInterface * pInterface, 
		float timeout = 10.f );
	void removeAll();

	BWENTITY_API void processOnce();

private:

	typedef BW::list< CondemnedInterfaceRemovalRecord > Container;
	Container container_;

	Mercury::EventDispatcher dispatcher_;
};

BW_END_NAMESPACE

#endif // CONDEMNED_INTERFACES_HPP

