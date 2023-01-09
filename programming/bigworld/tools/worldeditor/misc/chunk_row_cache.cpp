#include "pch.hpp"
#include "worldeditor/misc/chunk_row_cache.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include <limits>

BW_BEGIN_NAMESPACE

namespace
{
	const int32 OUTOFSPACE	= std::numeric_limits<int32>::min();
}


/**
 *	This constructs the ChunkRowCache.
 *
 *	@param windowSize	The number of rows above and below the row of interest 
 *						that should be cached too.
 */
/*explicit*/ ChunkRowCache::ChunkRowCache(uint32 windowSize /*= 0*/):
	row_(OUTOFSPACE),
	windowSize_(windowSize)
{
}


/**
 *	This is the ChunkRowCache destructor.  All cached chunks are unloaded.
 */
ChunkRowCache::~ChunkRowCache()
{
	// Decache everything:
	row(OUTOFSPACE);
}


/**
 *	This gets the window size set in the constructor.
 *
 *	@returns			The number of rows above and below the row of interest 
 *						that are cached too.
 */
uint32 ChunkRowCache::windowSize() const
{
	return windowSize_;
}


/**
 *	This sets the current row of interest.  This row is then loaded as well
 *	as windowSize rows above and below.  Any non-overlapping rows with the 
 *	last row of interest are removed from memory.
 *
 *  @param r			The new row of interest.
 */
void ChunkRowCache::row(int32 r)
{
	BW_GUARD;

	// Remove any cached chunks no longer needed:
	if (row_ != OUTOFSPACE)
	{
		for (int32 i = row_ - (int32)windowSize_; i <= row_ + (int32)windowSize_; ++i)
		{
			if (r == OUTOFSPACE || !(r - (int32)windowSize_ <= i && i <= r + (int32)windowSize_))
				decacheRow(i);
		}
	}

	row_ = r;

	// Cache new chunks that are needed:
	if (row_ != OUTOFSPACE)
	{
		for (int32 i = row_ - (int32)windowSize_; i <= row_ + (int32)windowSize_; ++i)
		{
			cacheRow(i);
		}
	}
}


/**
 *	This gets the current row of interest.
 *
 *	@returns		The currently loaded row.
 */
int32 ChunkRowCache::row() const
{
	return row_;
}


/**
 *	This forces a particular row into memory.
 *
 *  @param r		The row to force into memory.
 */
void ChunkRowCache::cacheRow(int32 r)
{
	BW_GUARD;

	GeometryMapping *mapping = WorldManager::instance().geometryMapping();

	for (int w = mapping->minLGridX(); w <= mapping->maxLGridX(); ++w)
	{
		BW::string chunkName;
		int16 wgx = (int16)w;
		int16 wgy = (int16)r;
		chunkID(chunkName, wgx, wgy);
		if (!chunkName.empty())
		{
			ChunkManager::instance().loadChunkNow
			( 
				chunkName,
				mapping
			);
		}
	}
}


/**
 *	This removes a row from memory.
 *
 *  @param r		The row to unload.
 */
void ChunkRowCache::decacheRow(int32 r)
{
	BW_GUARD;

	GeometryMapping *mapping = WorldManager::instance().geometryMapping();

	for (int w = mapping->minLGridX(); w <= mapping->maxLGridX(); ++w)
	{
		BW::string chunkName;
		int16 wgx = (int16)w;
		int16 wgy = (int16)r;
		chunkID(chunkName, wgx, wgy);
		if (!chunkName.empty())
		{
			Chunk *chunk = 
				ChunkManager::instance().findChunkByName
				(
					chunkName,
					mapping
				);
			if (chunk != NULL && chunk->removable())
			{
				chunk->unbind(false);
				chunk->unload();
			}
		}
	}
}
BW_END_NAMESPACE

