#include "pch.hpp"

#include "resmgr/bwresource.hpp"

#include "chunk_loader.hpp"
#include "chunk_manager.hpp"
#include "chunk_space.hpp"
#include "chunk.hpp"
#include "geometry_mapping.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/slot_tracker.hpp"


DECLARE_DEBUG_COMPONENT2( "Chunk", 0 );

BW_BEGIN_NAMESPACE

MEMTRACKER_DECLARE( Chunk, "Chunk", 0);

// -----------------------------------------------------------------------------
// Section: LoadChunkTask
// -----------------------------------------------------------------------------

/**
 *	This class is used to perform the background loading of chunks.
 */
class LoadChunkTask : public BackgroundTask
{
public:
	LoadChunkTask( Chunk * pChunk ) :
		BackgroundTask( "LoadChunkTask" ),
		pChunk_( pChunk )
	{
	}

	/**
	 *	Load the empty chunk pChunk now.
	 *
	 *	This is called from the loading thread
	 */
	virtual void doBackgroundTask( TaskManager & mgr )
	{
		load( pChunk_ );
	}

	static void load( Chunk * pChunk )
	{
		BW_GUARD;

		MEMTRACKER_SCOPED( Chunk );
		PROFILE_FILE_SCOPED( loadChunk );
		DataSectionPtr	pDS = BWResource::openSection(
			pChunk->resourceID() );
		//Keep the cData around in the cache for the duration of chunk->load
		DataSectionPtr	pCData = BWResource::openSection(
			pChunk->binFileName() );
		pChunk->load( pDS );

		// This spam is useful to some people. Disable locally, but do not 
		// remove. 
		TRACE_MSG( "ChunkLoader: Loaded chunk '%s'\n",
			pChunk->resourceID().c_str() );
	}

	/**
	 *	This method is called on a task that has not been run yet, but we want
	 *	to discard it. For chunk loading, be sure to mark the chunk as
	 *	not loaded so that ChunkManager::checkLoadingChunks() can remove the
	 *	chunk from its loading list. (opposite of ChunkLoader::load()).
	 *	@note this should only be called by TaskManager when clearing the
	 *	pending task list.
	 */
	void cancel( TaskManager & mgr )
	{
		TRACE_MSG( "LoadChunkTask::cancel: Cancel loading chunk '%s'\n",
			pChunk_->resourceID().c_str() );
		pChunk_->loading( false );
	}

private:
	Chunk * pChunk_;
};


// -----------------------------------------------------------------------------
// Section: FindSeedTask
// -----------------------------------------------------------------------------

FindSeedTask::FindSeedTask( ChunkSpace * pSpace, const Vector3 & where ) :
	BackgroundTask( "FindSeedTask" ),
	pSpace_( pSpace ),
	where_( where ),
	foundChunk_( NULL ),
	mapping_( NULL ),
	isComplete_(false),
	released_(false)
{
}


FindSeedTask::~FindSeedTask()
{
	MF_ASSERT(released_);
}


void FindSeedTask::doBackgroundTask( TaskManager & mgr )
{
	foundChunk_ = pSpace_->guessChunk( where_ );
	if (foundChunk_)
	{
		mapping_ = foundChunk_->mapping();
	}
	mgr.addMainThreadTask( this );
}


void FindSeedTask::doMainThreadTask( TaskManager & mgr )
{
	isComplete_ = true;

	// Check if we have already been released. Better clear the object then.
	if (released_)
	{
		release(true);
	}
}

Chunk* FindSeedTask::foundSeed()
{
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );
	return foundChunk_;
}

bool FindSeedTask::isComplete()
{
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );
	return isComplete_;
}

void FindSeedTask::release( bool destroyChunk )
{
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

	released_ = true;

	// Make sure we don't have a premature release and a request to not destroy
	// the chunk. Otherwise we need to store the destroy chunk option for call
	// during doMainThreadTask. But currently it isn't required.
	MF_ASSERT( ! (!isComplete_ && !destroyChunk) );

	if (!isComplete_)
	{
		return;
	}

	if (foundChunk_)
	{
		if (destroyChunk)
			bw_safe_delete(foundChunk_);	// it's just a stub so ok to delete it
		if (mapping_)
		{
			mapping_->decRef();
			mapping_ = NULL;
		}
	}
}


// -----------------------------------------------------------------------------
// Section: ChunkLoader
// -----------------------------------------------------------------------------

/**
 *	This method loads the empty chunk pChunk (its identifier is set).
 *
 *	This is called from the main thread
 *
 *	@param pChunk	The chunk to load.
 *	@param priority	The priority at which to load. 0 is normal, lower numbers
 *		mean that are loaded earlier.
 */
void ChunkLoader::load( Chunk * pChunk, int priority )
{
	BW_GUARD;
	MF_ASSERT( !pChunk->loading() );
	pChunk->loading( true );

// Will force loading to be synchronous
//	LoadChunkTask::load( pChunk ); // load in main thread.
  	FileIOTaskManager::instance().addBackgroundTask(
  		new LoadChunkTask( pChunk ),
  		priority );
}


/**
 *	This method loads the input chunk immediately in the main thread.
 */
void ChunkLoader::loadNow( Chunk * pChunk )
{
	BW_GUARD;
	MF_ASSERT( !pChunk->loading() );
	pChunk->loading( true );
	LoadChunkTask::load( pChunk );
}


/**
 *	Create a seed chunk for the given space at the given location.
 */
FindSeedTask* ChunkLoader::findSeed( ChunkSpace * pSpace, 
	const Vector3 & where )
{
	BW_GUARD;
	FindSeedTask * task_ = new FindSeedTask( pSpace, where );
	FileIOTaskManager::instance().addBackgroundTask( task_, 15 );
	return task_;
}

BW_END_NAMESPACE

// chunk_loader.cpp
