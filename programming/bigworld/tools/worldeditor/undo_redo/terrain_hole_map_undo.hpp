#ifndef TERRAIN_HOLE_MAP_UNDO_HPP
#define TERRAIN_HOLE_MAP_UNDO_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/undoredo.hpp"
#include "terrain/editor_base_terrain_block.hpp"
#include "terrain/terrain_hole_map.hpp"
#include "chunk/chunk.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class can be used to save and restore the hole map of a terrain 
 *  block.
 */
class TerrainHoleMapUndo : public UndoRedo::Operation
{
public:
    TerrainHoleMapUndo(Terrain::EditorBaseTerrainBlockPtr block, ChunkPtr chunk);

    virtual void undo();

    virtual bool iseq( const UndoRedo::Operation & oth ) const;

private:
    Terrain::EditorBaseTerrainBlockPtr  block_;
    ChunkPtr				            chunk_;
    Terrain::TerrainHoleMap::ImageType  holes_;
};

BW_END_NAMESPACE

#endif // TERRAIN_HOLE_MAP_UNDO_HPP
