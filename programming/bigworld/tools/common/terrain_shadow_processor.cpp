#include "pch.hpp"

#include "terrain_shadow_processor.hpp"
#include "editor_chunk_terrain_base.hpp"
#include "editor_chunk_terrain_cache.hpp"
#include "chunk/chunk_processor_manager.hpp"

BW_BEGIN_NAMESPACE

TerrainShadowProcessor::TerrainShadowProcessor(
	EditorChunkTerrainCache* cache,
	ChunkProcessorManager* manager,
	UnsavedList& unsavedList ) :

	cache_( cache ),
	unsavedList_( unsavedList ),
	lockedChunks_( manager ),
	processInBackground_( true )
{
	BW_GUARD;
	lockChunks( cache->pChunk() );

	Terrain::BaseTerrainBlock &b   = cache->pTerrain()->block();
	Terrain::HorizonShadowMap &hsm = b.shadowMap();

	height_ = hsm.height();
	width_ = hsm.width();

	heights_.resize( width_ * height_ );

	float blockSize = b.blockSize();
	for ( int32 z = 0; z < static_cast<int32>( height_ ); ++z )
	{
		for ( int32 x = width_ - 1; x >= 0; --x )		
		{
			float chunkX =
				blockSize *
				static_cast<float>( x ) / static_cast<float>( width_ - 1 );
			float chunkZ =
				blockSize *
				static_cast<float>( z ) / static_cast<float>( height_ - 1 );

			heights_[ z * height_ + x ] = b.heightAt( chunkX, chunkZ );
		}
	}

	Terrain::HorizonShadowMapHolder holder( &hsm, true );
	image_ = hsm.image();
}

TerrainShadowProcessor::~TerrainShadowProcessor()
{
	// Check chunks are unlocked
	if ( !lockedChunks_.empty() )
	{
		ERROR_MSG( "TerrainShadowProcessor is being destroyed when it has "
			"chunks locked\n" );
	}
}

bool TerrainShadowProcessor::processShadow()
{
	return cache_->pTerrain()->calculateShadows(
		image_,
		width_,
		height_,
		heights_ );
}

bool TerrainShadowProcessor::processInBackground(
	ChunkProcessorManager& manager )
{
	BW_GUARD;

	if (!cache_->pTerrain())
	{
		return false;
	}

	// If there is only one background thread available, then it is 
	// considered safe enough to process in the background thread.
	processInBackground_ = manager.numRunningThreads() == 1;
	if (processInBackground_)
	{
		return processShadow();
	}
	else
	{
		return true;
	}
}

bool TerrainShadowProcessor::processInMainThread(
	ChunkProcessorManager& manager,
	bool backgroundTaskResult )
{
	BW_GUARD;

	if (!processInBackground_ && backgroundTaskResult)
	{
		backgroundTaskResult = processShadow();
	}

	return cache_->calcDone( this, manager, backgroundTaskResult,
		image_, unsavedList_ );
}

void TerrainShadowProcessor::cleanup( ChunkProcessorManager& manager )
{
	BW_GUARD;
	ChunkProcessor::cleanup( manager );
	lockedChunks_.clear();
}

void TerrainShadowProcessor::lockChunks( Chunk * pChunk )
{
	BW_GUARD;
	BW::set<Chunk *> chunkList;
	findChunksEffectingChunk( chunkList, pChunk );

	for( BW::set<Chunk *>::const_iterator it = chunkList.cbegin();
		it != chunkList.cend(); ++it )
	{
		lockedChunks_.lock( *it, 1, 1 );
	}
}

BW_END_NAMESPACE

