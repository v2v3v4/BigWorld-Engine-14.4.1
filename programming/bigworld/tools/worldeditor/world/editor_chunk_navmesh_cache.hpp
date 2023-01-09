#ifndef EDITOR_CHUNK_NAVMESH_CACHE_HPP__
#define EDITOR_CHUNK_NAVMESH_CACHE_HPP__


#include "chunk/chunk_cache.hpp"
#include "chunk/chunk_processor.hpp"

#include "common/editor_chunk_navmesh_cache_base.hpp"

BW_BEGIN_NAMESPACE

class NavmeshView;
class EditorChunkNavmeshCache : public EditorChunkNavmeshCacheBase
{
	NavmeshView* navmeshView_;

public:
	EditorChunkNavmeshCache( Chunk& chunk )
		: EditorChunkNavmeshCacheBase( chunk ) {}

	virtual ~EditorChunkNavmeshCache();
	static void touch( Chunk & chunk );

	virtual void draw( Moo::DrawContext& drawContext );
	virtual bool load( DataSectionPtr, DataSectionPtr );

	bool requireProcessingInBackground() const;
	bool recalc( ChunkProcessorManager*, UnsavedList& );
	virtual void calcDone( ChunkProcessor* processor, bool backgroundTaskResult,
		const BW::vector<unsigned char>& navmesh, UnsavedList& unsavedList );

	static Instance<EditorChunkNavmeshCache> instance;
};

BW_END_NAMESPACE

#endif// EDITOR_CHUNK_NAVMESH_CACHE_HPP__
