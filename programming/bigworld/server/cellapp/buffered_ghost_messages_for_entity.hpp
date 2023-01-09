#ifndef BUFFERED_GHOST_MESSAGES_FOR_ENTITY_HPP
#define BUFFERED_GHOST_MESSAGES_FOR_ENTITY_HPP

#include "buffered_ghost_message_queue.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This class collects all buffered ghost message for an entity. It keeps
 *	these messages in queues from different CellApps. Only the ordering within
 *	these queues is guaranteed. This class works out the ordering between these
 *	queues.
 *
 *	The last message from a real entity to a ghost before being offloaded
 *	(ghostSetNextReal) informs us which is the next queue to process from.
 */
class BufferedGhostMessagesForEntity
{
public:
	BufferedGhostMessagesForEntity() {}

	void add( const Mercury::Address & addr,
		BufferedGhostMessage * pMessage );

	bool playSubsequence( const Mercury::Address & srcAddr );
	bool playNewLifespan();

	void delaySubsequence( const Mercury::Address & addr,
			BufferedGhostMessage * pFirstMessage );

	bool hasMessagesFrom( const Mercury::Address & addr ) const;
	bool isDelayingMessagesFor( const Mercury::Address & addr ) const;

private:
	BufferedGhostMessageQueuePtr find( const Mercury::Address & addr ) const;
	BufferedGhostMessageQueuePtr findOrCreate( const Mercury::Address & addr );

	typedef BW::map< Mercury::Address, BufferedGhostMessageQueuePtr > Queues;
	Queues queues_;
};

BW_END_NAMESPACE

#endif // BUFFERED_GHOST_MESSAGES_FOR_ENTITY_HPP
