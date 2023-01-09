#include "pch.hpp"
#include "worldeditor/undo_redo/linker_operations.hpp"
#include "worldeditor/world/editor_chunk_item_linker.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_item_linker_manager.hpp"
#include "common/properties_helper.hpp"

BW_BEGIN_NAMESPACE


/**
 *  This is the LinkerUndoChangeLinkOperation constructor.
 *
 *  @param startEcil	The source linker object.
 *  @param oldEcil		The linker object previously pointed at.
 *  @param newEcil		The linker object being added to the property.
 *  @param propIdx		The property index.
 */
LinkerUndoChangeLinkOperation::LinkerUndoChangeLinkOperation(
		const EditorChunkItemLinkable * startEcil,
		const EditorChunkItemLinkable * oldEcil,
		const EditorChunkItemLinkable * newEcil,
		PropertyIndex propIdx) :
	UndoRedo::Operation(size_t(typeid(LinkerUndoChangeLinkOperation).name())),
	propIdx_(propIdx)
{
	BW_GUARD;

	startUID_ = startEcil->guid();
	startCID_ = startEcil->getOutsideChunkId();

	oldUID_ = oldEcil->guid();
	oldCID_ = oldEcil->getOutsideChunkId();

	newUID_ = newEcil->guid();
	newCID_ = newEcil->getOutsideChunkId();
}


/**
 *  This restores the link information.
 */
/*virtual*/ void LinkerUndoChangeLinkOperation::undo()
{
	BW_GUARD;

	EditorChunkItemLinkable * startEcil =
		WorldManager::instance().linkerManager().forceLoad( startUID_, startCID_ );
	EditorChunkItemLinkable * oldEcil =
		WorldManager::instance().linkerManager().forceLoad( oldUID_, oldCID_ );
	EditorChunkItemLinkable * newEcil =
		WorldManager::instance().linkerManager().forceLoad( newUID_, newCID_ );
	
	// The force load should be successful for all cases
	if (!startEcil || !oldEcil || !newEcil)
	{
		ERROR_MSG( "LinkerUndoChangeLinkOperation::undo: "
					"One or more linker objects failed to force load\n" );
		return;
	}

	UndoRedo::instance().add(new LinkerUndoChangeLinkOperation(startEcil, newEcil, oldEcil, propIdx_));

	WorldManager::instance().linkerManager().addLinkInternal
		(
			startEcil,
			oldEcil,
			propIdx_
		);
}


/**
 *  This compares this operation with another.
 *
 *  @param other        The operation to compare.
 *  @returns            false.
 */
/*virtual*/ bool LinkerUndoChangeLinkOperation::iseq(
	UndoRedo::Operation const &other) const
{
    return false;
}


/**
 *  This is the LinkerUndoAddLinkOperation constructor.
 *
 *  @param startEcil	The source linker object.
 *  @param endEcil		The target linker object.
 *  @param propIdx		The property index.
 */
LinkerUndoAddLinkOperation::LinkerUndoAddLinkOperation(
		const EditorChunkItemLinkable * startEcil,
		const EditorChunkItemLinkable * endEcil,
		PropertyIndex propIdx) :
	UndoRedo::Operation(size_t(typeid(LinkerUndoAddLinkOperation).name())),
	propIdx_(propIdx)
{
	BW_GUARD;

	startUID_ = startEcil->guid();
	startCID_ = startEcil->getOutsideChunkId();

	endUID_ = endEcil->guid();
	endCID_ = endEcil->getOutsideChunkId();
}


/**
 *  This deletes the link information.
 */
/*virtual*/ void LinkerUndoAddLinkOperation::undo()
{
	BW_GUARD;

	EditorChunkItemLinkable * startEcil =
		WorldManager::instance().linkerManager().forceLoad( startUID_, startCID_ );
	EditorChunkItemLinkable * endEcil =
		WorldManager::instance().linkerManager().forceLoad( endUID_, endCID_ );
	
	// The force load should be successful for all cases
	if (!startEcil || !endEcil)
	{
		ERROR_MSG( "LinkerUndoAddLinkOperation::undo: "
					"One or more linker objects failed to force load\n" );
		return;
	}

	DataSectionPtr data = startEcil->propHelper()->propGet(propIdx_.valueAt(0));
	
	UndoRedo::instance().add(new LinkerUndoDeleteLinkOperation(startEcil, endEcil, data, propIdx_));

	// Need to delete the link here
	WorldManager::instance().linkerManager().deleteLinkInternal( startEcil, endEcil, propIdx_ );
}


/**
 *  This compares this operation with another.
 *
 *  @param other        The operation to compare.
 *  @returns            false.
 */
/*virtual*/ bool LinkerUndoAddLinkOperation::iseq(
	UndoRedo::Operation const &other) const
{
    return false;
}


/**
 *  This is the LinkerUndoDeleteLinkOperation constructor.
 *
 *  @param startEcil	The source linker object.
 *  @param endEcil		The target linker object.
 *  @param data			The data section containing the original property info.
 *  @param propIdx		The property index.
 */
LinkerUndoDeleteLinkOperation::LinkerUndoDeleteLinkOperation(
		const EditorChunkItemLinkable * startEcil,
		const EditorChunkItemLinkable * endEcil,
		DataSectionPtr data,
		PropertyIndex propIdx) :
	UndoRedo::Operation(size_t(typeid(LinkerUndoDeleteLinkOperation).name())),
	data_(data), propIdx_(propIdx)
{
	BW_GUARD;

	startUID_ = startEcil->guid();
	startCID_ = startEcil->getOutsideChunkId();

	endUID_ = endEcil->guid();
	endCID_ = endEcil->getOutsideChunkId();
}


/**
 *  This restores the link information.
 */
/*virtual*/ void LinkerUndoDeleteLinkOperation::undo()
{
	BW_GUARD;

	EditorChunkItemLinkable * startEcil =
		WorldManager::instance().linkerManager().forceLoad( startUID_, startCID_ );
	EditorChunkItemLinkable * endEcil =
		WorldManager::instance().linkerManager().forceLoad( endUID_, endCID_ );
	
	// The force load should be successful for all cases
	if (!startEcil || !endEcil)
	{
		ERROR_MSG( "LinkerUndoDeleteLinkOperation::undo: "
					"One or more linker objects failed to force load\n" );
		return;
	}

	UndoRedo::instance().add(new LinkerUndoAddLinkOperation(startEcil, endEcil, propIdx_));

	// Need to create the property
	startEcil->propHelper()->propSet( propIdx_.valueAt(0), data_ );

	WorldManager::instance().linkerManager().updateLink( startEcil, endEcil );

	// Save the changes
	startEcil->chunkItem()->edSave( startEcil->chunkItem()->pOwnSect() );
	if ( startEcil->chunkItem()->chunk() != NULL )
	{
		WorldManager::instance().changedChunk( 
			startEcil->chunkItem()->chunk(),
			*startEcil->chunkItem() );
		startEcil->chunkItem()->edPostModify();
	}

	endEcil->chunkItem()->edSave( endEcil->chunkItem()->pOwnSect() );
	if ( endEcil->chunkItem()->chunk() != NULL )
	{
		WorldManager::instance().changedChunk( 
			endEcil->chunkItem()->chunk(),
			*startEcil->chunkItem() );
		endEcil->chunkItem()->edPostModify();
	}

}


/**
 *  This compares this operation with another.
 *
 *  @param other        The operation to compare.
 *  @returns            false.
 */
/*virtual*/ bool LinkerUndoDeleteLinkOperation::iseq(
	UndoRedo::Operation const &other) const
{
    return false;
}


/**
 *  This is the LinkerUpdateLinkOperation constructor.
 *
 *  @param startEcil		The source linker object.
 *  @param targetUID		The target linker object's Unique Id.
 *  @param targetChunkID	The target linker object's Chunk Id.
 */
LinkerUpdateLinkOperation::LinkerUpdateLinkOperation(
		const EditorChunkItemLinkable * startEcil,
		BW::string targetUID,
		BW::string targetChunkID) :
	UndoRedo::Operation(size_t(typeid(LinkerUpdateLinkOperation).name()))
{
	BW_GUARD;

	startUID_ = startEcil->guid();
	startCID_ = startEcil->getOutsideChunkId();
	targetUID_ = UniqueID(targetUID);
	targetCID_ = targetChunkID;
}


/**
 *  This restores the link information for the entity.
 */
/*virtual*/ void LinkerUpdateLinkOperation::undo()
{
	BW_GUARD;

	EditorChunkItemLinkable* startEcil =
		WorldManager::instance().linkerManager().forceLoad(startUID_, startCID_);
	EditorChunkItemLinkable* targetEcil =
		WorldManager::instance().linkerManager().forceLoad(targetUID_, targetCID_);
	
	// The force load should be successful for all cases
	if (!startEcil || !targetEcil)
	{
		ERROR_MSG( "LinkerUpdateLinkOperation::undo: "
					"One or more linker objects failed to force load\n" );
		return;
	}

	UndoRedo::instance().add(new LinkerUpdateLinkOperation(startEcil, targetUID_, targetCID_));

	WorldManager::instance().linkerManager().updateLink(startEcil, targetEcil);

	// Save the changes
	startEcil->chunkItem()->edSave( startEcil->chunkItem()->pOwnSect() );
	if ( startEcil->chunkItem()->chunk() != NULL )
	{
		WorldManager::instance().changedChunk(
			startEcil->chunkItem()->chunk(),
			*startEcil->chunkItem() );
		startEcil->chunkItem()->edPostModify();
	}

	targetEcil->chunkItem()->edSave( targetEcil->chunkItem()->pOwnSect() );
	if ( targetEcil->chunkItem()->chunk() != NULL )
	{
		WorldManager::instance().changedChunk( 
			targetEcil->chunkItem()->chunk(),
			*startEcil->chunkItem() );
		targetEcil->chunkItem()->edPostModify();
	}
}


/**
 *  This compares this operation with another.
 *
 *  @param other        The operation to compare.
 *  @returns            false.
 */
/*virtual*/ bool LinkerUpdateLinkOperation::iseq(
	UndoRedo::Operation const &other) const
{
    return false;
}
BW_END_NAMESPACE

