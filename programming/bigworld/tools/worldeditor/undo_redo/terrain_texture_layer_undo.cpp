#include "pch.hpp"
#include "worldeditor/undo_redo/terrain_texture_layer_undo.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "romp/flora.hpp"
#include "moo/png.hpp"
#include "worldeditor/gui/pages/page_terrain_texture.hpp"


DECLARE_DEBUG_COMPONENT2("WorldEditor", 2)

BW_BEGIN_NAMESPACE

//----------------------------------------------------------------------------
// Section: class TerrainTextureLayerUndo
//----------------------------------------------------------------------------

/**
 *  This is the TerrainTextureLayerState constructor.
 *
 *  @param block            The terrain block whose blends should be 
 *                          done/undone.
 *  @param chunk            The chunk that the block belongs too.
 */
TerrainTextureLayerState::TerrainTextureLayerState(
	Terrain::EditorBaseTerrainBlockPtr block,
	ChunkPtr chunk )
	: block_( block )
	, chunk_( chunk )	
{
	BW_GUARD;

    if (block_ != NULL)
    {
        for (size_t i = 0; i < block_->numberTextureLayers(); ++i)
        {
            Terrain::TerrainTextureLayer &ttl = block_->textureLayer(i);
             
            LayerInfo layerInfo;
            layerInfo.textureName_ = ttl.textureName();
			layerInfo.bumpTextureName_ = ttl.bumpTextureName();
			if ( ttl.hasUVProjections() )
			{
				layerInfo.uProjection_ = ttl.uProjection();
				layerInfo.vProjection_ = ttl.vProjection();
			}
            Terrain::TerrainTextureLayerHolder holder(&ttl, true);
			layerInfo.compBlends_ = 
				Moo::compressImage
				(
					ttl.image(), 
					"BinaryBlock/TerrainTextureLayerUndo"
				);
            textureInfo_.push_back(layerInfo);
        }
    }
}


/**
 *	Restores the saved state to the block/chunk
 */
void TerrainTextureLayerState::restore()
{
	BW_GUARD;

    // Now apply our stored change:
    if (block_ != NULL)
    {
        // First remove all the existing layers:
        size_t numLayers = block_->numberTextureLayers();
        for (size_t i = 0; i < numLayers; ++i)
            block_->removeTextureLayer(0);

        // Now add back in the old layers:
        for (size_t i = 0; i < textureInfo_.size(); ++i)
        {
            LayerInfo &info = textureInfo_[i];
            size_t newLayer = block_->insertTextureLayer();
            Terrain::TerrainTextureLayer &ttl = block_->textureLayer(newLayer);
            ttl.textureName(textureInfo_[i].textureName_);
			ttl.bumpTextureName(textureInfo_[i].bumpTextureName_);
            if (ttl.hasUVProjections())
            {
                ttl.uProjection(info.uProjection_);
                ttl.vProjection(info.vProjection_);
            }
			Terrain::TerrainTextureLayer::ImageType img;
			Moo::decompressImage(info.compBlends_, img);
            Terrain::TerrainTextureLayerHolder holder(&ttl, true);
            ttl.image().blit(img);
        }

        // Let the block know that the layers have all changed:
        block_->rebuildCombinedLayers();

	    // reset the flora
	    Flora::floraReset();

        // ...and let everyone know it's changed:
        WorldManager::instance().chunkTexturesPainted(chunk_, false); // let WorldEditor do the LOD textures
        WorldManager::instance().changedTerrainBlock(chunk_, false);
    }
}


/**
 *	operator == that tests if two TerrainTextureLayerState are the same
 */
bool TerrainTextureLayerState::operator==( const TerrainTextureLayerState &oth ) const
{
	BW_GUARD;

	if (block_ != oth.block_ || textureInfo_.size() != oth.textureInfo_.size())
        return false;
    for (size_t i = 0; i < textureInfo_.size(); ++i)
    {
        if 
        (
            textureInfo_[i].textureName_ != oth.textureInfo_[i].textureName_
            ||
            textureInfo_[i].bumpTextureName_ != oth.textureInfo_[i].bumpTextureName_
            ||
            textureInfo_[i].uProjection_ != oth.textureInfo_[i].uProjection_
            ||
            textureInfo_[i].vProjection_ != oth.textureInfo_[i].vProjection_
            ||
            textureInfo_[i].compBlends_.getObject() != oth.textureInfo_[i].compBlends_.getObject()
        )
        {
            return false;
        }
    }
	return true;
}


//----------------------------------------------------------------------------
// Section: class TerrainTextureLayerUndo
//----------------------------------------------------------------------------

/**
 *  This is the TerrainTextureLayerUndo constructor.
 *
 *  @param block            The terrain block whose blends should be 
 *                          done/undone.
 *  @param chunk            The chunk that the block belongs too.
 */
TerrainTextureLayerUndo::TerrainTextureLayerUndo
(
    Terrain::EditorBaseTerrainBlockPtr  block,     
    ChunkPtr                        chunk
):
    UndoRedo::Operation(size_t(typeid(TerrainTextureLayerUndo).name())),
	layerState_( new TerrainTextureLayerState( block, chunk ) )
{
	BW_GUARD;

    addChunk(chunk);
}


/**
 *  This is the TerrainTextureLayerUndo constructor.
 *
 *  @param layerState      TerrainTextureLayerState object containing the state
 *                         that will be used on undo.
 */
TerrainTextureLayerUndo::TerrainTextureLayerUndo
(
    TerrainTextureLayerStatePtr  layerState
):
    UndoRedo::Operation(size_t(typeid(TerrainTextureLayerUndo).name())),
	layerState_( layerState )
{
	BW_GUARD;

	MF_ASSERT( layerState_ != NULL );
    addChunk( layerState_->chunk() );
}


/**
 *  This restores the terrain's texture's blends.
 */
/*virtual*/ void TerrainTextureLayerUndo::undo()
{
	BW_GUARD;

	if (WorldManager::instance().isMemoryLow( true /* test immediately */ ))
	{
		ERROR_MSG( "TerrainTextureLayerUndo::undo: Memory is Low, "
			"failed to undo/redo terrain texture painting on chunk %s\n",
			(layerState_->chunk() ? layerState_->chunk()->identifier().c_str() : "<unknown>" ));
		return;
	}

    // First add the current state of this block to the undo/redo list:
    UndoRedo::instance().add
    (
        new TerrainTextureLayerUndo( layerState_->block(), layerState_->chunk() )
    );

	layerState_->restore();
	PageTerrainTexture::instance()->updatePythonMask();
}


/**
 *  This tests to see if the undo operations are the same.
 */
/*virtual*/ bool TerrainTextureLayerUndo::iseq(const UndoRedo::Operation &oth) const
{
	BW_GUARD;

	const TerrainTextureLayerUndo& othTTL = static_cast<const TerrainTextureLayerUndo&>( oth );

	return *layerState_ == *othTTL.layerState_;
}
BW_END_NAMESPACE

