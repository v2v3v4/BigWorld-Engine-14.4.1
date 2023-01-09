#include "pch.hpp"
#include "chunk/chunk_manager.hpp"
#include "cstdmf/progress.hpp"
#include "unsaved_chunks.hpp"
#include "chunk_overlapper.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Save the chunks in this container.
 *	@param saver chunk saver to use in the save operation.
 *	@param progress the indicator to update with the progress of the operation.
 *	@return true on success.
 */
bool UnsavedChunks::saveChunks( IChunkSaver& saver, Progress* progress )
{
	BW_GUARD;

	bool result = true;

	for (UnsavedChunks::Chunks::iterator iter = chunks_.begin();
		iter != chunks_.end(); ++iter)
	{
		Chunk* chunk = *iter;
		MF_ASSERT( chunk != NULL );

		// 1. Deleting chunk
		// The chunk does not need to be bound
		// 2. Adding new chunk
		// 3. Modifying existing chunk
		// The chunks must be loaded and bound
		if (!saver.isDeleted( *chunk ))
		{
			// Wait for chunk to load
			// Chunk must be fully loaded before we can save it
			MF_ASSERT( chunk->loaded() || chunk->loading() );
			while (chunk->loading())
			{
				ChunkManager::instance().checkLoadingChunks();
			}
			MF_ASSERT( !chunk->loading() );
			MF_ASSERT( chunk->loaded() );
			MF_ASSERT( chunk->isBound() );
		}

		result &= saver.save( chunk );
		savedChunks_.insert( chunk );

		if (chunk->removable())
		{
			chunk->unbind( false );
			chunk->unload();
		}

		if (progress)
		{
			progress->step();
		}
	}

	chunks_.clear();

	return result;
}


/**
 *	Add a chunk to the list of chunks to save.
 *	
 *	It is ok if the chunk hasn't loaded yet, but it must at least be loading.
 *	Wait for it to load during the save operation (which has a progress bar).
 *	
 *	eg. ChunkProcessorManager::spreadInvalidateInternal invalidates chunks and
 *	marks them as needing saving. These chunks might not have loaded yet, they
 *	are just next to something that has been moved.
 *	
 *	@param chunk the chunk to save.
 */
void UnsavedChunks::add( Chunk* chunk )
{
	BW_GUARD;
	MF_ASSERT( chunk != NULL );
	MF_ASSERT( chunk->loaded() || chunk->loading() );
	chunks_.insert( chunk );
}


/**
 *	Clear the unsaved chunks list and the saved chunks list.
 */
void UnsavedChunks::clear()
{
	BW_GUARD;

	chunks_.clear();
	savedChunks_.clear();
}


/**
 *	Save the chunks that have been added to this container.
 *	@param saver chunk saver to use in the save operation.
 *	@param progress the indicator to update with the progress of the operation.
 *	@param loader chunk loader to use for loading chunks.
 *	@param extraChunks any additional chunks to load and save.
 *	@return true on success.
 */
bool UnsavedChunks::save( IChunkSaver& saver, Progress* progress,
	IChunkLoader* loader, ChunkIds extraChunks )
{
	BW_GUARD;

	bool result = true;

	for (ChunkIds::iterator iter = extraChunks.begin();
		iter != extraChunks.end();)
	{
		Chunk* chunk = loader->find( *iter );

		if (!chunk || chunk->isBound())
		{
			iter = extraChunks.erase( iter );
		}
		else
		{
			++iter;
		}
	}

	if (progress)
	{
		progress->length( float( chunks_.size() + extraChunks.size() ) );
	}

	result &= this->saveChunks( saver, progress );

	for( ChunkIds::const_iterator iter = extraChunks.begin();
		iter != extraChunks.end(); ++iter )
	{
		if (progress)
		{
			progress->step();
		}

		loader->load( *iter );

		result &= this->saveChunks( saver, progress );
	}

	return result;
}


/**
 *	Check if there are any unsaved chunks in this container.
 *	@return true if there are unsaved chunks.
 */
bool UnsavedChunks::empty() const
{
	return chunks_.empty();
}


/**
 *	Mark the chunks added to this container (and their overlappers)
 *	as non-removable.
 */
void UnsavedChunks::mark()
{
	BW_GUARD;

	UnsavedChunks::Chunks::iterator chunkIt;
	for (chunkIt = chunks_.begin(); chunkIt != chunks_.end(); ++chunkIt)
	{
		Chunk* pChunk = *chunkIt;

		MF_ASSERT( pChunk->loaded() || pChunk->loading() );

		pChunk->removable( false );

		if (pChunk->isOutsideChunk())
		{
			const ChunkOverlappers::Overlappers& overlappers =
				ChunkOverlappers::instance( *pChunk ).overlappers();

			ChunkOverlappers::Overlappers::const_iterator overlapperIt;
			for (overlapperIt = overlappers.begin();
				overlapperIt != overlappers.end(); ++overlapperIt)
			{
				ChunkOverlapperPtr pChunkOverlapper = *overlapperIt;
				pChunkOverlapper->pOverlapper()->removable( false );
			}
		}
	}
}


/**
 *	Filter out/remove already saved chunks from this container.
 *	@param uc the chunks to remove.
 */
void UnsavedChunks::filter( const UnsavedChunks& uc )
{
	BW_GUARD;

	for (Chunks::iterator iter = chunks_.begin();
		iter != chunks_.end();)
	{
		if (uc.savedChunks_.find( *iter ) != uc.savedChunks_.end())
		{
			iter = chunks_.erase( iter );
		}
		else
		{
			++iter;
		}
	}
}


/**
 *	Filter out/remove unsaved chunks from this container.
 *	(These chunks are not going to be saved)
 *	@param toFilter the chunks to remove.
 */
void UnsavedChunks::filterWithoutSaving( const Chunks& toFilter )
{
	BW_GUARD;

	for (Chunks::const_iterator iter = toFilter.begin();
		iter != toFilter.end(); ++iter)
	{
		Chunks::iterator myIter = chunks_.find( *iter );
		if (myIter != chunks_.end())
		{
			chunks_.erase( myIter );
		}
	}
}

BW_END_NAMESPACE


// unsaved_chunks.cpp
