#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "server/util.hpp"
#include "cell_chunk.hpp"
#include "entity.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: CellChunk
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
CellChunk::CellChunk( Chunk & chunk ) :
	chunk_( chunk ),
	pFirstEntity_( NULL ),
	pLastEntity_( NULL )
{
	chunk_.nextMark();
}

/**
 *	Destructor
 */
CellChunk::~CellChunk()
{
	this->clearAllEntities();
}

/**
 *	This chunk has been bound or bound to - clear out all entities and
 *	let them resettle ... possibly into us again - unless there is now
 *	a better chunk or we are being unloaded.
 */
void CellChunk::bind( bool isUnbind )
{
	this->clearAllEntities();
}


/**
 *	Add an entity which should live in this chunk
 */
void CellChunk::addEntity( Entity * pEntity )
{
	MF_ASSERT( pEntity->prevInChunk() == NULL );
	MF_ASSERT( pEntity->nextInChunk() == NULL );
	MF_ASSERT( !pEntity->isDestroyed() );
	MF_ASSERT( !pEntity->inDestroy() );

	if (pFirstEntity_ == NULL)
	{
		pFirstEntity_ = pEntity;
		pLastEntity_  = pFirstEntity_;
	}
	else
	{
		pFirstEntity_->prevInChunk( pEntity );
		pEntity->nextInChunk( pFirstEntity_ );
		pFirstEntity_ = pEntity;
	}
}


/**
 *	Remove an entity living in this chunk
 */
void CellChunk::removeEntity( Entity* pEntity )
{
	Entity* pPrev = pEntity->prevInChunk();
	Entity* pNext = pEntity->nextInChunk();

	if( pPrev != NULL )
		pPrev->nextInChunk(pNext);
	else
		pFirstEntity_ = pNext;

	if (pNext != NULL)
		pNext->prevInChunk(pPrev);
	else
		pLastEntity_ = pPrev;

	pEntity->removedFromChunk();

	MF_ASSERT(pEntity->prevInChunk() == NULL);
	MF_ASSERT(pEntity->nextInChunk() == NULL);
}


/**
 *	Clear out all entities from this chunk
 */
void CellChunk::clearAllEntities()
{
	// clear all entities
	Entity * pNext= pFirstEntity_;
//	for (EntityIterator it = this->begin(); it != this->end(); it++)
	for (Entity * pEntity = pNext; pEntity != NULL; pEntity = pNext)
	{
		pNext = pEntity->nextInChunk();
		pEntity->removedFromChunk();
		//(*it)->prevInChunk( NULL );	// duh...
		//(*it)->nextInChunk( NULL );
	}
	pFirstEntity_ = NULL;
	pLastEntity_ = NULL;
}


// static instance initialiser
CellChunk::Instance<CellChunk> CellChunk::instance;

/**
 *	Propagate noise through this chunk
 */
void CellChunk::propagateNoise( const Entity* who,
								float propagationRange,
								Vector3 position,
								float remainingRange,
								int event,
							   	int info,
								uint32 mark )
{
	if (mark == 0)
	{
		mark = chunk_.nextMark();
		chunk_.traverseMark( mark );
	}
	else if (mark == chunk_.traverseMark())
	{
		return;
	}
	else
	{
		chunk_.traverseMark( mark );
	}

	for (EntityIterator iter = this->begin(); iter != this->end(); iter++)
	{
		Entity * pEntity = *iter;

		if (pEntity != who)
		{
			float dist = (float)sqrt( ServerUtil::distSqrBetween(
				position, pEntity->position() ) );
			if (dist < remainingRange)
			{
				float travelDist = (propagationRange - remainingRange) + dist;
				pEntity->heardNoise( who, propagationRange, travelDist,
					event, info );
			}
		}
	}

	for (Chunk::piterator it = chunk_.pbegin(); it != chunk_.pend(); it++)
	{
		ChunkBoundary::Portal portal = *it;
		if (!portal.hasChunk() || !portal.permissive)
			continue;

		CellChunk & neighbour = CellChunk::instance( *portal.pChunk );

		//TODO: for outside chunks, we should use closest point on the portal
		Vector3 newPosition	= portal.centre;

		float dist = ServerUtil::distSqrBetween(position, newPosition);
		float distanceLeft = (float)(remainingRange - sqrt(dist));

		if (distanceLeft > 0.f)
			neighbour.propagateNoise( who, propagationRange, newPosition,
				distanceLeft, event, info, mark);
	}
}


/**
 *	Find the next entity in this chunk
 */
void CellChunk::EntityIterator::operator++(int)
{
	pEntity_ = pEntity_->nextInChunk();
}

BW_END_NAMESPACE

// cell_chunk.cpp
