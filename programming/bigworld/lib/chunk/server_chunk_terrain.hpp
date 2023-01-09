#ifndef CHUNK_TERRAIN_HPP
#define CHUNK_TERRAIN_HPP

#include "chunk_item.hpp"
#include "chunk.hpp"
#include "chunk_cache.hpp"
#include "cstdmf/smartpointer.hpp"
#include "terrain/terrain_block_cache.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is the chunk item for a terrain block
 */
class ServerChunkTerrain : public ChunkItem
{
	DECLARE_CHUNK_ITEM( ServerChunkTerrain )

public:
	ServerChunkTerrain();
	~ServerChunkTerrain();

	virtual void	toss( Chunk * pChunk );

	Terrain::BaseTerrainBlockPtr block()	{ return pCacheEntry_->pBlock(); }
	const BoundingBox & bb()		{ return bb_; }

protected:
	ServerChunkTerrain( const ServerChunkTerrain& );
	ServerChunkTerrain& operator=( const ServerChunkTerrain& );

	Terrain::TerrainBlockCacheEntryPtr pCacheEntry_;
	BoundingBox					bb_;		// in local coords

	bool load( DataSectionPtr pSection, Chunk * pChunk );

	void			calculateBB();

	friend class ServerChunkTerrainCache;
};



class ChunkTerrainObstacle;

/**
 *	This class is a one-per-chunk cache of the chunk terrain item
 *	for that chunk (if it has one). It allows easy access to the
 *	terrain block given the chunk, and adds the terrain obstacle.
 */
class ServerChunkTerrainCache : public ChunkCache
{
public:
	ServerChunkTerrainCache( Chunk & chunk );
	~ServerChunkTerrainCache();

	virtual int focus();

	void pTerrain( ServerChunkTerrain * pT );

	ServerChunkTerrain * pTerrain()				{ return pTerrain_; }
	const ServerChunkTerrain * pTerrain() const	{ return pTerrain_; }

	static Instance<ServerChunkTerrainCache>	instance;

private:
	Chunk * pChunk_;
	ServerChunkTerrain * pTerrain_;
	SmartPointer<ChunkTerrainObstacle>	pObstacle_;
};


#ifdef CODE_INLINE
#include "server_chunk_terrain.ipp"
#endif

BW_END_NAMESPACE

#endif // CHUNK_TERRAIN_HPP
