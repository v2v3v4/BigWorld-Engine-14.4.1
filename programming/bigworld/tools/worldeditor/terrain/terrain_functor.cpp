#include "pch.hpp"

#include "terrain_functor.hpp"

#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/world_manager.hpp"

#include "gizmo/tool.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This is the HeightPoleFunctor constructor.
 *
 *  @param pType		The Python type.
 */
/*explicit*/ TerrainFunctor::TerrainFunctor( PyTypeObject * pType ):
	ToolFunctor( false, pType )
{
}


void TerrainFunctor::stopApplying( Tool & tool, bool saveChanges )
{
	BW_GUARD;
	this->stopApplyCommitChanges( tool, true );
}


/** 
 *	This method handles mouse events and saves the state of the mouse buttons
 *  for dragging/painting to work with the tools, using the dragHandler member
 *
 *	@param keyEvent	KeyEvent containing the mouse button pressed or released
 *	@param tool		Tool to use. Not used in this method
 *	@return			The event was not handled (false).
 */
bool TerrainFunctor::handleKeyEvent( const KeyEvent & keyEvent, Tool & tool ) 
{
	BW_GUARD;

	if (InputDevices::isAltDown())
	{
		return false;
	}

	switch (keyEvent.key())
	{
	case KeyCode::KEY_LEFTMOUSE:
		dragHandler_.setDragging(
			MouseDragHandler::KEY_LEFTMOUSE,
			keyEvent.isKeyDown() );

		// Check for locked chunks:
		if (keyEvent.isKeyDown())
		{
			ChunkPtrVector & chunks = tool.relevantChunks();

			ChunkPtrVector::iterator it  = chunks.begin();
			ChunkPtrVector::iterator end = chunks.end();
			
			while (it != end)
			{
				Chunk* pChunk = *it++;

				if (!EditorChunkCache::instance( *pChunk ).edIsWriteable())
				{
					WorldManager::instance().addCommentaryMsg(
						LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/TOOL/TERRAIN_FUNCTOR/CANNOT_EDITOR_UNLOCKED_TERRAIN") );
					break;
				}
			}
		}
		break;

	case KeyCode::KEY_MIDDLEMOUSE:
		dragHandler_.setDragging(
			MouseDragHandler::KEY_MIDDLEMOUSE,
			keyEvent.isKeyDown() );
		break;

	case KeyCode::KEY_RIGHTMOUSE:
		dragHandler_.setDragging(
			MouseDragHandler::KEY_RIGHTMOUSE,
			keyEvent.isKeyDown() );
		break;
	}
	return false;
}


/**
 *	This gets called on mouse events.
 *
 *	@param keyEvent		The mouse event.
 *	@param tool			The current tool.
 *	@return				The event is not handled (false).
 */
/*virtual*/ bool TerrainFunctor::handleMouseEvent
( 
	const MouseEvent &	/*keyEvent*/, 
	Tool &				/*tool*/ 
) 
{ 
	return false; 
}


/**
 *	This gets called on context menus.
 *
 *	@param tool			The current tool.
 *	@return				The event is not handled (false).
 */
/*virtual*/ bool TerrainFunctor::handleContextMenu( Tool & /*tool*/ ) 
{ 
	return false; 
};


/**
 *	This method gets all the relevant poles, and calls the virtual 
 *	applyToSubBlock function for each.
 *
 *	@param tool		The tool to use.
 *	@param allVerts	If true then iterate over all vertices in the relevant 
 *					chunks.  If false then iterate over	all vertices covered by
 *					the square region underneath the tool's location.
 */
void TerrainFunctor::doApply( Tool & tool, bool allVerts )
{
	BW_GUARD;

	if (!tool.locator() || !tool.locator()->positionValid())
	{
		// The locator's position is out of bounds, so don't paint.
		return;
	}

	ChunkPtrVector& chunks = tool.relevantChunks();

	ChunkPtrVector::iterator it  = chunks.begin();
	ChunkPtrVector::iterator end = chunks.end();

	// Bail if any of the chunks are locked:
	while ( it != end )
	{
		Chunk * pChunk = *it++;

		if (!EditorChunkCache::instance( *pChunk ).edIsWriteable())
		{
			return;
		}
	}

	// For each chunk not yet encountered during this mouse-down event,
	// call onFirstApply.  For all affected chunks call onBeginApply.
	it = chunks.begin();
	while (it != end)
	{
		Chunk * pChunk = *it++;

		EditorChunkTerrain * pChunkTerrain =
			static_cast<EditorChunkTerrain*>(
				ChunkTerrainCache::instance( *pChunk ).pTerrain());

		if (pChunkTerrain != NULL)
		{		
			if (this->newTouchedChunk( pChunkTerrain ))
			{
				this->onFirstApply( *pChunkTerrain );
			}
			this->onBeginApply( *pChunkTerrain );
		}
	}

	// For all affected chunks calculate the area that intersects with the 
	// with the tool and call applyToSubBlock.
	it = chunks.begin();
	while (it != end)
	{
		Chunk * pChunk = *it++;

		EditorChunkTerrain * pChunkTerrain = 
			static_cast< EditorChunkTerrain * >(
				ChunkTerrainCache::instance( *pChunk ).pTerrain());

		if (pChunkTerrain != NULL)
		{					
			TerrainUtils::TerrainFormat format;
			this->getBlockFormat( *pChunkTerrain, format );

			Vector3 chunkOffset = pChunk->transform().applyToOrigin();

			Vector3 toolOffset = 
				tool.locator()->transform().applyToOrigin() - chunkOffset;	

			int32 xi;
			int32 zi;
			int32 numX;
			int32 numZ;

			if (allVerts)
			{
				xi = -1 + format.polesWidth /2;
				zi = -1 + format.polesHeight/2;
				numX = format.polesWidth;
				numZ = format.polesHeight;
			}
			else
			{
				xi   = (int) floorf( (toolOffset.x / format.poleSpacingX) + 0.5f );
				zi   = (int) floorf( (toolOffset.z / format.poleSpacingY) + 0.5f );
				// The number of pixels to touch is rounded too, but we also add
				// one to the width and height of the area affected to ensure we
				// hit all pixels, even in chunk borders.
				numX = (int) floorf( (tool.size() / format.poleSpacingX) + 0.5f ) + 1;
				numZ = (int) floorf( (tool.size() / format.poleSpacingY) + 0.5f ) + 1;
			}

			int32 minx = max( int32(-1), xi - numX/int32(2) );
			int32 minz = max( int32(-1), zi - numZ/int32(2) );
			int32 maxx = min( int32(format.blockWidth ) + 1, xi + numX/2 );
			int32 maxz = min( int32(format.blockHeight) + 1, zi + numZ/2 );
		
			this->applyToSubBlock(
				*pChunkTerrain, 
				toolOffset, 
				chunkOffset, 
				format, 
				minx, minz, maxx, maxz );
		}
	}

	// For all affected chunks call onEndApply.
	it = chunks.begin();
	while (it != end)
	{
		Chunk * pChunk = *it++;

		EditorChunkTerrain * pChunkTerrain = 
			static_cast< EditorChunkTerrain * >(
				ChunkTerrainCache::instance( *pChunk ).pTerrain());

		if (pChunkTerrain != NULL)
		{
			this->onEndApply( *pChunkTerrain );
		}
	}

	this->onApplied( tool );

	// Let WorldManager know about the changed terrain blocks.
	it = chunks.begin();
	while (it != chunks.end())
	{
		Chunk * pChunk = *it++;

		EditorChunkTerrain * pChunkTerrain = 
			static_cast< EditorChunkTerrain * >(
				ChunkTerrainCache::instance( *pChunk ).pTerrain());

		if (pChunkTerrain != NULL)
		{
			WorldManager::instance().changedTerrainBlock(
				pChunk, changeCollisionScene() );
		}
	}
}


/**
 *	This method is called for all chunks before any call to applyToSubBlock.
 *
 *  @param chunkTerrain	EditorChunkTerrain object
 */
void TerrainFunctor::onBeginApply( EditorChunkTerrain & chunkTerrain )
{
	// default empty implementation
}


/**
 *	This method is called for all chunks after all calls to applToSubBlock.
 *
 *  @param chunkTerrain	EditorChunkTerrain object
 */
void TerrainFunctor::onEndApply( EditorChunkTerrain & chunkTerrain )
{
	// default empty implementation
}


/**
 *	This function is called to determine whether a wait cursor should be 
 *	showed during onLastApply.  Derived class can override this if they 
 *	onLastApply call could take a while.
 *
 *	@return 		Don't show wait cursor (false).
 */
bool TerrainFunctor::showWaitCursorOnLastApply() const
{
	return false;
}


/**
 *	Derived classes must call this method when the tool starts to be
 *  applied by the user.
 */
void TerrainFunctor::beginApply()
{
	BW_GUARD;

	touchedChunks_.clear();
	ToolFunctor::beginApply();
}


void TerrainFunctor::stopApplyCommitChanges( Tool& tool, bool addUndoBarrier )
{
	BW_GUARD;
	if (applying_)
	{
		this->endApplying();
		touchedChunks_.clear();

		ToolFunctor::stopApplyCommitChanges( tool, addUndoBarrier );
	}
}


void TerrainFunctor::stopApplyDiscardChanges( Tool& tool )
{
	BW_GUARD;
	// Should never be called
	MF_ASSERT( false );
}


/**
 *	Derived classes can call this to signify that some extra chunks may be 
 *	modified.
 *
 *	@param pChunkTerrain	The newly touched terrain chunk.
 *	@return 		True if the chunk was new, false if it has already been
 *					considered.
 */
bool TerrainFunctor::newTouchedChunk( EditorChunkTerrain * pChunkTerrain )
{
	BW_GUARD;

	std::pair<ECTSet::iterator,bool> result = 
		touchedChunks_.insert( pChunkTerrain );
	return result.second;
}


/** 
 *	Derived classes must call this method when the tool has finished being
 *	applied.
 */
void TerrainFunctor::endApplying()
{
	BW_GUARD;

	if (showWaitCursorOnLastApply())
	{
		AfxGetApp()->DoWaitCursor( 1 ); 
	}

	for 
	(
		BW::set< EditorChunkTerrain * >::iterator it = touchedChunks_.begin();
		it != touchedChunks_.end();
		++it
	)
	{
		this->onLastApply(**it);
	}

	if (showWaitCursorOnLastApply())
	{
		AfxGetApp()->DoWaitCursor( -1 ); 
	}
}


/**
 *	This function gets the drag handler.
 *
 *  @return 			The mouse drag handler.
 */
MouseDragHandler &TerrainFunctor::dragHandler()
{
	return dragHandler_;
}
BW_END_NAMESPACE

