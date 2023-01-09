#ifndef EDITOR_CHUNK_TERRAIN_BASE_HPP
#define EDITOR_CHUNK_TERRAIN_BASE_HPP


#include "terrain/editor_base_terrain_block.hpp"
#include "terrain/terrain_height_map.hpp"
#include "chunk/chunk_terrain.hpp"
#include "editor_chunk_processor_cache.hpp"

BW_BEGIN_NAMESPACE

class UnsavedTerrainBlocks;


/**
 *	This class is the editor version of a chunk item for a terrain block.
 */
class EditorChunkTerrainBase : public ChunkTerrain
{
	DECLARE_EDITOR_CHUNK_ITEM( EditorChunkTerrainBase )
public:
    Terrain::EditorBaseTerrainBlock &block();
    const Terrain::EditorBaseTerrainBlock &block() const;

	virtual void toss( Chunk * pChunk );
};


typedef SmartPointer<EditorChunkTerrainBase>	EditorChunkTerrainBasePtr;
typedef BW::vector<EditorChunkTerrainBasePtr>	EditorChunkTerrainBasePtrs;


BW_END_NAMESPACE
#endif // EDITOR_CHUNK_TERRAIN_BASE_HPP
