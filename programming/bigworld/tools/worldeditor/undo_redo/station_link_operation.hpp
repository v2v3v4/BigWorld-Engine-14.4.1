#ifndef STATION_LINK_OPERATION_HPP
#define STATION_LINK_OPERATION_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_station.hpp"
#include "gizmo/undoredo.hpp"

BW_BEGIN_NAMESPACE

class StationLinkOperation : public UndoRedo::Operation
{
public:
	StationLinkOperation
    ( 
        EditorChunkStationNodePtr           a,
        EditorChunkStationNodePtr           b,
        ChunkLink::Direction                dir 
    );

	/*virtual*/ void undo();

	/*virtual*/ bool iseq( const UndoRedo::Operation & oth ) const;

protected:
    EditorChunkStationNodePtr getNodeA() const;

    EditorChunkStationNodePtr getNodeB() const;

private:
    UniqueID                            idA;
    UniqueID                            idGraphA;
    UniqueID                            idB;
    UniqueID                            idGraphB;
	ChunkLink::Direction                dir_;
};

BW_END_NAMESPACE

#endif // STATION_LINK_OPERATION_HPP
