#include "buffered_ghost_messages.hpp"

#include "buffered_ghost_messages_for_entity.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This method buffers a message for an entity.
 */
void BufferedGhostMessages::add( EntityID entityID, 
		const Mercury::Address & srcAddr, BufferedGhostMessage * pMessage )
{
	map_[ entityID ].add( srcAddr, pMessage );
}


/**
 *	This method plays any buffered messages for this entity that are valid to
 *	play.
 */
void BufferedGhostMessages::playSubsequenceFor( EntityID id,
	   const Mercury::Address & srcAddr )
{
	Map::iterator iter = map_.find( id );

	if (iter != map_.end())
	{
		// Note: 'this' may have been deleted already if false is returned
		if (iter->second.playSubsequence( srcAddr ))
		{
			map_.erase( iter );
		}
	}
}


/**
 *	This method attempts to play a buffered ghost lifespan for the given entity.
 *	the given id.
 */
void BufferedGhostMessages::playNewLifespanFor( EntityID id )
{
	Map::iterator iter = map_.find( id );

	if (iter != map_.end())
	{
		WARNING_MSG( "BufferedGhostMessages::playNewLifespanFor: %u\n",
				id );

		// Note: 'this' may have been deleted already if false is returned
		if (iter->second.playNewLifespan())
		{
			map_.erase( iter );
		}
	}
}


/**
 *	This method returns whether there are any buffered messages for an entity
 *	from the given address.
 */
bool BufferedGhostMessages::hasMessagesFor( EntityID entityID, 
		const Mercury::Address & addr ) const
{
	Map::const_iterator iter = map_.find( entityID );

	if (iter == map_.end())
	{
		return false;
	}

	return iter->second.hasMessagesFrom( addr );
}


/**
 *	This method returns messages are currently being delayed for an entity
 *	from the given address.
 */
bool BufferedGhostMessages::isDelayingMessagesFor( EntityID entityID,
			const Mercury::Address & addr ) const
{
	Map::const_iterator iter = map_.find( entityID );

	if (iter == map_.end())
	{
		return false;
	}

	return iter->second.isDelayingMessagesFor( addr );
}



/**
 *	This method delays the current subsequence of messages for an entity from
 *	the given address.
 */
void BufferedGhostMessages::delaySubsequence( EntityID entityID,
			const Mercury::Address & srcAddr,
			BufferedGhostMessage * pFirstMessage )
{
	map_[ entityID ].delaySubsequence( srcAddr, pFirstMessage );
}

BW_END_NAMESPACE

// buffered_ghost_messages.cpp
