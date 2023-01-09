#ifndef CHUNK_TERRAIN_HPP
#define CHUNK_TERRAIN_HPP

#include "chunk.hpp"
#include "chunk_cache.hpp"
#include "chunk_item.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/guard.hpp"
#include "terrain/base_render_terrain_block.hpp"
#include "terrain/horizon_shadow_map.hpp"

#if UMBRA_ENABLE
#include "umbra_proxies.hpp"
#endif

#ifndef MF_SERVER
#include "fmodsound/fmod_config.hpp"
#include "fmodsound/sound_occluder.hpp"
#endif

BW_BEGIN_NAMESPACE

namespace Terrain
{
	class   BaseTerrainBlock;
	typedef SmartPointer<BaseTerrainBlock> BaseTerrainBlockPtr;
}


const float MAX_TERRAIN_SHADOW_RANGE = 500.f;


/**
 *	This class is the chunk item for a terrain block
 */
class ChunkTerrain : public ChunkItem
{
	DECLARE_CHUNK_ITEM( ChunkTerrain )

public:
	ChunkTerrain();
	~ChunkTerrain();

	virtual CollisionSceneProviderPtr getCollisionSceneProvider( Chunk* ) const;
	virtual void	toss( Chunk * pChunk );
	virtual void	draw( Moo::DrawContext& drawContext );

	virtual uint32 typeFlags() const;

	Terrain::BaseTerrainBlockPtr block()				{ return block_; }
    const Terrain::BaseTerrainBlockPtr block() const	{ return block_; }
	const BoundingBox & bb() const						{ return bb_; }

    void calculateBB();

    static bool outsideChunkIDToGrid( const BW::string& chunkID, 
											int32& x, int32& z );

#if EDITOR_ENABLED
	static Terrain::BaseTerrainBlockPtr loadTerrainBlockFromChunk(
			 Chunk * pChunk );
#endif

	bool doingBackgroundTask() const;

	bool calculateShadows(
		Terrain::HorizonShadowMap::HorizonShadowImage& image,
		uint32 width, uint32 height, const BW::vector<float>& heights );

	virtual void syncInit();

	virtual bool reflectionVisible() { return true; }

protected:
	friend class ChunkTerrainCache;

	bool load( DataSectionPtr pSection, Chunk * pChunk, BW::string* errorString = NULL );
	virtual bool addYBounds( BoundingBox& bb ) const;

	ChunkTerrain( const ChunkTerrain& );
	ChunkTerrain& operator=( const ChunkTerrain& );

	Terrain::BaseTerrainBlockPtr    block_;
	BoundingBox						bb_;		// in local coords

#if FMOD_SUPPORT
	SoundOccluder soundOccluder_;
#endif
};



class ChunkTerrainObstacle;

/**
 *	This class is a one-per-chunk cache of the chunk terrain item
 *	for that chunk (if it has one). It allows easy access to the
 *	terrain block given the chunk, and adds the terrain obstacle.
 */
class ChunkTerrainCache : public ChunkCache
{
public:
	ChunkTerrainCache( Chunk & chunk );
	~ChunkTerrainCache();

	virtual int focus();

	void pTerrain( ChunkTerrain * pT );

	ChunkTerrain * pTerrain()				{ return pTerrain_; }
	const ChunkTerrain * pTerrain() const	{ return pTerrain_; }

	static Instance<ChunkTerrainCache>	instance;

private:
	Chunk * pChunk_;
	ChunkTerrain * pTerrain_;
	SmartPointer<ChunkTerrainObstacle>	pObstacle_;
};


#ifdef CODE_INLINE
#include "chunk_terrain.ipp"
#endif

BW_END_NAMESPACE

#endif // CHUNK_TERRAIN_HPP
