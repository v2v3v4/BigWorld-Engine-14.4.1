#include "pch.hpp"
#include "worldeditor/undo_redo/user_data_object_link_operation.hpp"
#include "worldeditor/world/world_manager.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This is the UserDataObjectLinkOperation constructor.
 *
 *  @param entity           The entity whose link information may need to be
 *                          restored.
 */
UserDataObjectLinkOperation::UserDataObjectLinkOperation
(
    EditorChunkUserDataObjectPtr        udo,
	const	BW::string&				linkName
): 
	UndoRedo::Operation(size_t(typeid(UserDataObjectLinkOperation).name())),
	udo_(udo), 
	linkName_( linkName )
{
	BW_GUARD;

	PropertyIndex propIdx =
		udo_->propHelper()->propGetIdx(linkName_);
	if ( propIdx.empty() )
	{
		//TODO: have a warning or error
		return;
	}
	udoLink_ = udo_->propHelper()->propGetString(propIdx);
	addChunk(udo_->chunk());
}


/**
 *  This restores the link information for the entity.
 */
/*virtual*/ void UserDataObjectLinkOperation::undo()
{
	BW_GUARD;

    UndoRedo::instance().add(new UserDataObjectLinkOperation(udo_, linkName_));
	PropertyIndex propIdx =
		udo_->propHelper()->propGetIdx(linkName_);
	if ( propIdx.empty() )
	{
		ERROR_MSG("Could not find property %s of the entity used to construct the link", linkName_);
		return;
	}
	udo_->propHelper()->propSetString(propIdx, udoLink_);
}


/**
 *  This compares this operation with another.
 *
 *  @param other        The operation to compare.
 *  @returns            false.
 */
/*virtual*/ bool 
UserDataObjectLinkOperation::iseq(UndoRedo::Operation const &other) const
{
    return false;
}
BW_END_NAMESPACE

