#ifndef NAVMESH_PROCESSOR_HPP
#define NAVMESH_PROCESSOR_HPP


#include "cstdmf/debug.hpp"
#include "cstdmf/watcher.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_overlapper.hpp"
#include "chunk/chunk_processor.hpp"
#include "chunk/chunk_processor_manager.hpp"
#include "chunk/geometry_mapping.hpp"
#include "chunk/scoped_locked_chunk_holder.hpp"

#include "chunk_flooder.hpp"
#include "girth.hpp"
#include "waypoint_generator/waypoint_generator.hpp"

BW_BEGIN_NAMESPACE

class EditorChunkNavmeshCacheBase;

/**
 *	Process/create a navmesh from the collision scene.
 */
class NavmeshProcessor : public ChunkProcessor
{
public:
	NavmeshProcessor(
		EditorChunkNavmeshCacheBase* navmeshCache,
		Chunk& chunk,
		UnsavedList& unsavedList,
		Girths girths,
		ChunkProcessorManager* manager );
	virtual ~NavmeshProcessor();

protected:
	Girths girths_;
	EditorChunkNavmeshCacheBase* navmeshCache_;
	BW::vector<unsigned char> navmesh_;
	Chunk& chunk_;
	UnsavedList& unsavedList_;
	ScopedLockedChunkHolder lockedChunks_;

	ChunkFlooder flooder_;
	WaypointGenerator gener_;

	SimpleMutex mutex_;
	bool bgtaskFinished_;

	virtual bool processInBackground( ChunkProcessorManager& manager );
	virtual bool processInMainThread( ChunkProcessorManager& manager,
		bool backgroundTaskResult );
	virtual void cleanup( ChunkProcessorManager& manager );
};


typedef SmartPointer<NavmeshProcessor> NavmeshProcessorPtr;

BW_END_NAMESPACE

#endif //NAVMESH_PROCESSOR_HPP
