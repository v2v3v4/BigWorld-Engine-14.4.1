#ifndef EDITOR_CHUNK_TERRAIN_CACHE_HPP
#define EDITOR_CHUNK_TERRAIN_CACHE_HPP


#include "chunk/chunk_cache.hpp"
#include "terrain/horizon_shadow_map.hpp"

#include "editor_chunk_processor_cache.hpp"

BW_BEGIN_NAMESPACE

class EditorChunkTerrainBase;

/**
 *	This class is a one-per-chunk cache of the chunk terrain item
 *	for that chunk (if it has one). It allows easy access to the
 *	terrain block given the chunk, and adds the terrain obstacle.
 */
class EditorChunkTerrainCache : public EditorChunkProcessorCache
{
	typedef Terrain::HorizonShadowMap::HorizonShadowImage HorizonShadowImage;

public:
	EditorChunkTerrainCache( Chunk & chunk )
		: pChunk_( &chunk ), shadowDirty_( true ), pTerrain_( 0 )
		{
			invalidateFlag_ = InvalidateFlags::FLAG_SHADOW_MAP;
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

	bool requireProcessingInBackground() const;

	bool readyToCalculate( ChunkProcessorManager* );

	bool loadChunkForCalculate( ChunkProcessorManager* );

	virtual bool invalidate(ChunkProcessorManager* mgr, bool spread,
					EditorChunkItem* pChangedItem = NULL );

	bool dirty();

	bool recalc( ChunkProcessorManager*, UnsavedList& );

	bool calcDone( ChunkProcessor* processor, ChunkProcessorManager& manager,
		bool result, const HorizonShadowImage& image,
		UnsavedList& unsavedList );

	const char* id() const { return "Shadow"; }

	static Instance<EditorChunkTerrainCache>	instance;

	static bool enableCalculation();
	static void enableCalculation( bool enable );

	Chunk * pChunk() { return pChunk_; }
protected:
	Chunk* pChunk_;
	bool shadowDirty_;
	EditorChunkTerrainBase* pTerrain_;

private:
	static bool s_enableCalculation_;

};

// for *pChunk, find chunks that may effect *pChunk if they are changed.
void findChunksEffectingChunk(BW::set<Chunk *> & outChunkList, Chunk * pChunk);

BW_END_NAMESPACE
#endif // EDITOR_CHUNK_TERRAIN_CACHE_HPP
