#ifndef RECAST_PROCESSOR_HPP
#define RECAST_PROCESSOR_HPP


#include "cstdmf/debug.hpp"
#include "cstdmf/watcher.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_overlapper.hpp"
#include "chunk/chunk_processor.hpp"
#include "chunk/chunk_processor_manager.hpp"
#include "chunk/geometry_mapping.hpp"
#include "chunk/scoped_locked_chunk_holder.hpp"

#include "navigation_recast/recast_generator.hpp"

#include "girth.hpp"

BW_BEGIN_NAMESPACE

class EditorChunkNavmeshCacheBase;


class RecastProcessor : public ChunkProcessor
{
public:
	RecastProcessor(
		EditorChunkNavmeshCacheBase* navmeshCache,
		Chunk* chunk,
		UnsavedList& unsavedList,
		Girths girths,
		ChunkProcessorManager* manager );

	virtual RecastProcessor::~RecastProcessor();

protected:
	virtual bool processInBackground( ChunkProcessorManager& manager );
	virtual bool processInMainThread( ChunkProcessorManager& manager,
		bool backgroundTaskResult );
	virtual void cleanup( ChunkProcessorManager& manager );
	virtual void stop();

	Girths girths_;
	EditorChunkNavmeshCacheBase* navmeshCache_;
	CollisionSceneProviders collisionSceneProvider_;
	RecastGenerator generator_;
	BW::vector<unsigned char> navmesh_;
	Chunk* chunk_;
	UnsavedList& unsavedList_;
	ScopedLockedChunkHolder lockedChunks_;

	SimpleMutex mutex_;
	bool bgtaskFinished_;
};


typedef SmartPointer<RecastProcessor> RecastProcessorPtr;

BW_END_NAMESPACE

#endif //RECAST_PROCESSOR_HPP
