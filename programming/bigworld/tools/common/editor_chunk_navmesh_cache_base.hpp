#ifndef EDITOR_CHUNK_NAVMESH_CACHE_BASE_HPP__
#define EDITOR_CHUNK_NAVMESH_CACHE_BASE_HPP__


#include "chunk/chunk_cache.hpp"
#include "editor_chunk_processor_cache.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This is a chunk cache that caches navmesh information.
 */
class EditorChunkNavmeshCacheBase : public EditorChunkProcessorCache
{
public:

	static const double IDLE_NAV_CALC_TIME;
	static bool s_doWaitBeforeCalculate;

	EditorChunkNavmeshCacheBase( Chunk& chunk )
		: chunk_( chunk ), dirty_( true ), dirtyTime_( 0 )
		{	
			invalidateFlag_ = InvalidateFlags::FLAG_NAV_MESH;
		}
	static void touch( Chunk & chunk );

	virtual bool load( DataSectionPtr, DataSectionPtr );
	void saveChunk( DataSectionPtr chunk );
	void saveCData( DataSectionPtr cdata );

	bool readyToCalculate( ChunkProcessorManager* );
	bool loadChunkForCalculate( ChunkProcessorManager* );

	virtual bool requireProcessingInBackground() const;
	virtual bool invalidate( ChunkProcessorManager* mgr, bool spread,
					EditorChunkItem* pChangedItem = NULL );
	virtual bool dirty();

	bool recalc( ChunkProcessorManager*, UnsavedList& );

	virtual void calcDone(
		ChunkProcessor* processor,
		bool backgroundTaskResult,
		const BW::vector<unsigned char>& navmesh, UnsavedList& unsavedList );

	static Instance<EditorChunkNavmeshCacheBase> instance;

protected:
	BW::vector<unsigned char> navmesh_;
	Chunk& chunk_;
	uint64 dirtyTime_;

	const char* id() const { return "NavMesh"; }

private:
	bool dirty_;
};


BW_END_NAMESPACE
#endif// EDITOR_CHUNK_NAVMESH_CACHE_BASE_HPP__
