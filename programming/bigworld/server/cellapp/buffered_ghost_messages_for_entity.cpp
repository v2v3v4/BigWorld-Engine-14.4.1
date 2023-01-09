#include "buffered_ghost_messages_for_entity.hpp"

#include "buffered_ghost_message_queue.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: BufferedGhostMessagesForEntity
// -----------------------------------------------------------------------------

/**
 *
 */
BufferedGhostMessageQueuePtr BufferedGhostMessagesForEntity::find(
		const Mercury::Address & addr ) const
{
	Queues::const_iterator iQueue = queues_.find( addr );

	return (iQueue != queues_.end()) ? iQueue->second : NULL;
}


/**
 *
 */
BufferedGhostMessageQueuePtr BufferedGhostMessagesForEntity::findOrCreate(
		const Mercury::Address & addr )
{
	BufferedGhostMessageQueuePtr pQueue = queues_[ addr ];

	// If none exists, make a new one.
	if (!pQueue)
	{
		pQueue = new BufferedGhostMessageQueue();
		queues_[ addr ] = pQueue;
	}

	return pQueue;
}


/**
 *  This method adds a buffered message to the stream for the specified
 *  address.
 */
void BufferedGhostMessagesForEntity::add( const Mercury::Address & addr,
	BufferedGhostMessage * pMessage )
{
	BufferedGhostMessageQueuePtr pQueue = this->findOrCreate( addr );
	pQueue->add( pMessage );
}


/**
 *  This method plays as many of the buffered messages in this object as
 *  possible.
 */
bool BufferedGhostMessagesForEntity::playSubsequence(
		const Mercury::Address & srcAddr )
{
	bool callerShouldDelete = false;

	// Find the queue that this entity wants to receive from.
	Queues::iterator iQueue = queues_.find( srcAddr );

	if (iQueue == queues_.end())
	{
		DEBUG_MSG( "BufferedGhostMessagesForEntity::play: "
				"Not playing buffered ghost messages (%" PRIzu " queues), "
				"still talking to real at %s.\n",
			queues_.size(), srcAddr.c_str() );

		return false;
	}

	if (iQueue->second->playSubsequence())
	{
		queues_.erase( iQueue );

		// The caller cannot simply check queues_.empty() because
		// playSubsequence may call this method recursively.
		callerShouldDelete = queues_.empty();
	}

	return callerShouldDelete;
}


/**
 *	This method checks to see if any queues start with a createMessage. If so,
 *	that message is played. This is called after this entity has been destroyed.
 */
bool BufferedGhostMessagesForEntity::playNewLifespan()
{
	bool callerShouldDelete = false;

	// Find the queue that this entity wants to receive from.
	Queues::iterator iQueue = queues_.begin();
	while (iQueue != queues_.end())
	{
		BufferedGhostMessageQueuePtr pQueue = iQueue->second;

		pQueue->promoteDelayedSubsequences();

		if (pQueue->isFrontCreateGhost())
		{
			if (pQueue->playSubsequence())
			{
				queues_.erase( iQueue );

				// The caller cannot simply check queues_.empty() because
				// playSubsequence may call this method recursively.
				callerShouldDelete = queues_.empty();
			}

			break;
		}

		++iQueue;
	}

	return callerShouldDelete;
}


/**
 *	This method returns whether there are any buffered messages for this entity
 *	from the given address.
 */
bool BufferedGhostMessagesForEntity::hasMessagesFrom(
		const Mercury::Address & addr ) const
{
	BufferedGhostMessageQueuePtr pQueue = this->find( addr );

	return pQueue && !pQueue->isEmpty();
}

/**
 *	This method returns whether messages are being delayed from the given
 *	address.
 */
bool BufferedGhostMessagesForEntity::isDelayingMessagesFor(
		const Mercury::Address & addr ) const
{
	BufferedGhostMessageQueuePtr pQueue = this->find( addr );

	return pQueue && pQueue->isDelaying();
}


/**
 *
 */
void BufferedGhostMessagesForEntity::delaySubsequence(
		const Mercury::Address & addr, BufferedGhostMessage * pFirstMessage )
{
	BufferedGhostMessageQueuePtr pQueue = this->findOrCreate( addr );
	pQueue->delaySubsequence( pFirstMessage );
}

BW_END_NAMESPACE

// buffered_ghost_messages_for_entity.cpp
