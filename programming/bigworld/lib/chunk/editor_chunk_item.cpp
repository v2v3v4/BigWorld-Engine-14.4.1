#include "pch.hpp"

#include "appmgr/options.hpp"
#include "resmgr/datasection.hpp"
#include "chunk_item.hpp"
#include "editor_chunk_item.hpp"
#include "chunk.hpp"
#include "chunk_space.hpp"
#include "resmgr/auto_config.hpp"
#include "gizmo/undoredo.hpp"
#include "common/editor_group.hpp"
#include "common/space_editor.hpp"


#ifndef CODE_INLINE
#include "editor_chunk_item.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )


BW_BEGIN_NAMESPACE

namespace
{
	/**
	 *	Cache class for sqrt calculations needed by edIsTooDistant.
	 */
	class FastSqrt
	{
		#define K 0.002f // K was tuned to best suit it's use in edIsTooDistant.
		static const int ENTRIES = 10000;

	public:
		FastSqrt() :
			overflow_( false )
		{
			BW_GUARD;

			for (int i = 0; i < ENTRIES; ++i)
			{
				lookup_[ i ] = sqrtf( float( i ) / K );
			}
		}

		float calc( float val ) const
		{
			BW_GUARD;

			int intVal = int( val * K ); 
			overflow_ = (intVal >= ENTRIES);
			int clampedVal = std::max( 0, std::min( ENTRIES - 1, intVal ) );
			return lookup_[ clampedVal ];
		}

		bool overflow() const
		{
			return overflow_;
		}

	private:
		float lookup_[ ENTRIES ];
		mutable bool overflow_;
	};
	FastSqrt s_fastSqrt;


	float s_screenPercentThreshold = -1.0f; // -1 means "unninitialised".
	AutoConfigString s_chunkItemMetaDataConfig( "editor/metaDataConfig/chunkItem", "helpers/meta_data/chunk_item.xml" );
} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: EditorChunkItem
// -----------------------------------------------------------------------------

/*static*/ bool EditorChunkItem::s_drawSelection_ = false;
/*static*/ bool EditorChunkItem::s_hideAllOutside_ = false;

/*static*/ EditorChunkItem::CallbackSet EditorChunkItem::s_onModifyCallback_;
/*static*/ EditorChunkItem::CallbackSet EditorChunkItem::s_onDeleteCallback_;

/*static*/ uint32 EditorChunkItem::currentSelectionMark_;


/**
 *	Constructor.
 */
EditorChunkItem::EditorChunkItem( WantFlags wantFlags ) :
	ChunkItemBase( wantFlags ),
	pGroup_( NULL ),
	groupMember_( false ),
	hasLoaded_( false ),
	transient_( false ),
	metaData_( (EditorChunkCommonLoadSave*)this ),
	isSelected_( false )
{
	BW_GUARD;

	if (s_screenPercentThreshold == -1.0f)
	{
		s_screenPercentThreshold =
			Options::getOptionFloat( "render/editorObjectsScreenPercentLOD", 0.8f ) / 100.f;
	}
}


void EditorChunkItem::edWorldBounds( BoundingBox & bbRet )
{
	if (chunk())
	{
		bbRet = BoundingBox::s_insideOut_;
		this->edBounds( bbRet );

		// in case the item doesn't have size at all
		if ( bbRet.insideOut() )
		{
			bbRet = BoundingBox( Vector3(0,0,0), Vector3(0,0,0) );
		}

		bbRet.transformBy( this->edTransform() );
		bbRet.transformBy( chunk()->transform() );
	}
}

/**
 *	Destructor.
 */
EditorChunkItem::~EditorChunkItem()
{
	BW_GUARD;

	for (CallbackSet::iterator it = s_onDeleteCallback_.begin();
		it != s_onDeleteCallback_.end(); ++it)
	{
		(**it)( this );
	}

	doItemDeleted();
}

void EditorChunkItem::edChunkBind()
{
	BW_GUARD;

	if (!hasLoaded_)
	{
		edMainThreadLoad();
		hasLoaded_ = true;
	}
}


/**
 *	Access the class name. Do NOT be tempted to use this in
 *	switch statements... make a virtual function for it!
 *	This should only be used for giving the user info about the item.
 */
Name EditorChunkItem::edClassName()
{
	BW_GUARD;

	STATIC_LOCALISE_NAME( s_className, "CHUNK/EDITOR/EDITOR_CHUNK_ITEM/UNKNOWN" );

	return s_className;
}


/**
 *	Get a nice description for this item. Most items will not need
 *	to override this method.
 */
Name EditorChunkItem::edDescription()
{
	BW_GUARD;

	const char * label = this->label();
	if (label != NULL && label[0])
		return LocaliseUTF8( L"CHUNK/EDITOR/EDITOR_CHUNK_ITEM/ED_DESCRIPTION_WITH_LABEL", edClassName().c_str(), label );
	return LocaliseUTF8( L"CHUNK/EDITOR/EDITOR_CHUNK_ITEM/ED_DESCRIPTION", edClassName().c_str() );
}


/**
 *	Find the drop chunk for this item
 */
Chunk * EditorChunkItem::edDropChunk( const Vector3 & lpos )
{
	BW_GUARD;

	if (!pChunk_)
	{
		ERROR_MSG( "%s has not been added to a chunk!\n", this->edDescription().c_str() );
		return NULL;
	}

	Vector3 npos = pChunk_->transform().applyPoint( lpos );

	Chunk * pNewChunk = pChunk_->space()->findChunkFromPointExact( npos );
	if (pNewChunk == NULL)
	{
		ERROR_MSG( "Cannot move %s to (%f,%f,%f) "
			"because it is not in any loaded chunk!\n",
			this->edDescription().c_str(), npos.x, npos.y, npos.z );
		return NULL;
	}

	return pNewChunk;
}

bool EditorChunkItem::edCommonSave( DataSectionPtr pSection )
{
	BW_GUARD;

	if (EditorChunkCommonLoadSave::edCommonSave( pSection ))
	{
		DataSectionPtr editorSection = pSection->openSection( "editorOnly", true );
		if (editorSection)
		{
			if (edGroup())
			{
				groupName_ = edGroup()->fullName();
				editorSection->writeString( "group", groupName_ );
			}
		}
		return metaData_.save( pSection );
	}
	return false;
}

bool EditorChunkItem::edCommonLoad( DataSectionPtr pSection )
{
	BW_GUARD;

	if (EditorChunkCommonLoadSave::edCommonLoad( pSection ))
	{
		DataSectionPtr editorSection = pSection->openSection( "editorOnly", false );
		if (editorSection == NULL) // for old stuff still load the group..
			editorSection = pSection;

		groupName_ = editorSection->readString( "group" );
		if (!groupName_.empty())
		{
			groupMember_ = true;
			// We don't use edGroup() here, as pOwnSect may not yet be valid to call
			//edGroup( EditorGroup::findOrCreateGroup( groupName_ ) );
			//pGroup_ = EditorGroup::findOrCreateGroup( groupName_ );
			//pGroup_->enterGroup( (ChunkItem*) this );

			// Don't add ourself to the group untill toss()
		}

		static MetaData::Desc s_dataDesc( s_chunkItemMetaDataConfig.value() );
		return metaData_.load( pSection, s_dataDesc );
	}
	return false;
}

bool EditorChunkItem::edCommonEdit( GeneralEditor& editor )
{
	BW_GUARD;

	metaData_.edit( editor, Name(), false );
	return true;
}


void EditorChunkItem::edCommonChanged()
{
	BW_GUARD;

	edSave( pOwnSect() );
	SpaceEditor::instance().changedChunk( chunk(),
		InvalidateFlags::FLAG_NONE );

	edPostModify();
}

void EditorChunkItem::toss( Chunk * pChunk )
{
	BW_GUARD;

	ChunkItemBase::toss( pChunk );

	if (groupMember_)
	{
		if (pChunk)
		{
			// Add it back to its group if we're returning from nowhere
			if (!pGroup_)
			{
				// We don't call edGroup(), as we don't want to mark the chunk as dirty
				pGroup_ = EditorGroup::findOrCreateGroup( groupName_ );
				pGroup_->enterGroup( (ChunkItem*) this );
			}
		}
		else
		{
			// Item is being moved to nowhere, temp remove it from its group
			edGroup( NULL );
		}
	}
	if (pChunk)
	{
		doItemRestored();
	}
	else
	{
		doItemRemoved();
	}
}


/**
 *	Move the item between chunks once they are transformed
 */
void EditorChunkItem::edMove( Chunk* pOldChunk, Chunk* pNewChunk )
{
	if (pOldChunk == pNewChunk)
	{
		pOldChunk->moveStaticItem( (ChunkItem*)this );
	}
	else
	{
		pOldChunk->delStaticItem( (ChunkItem*)this );
		pNewChunk->addStaticItem( (ChunkItem*)this );
	}
}


void EditorChunkItem::edGroup( EditorGroup * pGp )
{
	BW_GUARD;

	// NB, pGp may be the same as pGroup_, if it's name has changed or somesuch
	if (pGroup_)
		pGroup_->leaveGroup( (ChunkItem*) this );

	if (pGp == NULL)
	{
		pGroup_ = NULL;
	}
	else
	{
		groupMember_ = true;
		pGroup_ = pGp;
		groupName_ = pGroup_->fullName();
		pGroup_->enterGroup( (ChunkItem*) this );

		if ( pOwnSect() )
			edSave( pOwnSect() );
		if (chunk())
			SpaceEditor::instance().changedChunk( chunk() );

		edPostModify();
	}
}


/**
 * Tell the item it was just cloned from srcItem
 *
 * srcItem will be NULL if they shell we were in was cloned, rather than
 * us directly.
 */
void EditorChunkItem::edPostClone( EditorChunkItem* srcItem )	
{ 
	BW_GUARD;

	if (!UndoRedo::instance().isUndoing())
	{
		MetaData::updateCreationInfo( metaData_ );
	}

	this->syncInit();
}


/**
 * Tell the item it was just created (doesn't trigger on clone nor load)
 *
 * The item will either be a new one, or deleting it was just undone
 */
void EditorChunkItem::edPostCreate() 
{
	BW_GUARD;

	if (!UndoRedo::instance().isUndoing())
	{
		MetaData::updateCreationInfo( metaData_ );
	}

	this->syncInit();
}


/**
 * Tell the item it was just modified
 */
void EditorChunkItem::edPostModify() 
{
	BW_GUARD;

	if (!UndoRedo::instance().isUndoing())
	{
		MetaData::updateModificationInfo( metaData_ );
	}

	for (CallbackSet::iterator it = s_onModifyCallback_.begin();
		it != s_onModifyCallback_.end(); ++it)
	{
		(**it)( this );
	}
}


/**
 *	This method returns true if this item should be rendered 
 *  in the editor.
 *
 *	@return		True if the chunk item is to be drawn.
 */
bool EditorChunkItem::edShouldDraw() const
{
	BW_GUARD;

	return (!edHidden() || s_drawSelection_) && ( !s_hideAllOutside_ || !chunk()->isOutsideChunk() );
}


/**
 *	This method returns true if this item 'editable' and false
 *	if it's read-only.
 *
 *	@return		True if editable.
 */
bool EditorChunkItem::edIsEditable() const
{
	BW_GUARD;

	return chunk()	?
		SpaceEditor::instance().isChunkWritable( chunk() ) && !edFrozen() :
		!edFrozen();
}


/**
 *	This method returns true if the size on screen of a distant object would be
 *	smaller than the percent threshold defined in XML.
 *
 *  IMPORTANT; This method doesn't take into account the item's transform
 *	scaling for performance reasons, since it's only used by editor objects
 *	such as entities, light proxies, UDOs, etc.
 *
 *	@return		True if the chunk item is far away from the camera.
 */
bool EditorChunkItem::edIsTooDistant()
{
	BW_GUARD;

	if (!chunk())
	{
		return true;
	}

	if (s_screenPercentThreshold <= 0)
	{
		return false;
	}

	// Prepare item and camera positions for distance calculations
	Vector3 cameraPos = Moo::rc().invView().applyToOrigin();
	Vector3 itemPos = chunk()->transform().applyPoint( edTransform().applyToOrigin() );

	// Calculate the object size as the maximum side of it's bounding box. Note
	// that we don't care about scaling in edTransform because editor objects
	// don't have scaling.
	BoundingBox bb;
	edBounds( bb );
	Vector3 max = bb.maxBounds();
	Vector3 min = bb.minBounds();

	float objSize = std::max( std::max( max[0] - min[0], max[1] - min[1] ), max[2] - min[2] );

	// Update the cached sin( halfFOV ) value if the FOV has changed.
	static float s_lastFov = 0.0f;
	static float s_sinHalfFov = 0.0f;
	if (Moo::rc().camera().fov() != s_lastFov)
	{
		s_lastFov = Moo::rc().camera().fov();
		s_sinHalfFov = sinf( s_lastFov / 2.0f );
	}

	// Here we use the pythagoras theorem of a^2 + b^2 = c^2, or in our case
	// cameraPlaneDist^2 + halfCameraPlaneSize^2 = radious^2. Since
	// halfCameraPlaneSize =  s_sinHalfFov * radious, we can be reformulated
	// this as:
	// cameraPlaneDist^2 + s_sinHalfFov^2 * radious^2 = radious^2
	// cameraPlaneDist^2 / radious^2 + s_sinHalfFov^2 = 1
	// cameraPlaneDist^2 / radious^2 = 1 - s_sinHalfFov^2;
	// cameraPlaneDist^2 = radious^2 * (1 - s_sinHalfFov^2);
	// cameraPlaneDist = sqrt( radious^2 * (1 - s_sinHalfFov^2) );
	float distSq = (cameraPos - itemPos).lengthSquared();
	float radious = s_fastSqrt.calc( distSq / (1 - s_sinHalfFov * s_sinHalfFov) );
	if (s_fastSqrt.overflow())
	{
		// it's too distant, no matter the object's size.
		return true;
	}

	// Finally, we need to compare the object's size with the camera plane's
	// size and see if it's smaller than the percent, and now we can calculate
	// the camera plane size.
	float camPlaneSize = s_sinHalfFov * radious * 2.0f;

	// if the object is less than X% the size of the screen, it's too distant.
	return objSize < camPlaneSize * s_screenPercentThreshold;
}


/*static*/ void EditorChunkItem::drawSelection( bool drawSelection )
{
	s_drawSelection_ = drawSelection;
}

/*static*/ bool EditorChunkItem::drawSelection()
{
	return s_drawSelection_;
}

/*static*/ void EditorChunkItem::hideAllOutside( bool hide )
{
	s_hideAllOutside_ = hide;
}

/*static*/ bool EditorChunkItem::hideAllOutside()
{
	return s_hideAllOutside_;
}


/**
 *	This method allows other classes to register functors that will be called
 *	whenever a chunk item is modified.
 *	IMPORTANT: If you add a callback, you must delete it before exiting to
 *	avoid a mem leak!
 *	
 *	@param onModifyCallback	Functor to be called on modify.
 */
/*static*/ void EditorChunkItem::addOnModifyCallback( Callback * onModifyCallback )
{
	BW_GUARD;

	MF_ASSERT( onModifyCallback );
	s_onModifyCallback_.insert( onModifyCallback );
}


/**
 *	This method allows other classes to register functors that will be called
 *	whenever a chunk item is modified.
 *	
 *	@param onModifyCallback	Functor no longer needed.
 */
/*static*/ void EditorChunkItem::delOnModifyCallback( Callback * onModifyCallback )
{
	BW_GUARD;

	MF_ASSERT( onModifyCallback );
	s_onModifyCallback_.erase( onModifyCallback );
}


/**
 *	This method allows other classes to register functors that will be called
 *	whenever a chunk item is deleted.
 *	IMPORTANT: If you add a callback, you must delete it before exiting to
 *	avoid a mem leak!
 *	
 *	@param onDeleteCallback	Functor to be called on delete.
 */
/*static*/ void EditorChunkItem::addOnDeleteCallback( Callback * onDeleteCallback )
{
	BW_GUARD;

	MF_ASSERT( onDeleteCallback );
	s_onDeleteCallback_.insert( onDeleteCallback );
}


/**
 *	This method allows other classes to unregister functors that will be called
 *	whenever a chunk item is deleted.
 *	
 *	@param onDeleteCallback	Functor no longer needed.
 */
/*static*/ void EditorChunkItem::delOnDeleteCallback( Callback * onDeleteCallback )
{
	BW_GUARD;

	MF_ASSERT( onDeleteCallback );
	s_onDeleteCallback_.erase( onDeleteCallback );
}


void EditorChunkItem::recordMessage( BWMessageInfo * message )
{
	BW_GUARD;

	linkedMessages_.insert( message );
}

void EditorChunkItem::deleteMessage( BWMessageInfo * message )
{
	BW_GUARD;

	linkedMessages_.erase( message );
}

void EditorChunkItem::doItemDeleted()
{
	BW_GUARD;

	if (!linkedMessages_.empty())
	{
		for (BW::set< BWMessageInfo * >::iterator i = linkedMessages_.begin(); 
			i != linkedMessages_.end(); i ++)
		{
			(*i)->deleteItem();
		}
		MsgHandler::instance().forceRedraw( true );
	}
}


void EditorChunkItem::doItemRemoved()
{
	BW_GUARD;

	if (!linkedMessages_.empty())
	{
		for (BW::set< BWMessageInfo * >::iterator i = linkedMessages_.begin(); 
			i != linkedMessages_.end(); i ++)
		{
			(*i)->hideSelf();
		}
		MsgHandler::instance().forceRedraw( true );
	}
}


void EditorChunkItem::doItemRestored()
{
	BW_GUARD;

	if (!linkedMessages_.empty())
	{
		for (BW::set< BWMessageInfo * >::iterator i = linkedMessages_.begin(); 
			i != linkedMessages_.end(); i ++)
		{
			(*i)->displaySelf();
		}
		MsgHandler::instance().forceRedraw( true );
	}
}

BW_END_NAMESPACE

// editor_chunk_item.cpp
