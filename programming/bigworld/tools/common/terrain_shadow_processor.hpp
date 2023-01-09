#ifndef TERRAIN_SHADOW_PROCESSOR_HPP
#define TERRAIN_SHADOW_PROCESSOR_HPP

#include "chunk/chunk_processor.hpp"
#include "chunk/scoped_locked_chunk_holder.hpp"
#include "terrain/horizon_shadow_map.hpp"

BW_BEGIN_NAMESPACE

class EditorChunkTerrainCache;
class Chunk;

class TerrainShadowProcessor : public ChunkProcessor
{
public:
	TerrainShadowProcessor(
		EditorChunkTerrainCache* cache,
		ChunkProcessorManager* manager,
		UnsavedList& unsavedList );

	virtual ~TerrainShadowProcessor();

protected:
	bool processShadow();

	virtual bool processInBackground( ChunkProcessorManager& manager );

	virtual bool processInMainThread(
		ChunkProcessorManager& manager,
		bool backgroundTaskResult );

	virtual void cleanup( ChunkProcessorManager& manager );

	void lockChunks( Chunk * pChunk );

	bool processInBackground_; 
	UnsavedList& unsavedList_;
	EditorChunkTerrainCache* cache_;
	Terrain::HorizonShadowMap::HorizonShadowImage image_;
	uint32 width_;
	uint32 height_;
	BW::vector<float> heights_;
	ScopedLockedChunkHolder lockedChunks_;
};

typedef SmartPointer<TerrainShadowProcessor> TerrainShadowProcessorPtr;

BW_END_NAMESPACE
#endif // TERRAIN_SHADOW_PROCESSOR_HPP
