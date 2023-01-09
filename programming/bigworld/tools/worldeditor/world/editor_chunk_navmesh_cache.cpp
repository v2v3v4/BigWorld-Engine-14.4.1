#include "pch.hpp"

#include "editor_chunk_navmesh_cache.hpp"
#include "common/recast_processor.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/watcher.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_overlapper.hpp"
#include "chunk/geometry_mapping.hpp"

#include "navigation_recast/recast_generator.hpp"
#include "misc/navmesh_view.hpp"

#include "appmgr/options.hpp"

BW_BEGIN_NAMESPACE

int EditorChunkNavmeshCache_token = 0;


ChunkCache::Instance<EditorChunkNavmeshCache>
	EditorChunkNavmeshCache::instance( &EditorChunkNavmeshCacheBase::instance );

EditorChunkNavmeshCache::~EditorChunkNavmeshCache()
{
	bw_safe_delete( navmeshView_ );
}
void EditorChunkNavmeshCache::touch( Chunk& chunk )
{
	BW_GUARD;

	EditorChunkNavmeshCache::instance( chunk );
}


void EditorChunkNavmeshCache::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	bool drawMesh = Options::getOptionInt( "render/misc/drawNavMesh", 0 )
		? true : false;

	if (drawMesh && navmeshView_ &&
		(!EditorChunkItem::hideAllOutside() || !chunk_.isOutsideChunk() ))
	{
		float currentGirthToDraw = 
			Options::getOptionFloat( "render/misc/drawNavMeshGirth", 0.5f );

		navmeshView_->addToChannel(	drawContext,
			chunk_.boundingBox().centre(), currentGirthToDraw );
	}
}


bool EditorChunkNavmeshCache::load( DataSectionPtr chunk, DataSectionPtr cdata )
{
	BW_GUARD;

	navmeshView_ = NULL;

	if (EditorChunkNavmeshCacheBase::load( chunk, cdata ))
	{
		navmeshView_ = new NavmeshView( navmesh_, 1.5f );
	}

	return navmeshView_ != NULL;
}


/**
 *	Do not calculate navmesh in WE.
 *	@return true
 */
bool EditorChunkNavmeshCache::requireProcessingInBackground() const
{
	return false;
}


bool EditorChunkNavmeshCache::recalc( ChunkProcessorManager* manager,
	UnsavedList& unsavedList )
{
	BW_GUARD;

	navmeshView_ = NULL;

	return EditorChunkNavmeshCacheBase::recalc( manager, unsavedList );
}


void EditorChunkNavmeshCache::calcDone( ChunkProcessor* processor,
	bool backgroundTaskResult, const BW::vector<unsigned char>& navmesh,
	UnsavedList& unsavedList )
{
	BW_GUARD;

	if ( ( currentProcessor_ == processor ) && backgroundTaskResult )
	{
		// navmesh_ hasn't been set yet
		navmeshView_ = new NavmeshView( navmesh, 1.5f );
	}

	// If ( currentProcessor_ == processor )
	// sets navmesh_ = navmesh etc
	// then sets currentProcessor_ to NULL - do last
	EditorChunkNavmeshCacheBase::calcDone(
		processor, backgroundTaskResult, navmesh, unsavedList );

}
BW_END_NAMESPACE

