#ifndef EDITOR_CHUNK_THUMBNAIL_CACHE_HPP
#define EDITOR_CHUNK_THUMBNAIL_CACHE_HPP

#include "chunk/chunk.hpp"
#include "chunk/chunk_cache.hpp"

BW_BEGIN_NAMESPACE

class EditorChunkThumbnailCache : public ChunkCache
{
	Chunk& chunk_;
	DataSectionPtr  pThumbSection_;
	bool			thumbnailDirty_;

	void chunkThumbnailMode( bool mode );

public:
	EditorChunkThumbnailCache( Chunk& chunk );

	static void touch( Chunk& chunk );

	virtual bool load( DataSectionPtr, DataSectionPtr cdata );

	virtual void saveCData( DataSectionPtr cdata );

	void loadCleanFlags( const ChunkCleanFlags& cf );
	void saveCleanFlags( ChunkCleanFlags& cf );

	virtual bool dirty();

	virtual const char* id() const { return "Thumbnail"; }

	virtual bool readyToCalculate( ChunkProcessorManager* manager );

	virtual bool recalc( ChunkProcessorManager*, UnsavedList& );

	virtual bool invalidate( ChunkProcessorManager* mgr, bool spread,
					EditorChunkItem* pChangedItem = NULL );

	static Instance<EditorChunkThumbnailCache> instance;

	/** Retrieve the cached thumbnail section */
	DataSectionPtr pThumbSection();

	/** Retrieve the thumbnail as a texture */
	Moo::BaseTexturePtr thumbnail();

	/** Is there a cached thumbnail? */
	bool hasThumbnail() const;
};
BW_END_NAMESPACE

#endif // EDITOR_CHUNK_THUMBNAIL_CACHE_HPP
