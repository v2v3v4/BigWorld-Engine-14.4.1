#include "pch.hpp"

#include "editor_chunk_terrain_lod_cache.hpp"
#include "editor_chunk_terrain_base.hpp"

#include "chunk/chunk_clean_flags.hpp"
#include "chunk/chunk_processor_manager.hpp"

BW_BEGIN_NAMESPACE

namespace
{

int terrainLodDisableCount_ = 0;

};

bool EditorChunkTerrainLodCache::load( DataSectionPtr pSec, DataSectionPtr cdata )
{
	BW_GUARD;
	return true;
}


void EditorChunkTerrainLodCache::saveCData( DataSectionPtr cdata )
{
	BW_GUARD;
}

void EditorChunkTerrainLodCache::loadCleanFlags( const ChunkCleanFlags& cf )
{
	BW_GUARD;
	terrainLodDirty_ = cf.terrainLOD_ ? false : true;
}

void EditorChunkTerrainLodCache::saveCleanFlags( ChunkCleanFlags& cf )
{
	BW_GUARD;
	cf.terrainLOD_ = terrainLodDirty_ ? 0 : 1;
}


bool EditorChunkTerrainLodCache::readyToCalculate( ChunkProcessorManager* manager )
{
	BW_GUARD;

	if (terrainLodDisableCount_)
	{
		return false;
	}

	EditorChunkTerrainBase* terrain = pTerrain();

	if (terrain == nullptr)
	{
		return false;
	}
	
	Chunk* chunk = terrain->chunk();

	if (chunk == nullptr)
	{
		return false;
	}

	return chunk->loaded();
}


bool EditorChunkTerrainLodCache::loadChunkForCalculate( ChunkProcessorManager* manager )
{
	BW_GUARD;
	return manager->loadChunkForProcessing( pTerrain()->chunk(), 0, 0 );
}



/**
*	Set the scene for this cache as dirty
*	@param manager			the processor manager.
*	@param spread			not used in this case.
*	@param pChangedItem		the changed item that caused the chunk changed,
*								NULL means it's other reason caused the chunk changed
*	@return true on success.
*/
bool EditorChunkTerrainLodCache::invalidate( 
		ChunkProcessorManager* mgr,
		bool spread,
		EditorChunkItem* pChangedItem /*= NULL*/ )
{
	BW_GUARD;
	if (!pTerrain() || !pTerrain()->chunk()->isOutsideChunk())
	{
		return false;
	}

	terrainLodDirty_ = true;
	pTerrain_->block().dirtyLodTexture();

	mgr->updateChunkDirtyStatus( pTerrain()->chunk() );

	return true;
}


bool EditorChunkTerrainLodCache::requireProcessingInMainThread() const
{
	BW_GUARD;
	return EditorChunkTerrainLodCache::enableCalculation();
}


bool EditorChunkTerrainLodCache::dirty()
{
	BW_GUARD;
	if (!pTerrain() || !pTerrain()->chunk()->isOutsideChunk())
	{
		terrainLodDirty_ = false;

		return false;
	}

	return terrainLodDirty_ || pTerrain()->block().isLodTextureDirty();
}


bool EditorChunkTerrainLodCache::recalc(
	ChunkProcessorManager* mgr, UnsavedList& unsavedList )
{
	BW_GUARD;
	MF_ASSERT( readyToCalculate( mgr ) );

	if (pTerrain_->block().rebuildLodTexture( pTerrain_->chunk()->transform() ))
	{
		terrainLodDirty_ = pTerrain()->block().isLodTextureDirty();
		unsavedList.unsavedTerrainBlocks_.add( &pTerrain_->block() );
		unsavedList.unsavedCdatas_.add( pTerrain_->chunk() );
	}

	mgr->updateChunkDirtyStatus( pTerrain()->chunk() );

	return !pTerrain()->block().isLodTextureDirty();
}

ChunkCache::Instance<EditorChunkTerrainLodCache> EditorChunkTerrainLodCache::instance;


void EditorChunkTerrainLodCache::enable()
{
	++terrainLodDisableCount_;
}


void EditorChunkTerrainLodCache::disable()
{
	--terrainLodDisableCount_;
}

bool EditorChunkTerrainLodCache::s_enableCalculation_ = true;

/* static */ bool EditorChunkTerrainLodCache::enableCalculation()
{
	return EditorChunkTerrainLodCache::s_enableCalculation_;
}

/* static */ void EditorChunkTerrainLodCache::enableCalculation( bool enable )
{
	EditorChunkTerrainLodCache::s_enableCalculation_ = enable;
}

BW_END_NAMESPACE

