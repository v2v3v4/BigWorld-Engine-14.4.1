#ifndef CHUNK_ROW_CACHE_HPP
#define CHUNK_ROW_CACHE_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/misc/sync_mode.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This object helps do processing of chunks across a large space.  You can
 *	use it to step through the rows of a space and force chunks into memory.
 *	As you change the row it processes it unloads unused chunks.  You can
 *	also specifiy, via windowSize, the number of rows above and below the 
 *	current row that should also be loaded.  You still have to access the 
 *	chunks via the usual chunk access mechanisms.
 */
class ChunkRowCache
{
public:
	explicit ChunkRowCache(uint32 windowSize = 0);
	~ChunkRowCache();

	uint32 windowSize() const;

	void row(int32 r);
	int32 row() const;

protected:
	void cacheRow(int32 r);
	void decacheRow(int32 r);

private:
	// Not permitted
	ChunkRowCache(ChunkRowCache const &);
	ChunkRowCache &operator=(ChunkRowCache const &);

private:
	int32			row_;
	uint32			windowSize_;
	SyncMode		syncMode_;
};

BW_END_NAMESPACE

#endif // CHUNK_ROW_CACHE_HPP
