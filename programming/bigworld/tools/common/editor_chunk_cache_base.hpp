#ifndef EDITOR_CHUNK_CACHE_BASE_HPP
#define EDITOR_CHUNK_CACHE_BASE_HPP


#include "chunk/chunk.hpp"
#include "chunk/chunk_processor_manager.hpp"
#include "chunk/chunk_terrain.hpp"
#include "chunk/unsaved_chunks.hpp"
#include "gizmo/undoredo.hpp"
#include "editor_chunk_processor_cache.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

// We need to specialise EditorChunkCacheBase::instance()
class EditorChunkCacheBase;
template <>
EditorChunkCacheBase & ChunkCache::Instance<EditorChunkCacheBase>::operator()( Chunk & chunk ) const;

/**
 *	This is a chunk cache that caches editor specific chunk information.
 *	It is effectively the editor extensions to the Chunk class.
 *
 *	Note: Things that want to fiddle with datasections in a chunk should
 *	either keep the one they were loaded with (if they're an item) or use
 *	the root datasection stored in this cache, as it may be the only correct
 *	version of it. i.e. you cannot go back and get stuff from the .chunk
 *	file through BWResource as the cache will likely be well stuffed, and
 *	the file may not even be there (if the scene was saved with that chunk
 *	deleted and it's since been undone). i.e. use this datasection! :)
 */
class EditorChunkCacheBase : public EditorChunkProcessorCache
{
public:
	EditorChunkCacheBase( Chunk & chunk );

	virtual bool load( DataSectionPtr pSec, DataSectionPtr pCdata );

	static void touch( Chunk & chunk );

	bool edSave();
	bool edSaveCData();

	/**
	 * Returns an string to identify this ChunkCache uniquely
	 */
	const char* id() const { return "Lighting"; }

	DataSectionPtr	pChunkSection();
	DataSectionPtr	pCDataSection();

	static Instance<EditorChunkCacheBase>	instance;

protected:
	virtual bool saveCDataInternal( DataSectionPtr ds,  
									const BW::string& filename );

	Chunk & chunk_;
	DataSectionPtr	pChunkSection_;
};


class ChunkSaver : public UnsavedChunks::IChunkSaver
{
	virtual bool save( Chunk* chunk )
	{
		return EditorChunkCacheBase::instance( *chunk ).edSave();
	}
	bool isDeleted( Chunk& chunk ) const
	{
		return false;
	}
};


class CdataSaver : public UnsavedChunks::IChunkSaver
{
	virtual bool save( Chunk* chunk )
	{
		return EditorChunkCacheBase::instance( *chunk ).edSaveCData();
	}
	bool isDeleted( Chunk& chunk ) const
	{
		return false;
	}
};

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_CACHE_BASE_HPP
