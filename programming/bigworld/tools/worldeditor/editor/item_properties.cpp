#include "pch.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/items/editor_chunk_model_vlo_ref.hpp"
#include "worldeditor/world/items/editor_chunk_model_vlo.hpp"
#include "worldeditor/world/items/editor_chunk_tree.hpp"
#include "chunk/chunk_model_vlo.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_item.hpp"
#include "gizmo/undoredo.hpp"
#include "appmgr/commentary.hpp"
#include "common/editor_views.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	bool isChunkItemNormalSized( ChunkItemPtr pItem, Matrix & newTransform )
	{
		static const float MIN_LENGTH_LIMIT = 0.001f;

		//When we scale down a model, a EditorChunkModelVLORef might lose it's
		//chunk pointer as the VLO might not encompass the chunk anymore.
		//In such a situation, we just use the VLO's chunk instead
		EditorChunkModelVLORef * chunkVLORef = pItem->edIsVLO()
			? dynamic_cast< EditorChunkModelVLORef * >( pItem.get() )
			: NULL;
		EditorChunkModelVLO * chunkVLO = chunkVLORef == NULL ? NULL :
			dynamic_cast< EditorChunkModelVLO * >( chunkVLORef->object().get() );
		Chunk * pChunk = chunkVLO ? chunkVLO->vloChunk() : pItem->chunk();

		BoundingBox bbox;
		pItem->edBounds(bbox);
		bbox.transformBy( newTransform );
		Vector3 boxVolume = bbox.maxBounds() - bbox.minBounds();

		const float maxLengthLimit = pChunk->space()->gridSize();

		return (boxVolume.x < maxLengthLimit && boxVolume.z < maxLengthLimit &&
			boxVolume.x > MIN_LENGTH_LIMIT &&
			boxVolume.y > MIN_LENGTH_LIMIT && boxVolume.z > MIN_LENGTH_LIMIT);
	}

	Matrix maxVolume( ChunkItemPtr item, const Matrix& m1, const Matrix& m2 )
	{
		BoundingBox box1, box2;
		item->edBounds( box1 );
		box1.transformBy( m1 );
		item->edBounds( box2 );
		box2.transformBy( m2 );
		Vector3 volume1 = box1.maxBounds() - box1.minBounds();
		Vector3 volume2 = box2.maxBounds() - box2.minBounds();

		if (almostEqual( volume1.x, volume2.x ) &&
			almostEqual( volume1.z, volume2.z ))
		{
			return m1;
		}

		if (volume1.x < volume2.x &&
			almostEqual( volume1.z, volume2.z))
		{
			return m2;
		}

		if (volume1.z < volume2.z)
		{
			return m2;
		}

		return m1;
	}
}

// -----------------------------------------------------------------------------
// Section: ChunkItemMatrixOperation
// -----------------------------------------------------------------------------

class ChunkItemMatrixOperation : public UndoRedo::Operation
{
public:
	ChunkItemMatrixOperation( ChunkItemPtr pItem, Chunk * oldChunk,
			const Matrix & oldPose,
			ChunkItemPtr pReplacedItem ) :
		UndoRedo::Operation( size_t(typeid(ChunkItemMatrixOperation).name()) ),
		pItem_( pItem ),
		oldChunk_( oldChunk ),
		oldPose_( oldPose ),
		pReplacedItem_( pReplacedItem )
	{
		BW_GUARD;

		addChunk( oldChunk );
		addChunk( pItem_->chunk() );
	}

private:

	virtual void undo()
	{
		BW_GUARD;

		// first add the current state of this block to the undo/redo list
		UndoRedo::instance().add( new ChunkItemMatrixOperation(
			  pReplacedItem_ ? pReplacedItem_ : pItem_
			, pItem_->chunk(), pItem_->edTransform()
			, pReplacedItem_? pItem_ : NULL ) );

		Chunk * pChunk = pItem_->chunk();
		if (pChunk) //safety check for vlo references
		{
			BoundingBox oldBB, newBB;
			// Notify to update the chunks before move
			pItem_->edWorldBounds( oldBB );
			BW::set<Chunk*> chunks;
			WorldManager::instance().collectOccupiedChunks( oldBB, pChunk, chunks);
			WorldManager::instance().changedChunks( chunks, *pItem_ );

			// fix up the chunk if necessary
			if ((oldChunk_ != pItem_->chunk()))
			{
				pChunk->delStaticItem( pItem_ );
				oldChunk_->addStaticItem( pItem_ );
			}

			// now change the matrix back
			pItem_->edTransform( oldPose_ );

			// Notify to update the chunks after move
			pItem_->edWorldBounds( newBB );
			chunks.clear();
			WorldManager::instance().collectOccupiedChunks( newBB, pChunk, chunks );
			WorldManager::instance().changedChunks( chunks, *pItem_ );
			if (pReplacedItem_)
			{
				oldChunk_->addStaticItem( pReplacedItem_ );
				pReplacedItem_->edTransform( oldPose_ );
				pItem_->edPreDelete();
				if (pItem_->chunk())
				{
					pItem_->chunk()->delStaticItem( pItem_ );
				}
				else
				{
					pChunk->delStaticItem( pItem_ );
				}
			}
		}
	}

	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		BW_GUARD;

		return pItem_ ==
			static_cast<const ChunkItemMatrixOperation&>( oth ).pItem_;
	}

	ChunkItemPtr	pReplacedItem_;
	ChunkItemPtr	pItem_;
	Chunk *			oldChunk_;
	Matrix			oldPose_;
};




// -----------------------------------------------------------------------------
// Section: ChunkItemMatrix
// -----------------------------------------------------------------------------


/**
 *	Constructor.
 */
ChunkItemMatrix::ChunkItemMatrix( ChunkItemPtr pItem ) :
	pItem_( pItem ),
	origChunk_( NULL ),
	origPose_( Matrix::identity ),
	warned_( false ),
	haveRecorded_( false ),
	shownError_( false )
{
	BW_GUARD;

	movementSnaps_ = pItem_->edMovementDeltaSnaps();
	speedTree_ = dynamic_cast<ChunkTree*>( pItem.get() ) != nullptr;
	lastGoodPose_.setZero();
}

/**
 *	Destructor
 */
ChunkItemMatrix::~ChunkItemMatrix()
{
}

void ChunkItemMatrix::getMatrix( Matrix & m, bool world )
{
	BW_GUARD;

	m = pItem_->edTransform();
	if (world && pItem_->chunk() != NULL)
	{
		m.postMultiply( pItem_->chunk()->transform() );
	}
}

void ChunkItemMatrix::getMatrixContext( Matrix & m )
{
	BW_GUARD;

	if (pItem_->chunk() != NULL)
	{
		m = pItem_->chunk()->transform();
	}
	else
	{
		m = Matrix::identity;
	}
}

void ChunkItemMatrix::getMatrixContextInverse( Matrix & m )
{
	BW_GUARD;

	if (pItem_->chunk() != NULL)
	{
		m = pItem_->chunk()->transformInverse();
	}
	else
	{
		m = Matrix::identity;
	}
}

bool ChunkItemMatrix::setMatrix( const Matrix & m )
{
	BW_GUARD;

	Matrix newTransform = m;

	// Snap the transform of the matrix if it's asking for a different
	// translation
	Matrix currentTransform;
	getMatrix( currentTransform, false );
	if (!almostEqual( currentTransform.applyToOrigin(), newTransform.applyToOrigin() ))
	{
		Vector3 t = newTransform.applyToOrigin();

		Vector3 snaps = movementSnaps_;
		if (snaps == Vector3( 0.f, 0.f, 0.f ) && WorldManager::instance().snapsEnabled() )
			snaps = WorldManager::instance().movementSnaps();

		Snap::vector3( t, snaps );

		newTransform.translation( t );
	}

	if (speedTree_)
	{
		if (isChunkItemNormalSized( pItem_, newTransform ))
		{
			shownError_ = false;
			lastGoodPose_ = maxVolume( pItem_, newTransform, lastGoodPose_ );
		}
		else
		{
			if (!shownError_)
			{
				shownError_ = true;
				BW::string error =
					"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/TREE_TOO_BIG";
				error = LocaliseUTF8( error.c_str() );
				WorldManager::instance().addError(
					pItem_->chunk(), pItem_.get(), error.c_str() );
			}

			newTransform = lastGoodPose_;
			pItem_ ->edTransform( newTransform, true );
			return false;
		}
	}

	// always transient
	pItem_ ->edTransform( newTransform, true );
	return true;
}

void ChunkItemMatrix::recordState()
{
	BW_GUARD;

	origChunk_ = pItem_->chunk();
	origPose_ = pItem_->edTransform();
	lastGoodPose_ = origPose_;
	haveRecorded_ = true;
}

bool ChunkItemMatrix::commitState( bool revertToRecord, bool addUndoBarrier )
{
	BW_GUARD;

	if (!haveRecorded_)
		recordState();

	// find out where it is now
	Matrix destPose = pItem_->edTransform();
	bool normalSized = isChunkItemNormalSized( pItem_, destPose );
	ChunkPtr pChunk = pItem_->chunk();
	shownError_ = false;
	
	if (speedTree_)
	{
		if (!normalSized)
		{
			pItem_->edTransform( lastGoodPose_, true );
			/*
			BW::string error =
				"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/TREE_TOO_BIG";
			error = LocaliseUTF8( error.c_str() );
			WorldManager::instance().addError(
				pChunk, pItem_.get(), error.c_str() );*/
		}
	}
	
	// set it back so it starts from the old spot
	pItem_->edTransform( origPose_, true );

	// if we're reverting we stop now
	if (revertToRecord)
		return true;

	BW::set<Chunk*> chunks;
	BoundingBox oldBB, newBB;
	// Notify to update chunks before move.
	pItem_->edWorldBounds( oldBB );
	WorldManager::instance().collectOccupiedChunks( oldBB, pChunk, chunks );
	WorldManager::instance().changedChunks( chunks, *pItem_ );

	// attempt to set the matrix permanently
	bool okToCommit = true;
	ChunkItemPtr itemToReplace = NULL;

	EditorChunkModelVLORef * chunkVLO = pItem_->edIsVLO()
		? dynamic_cast< EditorChunkModelVLORef * >( pItem_.get() )
		: NULL;

	std::auto_ptr< EditorChunkModel::BigModelLoader > bigModelLoader;
	if( chunkVLO )
	{
		//Suppress model too big warning
		bigModelLoader.reset( new EditorChunkModel::BigModelLoader() );
	}

	if (!pItem_->edTransform( destPose ))
	{
		// set it back if that failed
		pItem_->edTransform( origPose_ );

		okToCommit = false;
	}
	else
	{
		Matrix world( pChunk->transform() );
		world.preMultiply( destPose );

		if (chunkVLO && normalSized)
		{
			ChunkItemPtr chunkModel = chunkVLO->convertToChunkModel( world );
			if (chunkModel != NULL)
			{
				itemToReplace = chunkModel;
			}
		}
		else if (normalSized || ( pItem_->edIsVLO() && normalSized == false ))
		{
			//No conversion necessary
		}
		else
		{
			ChunkItemPtr proxyVLORef =
				EditorChunkModelVLORef::convertToChunkModelVLO( pItem_, world );
			if( proxyVLORef )
			{
				itemToReplace = proxyVLORef;
				WorldManager::instance().addError( pChunk, pItem_.get(),
					LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/MODEL_TOO_BIG" ).c_str() );
			}
		}
	}

	ChunkItemPtr previousItem = pItem_;
	if( itemToReplace )
	{
		pItem_ = itemToReplace;
	}


	// Notify to update chunks after move.
	pItem_->edWorldBounds( newBB );
	chunks.clear();
	WorldManager::instance().collectOccupiedChunks( newBB, pItem_->chunk(), chunks );
	WorldManager::instance().changedChunks( chunks, *pItem_ );


	// add the undo operation for it
	UndoRedo::instance().add(
		new ChunkItemMatrixOperation( pItem_, origChunk_, origPose_,
			itemToReplace ? previousItem : NULL ) );
	pItem_->edPostModify();

	WorldManager::instance().needsDeferSelectionAddUndoBarrier( addUndoBarrier );
	if (itemToReplace)
	{
		WorldManager::instance().replaceSelection( previousItem, pItem_ );
		// tell it it's going to be deleted
		previousItem->edPreDelete();
		// delete it now
		if (previousItem->chunk())
		{
			previousItem->chunk()->delStaticItem( previousItem );
		}
		else
		{
			pChunk->delStaticItem( previousItem );
		}
	}
	else
	{
		if (addUndoBarrier)
		{
			// if the selection hasn't been replaced,
			// do not add undo barrier here
			if( !WorldManager::instance().hasPendingSelections() )
			{
				// set the barrier with a meaningful name
				UndoRedo::instance().barrier(
					LocaliseUTF8( L"GIZMO/UNDO/MOVE_PROPERTY" ) +
					" " + pItem_->edDescription().str(), false );
				// TODO: Don't always say 'Move ' ...
				//  figure it out from change in matrix
			}
		}
	}

	// check here, so push on an undo for multiselect
	if ( okToCommit )
	{
		if( !revertToRecord )
			warned_ = false;
		return true;
	}

	return false;
}

bool ChunkItemMatrix::hasChanged()
{
	BW_GUARD;

	return (origChunk_ != pItem_->chunk() || origPose_ != pItem_->edTransform());
}

float ChunkItemPositionProperty::length( ChunkItemPtr item )
{
	BW_GUARD;

	BoundingBox bb;
	item->edBounds( bb );
	if( bb.insideOut() )
		return 0.f;
	float length = ( bb.maxBounds() - bb.minBounds() ).length() * 10.f;
	length = Math::clamp( 10.f, length, 200.f );
	return length;
}

/**
 *	This static method in the MatrixProxy base class gets the default
 *	matrix proxy for a given chunk item. It may return NULL.
 */
MatrixProxyPtr WEMatrixProxyCreator::operator()( ChunkItemPtr pItem )
{
	BW_GUARD;
	return new ChunkItemMatrix( pItem );
}



// -----------------------------------------------------------------------------
// Section: ConstantChunkNameProxy
// -----------------------------------------------------------------------------

namespace Script
{
	PyObject * getData( const Chunk * pChunk );
	// in editor_chunk_portal.cpp
};

BW::string chunkPtrToString( Chunk * pChunk )
{
	BW_GUARD;

	PyObject * pPyRet = Script::getData( pChunk );
	MF_ASSERT( pPyRet != NULL );

	BW::wstring ret;
	extractUnicode( pPyRet, ret );

	Py_DECREF( pPyRet );
	return bw_wtoutf8( ret );
}


BW_END_NAMESPACE
// item_properties.cpp
