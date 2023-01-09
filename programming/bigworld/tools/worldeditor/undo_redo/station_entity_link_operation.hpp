#ifndef STATION_ENTITY_LINK_OPERATION_HPP
#define STATION_ENTITY_LINK_OPERATION_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_station.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "gizmo/undoredo.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class saves the station-node link information for an entity with this
 *  property and undoes changes to this link information.
 */
class StationEntityLinkOperation : public UndoRedo::Operation
{
public:
    explicit StationEntityLinkOperation
    (
        EditorChunkEntityPtr        entity
    );

	/*virtual*/ void undo();

    /*virtual*/ bool iseq(UndoRedo::Operation const &other) const;

private:
    StationEntityLinkOperation(StationEntityLinkOperation const &);
    StationEntityLinkOperation &operator=(StationEntityLinkOperation const &);

protected:
    BW::string                 entityNodeID_;
    BW::string                 entityGraphID_;
    EditorChunkEntityPtr        entity_;
};

BW_END_NAMESPACE

#endif // STATION_ENTITY_LINK_OPERATION_HPP
