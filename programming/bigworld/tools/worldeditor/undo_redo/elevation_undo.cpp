#include "pch.hpp"
#include "worldeditor/undo_redo/elevation_undo.hpp"
#include "worldeditor/import/elevation_blit.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "worldeditor/misc/sync_mode.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This function creates an empty ElevationUndoPos.
 *
 *  @param x        The x coordinate.
 *  @param y        The y coordinate.
 */
ElevationUndoPos::ElevationUndoPos
(
    int16   x, 
    int16   y
):
    x_(x), 
    y_(y)
{
}


/**
 *  This is the copy ctor for ElevationUndoPos.  Note that it is shallow and
 *  only the position is copied, not the raw data.
 *
 *  @param other    The ElevationUndoPos to copy from.
 */
ElevationUndoPos::ElevationUndoPos(ElevationUndoPos const &other):
    x_(other.x_), 
    y_(other.y_)
{
	// Don't copy data_ since copying images is slow and takes up
	// memory.  We never use the copied image.
}


/**
 *  This is the ElevationUndoPos dtor.
 */
ElevationUndoPos::~ElevationUndoPos() 
{ 
}


/**
 *  ElevationUndoPos assignment.
 *
 *  @param other    The ElevationUndoPos to copy from.
 *  @returns        *this.
 */
ElevationUndoPos &ElevationUndoPos::operator=(ElevationUndoPos const &other)
{
    if (this != &other)
    {
		BW_GUARD;

        data_.clear();
        x_ = other.x_;
        y_ = other.y_;
    }
    return *this;
}


/**
 *  This is the elevation undo structure.
 *
 *  @param positions    The positions of the affected chunks.
 *                      The positions data should be NULL.
 */
/*explicit*/ 
ElevationUndo::ElevationUndo
(
    ElevationUndoPosList const &positions
) :
    UndoRedo::Operation((size_t)(typeid(ElevationUndo).name())),
    positions_(positions)
{
	BW_GUARD;

    SyncMode chunkStopper;

    for
    (
        ElevationUndoPosList::iterator it = positions_.begin();
        it != positions_.end();
        ++it
    )
    {
        ElevationUndoPos &pos = *it;
        if (TerrainUtils::isEditable(pos.x_, pos.y_))
        {
            Terrain::TerrainHeightMap::ImageType terrainImage;
            TerrainUtils::getTerrain(pos.x_, pos.y_, terrainImage, false);
            if (!terrainImage.isEmpty())
            {
                pos.data_ = terrainImage;
            }
        }
    }
}


/**
 *  This is the elevation import undo operation.  It restores the height
 *  data.
 */
/*virtual*/ void ElevationUndo::undo()
{
	BW_GUARD;

    // Save the current state to the undo/redo stack:
    UndoRedo::instance().add(new ElevationUndo(positions_));

    SyncMode chunkStopper;

    for
    (
        ElevationUndoPosList::iterator it = positions_.begin();
        it != positions_.end();
        ++it
    )
    {
        ElevationUndoPos const &pos = *it;
        if (TerrainUtils::isEditable(pos.x_, pos.y_))
        {
            Terrain::TerrainHeightMap::ImageType terrainImage;
            TerrainUtils::TerrainGetInfo getInfo;
            TerrainUtils::getTerrain
            (
                pos.x_, pos.y_, 
                terrainImage, 
                false,
                &getInfo
            );

            if (!terrainImage.isEmpty() && !pos.data_.isEmpty())
            {
                terrainImage.blit(pos.data_);
                TerrainUtils::setTerrain
                (
                    pos.x_, pos.y_, 
                    terrainImage,
                    getInfo,
                    false
                );
            }
        }
    }
}


/**
 *  This compares this elevation undo operation with another operation.
 *
 *  @returns        false.  They are never the same.
 */
/*virtual*/ bool ElevationUndo::iseq(UndoRedo::Operation const &other) const
{
    return false;
}
BW_END_NAMESPACE

