#ifndef TERRAIN_TEXTURE_LAYER_UNDO
#define TERRAIN_TEXTURE_LAYER_UNDO


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/undoredo.hpp"
#include "terrain/editor_base_terrain_block.hpp"
#include "terrain/terrain_texture_layer.hpp"
#include "chunk/chunk.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class saves the state of the layers for a terrain block, and
 *	allows restoring the saved state.
 */
class TerrainTextureLayerState : public ReferenceCount
{
public:
    TerrainTextureLayerState
    (
        Terrain::EditorBaseTerrainBlockPtr  block, 
        ChunkPtr                        chunk  
    );

	void restore();

	bool operator==( const TerrainTextureLayerState &oth ) const;

	Terrain::EditorBaseTerrainBlockPtr block() const { return block_; }

	ChunkPtr chunk() const { return chunk_; }

private:
    struct LayerInfo
    {
        typedef Terrain::TerrainTextureLayer::ImageType ImageType;
        typedef SmartPointer<ImageType>             ImageTypePtr;

        BW::string     textureName_;
		BW::string		bumpTextureName_;
        Vector4         uProjection_;
        Vector4         vProjection_;
        BinaryPtr	    compBlends_;
    };

    typedef BW::vector<LayerInfo> LayerInfoVec;

    Terrain::EditorBaseTerrainBlockPtr          block_;
    ChunkPtr				                chunk_;
    LayerInfoVec                            textureInfo_;
};
typedef SmartPointer<TerrainTextureLayerState> TerrainTextureLayerStatePtr;


/**
 *  This class can be used to save and restore the a single texture layer of a 
 *  terrain block.
 */
class TerrainTextureLayerUndo : public UndoRedo::Operation
{
public:
    TerrainTextureLayerUndo
    (
        Terrain::EditorBaseTerrainBlockPtr  block, 
        ChunkPtr                        chunk  
    );

    explicit TerrainTextureLayerUndo
    (
		TerrainTextureLayerStatePtr layerState
    );

    virtual void undo();

    virtual bool iseq(const UndoRedo::Operation &oth) const;

private:
	TerrainTextureLayerStatePtr layerState_;
};

BW_END_NAMESPACE

#endif // TERRAIN_TEXTURE_LAYER_UNDO
