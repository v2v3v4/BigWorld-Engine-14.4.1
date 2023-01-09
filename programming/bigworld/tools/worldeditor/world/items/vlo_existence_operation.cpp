#include "pch.hpp"
#include "vlo_existence_operation.hpp"
#include "chunk/chunk_vlo.hpp"
#include "worldeditor/world/vlo_manager.hpp"
#include "worldeditor/world/world_manager.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: VLOExistenceOperation
// -----------------------------------------------------------------------------
VLOExistenceOperation::VLOExistenceOperation(
	ChunkVLOPtr pItem, Chunk * pOldChunk )
	: UndoRedo::Operation( 0 )
	, pItem_( pItem )
	, pOldChunk_( pOldChunk )
{
	BW_GUARD;

	addChunk( pOldChunk_ );
	if( pItem_ )
	{
		addChunk( pItem_->chunk() );
	}
}


/*virtual */void VLOExistenceOperation::undo()
{
	BW_GUARD;

	// Invalid op.
	if (!pItem_)
	{
		return;
	}

	// VLOs need a once-only update when undoing. When redoing, the VLO will
	// handle creating the new Undo point, etc, internally. The actual deletion
	// and recreation of the VLO is done by the default ChunkItemExistance
	// operation.
	VLOManager::instance()->markAsDeleted( pItem_->object()->getUID(), false );
	VLOManager::instance()->updateReferences( pItem_->object() );
	// Make sure the chunk(s) get marked as changed.
	if ( pOldChunk_ )
	{
		WorldManager::instance().changedChunk( pOldChunk_ );
	}
	if ( pItem_->chunk() && pItem_->chunk()!= pOldChunk_ )
	{
		WorldManager::instance().changedChunk( pItem_->chunk() );
	}
}


/*virtual */bool VLOExistenceOperation::iseq(
	const UndoRedo::Operation & oth ) const
{
	// these operations never replace each other
	return false;
}
BW_END_NAMESPACE

