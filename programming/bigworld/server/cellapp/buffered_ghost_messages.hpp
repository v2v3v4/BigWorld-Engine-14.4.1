#ifndef BUFFERED_GHOST_MESSAGES_HPP
#define BUFFERED_GHOST_MESSAGES_HPP

#include "buffered_ghost_messages_for_entity.hpp"
#include "cellapp_interface.hpp"

#include "network/basictypes.hpp"
#include "network/misc.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
class Address;
}

class BufferedGhostMessage;


/**
 *	This class is used to buffer ghost messages that have arrived out of order.
 *	Messages from a single CellApp are ordered but if the real entity is
 *	offload, some ghost messages may arrive too soon.
 *
 *	To get around this problem, we consider the lifespan of a ghost as being
 *	split into subsequence of messages from each CellApp that the real visits.
 *	The Real entity tells ghosts of the next address before offloading. This
 *	allows the ghost to chain these subsequences together and play them in the
 *	correct order.
 */
class BufferedGhostMessages
{
public:
	void playSubsequenceFor( EntityID id, const Mercury::Address & srcAddr );
	void playNewLifespanFor( EntityID id );

	bool hasMessagesFor( EntityID entityID, 
		const Mercury::Address & addr ) const;

	bool isDelayingMessagesFor( EntityID entityID,
			const Mercury::Address & addr ) const;

	void delaySubsequence( EntityID entityID,
			const Mercury::Address & srcAddr,
			BufferedGhostMessage * pFirstMessage );

	void add( EntityID entityID, const Mercury::Address & srcAddr,
				   BufferedGhostMessage * pMessage );

	bool isEmpty() const	{ return map_.empty(); }

	static BufferedGhostMessages & instance();

	static bool isSubsequenceStart( Mercury::MessageID id )
	{
		return id == CellAppInterface::ghostSetReal.id();
	}

	static bool isSubsequenceEnd( Mercury::MessageID id )
	{
		return id == CellAppInterface::delGhost.id() ||
			id == CellAppInterface::ghostSetNextReal.id();
	}

private:
	typedef BW::map< EntityID, BufferedGhostMessagesForEntity > Map;

	Map map_;
};

BW_END_NAMESPACE

#endif // BUFFERED_GHOST_MESSAGES_HPP
