#include "pch.hpp"
#include "worldeditor/undo_redo/entity_user_data_object_link_operation.hpp"
#include "worldeditor/world/world_manager.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This is the EntityUserDataObjectLinkOperation constructor.
 *
 *  @param entity           The entity whose link information may need to be
 *                          restored.
 */
EntityUserDataObjectLinkOperation::EntityUserDataObjectLinkOperation
(
    EditorChunkEntityPtr        entity,
	const	BW::string&		linkName
)
: 
UndoRedo::Operation(size_t(typeid(EntityUserDataObjectLinkOperation).name())),
entity_(entity), linkName_( linkName )
{
	BW_GUARD;

	PropertyIndex propIdx =
		entity_->propHelper()->propGetIdx(linkName_);
	if ( propIdx.empty() )
	{
		ERROR_MSG("Failed to create Undo, could not find property %s of the entity used to construct the link", linkName_);
		return;
	}
	entityLink_ = entity_->propHelper()->propGetString(propIdx);
	addChunk(entity_->chunk());
}


/**
 *  This restores the link information for the entity.
 */
/*virtual*/ void EntityUserDataObjectLinkOperation::undo()
{
	BW_GUARD;

    UndoRedo::instance().add(new EntityUserDataObjectLinkOperation(entity_, linkName_));
	PropertyIndex propIdx =
		entity_->propHelper()->propGetIdx(linkName_);
	if ( propIdx.empty() )
	{
		ERROR_MSG("Failed to execute Undo, could not find property %s of the entity used to construct the link", linkName_);
		return;
	}
	entity_->propHelper()->propSetString(propIdx, entityLink_);
}


/**
 *  This compares this operation with another.
 *
 *  @param other        The operation to compare.
 *  @returns            false.
 */
/*virtual*/ bool 
EntityUserDataObjectLinkOperation::iseq(UndoRedo::Operation const &other) const
{
    return false;
}
BW_END_NAMESPACE

