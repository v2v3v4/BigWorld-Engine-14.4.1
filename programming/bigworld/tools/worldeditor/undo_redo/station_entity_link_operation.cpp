#include "pch.hpp"
#include "worldeditor/undo_redo/station_entity_link_operation.hpp"
#include "worldeditor/world/world_manager.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This is the StationEntityLinkOperation constructor.
 *
 *  @param entity           The entity whose link information may need to be
 *                          restored.
 */
StationEntityLinkOperation::StationEntityLinkOperation
(
    EditorChunkEntityPtr        entity
): 
	UndoRedo::Operation(size_t(typeid(StationEntityLinkOperation).name())),
	entity_(entity)
{
	BW_GUARD;

    entityNodeID_   = entity->patrolListNode();
    entityGraphID_  = entity->patrolListGraphId();

	addChunk(entity_->chunk());
}


/**
 *  This restores the link information for the entity.
 */
/*virtual*/ void StationEntityLinkOperation::undo()
{
	BW_GUARD;

    UndoRedo::instance().add(new StationEntityLinkOperation(entity_));

	entity_->patrolListRelink( entityGraphID_, entityNodeID_ );
}


/**
 *  This compares this operation with another.
 *
 *  @param other        The operation to compare.
 *  @returns            false.
 */
/*virtual*/ bool 
StationEntityLinkOperation::iseq(UndoRedo::Operation const &other) const
{
    return false;
}

BW_END_NAMESPACE

