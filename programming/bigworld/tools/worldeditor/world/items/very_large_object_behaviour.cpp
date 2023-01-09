#include "pch.hpp"

#include "very_large_object_behaviour.hpp"
#include "chunk/chunk_vlo.hpp"
#include "worldeditor/world/vlo_manager.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This method updates our local vars from the transform
 */
/*static */void VeryLargeObjectBehaviour::updateLocalVars(
	VeryLargeObject & object, const Matrix & m, Chunk * pChunk )
{
	BW_GUARD;

	if (pChunk)
	{
		Matrix world(m);
		world.postMultiply( pChunk->transform() );
		object.updateLocalVars( world );
	}
}


/*static */BoundingBox VeryLargeObjectBehaviour::calcVLOBoundingBox(
	const Matrix & worldTransform, VeryLargeObject & object,
	const BoundingBox & vloBB )
{
	BW_GUARD;
	BoundingBox transformedBBox( vloBB );

	Matrix inverseLocalTransform = object.localTransform();
	inverseLocalTransform.invert( inverseLocalTransform );

	Matrix transform( worldTransform );
	transform.postMultiply( inverseLocalTransform );
	transformedBBox.transformBy( transform );
	 
	return transformedBBox;
}

/*static */bool VeryLargeObjectBehaviour::edTransform(
	VeryLargeObject & object, Chunk * pChunk, BoundingBox & vloBB,
	ChunkItemPtr chunkItem,
	Matrix & o_Transform, bool & o_DrawTransient,
	const Matrix & m, bool transient )
{
	BW_GUARD;

	MF_ASSERT( Moo::isRenderThread() );
	// it's permanent, so find out where we belong now
	Chunk * pOldChunk = pChunk;
	Chunk * pNewChunk = chunkItem->edDropChunk( m.applyToOrigin() );	
	if (pNewChunk == NULL)
	{
		return false;
	}

	o_DrawTransient = transient;
	// if this is only a temporary change, keep it in the same chunk
	if (transient)
	{
		o_Transform = m;
		updateLocalVars( object, o_Transform, pChunk );
		object.updateWorldVars( pNewChunk->transform() );
		return true;
	}

	// ok, accept the transform change then
	Matrix targetWorldTransform( m );
	targetWorldTransform.multiply( m, pOldChunk->transform() );

	Matrix oldTransform = o_Transform;
	o_Transform = targetWorldTransform;
	o_Transform.postMultiply( pNewChunk->transformInverse() );
	updateLocalVars( object, o_Transform, pNewChunk );

	BoundingBox targetBoundingBox = 
		calcVLOBoundingBox( targetWorldTransform, object, vloBB );
	// make sure the new chunks aren't readonly (we don't check the old chunks
	// because the water shouldn't be selectable if the old chunks are readonly
	// in the first place!).
	if (!VLOManager::instance()->writable( &object, targetBoundingBox ))
	{
		// Readonly, so restore position.
		o_Transform = oldTransform;
		updateLocalVars( object, o_Transform, pOldChunk );
		WorldManager::instance().addCommentaryMsg(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_VLO/POSITION_LOCKED"));
		return false;
	}

	// Tell the manager to mark the old and new areas have changed. To do this, 
	// we have to update the VLO to the old position again, and then restore to
	// the new position after marking the old chunks as changed.
	Matrix newTransform = o_Transform;
	o_Transform = oldTransform;
	updateLocalVars( object, o_Transform, pOldChunk );

	Matrix srcWorldTransform( oldTransform );
	srcWorldTransform.postMultiply( pOldChunk->transform() );
	VLOManager::instance()->markChunksChanged(
		&object,
		calcVLOBoundingBox( srcWorldTransform, object, vloBB ) );

	o_Transform = newTransform;
	updateLocalVars( object, o_Transform, pNewChunk );
	VLOManager::instance()->markChunksChanged( &object, targetBoundingBox );

	pOldChunk->delStaticItem( chunkItem );
	if ( pOldChunk != pNewChunk )
	{
		// If source and destination chunks are different, delete any
		// references to tho VLO object that are in the pNewChunk first.
		VLOManager::instance()->deleteReference( &object, pNewChunk );
	}
	pNewChunk->addStaticItem( chunkItem );
	// Notify the VLO manager, which will add/remove references accordingly.
	VLOManager::instance()->updateReferencesImmediately( &object );

	VLOManager::instance()->markAsDirty( object.getUID() );

	WorldManager::instance().changedChunk( pOldChunk );
	if (pNewChunk != pOldChunk )
	{
		WorldManager::instance().changedChunk( pNewChunk );
	}

	//NOTE: world vars will get updated when we are tossed into the new chunk
	return true;
}
BW_END_NAMESPACE

