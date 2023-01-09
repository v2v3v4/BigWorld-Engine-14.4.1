#ifndef EDITOR_CHUNK_TERRAIN_LOD_CACHE_HPP
#define EDITOR_CHUNK_TERRAIN_LOD_CACHE_HPP

#include "chunk/chunk_cache.hpp"

BW_BEGIN_NAMESPACE

class EditorChunkTerrainBase;

class EditorChunkTerrainLodCache : public ChunkCache
{
public:
	EditorChunkTerrainLodCache( Chunk & chunk ) :
		pTerrain_( NULL ),
		terrainLodDirty_( true )
	{
		invalidateFlag_ = InvalidateFlags::FLAG_TERRAIN_LOD;
	}

	void pTerrain( EditorChunkTerrainBase* pT )
	{
		pTerrain_ = pT;
	}

	EditorChunkTerrainBase* pTerrain()			{ return pTerrain_; }

	bool load( DataSectionPtr pSec, DataSectionPtr cdata );

	void saveCData( DataSectionPtr cdata );

	void loadCleanFlags( const ChunkCleanFlags& cf );
	void saveCleanFlags( ChunkCleanFlags& cf );

	bool readyToCalculate( ChunkProcessorManager* );

	bool loadChunkForCalculate( ChunkProcessorManager* );

	virtual bool invalidate(ChunkProcessorManager* mgr, bool spread,
					EditorChunkItem* pChangedItem = NULL );

	bool requireProcessingInMainThread() const;

	bool dirty();

	bool recalc( ChunkProcessorManager*, UnsavedList& );

	const char* id() const { return "TerrainLod"; }

	static Instance<EditorChunkTerrainLodCache>	instance;
	static void enable();
	static void disable();

	static bool enableCalculation();
	static void enableCalculation( bool enable );

protected:
	EditorChunkTerrainBase* pTerrain_;
	bool terrainLodDirty_;

private:
	static bool s_enableCalculation_;
};


BW_END_NAMESPACE
#endif // EDITOR_CHUNK_TERRAIN_LOD_CACHE_HPP
