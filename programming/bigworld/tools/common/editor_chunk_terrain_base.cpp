#include "pch.hpp"

#include "editor_chunk_terrain_base.hpp"
#include "chunk/geometry_mapping.hpp"
#include "chunk/chunk_clean_flags.hpp"

#include "editor_chunk_terrain_cache.hpp"
#include "editor_chunk_terrain_lod_cache.hpp"



DECLARE_DEBUG_COMPONENT2( "EditorChunkTerrainBase", 0 );

BW_BEGIN_NAMESPACE

/**
 *  This function accesses the underlying EditorBaseTerrainBlock.
 *
 *  @returns            The underlying EditorBaseTerrainBlock.
 */
Terrain::EditorBaseTerrainBlock &EditorChunkTerrainBase::block()
{
	BW_GUARD;

    return *static_cast<Terrain::EditorBaseTerrainBlock *>(ChunkTerrain::block().getObject());
}


/**
 *  This function accesses the underlying EditorBaseTerrainBlock.
 *
 *  @returns            The underlying EditorBaseTerrainBlock.
 */
Terrain::EditorBaseTerrainBlock const &EditorChunkTerrainBase::block() const
{
	BW_GUARD;

    return *static_cast<Terrain::EditorBaseTerrainBlock const*>(ChunkTerrain::block().getObject());
}


void EditorChunkTerrainBase::toss( Chunk * pChunk )
{
	BW_GUARD;

	if (pChunk_ != NULL)
	{
		EditorChunkTerrainCache::instance( *pChunk_ ).pTerrain( NULL );
		EditorChunkTerrainLodCache::instance( *pChunk_ ).pTerrain( NULL );
	}

	ChunkTerrain::toss( pChunk );

	if (pChunk_ != NULL)
	{
		EditorChunkTerrainCache::instance( *pChunk_ ).pTerrain( this );
		EditorChunkTerrainLodCache::instance( *pChunk_ ).pTerrain( this );
	}
}


// Use macros to write EditorChunkTerrainBase's static create method
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk, &errorString)
IMPLEMENT_CHUNK_ITEM( EditorChunkTerrainBase, terrain, 1 )

BW_END_NAMESPACE

