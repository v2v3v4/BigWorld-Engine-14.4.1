#include "pch.hpp"
#include "chunk_processor_manager.hpp"
#include "clean_chunk_list.hpp"

#include "chunk.hpp"
#include "chunk_clean_flags.hpp"
#include "chunk_item_amortise_delete.hpp"
#include "chunk_manager.hpp"
#include "chunk_overlapper.hpp"
#include "scoped_locked_chunk_holder.hpp"
#include "geometry_mapping.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/memory_load.hpp"
#include "cstdmf/progress.hpp"
#include "moo/render_context.hpp"
#include "romp/console_manager.hpp"

#include "resmgr/data_section_cache.hpp"
#include "resmgr/data_section_census.hpp"

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )

BW_BEGIN_NAMESPACE

ChunkProcessorManager* ChunkProcessorManager::s_instance;

/**
 *	Constructor.
 *	@param threadNum the number of threads to run for processing.
 */
ChunkProcessorManager::ChunkProcessorManager( int threadNum )
	: TaskManager( false )
{
	BW_GUARD;

	if ( !s_instance )
	{
		s_instance = this;
	}

	this->startNumThreads( threadNum );
}


/**
 *	Destructor. Must unlock all chunks from memory before destroying.
 */
ChunkProcessorManager::~ChunkProcessorManager()
{
	BW_GUARD;
	// Check chunks are unlocked
	if ( !allLockedChunks_.empty() )
	{
		ERROR_MSG( "ChunkProcessorManager is being destroyed when it has "
			"chunks locked\n" );
	}

	if (s_instance == this)
	{
		s_instance = NULL;
	}
}

void ChunkProcessorManager::startNumThreads( int threadNum )
{
	BW_GUARD;
	if ( threadNum > 0 )
	{
		this->startThreads( "Chunk Processor Manager", threadNum );
	}
}


/**
 *	Stop all threads and check that the threads unlocked all of their chunks.
 *	@param discardPendingTasks
 *	@param waitForThreads
 */
void ChunkProcessorManager::stopAll(
	bool discardPendingTasks,
	bool waitForThreads )
{
	BW_GUARD;
	TaskManager::stopAll( discardPendingTasks, waitForThreads );
	MF_ASSERT( allLockedChunks_.empty() );
}

namespace
{
	bool isPointOutside( int x, int z, 
		int gridMinX, int gridMaxX, int gridMinZ, int gridMaxZ )
	{
		return (x < gridMinX || x > gridMaxX ||
			z < gridMinZ || z > gridMaxZ);
	}
}

/**
 *	Reset chunk marks for chunks that are outside of specified grid
 *  Unload the loaded chunks that are not marked.
 *	Does not unload pending or currently loading chunks.
 */
void ChunkProcessorManager::unloadChunksOutsideOfGridBox(
	 int minX, int maxX, int minZ, int maxZ, const BW::set<Chunk*>& keepLoadedChunks )
{
	DEBUG_MSG( "Memory load %3.1f, unloading chunks outside of minX=%d, maxX=%d, minZ=%d, maxZ=%d\n",
		Memory::memoryLoad(), minX, maxX, minZ, maxZ );

	ChunkSpacePtr& space = geometryMapping()->pSpace();
	const ChunkMap& chunkMap = space->chunks();

	for (ChunkMap::const_iterator iter = chunkMap.begin();
		iter != chunkMap.end(); ++iter)
	{
		for (BW::vector<Chunk*>::const_iterator cit = iter->second.begin();
			cit != iter->second.end(); ++cit)
		{
			Chunk* chunk = *cit;

			if (keepLoadedChunks.find(chunk) != keepLoadedChunks.end())
				continue;

			if (chunk->isBound())
			{
				if (chunk->isOutsideChunk())
				{
					if (isPointOutside( chunk->x(), chunk->z(),
										 minX, maxX, minZ, maxZ ))
					{
						chunk->removable( true );
					}
				}
				else
				{
					// indoor chunks do not have valid x,z coordinate
					// use overlappers position instead
					Chunk* overlapped[4];
					bool bAllOutside = true;
					chunk->collectOverlappedOutsideChunksForShell( overlapped );
					// make sure that all overlappers are outside of the box
					for (int i = 0; i < 4; i++)
					{
						Chunk* chunk = overlapped[i];
						if (chunk)
						{
							bAllOutside &= isPointOutside( chunk->x(), chunk->z(),
											 minX, maxX, minZ, maxZ );
						}
					}
					if (bAllOutside)
					{
						chunk->removable( true );
					}
				}
			}
		}
	}

	mark();

	this->unloadRemovableChunks();

}

/**
 *	Unload and purge all chunks which are marked as removable
 */

void ChunkProcessorManager::unloadRemovableChunks()
{
	geometryMapping()->pSpace()->unloadChunks();

	DataSectionCache::instance()->clear();
	DataSectionCensus::clear();

	AmortiseChunkItemDelete::instance().purge();
}

/**
 *	Run processing tasks.
 */
bool ChunkProcessorManager::tick()
{
	BW_GUARD;

	Moo::rc().preloadDeviceResources( 10000000 );
	ChunkManager::instance().processPendingChunks();
	ChunkManager::instance().checkLoadingChunks();

	ChunkSpacePtr cameraSpace = ChunkManager::instance().cameraSpace();
	if (cameraSpace)
	{
		cameraSpace->focus( Vector3::zero() );
	}

	TaskManager::tick();

	return true;
}


/**
 *	Check if chunk is ready to be processed.
 *	@param primary the chunk we're trying to process.
 *	@param expandOnGridX how many chunks in the x-direction to check for
 *		neighbours.
 *	@param expandOnGridZ how many chunks in the z-direction to check for
 *		neighbours.
 */
bool ChunkProcessorManager::isChunkReadyToProcess(
	Chunk* primary, int expandOnGridX, int expandOnGridZ )
{
	BW_GUARD;

	int16 gridX, gridZ;

	if ( primary->isOutsideChunk() )
	{
		MF_VERIFY( primary->mapping()->gridFromChunkName(
				primary->identifier(), gridX, gridZ ) );

		BW::set<Chunk*> chunks;

		for ( int16 z = gridZ - expandOnGridZ;
			z <= gridZ + expandOnGridZ;
			++z )
		{
			for ( int16 x = gridX - expandOnGridX;
				x <= gridX + expandOnGridX;
				++x )
			{
				BW::string chunkName =
					primary->mapping()->outsideChunkIdentifier( x, z );

				if (chunkName.empty())
				{
					continue;
				}

				Chunk* chunk =
					ChunkManager::instance().findChunkByName(
						chunkName,
						primary->mapping() );

				if ( !chunk->completed() )
				{
					return false;
				}

				chunks.insert( chunk );
			}
		}

		BW::set<Chunk*>::iterator iter;
		for (iter = chunks.begin(); iter != chunks.end(); ++iter)
		{
			Chunk* pChunk = *iter;
			MF_ASSERT( pChunk );

			if (pChunk->isOutsideChunk())
			{
				const ChunkOverlappers::Overlappers& overlappers =
					ChunkOverlappers::instance( *pChunk ).overlappers();

				ChunkOverlappers::Overlappers::const_iterator iter;
				for (iter = overlappers.begin();
					iter != overlappers.end();
					++iter)
				{
					ChunkOverlapperPtr pChunkOverlapper = *iter;
					MF_ASSERT( pChunkOverlapper.exists() );
					if (!pChunkOverlapper->pOverlappingChunk()->completed())
					{
						return false;
					}
				}
			}
		}

		return true;
	}
	else
	{
		Chunk* overlapped[4];

		primary->collectOverlappedOutsideChunksForShell( overlapped );

		for ( int i = 0; i < 4; ++i )
		{
			if ( Chunk* chunk = overlapped[i] )
			{
				if ( !ChunkProcessorManager::isChunkReadyToProcess(
					chunk, expandOnGridX, expandOnGridZ ) )
				{
					return false;
				}
			}
		}

		return true;
	}
}


/**
 *	Load a chunk for processing.
 *	@param primary
 *	@param expandOnGridX how many chunks in the x-direction to check for
 *		neighbours.
 *	@param expandOnGridZ how many chunks in the z-direction to check for
 *		neighbours.
 *
 *	@return true if the chunks have all been loaded. Otherwise chunks
 *		are loading.
 */
bool ChunkProcessorManager::loadChunkForProcessing( Chunk* primary,
	int expandOnGridX,
	int expandOnGridZ )
{
	BW_GUARD;
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

	int16 gridX, gridZ;

	ChunkManager::instance().cameraSpace()->focus( primary->centre() );

	// Only expand on X/Z if this is an outside chunk
	if ( primary->isOutsideChunk() )
	{
		MF_VERIFY( primary->mapping()->gridFromChunkName(
				primary->identifier(), gridX, gridZ ) );

		for ( int16 z = gridZ - expandOnGridZ;
			z <= gridZ + expandOnGridZ; ++z)
		{
			for (int16 x = gridX - expandOnGridX;
				x <= gridX + expandOnGridX; ++x)
			{
				BW::string chunkName = primary->mapping()->
					outsideChunkIdentifier( x, z );

				if (chunkName.empty())
				{
					continue;
				}

				Chunk* chunk = ChunkManager::instance().findChunkByName(
					chunkName,
					primary->mapping() );
				MF_ASSERT( chunk );

				// Start the chunk loading if it's not already loading or loaded
				if (!chunk->loaded() && !chunk->loading())
				{
					ChunkManager::instance().loadChunkExplicitly( chunkName,
						primary->mapping() );
				}

				// Load chunk's overlappers
				// Chunk must finish loading first?
				if (chunk->loaded())
				{
					MF_ASSERT( chunk->isOutsideChunk() );

					const ChunkOverlappers::Overlappers& overlappers = 
						ChunkOverlappers::instance( *chunk ).overlappers();

					ChunkOverlappers::Overlappers::const_iterator iter;
					for (iter = overlappers.begin();
						iter != overlappers.end();
						++iter)
					{
						ChunkOverlapperPtr chunkOverlapper = *iter;
						MF_ASSERT( chunkOverlapper.exists() );

						Chunk* pOverlappingChunk =
							chunkOverlapper->pOverlappingChunk();
						MF_ASSERT( pOverlappingChunk );

						// Set it to load
						if (!pOverlappingChunk->loading() &&
							!pOverlappingChunk->loaded())
						{
							ChunkManager::instance().loadChunkExplicitly(
								pOverlappingChunk->identifier(),
								primary->mapping(),
								true );
						}
					}
				}
			}
		}

		ChunkManager::instance().cameraSpace()->focus( primary->centre() );

		return true;

	}

	// Otherise find outside chunks which are overlapping this indoor chunk
	else
	{
		bool result = true;
		Chunk* overlapped[4];

		primary->collectOverlappedOutsideChunksForShell( overlapped );

		for (int i = 0; i < 4; ++i)
		{
			if (Chunk* chunk = overlapped[i])
			{
				result &= loadChunkForProcessing(
					chunk, expandOnGridX, expandOnGridZ );
			}
		}

		return result;
	}
}


/**
 *	Ensure that neighbouring chunks are loaded.
 *	@param chunk
 *	@param portalDepth
 */
bool ChunkProcessorManager::ensureNeighbourChunkLoadedInternal( Chunk* chunk,
	int portalDepth )
{
	BW_GUARD;

	if (chunk->completed())
	{
		if (portalDepth != 0)
		{
			if (chunk->traverseMark() != Chunk::s_nextMark_)
			{
				chunk->traverseMark( Chunk::s_nextMark_ );

				for (ChunkBoundaries::iterator bit = chunk->joints().begin();
					bit != chunk->joints().end(); ++bit)
				{
					for (ChunkBoundary::Portals::iterator ppit =
						(*bit)->unboundPortals_.begin();
						ppit != (*bit)->unboundPortals_.end(); ++ppit)
					{
						ChunkBoundary::Portal* pit = *ppit;

						if (pit->hasChunk())
						{
							return false;
						}
					}
				}

				for (ChunkBoundaries::iterator bit = chunk->joints().begin();
					bit != chunk->joints().end(); ++bit)
				{
					for (ChunkBoundary::Portals::iterator ppit =
						(*bit)->boundPortals_.begin();
						ppit != (*bit)->boundPortals_.end(); ++ppit)
					{
						ChunkBoundary::Portal* pit = *ppit;

						if (pit->hasChunk())
						{
							if (!ChunkProcessorManager::
								ensureNeighbourChunkLoadedInternal(
								pit->pChunk, portalDepth - 1 ))
							{
								return false;
							}
						}
					}
				}
			}
		}

		return true;
	}

	return false;
}


/**
 *	Ensure that neighbouring chunks are loaded.
 *	@param chunk
 *	@param portalDepth
 */
void ChunkProcessorManager::loadNeighbourChunkInternal( Chunk* chunk,
	int portalDepth,
	BW::map<Chunk*, int>& chunkDepths )
{
	BW_GUARD;
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

	if (portalDepth != 0)
	{
		BW::map<Chunk*, int>::iterator itDepth = chunkDepths.find( chunk );

		if (itDepth != chunkDepths.end())
		{
			if (portalDepth <= (*itDepth).second)
			{
				// We have already traversed this chunk deeper than now, so
				// skip.
				return;
			}
		}

		chunkDepths[ chunk ] = portalDepth;

		// Load chunks
		BW::set<Chunk*> chunksToLoad;

		for (ChunkBoundaries::iterator bit = chunk->joints().begin();
			bit != chunk->joints().end(); ++bit)
		{
			for (ChunkBoundary::Portals::iterator ppit =
				(*bit)->unboundPortals_.begin();
				ppit != (*bit)->unboundPortals_.end(); ++ppit)
			{
				ChunkBoundary::Portal* pit = *ppit;

				if( pit->hasChunk() && !pit->pChunk->loaded() &&
					!pit->pChunk->loading())
				{
					chunksToLoad.insert( pit->pChunk );
				}
				else
				{
					BW::string chunkName = chunk->mapping()->
						outsideChunkIdentifier( pit->centre );

					if (!chunkName.empty())
					{
						Chunk* pOutsideChunk = ChunkManager::instance().
							findChunkByName( chunkName, chunk->mapping() );

						if (pOutsideChunk && !pOutsideChunk->loaded() &&
							!pOutsideChunk->loading())
						{
							chunksToLoad.insert( pOutsideChunk );
						}
					}
				}
			}
		}

		BW::set<Chunk *>::iterator it;
		for (it = chunksToLoad.begin(); it != chunksToLoad.end(); ++it)
		{
			Chunk* pChunk = *it;
			MF_ASSERT( pChunk );

			// Load the chunk
			ChunkManager::instance().loadChunkExplicitly( pChunk->identifier(),
				chunk->mapping() );

			// Load the chunk's overlappers
			if (pChunk->loaded() && pChunk->isOutsideChunk())
			{
				const ChunkOverlappers::Overlappers& overlappers = 
					ChunkOverlappers::instance( *pChunk ).overlappers();

				ChunkOverlappers::Overlappers::const_iterator iter;
				for (iter = overlappers.begin();
					iter != overlappers.end();
					++iter)
				{
					ChunkOverlapperPtr pChunkOverlapper = *iter;
					MF_ASSERT( pChunkOverlapper.exists() );

					Chunk* pOverlappingChunk =
						pChunkOverlapper->pOverlappingChunk();

					// Load the overlapping chunk
					if (!pOverlappingChunk->loading() &&
						!pOverlappingChunk->loaded())
					{
						ChunkManager::instance().loadChunkExplicitly(
							pOverlappingChunk->identifier(),
							pOverlappingChunk->mapping() );
					}
				}
			}
		}

		// Recurse chunks
		chunksToLoad.clear();

		for (ChunkBoundaries::iterator bit = chunk->joints().begin();
			bit != chunk->joints().end(); ++bit)
		{
			for (ChunkBoundary::Portals::iterator ppit =
				(*bit)->boundPortals_.begin();
				ppit != (*bit)->boundPortals_.end(); ++ppit)
			{
				ChunkBoundary::Portal* pit = *ppit;

				if (pit->hasChunk())
				{
					chunksToLoad.insert( pit->pChunk );
				}
			}
		}

		for (it = chunksToLoad.begin(); it != chunksToLoad.end(); ++it)
		{
			Chunk* pChunk = *it;
			MF_ASSERT( pChunk );

			ChunkProcessorManager::loadNeighbourChunkInternal(pChunk,
				portalDepth - 1,
				chunkDepths );
		}
	}
}


/**
 *	Check if a chunk is ready to be processed.
 *	ie. it has been loaded.
 *	@param chunk the chunk to check.
 *	@param portalDepth
 */
bool ChunkProcessorManager::isChunkReadyToProcess( Chunk* chunk,
	int portalDepth )
{
	BW_GUARD;

	Chunk::nextMark();

	return ChunkProcessorManager::ensureNeighbourChunkLoadedInternal(
		chunk, portalDepth );
}


/**
 *	Load a chunk for processing.
 *	@param chunk the chunk to load.
 *	@param portalDepth
 */
bool ChunkProcessorManager::loadChunkForProcessing( Chunk* chunk,
	int portalDepth )
{
	BW_GUARD;

	BW::map<Chunk*, int> chunkDepths;

	ChunkManager::instance().cameraSpace()->focus( chunk->centre() );

	ChunkProcessorManager::loadNeighbourChunkInternal(
		chunk, portalDepth, chunkDepths );

	ChunkManager::instance().cameraSpace()->focus( chunk->centre() );

	return true;
}

/**
*	spread invalidate for a chunk, add nextMark, 
*
*	@param primary			the chunk started with
*	@param expandOnGridX	how many chunks in the x-direction to check for
*								neighbours.
*	@param expandOnGridZ	how many chunks in the z-direction to check for
*								neighbours.
*	@param cacheIndex		the index of the cache we want to update
*	@param pChangedItem		the changed item that caused the chunk changed,
*								NULL means it's other reason caused the chunk changed
*/
void ChunkProcessorManager::spreadInvalidate(
		Chunk* primary,
		int expandOnGridX, 
		int expandOnGridZ, 
		int cacheIndex ,
		EditorChunkItem* pChangedItem )
{
	BW_GUARD;

	Chunk::nextMark();

	this->spreadInvalidateInternal( primary, 
		expandOnGridX, expandOnGridZ, cacheIndex,
		pChangedItem );
}


/**
*	spread invalidate for a chunk, check nextMark, 
*
*	@param primary			the chunk started with
*	@param expandOnGridX	how many chunks in the x-direction to check for
*								neighbours.
*	@param expandOnGridZ	how many chunks in the z-direction to check for
*								neighbours.
*	@param cacheIndex		the index of the cache we want to update
*	@param pChangedItem		the changed item that caused the chunk changed,
*								NULL means it's other reason caused the chunk changed
*/
void ChunkProcessorManager::spreadInvalidateInternal(
	Chunk* primary,
	int expandOnGridX, 
	int expandOnGridZ, 
	int cacheIndex ,
	EditorChunkItem* pChangedItem )
{
	BW_GUARD_PROFILER( SpreadInvalidate );

	int16 gridX, gridZ;

	if (primary->isOutsideChunk())
	{
		MF_VERIFY( primary->mapping()->gridFromChunkName(
			primary->identifier(), gridX, gridZ ) );

		BW::set<Chunk*> chunks;
		for (int16 z = gridZ - expandOnGridZ;
			z <= gridZ + expandOnGridZ; ++z)
		{
			for (int16 x = gridX - expandOnGridX;
				x <= gridX + expandOnGridX; ++x)
			{
				BW::string chunkName =
					primary->mapping()->outsideChunkIdentifier( x, z );

				if (chunkName.empty())
				{
					continue;
				}

				Chunk* chunk = ChunkManager::instance().
					findChunkByName( chunkName, primary->mapping() );

				if (chunk->traverseMark() != Chunk::s_nextMark_)
				{
					chunk->traverseMark( Chunk::s_nextMark_ );

					if (ChunkCache* cc = chunk->cache( cacheIndex ))
					{
						cc->invalidate( this, false, pChangedItem );

						unsavedChunks_.add( chunk );
					}
					chunks.insert( chunk );
				}
			}

			for (BW::set<Chunk*>::iterator iter = chunks.begin();
				iter != chunks.end(); ++iter)
			{
				if ((*iter)->loaded())
				{
					const ChunkOverlappers::Overlappers& overlappers = 
						ChunkOverlappers::instance( **iter ).overlappers();

					for (ChunkOverlappers::Overlappers::const_iterator iter = 
						overlappers.begin();
						iter != overlappers.end(); ++iter)
					{
						Chunk* chunk = (*iter)->pOverlapper();
						if (chunk->traverseMark() != Chunk::s_nextMark_)
						{
							chunk->traverseMark( Chunk::s_nextMark_ );

							if (ChunkCache* cc = chunk->cache( cacheIndex ))
							{
								cc->invalidate( this, false, pChangedItem );

								unsavedChunks_.add( chunk );
							}
						}
					}
				}
			}
		}
	}
	else
	{
		Chunk* overlapped[4];

		primary->collectOverlappedOutsideChunksForShell( overlapped );

		for (int i = 0; i < 4; ++i)
		{
			if (Chunk* chunk = overlapped[i])
			{
				this->spreadInvalidate( chunk,
					expandOnGridX,
					expandOnGridZ,
					cacheIndex,
					pChangedItem );
			}
		}
	}
}


/**
 *	spread invalidate for a chunk, check the nextMark, 
 *
 *	@param chunk			the chunk started with
 *	@param portalDepth		how deep we want to traverse
 *	@param cacheIndex		the index of the cache we want to update
 *	@param pChangedItem		the changed item that caused the chunk changed,
 *								NULL means it's other reason caused the chunk changed
*/
void ChunkProcessorManager::spreadInvalidateInternal( 
		Chunk* chunk,
		int portalDepth,
		int cacheIndex,
		EditorChunkItem* pChangedItem )
{
	BW_GUARD_PROFILER( SpreadInvalidate );

	if (chunk->traverseMark() != Chunk::s_nextMark_)
	{
		chunk->traverseMark( Chunk::s_nextMark_ );

		if (ChunkCache* cc = chunk->cache( cacheIndex ))
		{
			cc->invalidate( this, false, pChangedItem );

			unsavedChunks_.add( chunk );
		}

		if (portalDepth != 0)
		{
			for (ChunkBoundaries::iterator bit = chunk->joints().begin();
				bit != chunk->joints().end(); ++bit)
			{
				for (ChunkBoundary::Portals::iterator ppit =
					(*bit)->boundPortals_.begin();
					ppit != (*bit)->boundPortals_.end(); ++ppit)
				{
					ChunkBoundary::Portal* pit = *ppit;

					if (pit->hasChunk())
					{
						this->spreadInvalidateInternal(
							pit->pChunk, portalDepth - 1, cacheIndex, pChangedItem );
					}
				}
			}
		}
	}
}


/**
 *	spread invalidate for a chunk, add nextMark, 
 *
 *	@param chunk			the chunk started with
 *	@param portalDepth		how deep we want to traverse
 *	@param cacheIndex		the index of the cache we want to update
 *	@param pChangedItem		the changed item that caused the chunk changed,
 *								NULL means it's other reason caused the chunk changed
*/
void ChunkProcessorManager::spreadInvalidate( 
		Chunk* chunk,
		int portalDepth,
		int cacheIndex,
		EditorChunkItem* pChangedItem )
{
	BW_GUARD;

	Chunk::nextMark();

	this->spreadInvalidateInternal( chunk, portalDepth, cacheIndex, pChangedItem );
}


/**
 *	Do main thread chunk processing tasks.
 */
void ChunkProcessorManager::processMainthreadTask()
{
	BW_GUARD_PROFILER( ProcessChunks );

	RecursiveMutexHolder lock( dirtyChunkListsMutex_ );

	for ( size_t i = 0; i < dirtyChunkLists_.size(); ++i )
	{
		DirtyChunkList& chunks = dirtyChunkLists_[ i ];

		if ( chunks.requireProcessingInMainThread() )
		{
			for ( DirtyChunkList::iterator iter = chunks.begin();
				iter != chunks.end();
				++iter )
			{
				Chunk* pChunk = iter->second;

				if ( pChunk->isBound() && this->isChunkEditable( pChunk ) )
				{
					for ( int i = 0; i < ChunkCache::cacheNum(); ++i )
					{
						if ( ChunkCache* cc = pChunk->cache( i ) )
						{
							// Start processing
							if ( cc->dirty() &&
								cc->requireProcessingInMainThread() &&
								cc->readyToCalculate( this ) &&
								!cc->isBeingCalculated() )
							{
								cc->recalc( this, *this );
								// The iterator has been invalidated here.
								return;
							}
						}
					}
				}
			}
		}
	}
}


/**
 *	Do background chunk processing tasks.
 *	@param cameraPosition the position for finding the nearest chunks.
 *	@param excludedChunks set of chunks to exclude from processing.
 */
void ChunkProcessorManager::processBackgroundTask(
	const Vector3& cameraPosition,
	const BW::set<Chunk*>& excludedChunks )
{
	BW_GUARD_PROFILER( ProcessChunks );

	Chunk* nearestChunk = NULL;
	float distance = 99999999.f;
	RecursiveMutexHolder lock( dirtyChunkListsMutex_ );

	for ( size_t i = 0; i < dirtyChunkLists_.size(); ++i )
	{
		DirtyChunkList& chunks = dirtyChunkLists_[ i ];

		if ( chunks.requireProcessingInBackground() )
		{
			for ( DirtyChunkList::iterator iter = chunks.begin();
				iter != chunks.end();
				++iter)
			{
				float dist;

				if ( iter->second->isOutsideChunk() )
				{
					float distX = iter->second->centre().x - cameraPosition.x;
					float distZ = iter->second->centre().z - cameraPosition.z;

					dist = distX * distX + distZ * distZ;
				}
				else
				{
					dist = ( iter->second->centre() - cameraPosition )
						.lengthSquared();
				}

				if ( dist < distance &&
					iter->second->isBound() &&
					this->isChunkEditable( iter->second ) &&
					excludedChunks.find( iter->second ) == excludedChunks.end()
					)
				{
					for ( int i = 0; i < ChunkCache::cacheNum(); ++i )
					{
						if ( ChunkCache* cc = iter->second->cache( i ) )
						{
							if ( cc->dirty() &&
								cc->requireProcessingInBackground() &&
								cc->readyToCalculate( this ) &&
								!cc->isBeingCalculated() )
							{
								distance = dist;
								nearestChunk = iter->second;
							}
						}
					}
				}
			}
		}
	}

	if ( nearestChunk )
	{
		for ( int i = 0; i < ChunkCache::cacheNum(); ++i )
		{
			if ( ChunkCache* cc = nearestChunk->cache( i ) )
			{
				// Start processing
				if ( cc->dirty() &&
					cc->requireProcessingInBackground() &&
					cc->readyToCalculate( this ) &&
					!cc->isBeingCalculated() )
				{
					cc->recalc( this, *this );
				}
			}
		}

	}
}

/**
 *	Process a dirty chunk and then add it to the unsaved chunks list.
 *	@param chunk to process, must be bound.
 */
bool ChunkProcessorManager::processChunk( Chunk* chunk )
{
	BW_GUARD;

	if (!chunk->loaded())
	{
		// Make sure all cached chunks items are removed to free memory.
		AmortiseChunkItemDelete::instance().purge();

		ChunkManager::instance().loadChunkNow( chunk );
		ChunkManager::instance().checkLoadingChunks();
	}

	// Make sure all cached chunks items are removed to free memory.
	AmortiseChunkItemDelete::instance().purge();

	ChunkManager::instance().cameraSpace()->focus( chunk->centre() );

	if (chunk->isBound())
	{
		bool dirty = true;

		do
		{
			dirty = false;

			for (int i = 0; i < ChunkCache::cacheNum(); ++i)
			{
				if (ChunkCache* cc = chunk->cache( i ))
				{
					if (cc->dirty() &&
						(cc->requireProcessingInBackground() ||
						 cc->requireProcessingInMainThread()))
					{
						dirty = true;

						// Start processing
						if (!cc->isBeingCalculated() &&
							cc->loadChunkForCalculate( this ) &&
							cc->readyToCalculate( this ))
						{
							cc->recalc( this, *this );
						}
					}
				}
			}

			TaskManager::tick();
		}
		while (dirty);


		unsavedChunks_.add( chunk );

		while (this->hasTasks())
		{
			TaskManager::tick();
		}

		return true;
	}
	else
	{
		ERROR_MSG( "chunk %s is marked as dirty, but isn't bound!\n", 
			chunk->identifier().c_str() );
	}

	return false;
}

// progressive unloading description
//
// sort chunks by x and z
// to be able to process them line by line
// current implementation
// legend:
// x - chunk we are done with which we can unload
// o - chunk we are currently processing
// . - chunks are in memory
// # - chunk we are loading atm
// * - chunks we didn't touch yet
// 
// A: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// B: ...................................
// C: .......oooooooooooo.###############
// D: ....................###############
// E: ***********************************
//
// we only need to keep 3 lines of chunks in memory
// when code finishes the chunk we check what z line it is on
// if it's the next z line we can unload previous lines
// in this example when we finish first chunk at line D we can unload
// lines A and B
// result of unloading should look like this
//
// A: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// B: xxxxxxxxxxxxxxxxxxxxxxx............
// C: ........................ooooooooooo
// D: o..................................
// E: ..###############******************
// F: #################******************
//
// note that active chunks and their neighbors are locked in memory
// we only unload chunks we don't need
// so some chunks at line B will remain in memory
// and will be unloaded on the next unload call
//
// TODO: can make it slightly more memory efficient
// by creating 3x3 process clusters and unload more often
// we can't use indoor chunk grid position cause it's meaningless
// so we check indoor chunks overlappers positions instead
// 
bool ChunkSorterPred (Chunk*& chunk0, Chunk*& chunk1)
{
	GeometryMapping* map0 = chunk0->mapping();
	GeometryMapping* map1 = chunk1->mapping();

	int gridSizeY0 = map0->maxGridY() - map0->minGridY();
	int gridSizeY1 = map1->maxGridY() - map1->minGridY();

	int chunk0Pos = (int)chunk0->x() + (int)chunk0->z() * gridSizeY0;
	int chunk1Pos = (int)chunk1->x() + (int)chunk1->z() * gridSizeY1;

	return chunk0Pos < chunk1Pos;

}

/**
 *	Save a list of chunks.
 *	Process dirty chunks before saving them.
 *	@param chunksToSave the list of chunks to save
 *	@param chunkSaver chunk saving implementation to use.
 *	@param thumbnailSaver chunk thumbnail saver implementation to use.
 *	@param progress progress indicator to update, can be NULL.
 */
bool ChunkProcessorManager::saveChunks( const ChunkIdSet& chunksToSave,
	IChunkSaver& chunkSaver,
	IChunkSaver& thumbnailSaver,
	Progress* progress, bool bProgressiveUnloading )
{

	BW_GUARD_PROFILER(SaveChunks);
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

	ChunkVector pendingChunks;
	ChunkVector pendingIndoorChunks;

	ScopedLockedChunkHolder activeChunks( this );

	// -- Get list of already loaded chunks, these will stay loaded during progressive unloading
	BW::set<Chunk*> keepLoadedChunks;
	{
		ChunkIdSet::const_iterator iter;

		for (iter = chunksToSave.begin();
			iter != chunksToSave.end();
			++iter)
		{
			if (geometryMapping()->chunkFileExists( *iter ) &&
				!cleanChunkList_->isClean( *iter ))
			{
				Chunk* pChunk = ChunkManager::instance().findChunkByName(
					*iter, geometryMapping(), false );
				if (pChunk)
				{
					keepLoadedChunks.insert( pChunk );
				}
			}
		}

		// if chunks are locked they are likely still needed for some
		// background processing task so make sure we keep those loaded.
		const LockedChunks & locked = allLockedChunks();
		for (BW::map<Chunk*, int>::const_iterator lockedIt = locked.cbegin(); 
			lockedIt != locked.cend(); ++lockedIt)
		{
			keepLoadedChunks.insert( lockedIt->first );
		}
	}

	// -- Sync timestamps in clean chunk list
	cleanChunkList_->sync( progress );
	if ( progress && progress->isCancelled() )
	{
		cleanChunkList_->save();
		return true;
	}

	// -- Find dirty editable chunks to save
	this->findDirtyChunks( chunksToSave, pendingChunks, pendingIndoorChunks );

	// sort pending outdoor chunks based on grid x and z positions
	std::sort( pendingChunks.begin(), pendingChunks.end(), ChunkSorterPred );

	// -- Process chunks, generate a clean list to save
	const size_t maxJobs = numRunningThreads() + 1;
	const size_t totalTasks = pendingChunks.size() + pendingIndoorChunks.size();

	if (progress)
	{
		progress->name( "Saving chunks" );
		progress->length( static_cast<float>( totalTasks ) );
	}

	// Move dirty chunks to active chunks and process them
	// Note: if one of the processors is malfunctioning, we might end up
	// with an infinite loop here!

	int currentZ = -0x7FFFFFFF;

	while (!pendingChunks.empty() || !activeChunks.empty())
	{
		this->tick();
		this->tickSavingChunks();
		if (progress && progress->isCancelled())
		{
			break;
		}

		// Set progress
		if (progress)
		{
			progress->set( static_cast<float>(
				totalTasks - pendingChunks.size() -
				pendingIndoorChunks.size() - activeChunks.size() ) );
		}

		// Too many bound chunks, unload some

		// naive implementation
		// loading and unloading large number of chunks
		// causes VA space fragmentation because D3D keeps VA space reserved
		// after you release a d3d resource
		// as a result this leads to disparity between reported free memory and
		// available virtual address space
		// might be fine if you do it once or twice
		if (this->needToUnload())
		{
			this->unloadChunks();
		}

		// Give some time to other threads
		if (this->numBgTasksLeft() > maxJobs)
		{
			Sleep( 1 );

			continue;
		}

		// Move some more dirty chunks into active processing list
		// If there is enough time for them
		if (activeChunks.size() < maxJobs && !pendingChunks.empty())
		{
			this->takeChunkLock( pendingChunks, activeChunks );
		}

		// Process active chunks
		bool allDirty = this->processActiveChunks( activeChunks );

		// Some of the chunks are clean - save them
		if (!allDirty)
		{
			this->saveAllUnsaved( chunkSaver, thumbnailSaver );
		}

		// checks pending indoor chunks to see
		// if any of them are referenced by completed outside chunks
		// move referenced indoor chunks to the regular pending list
		this->processPendingIndoorChunks( activeChunks, pendingChunks, pendingIndoorChunks);

		// alexanderr's note:
		// ideally we want to turn on aggressive unloading if we have a lot of chunks to save
		// usually this is the case when WE or offline processor
		// processes all chunks at once
		// for 2.1  decision was made to keep WE as is to reduce the risk
		// WE doesn't have unloadChunksOutsideOfGridBox implemented
		if (bProgressiveUnloading)
		{
			// update currentZ line
			bool bNextScanline = false;
			for (ChunkSet::iterator iter = activeChunks.begin();
				iter != activeChunks.end() && !bNextScanline; ++iter)
			{
				Chunk* chunk = *iter;

				if (chunk->isOutsideChunk() &&
					chunk->isBound() && !chunk->dirty())
				{
					// chunk is clean, update currentZ
					if (chunk->z() > currentZ)
					{
						// next scanline
						currentZ = chunk->z();
						bNextScanline = true;
					}
				}
			}

			if (bNextScanline)
			{
				// unload all chunks which have grid z less than currentZ - 1
				this->unloadChunksOutsideOfGridBox( geometryMapping()->minGridX(),
													geometryMapping()->maxGridX(),
													currentZ - 1,
													geometryMapping()->maxGridY(),
													keepLoadedChunks);
			}
		}

		// remove finished active chunks
		this->removeFinishedChunks( activeChunks );
	}

	// report editable and unreferenced indoor chunks
	for (ChunkVector::iterator indoorIter = pendingIndoorChunks.begin();
		indoorIter != pendingIndoorChunks.end(); ++indoorIter)
	{
		Chunk* indoorChunk = *indoorIter;

		// Emit a warning message if chunk is editable and unreferenced by outdoor chunks
		// so the user has a chance to fix this situation by manually deleting 
		// the chunk files.
		WARNING_MSG( "Unreferenced indoor chunk '%s' found, "
			"consider manually deleting.\n", 
			indoorChunk->identifier().c_str() );
	}
	// Processing done - save clean chunks
	cleanChunkList_->save();

	return true;
}

/**
 *	Find dirty chunks and put them in a list.
 *	
 *	@param chunksToSave the set of chunks to search.
 *	@param dirtyOutdoorChunks the set of dirty outdoor chunks to return.
 *	@param dirtyIndoorChunks the set of dirty indoor chunks to return.
 */
void ChunkProcessorManager::findDirtyChunks(
	const ChunkIdSet& chunksToSave,
	ChunkVector& dirtyOutdoorChunks,
	ChunkVector& dirtyIndoorChunks )
{
	BW_GUARD;

	ChunkIdSet::const_iterator iter;

	for (iter = chunksToSave.begin();
		iter != chunksToSave.end();
		++iter)
	{
		if (geometryMapping()->chunkFileExists( *iter ) &&
			!cleanChunkList_->isClean( *iter ))
		{
			Chunk* pChunk = ChunkManager::instance().findChunkByName(
				*iter, geometryMapping() );

			if (this->isChunkEditable( pChunk ))
			{
				if (pChunk->isOutsideChunk())
				{
					dirtyOutdoorChunks.push_back( pChunk );
				}
				else
				{
					dirtyIndoorChunks.push_back( pChunk );
				}
			}
		}
	}
}
/**
 *	Iterate over completed outside chunks and 
 *  see if any of them overlap pending indoor chunks
 *  move referenced indoor chunks to the regular pending list
 *  to be processed
 *	
 *	@param activeChunks the list of active chunks
 *	@param pendingChunks the set of pending chunks
 *	@param pendingIndoorChunks the set of dirty indoor chunks
 */
void ChunkProcessorManager::processPendingIndoorChunks(
	const ScopedLockedChunkHolder& activeChunks,
	ChunkVector& pendingChunks,
	ChunkVector& pendingIndoorChunks )
{
	for (ChunkSet::const_iterator activeIter = activeChunks.begin();
		activeIter != activeChunks.end(); ++activeIter)
	{
		Chunk* activeChunk = *activeIter;
		bool bCompletedChunk = activeChunk->isBound() && !activeChunk->dirty();
		// skip indoor active chunks
		// skip non-completed chunks
		if (bCompletedChunk && activeChunk->isOutsideChunk())
		{
			// iterate over active chunk overlappers
			const ChunkOverlappers::Overlappers& overlappers =
				ChunkOverlappers::instance( *activeChunk ).overlappers();
			ChunkOverlappers::Overlappers::const_iterator overIter;
			for (overIter = overlappers.begin();
				overIter != overlappers.end(); ++overIter)
			{
				// check all pending indoor chunks to see if completed
				// outside chunk overlaps any of them
				const ChunkOverlapperPtr& overlapper = *overIter;
				for (ChunkVector::iterator indoorIter = pendingIndoorChunks.begin();
					indoorIter != pendingIndoorChunks.end();)
				{
					Chunk* indoorChunk = *indoorIter;
					// compare strings because indoor chunk isn't loaded yet
					if (indoorChunk->identifier() == overlapper->overlapperID() )
					{
						// this indoor chunk has at least one overlapper
						// move it to the regular pending list
						pendingChunks.insert( pendingChunks.begin(), indoorChunk );
						indoorIter = pendingIndoorChunks.erase( indoorIter);

					}
					else
					{
						++indoorIter;
					}
				}
			}
		}
	}
}

/**
 *	Remove a chunk from the pending processing list and add it to the
 *	active chunk processing list.
 *	@param pendingChunks the list to get the chunk from.
 *	@param activeChunks the list to place the chunk in.
 */
void ChunkProcessorManager::takeChunkLock(
	ChunkVector& pendingChunks,
	ScopedLockedChunkHolder& activeChunks )
{
	BW_GUARD;

	Chunk* pChunk = *pendingChunks.begin();
	pendingChunks.erase( pendingChunks.begin() );

	this->loadAndLockChunkInMemory( pChunk, activeChunks );
}

/**
 *	Process active chunks.
 *	@param activeChunks the active chunks list.
 *	@return true if all of the active chunks are dirty.
 */
bool ChunkProcessorManager::processActiveChunks(
	ScopedLockedChunkHolder& activeChunks )
{
	BW_GUARD;

	bool allDirty = true;

	for (ChunkSet::iterator iter = activeChunks.begin();
		iter != activeChunks.end();
		++iter)
	{
		Chunk* pChunk = *iter;

		// Check if chunk loading has been completed
		if (pChunk->completed())
		{
			// Process chunk
			this->processActiveChunk( *pChunk );
			// Add up if all of the chunks are dirty
			allDirty &= pChunk->dirty();
		}
	}

	return allDirty;
}

/**
 *	Process active chunk.
 *	@param chunk chunk tp process.
 */
void ChunkProcessorManager::processActiveChunk( Chunk& chunk )
{
	BW_GUARD;

	// Go through all chunks in chunk cache
	for (int i = 0; i < ChunkCache::cacheNum(); ++i)
	{
		// Get chunk from cache
		if (ChunkCache* cc = chunk.cache( i ))
		{
			MF_ASSERT( cc );

			// Start processing
			// Check if chunk is dirty and requires processing
			if ( cc->dirty() &&
				( cc->requireProcessingInBackground() ||
				cc->requireProcessingInMainThread() ) &&
				!cc->isBeingCalculated() )
			{
				// Process chunk
				cc->loadChunkForCalculate( this );

				if ( cc->readyToCalculate( this ) )
				{
					cc->recalc( this, *this );
				}
			}
		}
	}
}

/**
 *	Remove any chunks that have finished processing.
 *	@param activeChunks the set of chunks to remove the finished ones from.
 */
void ChunkProcessorManager::removeFinishedChunks(
	ScopedLockedChunkHolder& activeChunks )
{
	BW_GUARD;

	for (ChunkSet::iterator iter = activeChunks.begin();
		iter != activeChunks.end();)
	{
		// Chunk is still dirty - continue
		if (!(*iter)->isBound() || (*iter)->dirty())
		{
			++iter;
		}
		// Chunk is clean - remove from active processing queue
		else
		{
			iter = activeChunks.erase( iter );
		}
	}
}

/**
 *	Save chunks that are clean.
 *	@param chunkSaver
 *	@param thumbnailSaver
 */
void ChunkProcessorManager::saveAllUnsaved( IChunkSaver& chunkSaver,
	IChunkSaver& thumbnailSaver )
{
	BW_GUARD;

	this->mark();

	unsavedTerrainBlocks_.save();
	unsavedChunks_.save( chunkSaver );
	unsavedCdatas_.filter( unsavedChunks_ );
	unsavedCdatas_.save( thumbnailSaver );

	clearUnsavedData();
}


/**
 *	Check if we have loaded too many chunks and need to unload some.
 *	@return true when unload is required.
 */
bool ChunkProcessorManager::needToUnload() const
{
	BW_GUARD;
	return this->isMemoryLow();
}


/**
 *	Invalidate all editable chunks in a given space.
 *	@param mapping the resource directory of the chunks to invalidate.
 *	@param progress progress indicator to update, can be NULL.
 *	@param invalidateOnlyNavMesh special case option to invalidate only the navmesh
 */
bool ChunkProcessorManager::invalidateAllChunks( GeometryMapping* mapping, Progress* progress,
												bool invalidateOnlyNavMesh )
{
	BW_GUARD;

	BW::string spacePath = mapping->path();
	BW::set<BW::string> chunks = mapping->gatherChunks();

	if (progress)
	{
		progress->name( "Invalidating all chunks" );
		progress->length( (float)chunks.size() );
	}

	double lastTick = stampsToSeconds( timestamp() );

	for( BW::set<BW::string>::iterator iter = chunks.begin();
		iter != chunks.end(); ++iter )
	{
		if (this->isChunkEditable( *iter ))
		{
			if (stampsToSeconds( timestamp() ) - lastTick > 0.1)
			{
				lastTick = stampsToSeconds( timestamp() );

				if (progress && progress->isCancelled())
				{
					return false;
				}
			}

			DataSectionPtr cdata = BWResource::openSection(
				spacePath + (*iter) + ".cdata" );

			if (cdata)
			{
				bool flag = true;
				DataSectionPtr navmeshSec = cdata->openSection( "navmeshDirty", true );
				navmeshSec->setBinary( new BinaryBlock( &flag, sizeof( flag ),
					"BinaryBlock/ChunkProcessorManager" ) );
				navmeshSec->setParent( cdata );
				navmeshSec->save();
				navmeshSec->setParent( NULL );

				if (!invalidateOnlyNavMesh)
				{
					ChunkCleanFlags cf( cdata );
					cf.shadow_ = cf.terrainLOD_ = cf.thumbnail_ = 0;
					cf.save();
				}
				cdata->save();
			}
		}

		if (progress)
		{
			progress->set( (float)std::distance( chunks.begin(), iter ) );
		}
	}

	return true;
}

/**
 * Clear unsaved data.
 * @brief Will clear all unsaved terrain blocks, chunks, and cdatas.
 */
void ChunkProcessorManager::clearUnsavedData()
{
	this->unsavedTerrainBlocks_.clear();
	this->unsavedChunks_.clear();
	this->unsavedCdatas_.clear();
}

/**
 *	Lock a chunk in memory so that it is guaranteed not to be unloaded.
 *	@param chunk the chunk to lock.
 *	@param expandOnGridX how many chunks in the x-direction to check for
 *		neighbours.
 *	@param expandOnGridZ how many chunks in the z-direction to check for
 *		neighbours.
 *	@param chunks
 */
void ChunkProcessorManager::lockChunkInMemory(
	Chunk* chunk,
	int expandOnGridX,
	int expandOnGridZ,
	ScopedLockedChunkHolder& chunkHolder )
{
	BW_GUARD;

	int16 gridX, gridZ;

	// If seed chunk is outdoors
	if (chunk->isOutsideChunk())
	{
		// Expand on X/Z locking neighbours
		MF_VERIFY( geometryMapping()->gridFromChunkName(
			chunk->identifier(), gridX, gridZ ) );

		ChunkSet chunks;

		for (int16 z = gridZ - expandOnGridZ;
			z <= gridZ + expandOnGridZ; ++z)
		{
			for (int16 x = gridX - expandOnGridX;
				x <= gridX + expandOnGridX; ++x)
			{
				BW::string chunkName =
					geometryMapping()->outsideChunkIdentifier( x, z );

				if (chunkName.empty())
				{
					continue;
				}

				Chunk* pChunk =
					ChunkManager::instance().findChunkByName(
					chunkName, geometryMapping() );

				if (chunkHolder.find( pChunk ) == chunkHolder.end())
				{
					chunkHolder.lock( pChunk );
					chunks.insert( chunk );
				}
			}
		}

		// Go through neighbours and lock their overlappers
		for (ChunkSet::iterator iter = chunks.begin();
			iter != chunks.end(); ++iter)
		{
			Chunk* pChunk = *iter;

			const ChunkOverlappers::Overlappers& overlappers = 
				ChunkOverlappers::instance( *pChunk ).overlappers();

			ChunkOverlappers::Overlappers::const_iterator oIter;
			for (oIter = overlappers.begin();
				oIter != overlappers.end(); ++oIter)
			{
				ChunkOverlapperPtr pChunkOverlapper = *oIter;
				MF_ASSERT( pChunkOverlapper.exists() );

				Chunk* pOverlappingChunk =
					pChunkOverlapper->pOverlappingChunk();
				MF_ASSERT( pOverlappingChunk );

				if (chunkHolder.find( pOverlappingChunk ) ==
					chunkHolder.end())
				{
					chunkHolder.lock( pOverlappingChunk );
				}
			}
		}
	}
	else
	{
		Chunk* overlapped[4];

		chunk->collectOverlappedOutsideChunksForShell( overlapped );

		for (int i = 0; i < 4; ++i)
		{
			if (Chunk* o = overlapped[i])
			{
				this->lockChunkInMemory(
					o, expandOnGridX, expandOnGridZ, chunkHolder );
			}
		}
	}
	MF_ASSERT( chunkHolder.count( chunk ) != 0 );
}


/**
 *	Find which chunks to lock in memory for a given chunk and portal depth.
 *	
 *	@param chunk the chunk to lock.
 *	@param expandOnGridX how many chunks in the x-direction to check for
 *		neighbours.
 *	@param expandOnGridZ how many chunks in the z-direction to check for
 *		neighbours.
 *	@param chunks a set of chunks to accumulate into.
 */
void ChunkProcessorManager::lockChunkInMemoryInternal( Chunk* chunk,
	int portalDepth,
	ChunkSet& chunks ) const
{
	BW_GUARD;

	if (chunk->traverseMark() != Chunk::s_nextMark_)
	{
		chunk->traverseMark( Chunk::s_nextMark_ );

		chunks.insert( chunk );

		if (portalDepth != 0)
		{
			for (ChunkBoundaries::iterator bit = chunk->joints().begin();
				bit != chunk->joints().end(); ++bit)
			{
				for (ChunkBoundary::Portals::iterator ppit =
					(*bit)->boundPortals_.begin();
					ppit != (*bit)->boundPortals_.end(); ++ppit)
				{
					ChunkBoundary::Portal* pit = *ppit;

					if (pit->hasChunk())
					{
						this->lockChunkInMemoryInternal(
							pit->pChunk, portalDepth - 1, chunks );
					}
				}
			}
		}
	}
}


/**
 *	Lock a chunk in memory so that it is guaranteed not to be unloaded.
 *	Recursively goes through portals locking neighbours until depth is reached.
 *	
 *	@param chunk the chunk to lock.
 *	@param portalDepth the depth to recurse.
 *	@param chunks the holder to lock the chunks in.
 */
void ChunkProcessorManager::lockChunkInMemory( Chunk* chunk, int portalDepth,
	ScopedLockedChunkHolder& chunkHolder ) const
{
	BW_GUARD;

	Chunk::nextMark();

	// Accumulate list of chunks to lock
	ChunkSet chunks;
	this->lockChunkInMemoryInternal( chunk, portalDepth, chunks );

	// Lock the chunks
	for (ChunkSet::iterator iter = chunks.begin();
		iter != chunks.end(); ++iter)
	{
		Chunk* pChunk = *iter;

		chunkHolder.lock( pChunk );
	}
	MF_ASSERT( chunks.count( chunk ) != 0 );
}


/**
 *	Load a chunk in memory and lock it so that it is guaranteed not to be
 *	unloaded.
 *	@param chunk the chunk to load and lock.
 */
bool ChunkProcessorManager::loadAndLockChunkInMemory( Chunk* chunk,
	ScopedLockedChunkHolder& chunkHolder )
{
	BW_GUARD;

	if (!chunk->loaded() && !chunk->loading() &&
		geometryMapping()->chunkFileExists( chunk->identifier() ))
	{
		ChunkManager::instance().loadChunkExplicitly(
			chunk->identifier(), geometryMapping() );
	}

	if (chunk->loaded() || chunk->loading())
	{
		chunkHolder.lock( chunk );
	}

	return chunk->loaded() || chunk->loading();
}


/**
 *	Do tasks when changing space.
 */
void ChunkProcessorManager::onChangeSpace()
{
	BW_GUARD;

	cleanChunkList_ = new CleanChunkList( *geometryMapping() );
}


/**
 *	Do tasks when a chunk's dirty status is changed.
 *	@param chunk the chunk that changed.
 */
void ChunkProcessorManager::onChunkDirtyStatusChanged( Chunk* chunk )
{
	BW_GUARD;

	cleanChunkList_->update( chunk );
}


/**
*	clean the dirty status for the specified chunk
*	@param pChunk the chunk to be cleaned
*/
void ChunkProcessorManager::cleanChunkDirtyStatus( Chunk* pChunk )
{
	BW_GUARD;
	RecursiveMutexHolder lock( dirtyChunkListsMutex_ );
	dirtyChunkLists_.clean( pChunk->identifier() );
}

/**
 *	Update chunk dirty status.
 *	@param pChunk the chunk to update.
 */
void ChunkProcessorManager::updateChunkDirtyStatus( Chunk* pChunk )
{
	BW_GUARD;

	if (this->isChunkEditable( pChunk ))
	{
		RecursiveMutexHolder lock( dirtyChunkListsMutex_ );

		for (int i = 0; i < ChunkCache::cacheNum(); ++i)
		{
			if (ChunkCache* cc = pChunk->cache( i ))
			{
				if (cc->dirty())
				{
					dirtyChunkLists_[ cc->id() ].dirty( pChunk );

					if (cc->requireProcessingInBackground())
					{
						dirtyChunkLists_[ cc->id() ].
							requireProcessingInBackground( true );
					}

					if (cc->requireProcessingInMainThread())
					{
						dirtyChunkLists_[ cc->id() ].
							requireProcessingInMainThread( true );
					}
				}
				else
				{
					dirtyChunkLists_[ cc->id() ].clean( pChunk->identifier() );
				}
			}
		}

		this->onChunkDirtyStatusChanged( pChunk );
	}
}


/**
 *	Check if a chunk is editable.
 *	@param pChunk the chunk to check.
 *	@return true if chunk is editable.
 */
bool ChunkProcessorManager::isChunkEditable( Chunk* pChunk ) const
{
	BW_GUARD;

	return this->isChunkEditable( pChunk->identifier() );
}


/**
 *	Check if a chunk is editable.
 *	@param chunk the chunk to check.
 *	@return true if chunk is editable.
 */
bool ChunkProcessorManager::isChunkEditable( const BW::string& chunk ) const
{
	return true;
}


/**
 *	Save the clean chunk list.
 */
void ChunkProcessorManager::writeCleanList()
{
	BW_GUARD;

	cleanChunkList_->save();
}


/**
 *	Mark all locked and unsaved chunks.
 */
void ChunkProcessorManager::mark()
{
	BW_GUARD;

	allLockedChunks_.mark();
	unsavedChunks_.mark();
	unsavedCdatas_.mark();
}

/**
 *	Accessor for dirtyChunkLists.
 */
const DirtyChunkLists& ChunkProcessorManager::dirtyChunkLists() const
{
	return dirtyChunkLists_;
}

BW_END_NAMESPACE

// chunk_processor_manager.cpp
