#ifndef TERRAIN_TEX_PROJ_UNDO_HPP
#define TERRAIN_TEX_PROJ_UNDO_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/undoredo.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to save/restore the projection of terrain texture 
 *	layer.
 */
class TerrainTexProjUndo : public UndoRedo::Operation
{
public:
	TerrainTexProjUndo
	(
		EditorChunkTerrainPtr			terrain,
		size_t							layerIdx
	);

    virtual void undo();

    virtual bool iseq(const UndoRedo::Operation &oth) const;

private:
	EditorChunkTerrainPtr				terrain_;
	size_t								layerIdx_;
	Vector4								uProj_;
	Vector4								vProj_;
};

BW_END_NAMESPACE

#endif // TERRAIN_TEX_PROJ_UNDO_HPP
