#include "pch.hpp"
#include "worldeditor/gui/pages/chunk_watcher.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This is the ChunkWatcher constructor.
 */
ChunkWatcher::ChunkWatcher():
	image_(),
	minX_(std::numeric_limits<int32>::max()),
	minZ_(std::numeric_limits<int32>::max()),
	numbersValid_(false),
	numLoaded_(0),
	numUnloaded_(0),
	numDirty_(0),
	numCalced_(0)
{
}


/**
 *	This function gets the state of a particular outdoor chunk.
 *
 *  @param x		The x-coordinate of the chunk to get.
 *  @param z		The z-coordinate of the chunk to get.
 *	@returns		The state, not including unloadable status, of the 
 *					requested chunk.
 */
uint32 ChunkWatcher::state(int32 x, int32 z) const
{
	BW_GUARD;

	x -= minX_; z -= minZ_;
	return (image_.get(x, z) & STATE_MASK);
}


/**
 *	This functions sets the state of a particular outdoor chunk.
 *
 *  @param x		The x-coordinate of the chunk to set.
 *  @param z		The z-coordinate of the chunk to set.
 *  @param state	The new state.
 */
void ChunkWatcher::state(int32 x, int32 z, uint32 state)
{
	BW_GUARD;

	numbersValid_ = false;
	x -= minX_; z -= minZ_;
	if (0 <= x && x < (int)image_.width() && 
		0 <= z && z < (int)image_.height())
	{
		uint32 s = (image_.get(x, z) & ~STATE_MASK);
		s |= (state & STATE_MASK);
		image_.set(x, z, s);
	}
	else
	{
		WARNING_MSG( "ChunkWatcher::state: "
			"Chunk grid coords ( %d, %d ) are out of the space's grid.\n",
			x, z );
	}
}


/**
 *	This function gets the unloadable status of a chunk.
 *
 *  @param x		The x-coordinate of the chunk to get.
 *  @param z		The z-coordinate of the chunk to get. 
 *  @returns		True if the chunk is unloadable.
 */
bool ChunkWatcher::canUnload(int32 x, int32 z) const
{
	BW_GUARD;

	x -= minX_; z -= minZ_;
	return (image_.get(x, z) & CANNOT_UNLOAD) == 0;
}


/**
 *	This function sets the unloadable status of a chunk.
 *
 *  @param x		The x-coordinate of the chunk to set.
 *  @param z		The z-coordinate of the chunk to set. 
 *  @param unloadable	True if the chunk is unloadable.
 */
void ChunkWatcher::canUnload(int32 x, int32 z, bool unloadable)
{
	BW_GUARD;

	numbersValid_ = false;
	x -= minX_; z -= minZ_;
	if (0 <= x && x < (int)image_.width() && 
		0 <= z && z < (int)image_.height())
	{
		uint32 s = (image_.get(x, z) & STATE_MASK);
		if (unloadable)
			s &= ~CANNOT_UNLOAD; 
		else
			s |= CANNOT_UNLOAD;
		image_.set(x, z, s);
	}
	else
	{
		WARNING_MSG( "ChunkWatcher::canUnload: "
			"Chunk grid coords ( %d, %d ) are out of the space's grid.\n",
			x, z );
	}
}


/**
 *	This is called when a new space is created.
 *
 *  @param minX			The minimum x-extents of chunks.
 *  @param minZ			The minimum z-extents of chunks.
 *  @param maxX			The maximum x-extents of chunks.
 *  @param maxZ			The maximum z-extents of chunks.
 */
void ChunkWatcher::onNewSpace(int32 minX, int32 minZ, int32 maxX, int32 maxZ)
{
	BW_GUARD;

	// Normalise coordinates:
	if (minX > maxX) std::swap(minX, maxX);
	if (minZ > maxZ) std::swap(minZ, maxZ);

	minX_ = minX;
	minZ_ = minZ;

	image_.resize(maxX - minX + 1, maxZ - minZ + 1);
	image_.fill(UNLOADED);
}


/**
 *	This should be called when a chunk is loaded.
 *
 *  @param x			The x coordinate of the chunk.
 *  @param z			The z coordinate of the chunk.
 */
void ChunkWatcher::onLoadChunk(int32 x, int32 z)
{
	BW_GUARD;

	state(x, z, LOADED);
}


/**
 *	This should be called when a chunk is unloaded.
 *
 *  @param x			The x coordinate of the chunk.
 *  @param z			The z coordinate of the chunk.
 */
void ChunkWatcher::onUnloadChunk(int32 x, int32 z)
{
	BW_GUARD;

	state(x, z, UNLOADED);
}


/**
 *	This should be called when a chunk is marked as dirty (and needs a shadow
 *	recalculation).
 *
 *  @param x			The x coordinate of the chunk.
 *  @param z			The z coordinate of the chunk.
 */
void ChunkWatcher::onDirtyChunk(int32 x, int32 z)
{
	BW_GUARD;

	state(x, z, DIRTY_NEEDS_SHADOW_CALC);
}


/**
 *	This should be called when a chunk's shadow is calculated.
 *
 *  @param x			The x coordinate of the chunk.
 *  @param z			The z coordinate of the chunk.
 */
void ChunkWatcher::onCalcShadow(int32 x, int32 z)
{
	BW_GUARD;

	state(x, z, DIRTYSHADOW_CALCED);
}


/**
 *	This gets the number of loaded chunks.
 *
 *  @return				The number of loaded chunks.
 */
size_t ChunkWatcher::numberLoadedChunks() const
{
	BW_GUARD;

	if (!numbersValid_)
		calculateStats();
	return numLoaded_;
}


/**
 *	This gets the number of unloaded chunks.
 *
 *  @return				The number of unloaded chunks.
 */
size_t ChunkWatcher::numberUnloadedChunks() const
{
	BW_GUARD;

	if (!numbersValid_)
		calculateStats();
	return numUnloaded_;
}


/**
 *	This gets the number of dirty chunks.
 *
 *  @return				The number of dirty chunks.
 */
size_t ChunkWatcher::numberDirtyChunks() const
{
	BW_GUARD;

	if (!numbersValid_)
		calculateStats();
	return numDirty_;
}


/**
 *	This gets the number of calculated chunks.
 *
 *  @return				The number of calculated chunks.
 */
size_t ChunkWatcher::numberCalcedChunks() const
{
	BW_GUARD;

	if (!numbersValid_)
		calculateStats();
	return numCalced_;
}


/**
 *	This updates the stats on the chunks loaded etc.
 */
void ChunkWatcher::calculateStats() const
{
	BW_GUARD;

	numLoaded_		= 0;
	numUnloaded_	= 0;
	numDirty_		= 0;
	numCalced_		= 0;
	for (uint32 z = 0; z < image_.height(); ++z)
	{
		for (uint32 x = 0; x < image_.width(); ++x)
		{
			switch (image_.get(x, z) & STATE_MASK)
			{
			case UNLOADED:
				++numUnloaded_;
				break;
			case LOADED:
				++numLoaded_;
				break;
			case DIRTY_NEEDS_SHADOW_CALC:
				++numDirty_;
				break;
			case DIRTYSHADOW_CALCED:
				++numCalced_;
				break;
			}
		}
	}
	numbersValid_ = true;
}


/**
 *	This gets the minimum x position of the current space.
 *
 *  @returns	The minimum x position of the current space.
 */
int32 ChunkWatcher::minX() const
{
	return minX_;
}


/**
 *	This gets the minimum z position of the current space.
 *
 *  @returns	The minimum z position of the current space.
 */
int32 ChunkWatcher::minZ() const
{
	return minZ_;
}


/**
 *	This gets the maximum x position of the current space.  Note that this is
 *	one past the last chunk.
 *
 *  @returns	The maximum x position of the current space.
 */
int32 ChunkWatcher::maxX() const
{
	return minX_ + image_.width();
}


/**
 *	This gets the maximum z position of the current space.  Note that this is
 *	one past the last chunk.
 *
 *  @returns	The maximum z position of the current space.
 */
int32 ChunkWatcher::maxZ() const
{
	return minZ_ + image_.height();
}
BW_END_NAMESPACE

