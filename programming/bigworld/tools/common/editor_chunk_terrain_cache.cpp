#include "pch.hpp"

#include "chunk/chunk_clean_flags.hpp"
#include "chunk/chunk_terrain.hpp"
#include "chunk/chunk_processor_manager.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/geometry_mapping.hpp"

#include "romp/enviro_minder.hpp"
#include "romp/time_of_day.hpp"

#include "common/terrain_shadow_processor.hpp"
#include "common/space_editor.hpp"

#include "editor_chunk_terrain_cache.hpp"
#include "editor_chunk_terrain_base.hpp"
#include "editor_chunk_terrain_lod_cache.hpp"

BW_BEGIN_NAMESPACE

static void findChunksEffectedByChunk(BW::set<Chunk *> & outChunkList, 
	Chunk * pChunk);

/**
*	Set the scene for this cache as dirty, needs shadow calculation.
*	@param manager			the processor manager.
*	@param spread			which way to expand the scene.
*	@param pChangedItem		the changed item that caused the chunk changed,
*								NULL means it's other reason caused the chunk changed
*	@return true on success.
*/
bool EditorChunkTerrainCache::invalidate(
		ChunkProcessorManager* manager,
		bool spread,
		EditorChunkItem* pChangedItem /*= NULL*/ )
{
	BW_GUARD;
	if (spread)	
	{
		BW::set<Chunk *> chunkList;
		findChunksEffectedByChunk( chunkList, this->pChunk() );

		bool result = true;
		for( BW::set<Chunk *>::const_iterator it = chunkList.cbegin();
			it != chunkList.cend(); ++it )
		{
			manager->spreadInvalidate( *it, 1, 1, instance.index(), pChangedItem );
		}

		return true;
	}

	if (!pTerrain() || !pChunk_->isOutsideChunk())
	{
		return false;
	}

	shadowDirty_ = true;
	currentProcessor_ = NULL;

	manager->updateChunkDirtyStatus( pTerrain()->chunk() );

	return true;
}


bool EditorChunkTerrainCache::dirty()
{
	BW_GUARD;
	if (!pTerrain() || !pChunk_->isOutsideChunk())
	{
		shadowDirty_ = false;
	}

	return shadowDirty_;
}

bool EditorChunkTerrainCache::recalc( ChunkProcessorManager* manager,
	UnsavedList& unsavedList )
{
	BW_GUARD;
	MF_ASSERT( readyToCalculate( manager ) );
	MF_ASSERT( shadowDirty_ );

	if (pTerrain())
	{
		TerrainShadowProcessorPtr task = new TerrainShadowProcessor(
			this, manager, unsavedList );

		currentProcessor_ = task;
		task->process( *manager );

		// record this processor in case we want to
		// cancel back ground ChunkProcessors later
		this->registerProcessor( task );
	}

	return true;
}


bool EditorChunkTerrainCache::calcDone( ChunkProcessor* processor,
	ChunkProcessorManager& manager, bool result, const HorizonShadowImage& image,
	UnsavedList& unsavedList )
{
	BW_GUARD;
	if (currentProcessor_ == processor)
	{
		if (pTerrain() && result)
		{
			Terrain::EditorBaseTerrainBlock &b   = pTerrain()->block();
			Terrain::HorizonShadowMap       &hsm = b.shadowMap();

			if (!hsm.lock( false ))
			{
				return false;
			}

			for (int32 z = 0; z < (int32)hsm.height(); ++z)
			{
				const Terrain::HorizonShadowMap::PixelType* src = image.getRow( z );
				Terrain::HorizonShadowMap::PixelType* dst = hsm.image().getRow( z );
				Terrain::HorizonShadowMap::PixelType* end = dst + hsm.image().width();

				while ( dst != end )
					*dst++ = *src++;
			}

			hsm.unlock();

			shadowDirty_ = false;
			unsavedList.unsavedTerrainBlocks_.add( &b );
			unsavedList.unsavedCdatas_.add( pTerrain()->chunk() );
			SpaceEditor::instance().changedChunk( pTerrain()->chunk(), InvalidateFlags::FLAG_THUMBNAIL );
			manager.updateChunkDirtyStatus( pTerrain()->chunk() );
			manager.chunkShadowUpdated( pTerrain()->chunk() );
		}

		currentProcessor_ = NULL;
	}

	return true;
}


void EditorChunkTerrainCache::loadCleanFlags( const ChunkCleanFlags& cf )
{
	BW_GUARD;
	shadowDirty_ = cf.shadow_ ? false : true;
}


void EditorChunkTerrainCache::saveCleanFlags( ChunkCleanFlags& cf )
{
	BW_GUARD;
	cf.shadow_ = this->dirty() ? 0 : 1;
}


bool EditorChunkTerrainCache::load( DataSectionPtr pSec, DataSectionPtr cdata )
{
	BW_GUARD;
	return true;
}


void EditorChunkTerrainCache::saveCData( DataSectionPtr cdata )
{
	BW_GUARD;
}


bool EditorChunkTerrainCache::readyToCalculate( ChunkProcessorManager* manager )
{
	BW_GUARD;
	if ( isBeingCalculated() ) return false;

	BW::set<Chunk *> chunkList;
	findChunksEffectingChunk( chunkList, this->pChunk() );

	bool result = true;
	for( BW::set<Chunk *>::const_iterator it = chunkList.cbegin();
		it != chunkList.cend(); ++it )
	{
		result &= manager->isChunkReadyToProcess( *it, 1, 1 );
	}

	return result;
}


bool EditorChunkTerrainCache::requireProcessingInBackground() const
{
	BW_GUARD;
	return EditorChunkTerrainCache::enableCalculation();
}


bool EditorChunkTerrainCache::loadChunkForCalculate( ChunkProcessorManager* manager )
{
	BW_GUARD;
	BW::set<Chunk *> chunkList;
	findChunksEffectingChunk( chunkList, this->pChunk() );

	bool result = true;
	for( BW::set<Chunk *>::const_iterator it = chunkList.cbegin();
		it != chunkList.cend(); ++it )
	{
		result &= manager->loadChunkForProcessing( *it, 2, 2 );
	}

	return result;
}


// findRelevantChunks is used to find nearby chunks that may effect each other
// taking into account the shadow direction. It only gives you a minimal set 
// of chunks. It does not cast the shadow of the chunk through the space, it
// only casts a line to find a set of origin chunks to use as a basis in
// other functions. See usage with functions like isChunkReadyToProcess 
// that lets you specify a radius around a chunk.
// 
// chunkDependencies is false : for *pChunk, find chunks that may effect 
// *pChunk if they are changed.
//
// chunkDependencies is true  : for *pChunk, find chunks *pChunk may have an 
// effect on if changed.
static void findRelevantChunks( BW::set<Chunk *> & outChunkList, 
						Chunk * pChunk, bool chunkDependencies )
{
	BW_GUARD;
	MF_ASSERT( pChunk );

	ChunkManager & chunkManager = ChunkManager::instance();
	ChunkProcessorManager & chunkProcessorManager = 
		ChunkProcessorManager::instance();

	GeometryMapping * pMapping = 
		const_cast<GeometryMapping *>(chunkProcessorManager.geometryMapping());
	MF_ASSERT( pChunk->mapping() == pMapping );

	EnviroMinder & enviro = chunkManager.cameraSpace()->enviro();
	Vector3 lightDir = enviro.timeOfDay()->lighting().mainLightDir();
	lightDir.normalise();
	if ( chunkDependencies )
	{
		// reverse the light direction as we are wanting to find the chunks
		// pChunk potentially casts shadows onto. Not technically correct but
		// we only really care about the x and z axis and not about the y.
		lightDir *= -1.f;
	}

	// when counting chunks, we ignore the y direction because that doesn't
	// matter, the chunks are laid out only through the xz plane.
	const Vector2 xzDir( lightDir.x, lightDir.z );
	const float distance = (xzDir * MAX_TERRAIN_SHADOW_RANGE).length();
	const float gridSize = chunkManager.cameraSpace()->gridSize();
	const uint32 numChunks = std::max((uint32)(std::ceil(distance/gridSize)), 1u);
	const Vector3 adjustPos = -lightDir * gridSize;

	// start from the centre of a chunk so maths errors are less likely to
	// effect the chunk we find
	Vector3 curPos = pChunk->centre();
	for( uint32 i = 0; i < numChunks; ++i, curPos += adjustPos )
	{
		ChunkSpacePtr pSpace = pMapping->pSpace();
		int16 gridX = pSpace->pointToGrid( curPos.x );
		int16 gridZ = pSpace->pointToGrid( curPos.z );

		BW::string chunkName = pMapping->
			outsideChunkIdentifier( gridX, gridZ );

		if (chunkName.empty())
		{
			continue;
		}

		Chunk* pCurChunk = ChunkManager::instance().findChunkByName(
			chunkName,
			pMapping, true );

		if ( pCurChunk )
		{
			MF_ASSERT( pCurChunk->mapping() == pMapping );
			outChunkList.insert( pCurChunk );
		}
	}
}

// for *pChunk, find chunks that may effect *pChunk if they are changed.
void findChunksEffectingChunk(BW::set<Chunk *> & outChunkList, Chunk * pChunk)
{
	BW_GUARD;
	return findRelevantChunks( outChunkList, pChunk, false );
}

// for *pChunk, find chunks *pChunk may have an effect on if changed.
static void findChunksEffectedByChunk(BW::set<Chunk *> & outChunkList, 
									  Chunk * pChunk)
{
	BW_GUARD;
	return findRelevantChunks( outChunkList, pChunk, true );
}


ChunkCache::Instance<EditorChunkTerrainCache> EditorChunkTerrainCache::instance;



bool EditorChunkTerrainCache::s_enableCalculation_ = true;

/* static */ bool EditorChunkTerrainCache::enableCalculation()
{
	return EditorChunkTerrainCache::s_enableCalculation_;
}

/* static */ void EditorChunkTerrainCache::enableCalculation( bool enable )
{
	EditorChunkTerrainCache::s_enableCalculation_ = enable;
}
BW_END_NAMESPACE

