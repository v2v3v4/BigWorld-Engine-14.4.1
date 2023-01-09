#include "pch.hpp"
#include "worldeditor/undo_redo/terrain_tex_proj_undo.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "terrain/terrain_texture_layer.hpp"

BW_BEGIN_NAMESPACE

TerrainTexProjUndo::TerrainTexProjUndo
(
	EditorChunkTerrainPtr			terrain,
	size_t							layerIdx
):
	UndoRedo::Operation(size_t(typeid(TerrainTexProjUndo).name())),
	terrain_(terrain),
	layerIdx_(layerIdx)
{
	BW_GUARD;

	addChunk(terrain_->chunk());
	Terrain::TerrainTextureLayer &layer = 
		terrain_->block().textureLayer(layerIdx_);
	if (layer.hasUVProjections())
	{
		uProj_ = layer.uProjection();
		vProj_ = layer.vProjection();
	}
}


/*virtual*/ void TerrainTexProjUndo::undo()
{
	BW_GUARD;

    UndoRedo::instance().add
    (
        new TerrainTexProjUndo(terrain_, layerIdx_)
    );

	Terrain::TerrainTextureLayer &layer = 
		terrain_->block().textureLayer(layerIdx_);
	if (layer.hasUVProjections())
	{
		layer.uProjection(uProj_);
		layer.vProjection(vProj_);
		WorldManager::instance().chunkTexturesPainted(terrain_->chunk(), false); // let WorldEditor do the LOD textures
	}
}


/*virtual*/ bool TerrainTexProjUndo::iseq(const UndoRedo::Operation &oth) const
{
	BW_GUARD;

	TerrainTexProjUndo const *other = 
		reinterpret_cast<TerrainTexProjUndo const *>(&oth);
	return 
		other->terrain_ == terrain_
		&& 
		other->layerIdx_ == layerIdx_
		&&
		other->uProj_ == uProj_ 
		&&
		other->vProj_ == vProj_;
}
BW_END_NAMESPACE

