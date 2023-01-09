#include "pch.hpp"

#include "scoped_locked_chunk_holder.hpp"

#ifndef CODE_INLINE
#include "scoped_locked_chunk_holder.ipp"
#endif

#include "chunk_manager.hpp"
#include "chunk_processor_manager.hpp"
#include "geometry_mapping.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *	@param manager a pointer to the ChunkProcessorManager, cannot be NULL.
 *		We need the manager to
 *		1 - provide us with the global list of locked chunks.
 *		2 - provide us with the geometry mapping to expand when looking for
 *			neighbouring chunks to load.
 */
ScopedLockedChunkHolder::ScopedLockedChunkHolder(
	ChunkProcessorManager* manager ) :
	manager_( manager )
{
	MF_ASSERT( manager_ );
}

/**
 *	Destructor. Must guarantee unlocking of all of our chunks!
 */
ScopedLockedChunkHolder::~ScopedLockedChunkHolder()
{
	this->clear();
}


/**
 *	Lock a chunk in memory with 0 expansion.
 *	@param chunk the chunk to lock.
 */
void ScopedLockedChunkHolder::lock( Chunk* pChunk )
{
	BW_GUARD;
	this->insert( pChunk );
}


/**
 *	Lock a chunk in memory so that it is guaranteed not to be unloaded.
 *	@param chunk the chunk to lock.
 *	@param expandOnGridX how many chunks in the x-direction to check for
 *		neighbours.
 *	@param expandOnGridZ how many chunks in the z-direction to check for
 *		neighbours.
 */
void ScopedLockedChunkHolder::lock( Chunk* pChunk,
	int expandOnGridX,
	int expandOnGridZ )
{
	BW_GUARD;

	if ((expandOnGridX == 0) && (expandOnGridX == 0))
	{
		this->lock( pChunk );
	}
	else
	{
		manager_->lockChunkInMemory(
			pChunk, expandOnGridX, expandOnGridZ, *this );
	}
}


/**
 *	Lock a chunk in memory so that it is guaranteed not to be unloaded.
 *	@param chunk the chunk to lock.
 *	@param portalDepth
 */
void ScopedLockedChunkHolder::lock( Chunk* pChunk, int portalDepth )
{
	BW_GUARD;
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

	if (portalDepth == 0)
	{
		this->lock( pChunk );
	}
	else
	{
		manager_->lockChunkInMemory( pChunk, portalDepth, *this );
	}
}


/**
 *	Internal function to insert into our chunk list AND the global chunk list.
 *	Use this to insert into chunks_, don't just use chunks_.insert.
 *	@param chunk the chunk to lock.
 */
void ScopedLockedChunkHolder::insert( Chunk* pChunk )
{
	BW_GUARD;
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

	// Don't lock twice
	if ( chunks_.count( pChunk ) > 0 )
	{
		return;
	}

	// Must be loaded
	MF_ASSERT( pChunk->loaded() || pChunk->loading() );

	chunks_.insert( pChunk );
	manager_->allLockedChunks().lock( pChunk );
}


/**
 *	Unlock a given chunk.
 *	@param position iterator to the chunk to erase.
 */
ChunkSet::iterator ScopedLockedChunkHolder::erase(
	ChunkSet::iterator position )
{
	BW_GUARD;

	// Don't remove twice - invalid iterator!
	MF_ASSERT( chunks_.count( *position ) > 0 );

	// Unlock
	manager_->allLockedChunks().unlock( *position );
	return chunks_.erase( position );
}


/**
 *	Unlock all of our chunks.
 */
void ScopedLockedChunkHolder::clear()
{
	BW_GUARD;
	manager_->allLockedChunks().unlock( chunks_ );
	chunks_.clear();
}

BW_END_NAMESPACE

// scoped_locked_chunk_holder.cpp
