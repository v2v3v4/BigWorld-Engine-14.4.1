#include "pch.hpp"
#include "worldeditor/undo_redo/terrain_height_map_undo.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "romp/flora.hpp"
#include "moo/png.hpp"


DECLARE_DEBUG_COMPONENT2("WorldEditor", 2)

BW_BEGIN_NAMESPACE

/**
 *  This is the TerrainHeightMapUndo constructor.
 *
 *  @param block            The terrain block whose heights should be 
 *                          done/undone.
 *  @param chunk            The chunk that the block belongs too.
 */
TerrainHeightMapUndo::TerrainHeightMapUndo
(
    Terrain::EditorBaseTerrainBlockPtr  block, 
    ChunkPtr							chunk
):
    UndoRedo::Operation(size_t(typeid(TerrainHeightMapUndo).name())),
    block_(block),
    chunk_(chunk)
{
	BW_GUARD;

    addChunk(chunk_);
    if (block_ != NULL)
    {
        Terrain::TerrainHeightMap &thm = block_->heightMap();
        Terrain::TerrainHeightMapHolder holder(&thm, true);
		Terrain::TerrainHeightMap::ImageType &image = thm.image();
		float *heights = new float [image.rawDataSize()];
		image.copyTo(heights);
		BinaryPtr imageData = 
			new BinaryBlock
			(
				heights, 
				image.rawDataSize(),
				"BinaryBlock/TerrainHeightMapUndo"
			);
		bw_safe_delete_array(heights);
		heightsCompressed_ = imageData->compress();
    }
}


/**
 *  This restores the terrain heights.
 */
/*virtual*/ void TerrainHeightMapUndo::undo()
{
	BW_GUARD;

    // First add the current state of this block to the undo/redo list:
    UndoRedo::instance().add(new TerrainHeightMapUndo(block_, chunk_));
    // Now apply our stored change:
    if (heightsCompressed_ != NULL && block_ != NULL)
    {
        Terrain::TerrainHeightMap &thm = block_->heightMap();
		{
			Terrain::TerrainHeightMapHolder holder(&thm, false);
			Terrain::TerrainHeightMap::ImageType &image = thm.image();
			BinaryPtr decompData = heightsCompressed_->decompress();
			image.copyFrom((float const*)decompData->data());
		}
		block_->rebuildNormalMap( Terrain::NMQ_NICE );
    }
	
	EditorChunkTerrain *ect = 
		static_cast<EditorChunkTerrain*>
		(
			ChunkTerrainCache::instance( *chunk_ ).pTerrain()
		);
	if (ect != NULL)
	{
		ect->onTerrainChanged();
	}

	// reset the flora
	Flora::floraReset();

    WorldManager::instance().changedTerrainBlock( chunk_ );
}


/**
 *  This tests to see if the undo operations are the same.
 */
/*virtual*/ bool TerrainHeightMapUndo::iseq(const UndoRedo::Operation &oth) const
{
	BW_GUARD;

	const TerrainHeightMapUndo& othTHM = static_cast<const TerrainHeightMapUndo&>(oth);
    return 
		block_ == othTHM.block_ &&
        heightsCompressed_.getObject() == othTHM.heightsCompressed_.getObject();
}
BW_END_NAMESPACE

