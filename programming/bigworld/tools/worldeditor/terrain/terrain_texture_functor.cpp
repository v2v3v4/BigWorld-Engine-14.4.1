#include "pch.hpp"

#include "terrain_texture_functor.hpp"

#include "worldeditor/terrain/mask_overlay.hpp"
#include "worldeditor/terrain/terrain_texture_utils.hpp"
#include "worldeditor/undo_redo/terrain_texture_layer_undo.hpp"
#include "worldeditor/import/import_image.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/height/height_module.hpp"
#include "worldeditor/misc/editor_renderable.hpp"
#include "worldeditor/gui/pages/page_terrain_texture.hpp"

#include "common/editor_chunk_terrain_lod_cache.hpp"

#include "chunk/base_chunk_space.hpp"

#include "romp/flora.hpp"

#include "gizmo/tool.hpp"

#include "cstdmf/bwrandom.hpp"

BW_BEGIN_NAMESPACE

/*
 *	Most of the code below is self-explanatory.  Here we summarise a few 
 *	non-obvious parts of the code.
 *
 *	The original strength used for terrain painting was an arbitrary scale
 *	from 0 to 4000.  It was highly non-linear - for example the difference at 
 *	painting at 0 and at 20 were similar to the changes from 100 to 4000.  We
 *	now convert the scale to the range 0 to 100 and then 'linearise' it by 
 *	hand by using a LUT with linear interpolation between the hand-chosen 
 *	values.  The default values can be overridden by creating a file 
 *	resources/data/terrain_paint_strengths.xml.
 *
 *	When painting a layer on a chunk we try and see if the layer already 
 *	exists.  If it doesn't then we try to add the layer.  This can if the user
 *	has limited the
 *	number of layers.  In this case we replace the weakest layer (the one whose
 *	blends sum up to the least value) with the new layer.
 *
 *	For the textures on the terrain the blends at a point must add to 255.  
 *	When painting we increase the contribution of the layer being painted, and 
 *	so we must reduce the contribution by other layers.  If we are replacing 
 *	textures then we find the sum of their contribution, subtract from it the 
 *	increase made to the layer being painted and re-weight their contributions 
 *	so that relative values remain the same but now sum to the value.  
 *	Finally we look at the layers not yet considered and re-weight them so 
 *	that the sum of all layers is 255 and so that their relative contributions 
 *	remain the same.  A lot of this code is optimised, for example quite often 
 *	the layer being painted will be at full strength (255) and so the 
 *	contribution of other layers must be zero.
 *
 *	When doing the re-weighting of layers we use 32 fixed point arithmetic.  
 *	This helps prevent arithmetic errors and overflow/underflow conditions.  As
 *	with most fixed-point arithmetic we have to be careful when, for example,
 *	converting from the fixed point values to integer values by adding on 0.5
 *	in the fixed point system (see HALF_STRENGTH below) before shifting right.
 *
 *	One other problem encountered is that although we use 32 bit arithmetic for
 *	intermediate results the blend values themselves are always 8 bit.  This 
 *	means that accumulation errors can occur which can result in banding.  At
 *	low strengths this is very easily seen.  To combat this we modulate the
 *	strength at low values by a randomness factor which results in a dithered
 *	pattern.
 */


namespace 
{

// The size of the masked overlay image.
const uint32	MASK_OVERLAY_SIZE		= 64;

// Size to expand the tool's influence for drawing mask overlays:
const float		MASK_OVERFLOW_SIZE		= 50.0f;

// These values are used in fixed-point inside the painting calculations:
const int		STRENGTH_EXP			= 20;
const int		FULL_STRENGTH			= 1 << STRENGTH_EXP;
const int		HALF_STRENGTH			= 1 << (STRENGTH_EXP - 1);

// These are used to preventing banding at low strengths
const float		LOW_STRENGTH_DITHER		= 8.75f;
const float		LOW_RANDOM_DITHER		= 0.5f;
const float		HIGH_RANDOM_DITHER		= 1.0f;

} // anonymous namespace


PY_TYPEOBJECT( TerrainTextureFunctor )

PY_BEGIN_METHODS( TerrainTextureFunctor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TerrainTextureFunctor )
	PY_ATTRIBUTE( mode			    )
    PY_ATTRIBUTE( paintBrush        )
	PY_ATTRIBUTE( displayOverlay    )
	PY_ATTRIBUTE( lastPaintedLayer  )
	PY_ATTRIBUTE( lastPaintedPos    )    
	PY_ATTRIBUTE( hadEscapeKey      )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( TerrainTextureFunctor, "TerrainTextureFunctor", Functor )

FUNCTOR_FACTORY( TerrainTextureFunctor )


/**
 *	Constructor.
 */
TerrainTextureFunctor::TerrainTextureFunctor( PyTypeObject * pType ):
	TerrainFunctor( pType ),
	mode_( PageTerrainTexture::PAINTING ),
    falloff_( 0.0f ),
	randomStrength_( 1.0f ),
	randomness_( 1.0f ),
    pCurrentTerrain_( NULL ),
    pPaintBrush_( TerrainPaintBrushPtr( new TerrainPaintBrush(), true ) ),
	displayOverlay_( 1 ),
	lastPaintedLayer_( (int)EditorChunkTerrain::NO_DOMINANT_BLEND ),
	hadEscKey_( 0 ),
	currentStrength_( 0.0f )
{
	BW_GUARD;

	undoName_ = LocaliseUTF8( L"GIZMO/UNDO/TERRAIN_TEXTURE" );

	BW::vector<Vector2> strengthTable;

	// Try and read the strength table from a file:
	DataResource strengths;
	if (
		strengths.load( "resources/data/terrain_paint_strengths.xml" ) 
		== 
		DataHandle::DHE_NoError )
	{
		DataSectionPtr pSection = strengths.getRootSection();
		if (pSection)
		{
			Vector2 infVec( 
				std::numeric_limits< float >::max(), 
				std::numeric_limits< float >::max() );
			BW::vector< DataSectionPtr > strengthSects;
			pSection->openSections( "strength", strengthSects);
			for (size_t i = 0; i < strengthSects.size(); ++i)
			{
				Vector2 s = strengthSects[ i ]->asVector2( infVec );
				if (s != infVec)
				{
					strengthTable.push_back( s );
				}
			}
		}

		// Handle the degenerate case where there was one entry.  We construct
		// a good table below based upon the table being entry if something
		// went wrong.
		if (strengthTable.size() == 1)
		{
			strengthTable.clear();
		}
	}

	// If the strength table is still empty then create a known, good table:
	if (strengthTable.size() == 0)
	{
		strengthTable.push_back( Vector2(   0.0f,     0.0f ) );
		strengthTable.push_back( Vector2(  20.0f,   200.0f ) );
		strengthTable.push_back( Vector2(  50.0f,   500.0f ) );
		strengthTable.push_back( Vector2(  80.0f,  1200.0f ) );
		strengthTable.push_back( Vector2(  99.0f,  4000.0f ) ); //  99% may not give full strength straight away
		strengthTable.push_back( Vector2( 100.0f, 40000.0f ) ); // 100% is full strength in a single frame
	}

	strengthLUT_.data( strengthTable );
	strengthLUT_.lowerBoundaryCondition( LinearLUT::BC_ZERO            );
	strengthLUT_.lowerBoundaryCondition( LinearLUT::BC_CONSTANT_EXTEND );
}


/**
 *	This is called when a mouse button or key is pressed.  We use this to
 *	detect if the escape key has been pressed.
 *
 *	@param keyEvent		The event that has occurred.
 *	@param tool			The current tool.
 *	@return				The base class return value.
 */
/*virtual*/ bool TerrainTextureFunctor::handleKeyEvent(
	const KeyEvent &	keyEvent, 
	Tool &				tool )
{
	BW_GUARD;

	if (keyEvent.key() == KeyCode::KEY_ESCAPE)
	{
		hadEscKey_ = 1;
	}
	return TerrainFunctor::handleKeyEvent( keyEvent, tool );
}


/**
 *	This method is called once per frame, and if the left mouse button
 *	is down, applies the alpha blending function.
 *
 *	@param	dTime	the change in time since the last frame.
 *	@param	tool	the tool we are using.
 */
void TerrainTextureFunctor::update( float dTime, Tool & tool )
{
	BW_GUARD;

	this->updateMaskProjection( tool );

	bool isPainting =
		mode_ == PageTerrainTexture::PAINTING 
		&& 
		this->dragHandler().isDragging(MouseDragHandler::KEY_LEFTMOUSE)
		&&
		!InputDevices::isShiftDown()
		&&
		!InputDevices::isCtrlDown()
		&&
		!InputDevices::isAltDown()
		&&
		!WorldManager::instance().isMemoryLow( true /* test immediately */ );

	// Did the user stop painting?
	if (!isPainting)
	{
		this->stopApplying( tool, true );
	}

	// Is the user painting?
	else
	{
		this->doApply( dTime, tool, isPainting );
	}
}


void TerrainTextureFunctor::doApply( float dTime,
	Tool & tool,
	bool isPainting,
	bool allVerts )
{
	BW_GUARD;

	falloff_			= 2.0f/tool.size();
	currentStrength_	= strengthLUT_( tool.strength() )*dTime;

	// Set the randomness based on the strength.  Anything below 15 can 
	// quantise badly.  To fix this we multiply the strength at each point
	// by a random number between randomness and 1.0.
	randomness_ = 
		Math::lerp(
		tool.strength(), 
		0.0f, LOW_STRENGTH_DITHER, 
		LOW_RANDOM_DITHER, HIGH_RANDOM_DITHER );
	randomness_ = 
		Math::clamp( LOW_STRENGTH_DITHER, randomness_, HIGH_RANDOM_DITHER );

	// Did the user start painting?
	if (!applying_)
	{
		EditorChunkTerrainLodCache::disable();
		TextureMaskCache::instance().paintBrush( pPaintBrush_ );
		this->beginApply();
	}

	// Set currentTerrain_ to the current chunk:
	Chunk * pChunk = tool.currentChunk();

	pCurrentTerrain_  = NULL;

	if (pChunk != NULL)
	{
		pCurrentTerrain_ = 
			static_cast<EditorChunkTerrain*>
			(
			ChunkTerrainCache::instance( *pChunk ).pTerrain()
			);

		if (pCurrentTerrain_)
		{
			lastPaintedLayer_ = static_cast<int>(
				this->blendLevel( pCurrentTerrain_->block() ) );
			lastPaintedPos_   = tool.locator()->transform().applyToOrigin();
		}
	}

	if (!pCurrentTerrain_)
	{
		lastPaintedLayer_ = (int)EditorChunkTerrain::NO_DOMINANT_BLEND;
		lastPaintedPos_	  = Vector3::zero();
	}

	// Calculate the random strength multiplier once per frame. Before,
	// this was done per pixel, but it resulted in noisy blends and, more
	// importantly, in seams at chunk borders.
	randomStrength_ = bw_random( randomness_, 1.0f );

	TerrainFunctor::doApply( tool );
}


void TerrainTextureFunctor::onBeginUsing( Tool & tool )
{
	BW_GUARD;
}


/**
 *	This is called when the tool that has the functor is being popped.
 *
 *	@param tool		The tool being popped.
 */
void TerrainTextureFunctor::onEndUsing( Tool & tool )
{
	BW_GUARD;

	this->invalidateOverlays();
	this->stopApplying( tool, true );
}


/**
 *	This method handles right click events for the TerrainTextureFunctor.
 *	Currently, it's used to tell right-clicks from mouse camera movement.
 *
 *	@param tool		The tool to use.
 *	@return true	The key event was handled.
 */
bool TerrainTextureFunctor::handleContextMenu( Tool & tool )
{
	BW_GUARD;

	WorldManager::instance().chunkTexturesContextMenu( tool.currentChunk() );

	return true;
}


/**
 * This method validates the alpha-values in the chunk.
 * It is called once per chunk that is under the brush.
 *
 * @param ect	The editorChunkTerrain that is being valid.
 */
void TerrainTextureFunctor::getBlockFormat(
    const EditorChunkTerrain &		chunkTerrain,
    TerrainUtils::TerrainFormat &	format ) const
{
	BW_GUARD;

    const Terrain::EditorBaseTerrainBlock & block = 
    	chunkTerrain.block();
	size_t numLayers = block.numberTextureLayers();
    if (numLayers != 0)
    {
        // For the moment all blend layers are the same size, in the future 
        // they won't be.
		const Terrain::TerrainTextureLayer & layer = block.textureLayer(0);
        format.polesWidth   = layer.width();
        format.polesHeight  = layer.height();
        // TODO: find out if this -1 is correct. It makes terrain painting
        // accurate, but the actual error might be somewhere else.
        format.blockWidth   = format.polesWidth  - 1;
        format.blockHeight  = format.polesHeight - 1;
        format.poleSpacingX = block.blockSize()/format.blockWidth ;
        format.poleSpacingY = block.blockSize()/format.blockHeight;
    }
}


/**
 *	This is called whenever a new terrain is touched by the tool, it can be used
 *	to save the undo/redo buffer for example.
 *
 *  @param chunkTerrain	The new terrain touched.
 */
void TerrainTextureFunctor::onFirstApply( EditorChunkTerrain & chunkTerrain )
{
	BW_GUARD;

    if (!pPaintBrush_ || pPaintBrush_->paintTexture_.empty())
	{
        return;
	}

	TerrainTextureLayerUndo * pUndo = 
		new TerrainTextureLayerUndo( &chunkTerrain.block(), chunkTerrain.chunk() );
	UndoRedo::instance().add( pUndo );

	Terrain::EditorBaseTerrainBlock & block = chunkTerrain.block();

	chunkTerrain.brushTextureExist_ = false;

    // Find the layer with the texture in it:
	size_t level = this->blendLevel( block );

	// If the block has this texture we may still need to cut down on the 
	// number of layers.
	if (level != Terrain::EditorBaseTerrainBlock::INVALID_LAYER)
	{
		// If we are asked to limit the number of layers then do so.
		if (pPaintBrush_->maxLayerLimit_ && pPaintBrush_->maxLayers_ != 0)
		{
			TerrainTextureUtils::limitLayers( &chunkTerrain, 
				pPaintBrush_->maxLayers_, level );
		}
		// Reget the level, it could have moved around
		chunkTerrain.brushTextureExist_ = true;
	}

}


/**
 *	This method paints alpha into a height pole.
 *	It is called once per height pole per chunk.
 *
 *	@param	chunkTerrain The terrain that is being considered.
 *	@param	toolOffset	The offset of the tool.
 *	@param	chunkOffset	The offset of the chunk.
 *  @param  format		The format of the area being edited.
 *  @param  minx		The minimum x coord of the area of the block to change.
 *  @param  minz		The minimum z coord of the area of the block to change.
 *  @param  maxx		The maximum x coord of the area of the block to change.
 *  @param  maxz		The maximum z coord of the area of the block to change.
 */
void TerrainTextureFunctor::applyToSubBlock(
	EditorChunkTerrain &				chunkTerrain,
	const Vector3 & 					toolOffset,
	const Vector3 &						chunkOffset,
	const TerrainUtils::TerrainFormat & format,
	int32								minx,
	int32								minz,
	int32								maxx,
	int32								maxz )
{
	BW_GUARD;

    if (!pPaintBrush_
    	||
		pPaintBrush_->paintTexture_.empty() 
		|| 
		fullBlocks_.find( &chunkTerrain.block() ) != fullBlocks_.end() )
	{
        return;
	}

	if (WorldManager::instance().isMemoryLow( true /* test immediately */ ))
	{
		ERROR_MSG( "TerrainTextureFunctor: Memory is Low, "
			"failed to edit terrain textures on %s\n",
			chunkTerrain.block().resourceName().c_str() );
        return;
	}


	Terrain::EditorBaseTerrainBlock & block = chunkTerrain.block();
	size_t numLayers  = block.numberTextureLayers();

	static size_t MAX_TEXTURE_LAYERS = 16;

	if (!pPaintBrush_->maxLayerLimit_ && (numLayers > MAX_TEXTURE_LAYERS ||
		!chunkTerrain.brushTextureExist_ && (numLayers >= MAX_TEXTURE_LAYERS)))
	{
		ERROR_MSG( "There are too many texture layers on this chunk, "
			"please use the Automatic Layer Limit if needed. \n"
			"Failed to edit terrain textures on %s\n",
			chunkTerrain.block().resourceName().c_str() );
		return;
	}

	if (!chunkTerrain.brushTextureExist_)
	{
		// texture layer not exists, try to create one
		bool needCreate = false;

		if (pPaintBrush_->textureMask_)  // has mask, check if tool is visible
		{
			MaskResult * pMaskResult = 
				TextureMaskCache::instance().mask( &block );
			MaskImage  * pMaskImage = &pMaskResult->image_;
			
			if (pMaskImage)
			{
				for (int32 z = minz; !needCreate && z <= maxz; z++)
				{
					for (int32 x = minx; !needCreate && x <= maxx; x++)
					{
						MaskPixel pixel = pMaskImage->get(x, z);
						needCreate = pixel.fuzziness_ > 0 && pixel.maxValue_ > 0;
					}
				}
			}

			if (!needCreate)
				return;
		}

		needCreate = true;

		// create new layer
		// check limitation
		if (pPaintBrush_->maxLayerLimit_ && pPaintBrush_->maxLayers_)
		{
			// If there is a mask texture and this block has it then replace
			// the mask texture with what we are painting.
			size_t mlevel = this->maskLevel( block );
			if (mlevel != Terrain::EditorBaseTerrainBlock::INVALID_LAYER)
			{
				TerrainTextureUtils::replaceTexture(
					mlevel, 
					&chunkTerrain, 
					pPaintBrush_->paintTexture_, 
					pPaintBrush_->paintBumpTexture_,
					pPaintBrush_->paintUProj_, pPaintBrush_->paintVProj_, 
					false, // we do undo ourselves
					false, // don't add an undo barrier
					false, // just this chunk, don't do a floodfill
					false ); // don't regenerate texture LODs immediately
				TerrainTextureUtils::limitLayers( &chunkTerrain, 
					pPaintBrush_->maxLayers_, mlevel );
				needCreate = false;
			}
			else if (pPaintBrush_->maxLayers_ == numLayers)  // full, need remove some layer
			{
				TerrainTextureUtils::limitLayers( &chunkTerrain, 
					pPaintBrush_->maxLayers_ - 1 );
			}
		}

		if (needCreate)
		{
			size_t level = block.insertTextureLayer();
			if (level == Terrain::EditorBaseTerrainBlock::INVALID_LAYER)
			{
				// Adding the new layer failed, print an error message and add it
				// to the list of full blocks and exit
				fullBlocks_.insert( &block );
				ERROR_MSG(
					"Can't paint chunk %s, it reached the maximum number of textures (%d)\n",
					chunkTerrain.chunk()->identifier().c_str(),
					block.numberTextureLayers() );
				return;
			}

			Terrain::TerrainTextureLayer &ttl = block.textureLayer( level );
			if (!ttl.textureName( pPaintBrush_->paintTexture_ ))
			{
				// couldn't load the image for some reason, so undo the latest
				// changes and return.
				block.removeTextureLayer( level );
				pPaintBrush_->paintTexture_.clear();
				return;
			}

			if (!ttl.bumpTextureName( pPaintBrush_->paintBumpTexture_ ))
			{
				// couldn't load the image for some reason, so undo the latest
				// changes and return.
				block.removeTextureLayer( level );
				pPaintBrush_->paintBumpTexture_.clear();
				return;
			}

			Terrain::TerrainTextureLayerHolder holder( &ttl, false );
			if (ttl.hasUVProjections())
			{
				ttl.uProjection( pPaintBrush_->paintUProj_ );
				ttl.vProjection( pPaintBrush_->paintVProj_ );
			}
			
			numLayers = block.numberTextureLayers();

			TextureMaskCache::instance().changedLayers( &block );
		}

		chunkTerrain.brushTextureExist_ = true;

	}

    // Resize our temporary buffers:
    images_.resize( numLayers );

    // Lock all of the textures and get the images:
    for (size_t i = 0; i < numLayers; ++i)
    {
        chunkTerrain.block().textureLayer( i ).lock( false );
        images_[ i ] = &chunkTerrain.block().textureLayer( i ).image();
    }

	// TODO: this code assumes that blends are all the same size
	int width  = images_[0]->width();
	int height = images_[0]->height();
	minx = Math::clamp( 0, int( minx ), width  - 1 );
	minz = Math::clamp( 0, int( minz ), height - 1 );
	maxx = Math::clamp( 0, int( maxx ), width  - 1 );
	maxz = Math::clamp( 0, int( maxz ), height - 1 );

	// Choose the more optimal version (no masking) if possible:
	if (pPaintBrush_->textureMask_ == 0)
	{
		this->applyToSubBlockNoTexMask(
			chunkTerrain, 
			toolOffset, 
			chunkOffset, 
			format, minx, minz, maxx, maxz );
	}
	else
	{
		this->applyToSubBlockTexMask(
			chunkTerrain, 
			toolOffset, 
			chunkOffset, 
			format, minx, minz, maxx, maxz );
	}

    // Unlock all of the textures and get the images:
    for (size_t i = 0; i < numLayers; ++i)
    {
        chunkTerrain.block().textureLayer( i ).unlock();
    }

	// Let the block know that the layers have all changed:
	chunkTerrain.block().rebuildCombinedLayers( true, false ); // don't generate materials
	
	// Let WorldEditor know about the change in textures, it may be interested:
	WorldManager::instance().chunkTexturesPainted( 
		chunkTerrain.chunk(), 
		false );

	// rebuildLodTexture() is called in onLastApply (on mouse up)
}


/**
 *  This is called when the functor has finished updating a block.
 *
 *	@param t		The tool being applied.
 */
void TerrainTextureFunctor::onApplied( Tool & t )	
{
	BW_GUARD;

	for	( 
		ChunkPtrVector::iterator it = t.relevantChunks().begin();
		it != t.relevantChunks().end();
		++it)
	{
		if (*it)
		{
			EditorChunkTerrain * pChunkTerrain = 
				static_cast<EditorChunkTerrain*>(
					ChunkTerrainCache::instance(**it).pTerrain() );
			if (pChunkTerrain != NULL)
			{
				if (newTouchedChunk( pChunkTerrain ))
				{
					this->onFirstApply( *pChunkTerrain );
				}
			}
		}
	}
};


/**
 *  This method is called once a chunk has been changed and the user has
 *  finished painting.
 *
 *  @param chunkTerrain	The chunk terrain that the blend was applied to.
 */ 
void TerrainTextureFunctor::onLastApply( EditorChunkTerrain & chunkTerrain )
{
	BW_GUARD;

    if (pPaintBrush_->paintTexture_.empty() ||
		WorldManager::instance().isMemoryLow( true /* test immediately */ ))
	{
        return;
	}

	Terrain::EditorBaseTerrainBlock &block = chunkTerrain.block();

	TerrainTextureUtils::deleteEmptyLayers(
		block, 
		TerrainTextureUtils::REMOVE_LAYER_THRESHOLD	);

	// If we are asked to limit the layers then need to update overlay mask:
	if (pPaintBrush_->maxLayerLimit_ && pPaintBrush_->maxLayers_ && 
		pPaintBrush_->maxLayers_ == block.numberTextureLayers())
	{
		PageTerrainTexture::instance()->updatePythonMask();
	}

	TextureMaskCache::instance().changedLayers( &block );

    // Let the block know that the layers have all changed:	
    block.rebuildCombinedLayers( true, true );
	bool rebuiltLodTexture = false; 
	// Uncomment the code below to regenerate LOD textures when the mouse
	// goes up.  This makes painting slower, but the visual feedback is more
	// accurate.
	/* rebuiltLodTexture = block.rebuildLodTexture( ect.chunk()->transform() );
	if (!rebuiltLodTexture )
	{
		WARNING_MSG("There was problem refreshing lod texture for block,"
			" lod texture was not updated.\n" );
	}*/

	// Let WorldEditor know about the change in textures, it may be interested:
	WorldManager::instance().chunkTexturesPainted( 
		chunkTerrain.chunk(), rebuiltLodTexture );

	// Reset the set of chunks with all texture layers full
	fullBlocks_.clear();
}


/**
 *	This function is called to determine whether a wait cursor should be 
 *	showed during onLastApply.  Since this operation could take a while this
 *	returns true.
 *
 *	@return 		This operation could take a while, show the wait cursor.
 */
bool TerrainTextureFunctor::showWaitCursorOnLastApply() const
{
	return true;
}


/**
 *	This function sets the mode of the functor.  See PageTerrainTexture::Mode 
 *	for more details.
 *
 *  @param m		The new mode.
 */
void TerrainTextureFunctor::mode( int m )
{
	BW_GUARD;

	mode_ = m;
	if (mode_ != PageTerrainTexture::PAINTING)
	{
		this->invalidateOverlays();
	}
}


/**
 *	This function gets the mode of the functor.  See PageTerrainTexture::Mode 
 *	for more details.
 *
 *  @return 		The mode.
 */
int TerrainTextureFunctor::mode() const
{
	return mode_;
}


/**
 *	This gets whether the mask overlay is displayed.
 *
 *  @return 			Non-zero if the display overlay is displayed.
 */
int TerrainTextureFunctor::displayOverlay() const
{
	return displayOverlay_;
}


/**
 *  This gets the terrain paint brush.
 *
 *  @return             The terrain paint brush.
 */
TerrainPaintBrushPtr TerrainTextureFunctor::paintBrush() const
{
    return pPaintBrush_;
}


/**
 *  This sets the terrain paint brush.
 *
 *  @param brush		The terrain paint brush.
 */
void TerrainTextureFunctor::paintBrush( TerrainPaintBrushPtr brush )
{
	BW_GUARD;

    if (!pPaintBrush_ || (brush && *brush != *pPaintBrush_))
    {
        pPaintBrush_ = brush;
    }

    this->invalidateOverlays();

}


/**
 *	This sets whether the mask overlay is displayed.
 *
 *  @param display		If non-zero then the mask overlay is displayed.
 */
void TerrainTextureFunctor::displayOverlay( int display )
{
	BW_GUARD;

	if (displayOverlay_ != display)
	{
		displayOverlay_ = display;
		this->invalidateOverlays();
	}
}


/**
 *	This gets the index of the last painted layer.
 *
 *	@return 			The index of the last painted layer.
 */
int TerrainTextureFunctor::lastPaintedLayer() const
{
	return lastPaintedLayer_;
}


/**
 *	This sets the index of the last painted layer.
 *
 *	@param layer		The index of the last painted layer.
 */
void TerrainTextureFunctor::lastPaintedLayer( int layer )
{
	lastPaintedLayer_ = layer;
}


/**
 *	This gets the position of the last paint.
 *
 *  @return 			The position that the last paint was done.
 */
const Vector3 & TerrainTextureFunctor::lastPaintedPos() const
{
	return lastPaintedPos_;
}


/**
 *	This sets the position of the last paint.
 *
 *  @param pos			The position that the last paint was done.
 */
void TerrainTextureFunctor::lastPaintedPos( const Vector3 & pos )
{
	lastPaintedPos_ = pos;
}


/**
 *	This returns whether the user has pressed the ESC key.
 *
 *	@return 			Non-zero if the user pressed the ESC key.
 */
int TerrainTextureFunctor::hadEscapeKey() const
{
	return hadEscKey_;
}


/**
 *	This sets whether the user has pressed the ESC key.
 *
 *	@param hadESCKey	If non-zero then the user has pressed the escape key.
 */
void TerrainTextureFunctor::hadEscapeKey( int hadEscKey )
{
	hadEscKey_= hadEscKey;
}


/**
 *	Find the blend level for the given texture and projections.
 *
 *  @param block	The terrain to find the level for.
 *  @return 		The index of the blend level, 
 *					EditorChunkTerrain::NO_DOMINANT_BLEND if there is no 
 *					appropriate	level.
 */
size_t TerrainTextureFunctor::blendLevel(
	const Terrain::EditorBaseTerrainBlock & block) const
{
	BW_GUARD;

	return 
		block.findLayer(
			pPaintBrush_->paintTexture_, pPaintBrush_->paintBumpTexture_,
			pPaintBrush_->paintUProj_, pPaintBrush_->paintVProj_, 
			TerrainTextureUtils::PROJECTION_COMPARISON_EPSILON );
}


/**
 *	Find the level that corresponds to the mask texture.  If the mask texture
 *  is not being used or it is inverted then we return 
 *	EditorChunkTerrain::NO_DOMINANT_BLEND.
 *
 *  @param block	The terrain to find the level for.
 *  @return 		The index of the mask level, 
 *					EditorChunkTerrain::NO_DOMINANT_BLEND if there is no 
 *					appropriate level.
 */
size_t TerrainTextureFunctor::maskLevel(
	const Terrain::EditorBaseTerrainBlock & block) const
{
	BW_GUARD;

	if (pPaintBrush_->textureMask_ && !pPaintBrush_->textureMaskInvert_)
	{
		for (size_t i = 0; i < block.numberTextureLayers(); ++i)
		{
			Terrain::TerrainTextureLayer const &layer = block.textureLayer(i);
			if (layer.textureName() != pPaintBrush_->textureMaskTexture_)
			{
				continue;
			}
			if 
			(
				pPaintBrush_->textureMaskIncludeProj_ 
				&& 
				!layer.sameTexture(
					pPaintBrush_->textureMaskTexture_, 
					pPaintBrush_->textureMaskIncludeBump_ ? pPaintBrush_->bumpTextureMaskTexture_ : layer.bumpTextureName(),
					pPaintBrush_->textureMaskUProj_, 
					pPaintBrush_->textureMaskVProj_, 
					TerrainTextureUtils::PROJECTION_COMPARISON_EPSILON )
			)
			{
				continue;
			}
			return i;
		}
	}
	return EditorChunkTerrain::NO_DOMINANT_BLEND;
}


/**
 *	This is called to update the mask projection textures.
 *
 *  @param tool		The current tool.
 */
void TerrainTextureFunctor::updateMaskProjection( Tool & tool )
{
	BW_GUARD;

	if (displayOverlay_ == 0 || mode_ != PageTerrainTexture::PAINTING)
	{
		return;
	}

	// Temporarily increase the size of the tool so that we display masks
	// for nearby chunks too.
	tool.findRelevantChunks( MASK_OVERFLOW_SIZE );

	// Get the relevant chunk terrain items:
	ChunkPtrVector & chunks = tool.relevantChunks();
	BW::vector <EditorChunkTerrain * > chunkTerrains;
	for (size_t i = 0; i < chunks.size(); ++i)
	{
		EditorChunkTerrain *pEct = 
            static_cast<EditorChunkTerrain*>(
                ChunkTerrainCache::instance( *chunks[ i ]).pTerrain() );
		if (pEct != NULL)
		{
			chunkTerrains.push_back( pEct );
		}
	}

	// Remove any items in chunkRenderMap_ that are no longer relevant:
	for 
	(
		ChunkRenderMap::iterator it = chunkRenderMap_.begin(); 
		it != chunkRenderMap_.end(); 		
	)
	{
		bool found = false;
		for (size_t i = 0; i < chunkTerrains.size(); ++i)
		{
			if (chunkTerrains[ i ] == it->first)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			WorldManager::instance().removeRenderable( it->second );
			it = chunkRenderMap_.erase( it );
		}
		else
		{
			++it;
		}
	}

	// Add new items:
	for (size_t i = 0; i < chunkTerrains.size(); ++i)
	{
		if (chunkRenderMap_[ chunkTerrains[ i ] ] == NULL)
		{
			Moo::Image< uint8 > img;
			this->createOverlayImage( chunkTerrains[ i ], img );
			MaskOverlay * pOverlay = new MaskOverlay( chunkTerrains[ i ], img );
			chunkRenderMap_[ chunkTerrains[ i ] ] = pOverlay;
			WorldManager::instance().addRenderable( pOverlay );
		}
	}

	tool.findRelevantChunks(); // Reset size and get relevant chunks again
}


/**
 *	This creates the mask overlay image for the given terrain.
 *
 *  @param terrain			The terrain to create the overlay image for.
 *  @param img				The overlay image.
 */
void TerrainTextureFunctor::createOverlayImage(
	EditorChunkTerrainPtr	pChunkTerrain, 
	Moo::Image< uint8 > &	img ) const
{
	BW_GUARD;

	img.resize( MASK_OVERLAY_SIZE, MASK_OVERLAY_SIZE );

	TerrainUtils::TerrainFormat format;
	getBlockFormat( *pChunkTerrain, format );

	Vector3 terrainPos = pChunkTerrain->chunk()->transform().applyToOrigin();

	BW::vector< bool > maskLayers;
	TextureMaskCache::beginFuzziness( 
		&(pChunkTerrain->block()), 
		*pPaintBrush_, 
		maskLayers );

	for (uint32 y = 0; y < img.height(); ++y)
	{
		uint32 sy = Math::lerp( y, (uint32)0, img.height(), (uint32)0, format.polesHeight );
		for (uint32 x = 0; x < img.width(); ++x)
		{
			uint32 sx = Math::lerp( x, (uint32)0, img.width(), (uint32)0, format.polesWidth );
			Vector3 absPos( sx*format.poleSpacingX, 0.f, sy*format.poleSpacingY );
			MaskPixel maskPixel = 
				TextureMaskCache::fuzziness( &(pChunkTerrain->block()), terrainPos, absPos, *pPaintBrush_, maskLayers );
			img.set( x, y, (uint8)255 - maskPixel.fuzziness_ );
		}
	}

	TextureMaskCache::endFuzziness( &(pChunkTerrain->block()), *pPaintBrush_, maskLayers );
}


/**
 *	This is called whenever the masking options are changed, and so the 
 *	overlays are no longer valid.
 */
void TerrainTextureFunctor::invalidateOverlays()
{
	BW_GUARD;

	for (
		ChunkRenderMap::iterator it = chunkRenderMap_.begin(); 
		it != chunkRenderMap_.end();
		++it)
	{
		WorldManager::instance().removeRenderable( it->second );
	}
	chunkRenderMap_.clear();
}


/**
 *	This method paints alpha into a height pole when there is no texture mask.
 *	It is called once per height pole per chunk.
 *
 *	@param	block		The block that is being considered.
 *	@param	toolOffset	The offset of the tool.
 *	@param	chunkOffset	The offset of the chunk.
 *  @param  format		The format of the area being edited.
 *  @param  minx		The minimum x coord of the area of the block to change.
 *  @param  minz		The minimum z coord of the area of the block to change.
 *  @param  maxx		The maximum x coord of the area of the block to change.
 *  @param  maxz		The maximum z coord of the area of the block to change.
 */
void TerrainTextureFunctor::applyToSubBlockNoTexMask( 
	EditorChunkTerrain &		chunkTerrain,
	const Vector3 &				toolOffset, 
	const Vector3 &				chunkOffset,
	const TerrainUtils::TerrainFormat & format,
	int32						minx, 
	int32						minz, 
	int32						maxx, 
	int32						maxz )
{
	BW_GUARD;

	MaskResult * pMaskResult = 
		TextureMaskCache::instance().mask( &chunkTerrain.block() );
	MaskImage * pMaskImage = &pMaskResult->image_;
	if (pMaskImage == NULL)
	{
		return;
	}

	size_t level      = blendLevel( chunkTerrain.block() );
    size_t numLayers  = chunkTerrain.block().numberTextureLayers();

	if (level == EditorChunkTerrain::NO_DOMINANT_BLEND)
	{
		return;
	}

	uint8 ** pLayers = new uint8*[ numLayers ];

	for (int32 z = minz; z <= maxz; ++z)	
	{
		for ( size_t i = 0; i < numLayers; i++ )
		{
			pLayers[ i ] = images_[ i ]->getRow( z ) + minx;
		}
		for (int32 x = minx; x <= maxx; ++x)
		{
			//Get the pole's relative position to the brush.
			//Currently, the y-value is zeroed out.
			Vector3 absPos( float(x) * format.poleSpacingX, 0.f, float(z) * format.poleSpacingY );

			float rx = absPos.x - toolOffset.x;
			float rz = absPos.z - toolOffset.z;
			float rlen = ::sqrtf(rx*rx + rz*rz);		
			float adjustedStrength = (1.0f - rlen*falloff_)*currentStrength_/255.0f;
			if (adjustedStrength > 0.0f)
			{
				MaskPixel maskValue = pMaskImage->get( x, z );
				adjustedStrength *= maskValue.fuzziness_;
				if (adjustedStrength > 0.0f)
				{
					// Perturb the strength by some noise.  This helps prevent
					// banding due to the 8-bit quantization, but the random
					// multiplier is calculated once per tool apply, to avoid
					// noisy results and, more importantly, seams in at the
					// chunk boundary.
					adjustedStrength *= randomStrength_;

					// Set the value for this layer:
					uint8 *pPixel    = pLayers[ level ]; 
					uint8 startValue = *pPixel;
					uint8 newValue   = (uint8)std::min((int)( startValue + adjustedStrength), (int)maskValue.maxValue_ );
					uint8 value		 = std::max( newValue, startValue );
					*pPixel          = value;

					// This is an optimization that can happen quite often: if 
					// this layer is fully saturated then the other layers must
					// be zero:
					if (value == 255)
					{
						for (size_t i = 0; i < level; ++i)
						{
							uint8 *pixel = pLayers[ i ];
							*pixel = 0;
						}
						for (size_t i = level + 1; i < numLayers; ++i)
						{
							uint8 *pixel = pLayers[ i ];
							*pixel = 0;
						}
					}
					else 
					{			
						// Normalise the remaining layers not yet 
						// considered.
						uint32 sumOther = 0;
						for (size_t i = 0; i < level; ++i)
						{
							sumOther += *pLayers[ i ];
						}
						for (size_t i = level + 1; i < numLayers; ++i)
						{
							sumOther += *pLayers[ i ];
						}
						if (sumOther != 0)
						{
							int ratio = ((255 - value) << STRENGTH_EXP)/sumOther;
							for (size_t i = 0; i < level; ++i)
							{
								uint8 * pPixel = pLayers[ i ];
								*pPixel = (uint8)((*pPixel*ratio + HALF_STRENGTH) >> STRENGTH_EXP);
							}
							for (size_t i = level + 1; i < numLayers; ++i)
							{
								uint8 *pPixel = pLayers[ i ];
								*pPixel = (uint8)((*pPixel*ratio + HALF_STRENGTH) >> STRENGTH_EXP);
							}
						}
					}
				}
			}
			// Increment the layer pointers:
			for (size_t i = 0; i < numLayers; ++i)
			{
				++pLayers[ i ];
			}
		}
	}

	bw_safe_delete_array(pLayers);
}


/**
 *	This method paints alpha into a height pole when there is a texture mask.
 *	It is called once per height pole per chunk.
 *
 *	@param	chunkTerrain The terrain that is being considered.
 *	@param	toolOffset	The offset of the tool.
 *	@param	chunkOffset	The offset of the chunk.
 *  @param  format		The format of the area being edited.
 *  @param  minx		The minimum x coord of the area of the block to change.
 *  @param  minz		The minimum z coord of the area of the block to change.
 *  @param  maxx		The maximum x coord of the area of the block to change.
 *  @param  maxz		The maximum z coord of the area of the block to change.
 */
void TerrainTextureFunctor::applyToSubBlockTexMask
( 
	EditorChunkTerrain &		chunkTerrain,
	const Vector3 &				toolOffset, 
	const Vector3 &				chunkOffset,
	const TerrainUtils::TerrainFormat & format,
	int32						minx, 
	int32						minz, 
	int32						maxx, 
	int32						maxz )
{
	BW_GUARD;

	MaskResult * pMaskResult = 
		TextureMaskCache::instance().mask( &chunkTerrain.block() );
	MaskImage  * pMaskImage = &pMaskResult->image_;
	if (pMaskImage == NULL)
	{
		return;
	}

	size_t level      = this->blendLevel( chunkTerrain.block() );
    size_t numLayers  = chunkTerrain.block().numberTextureLayers();

	if (level == EditorChunkTerrain::NO_DOMINANT_BLEND)
	{
		return;
	}

	uint8 ** pLayers = new uint8*[ numLayers ];

	for (int32 z = minz; z <= maxz; ++z)	
	{
		for ( size_t i = 0; i < numLayers; i++ )
		{
			pLayers[ i ] = images_[ i ]->getRow( z ) + minx;
		}
		for (int32 x = minx; x <= maxx; ++x)
		{
			//Get the pole's relative position to the brush.
			//Currently, the y-value is zeroed out.
			Vector3 absPos( float(x) * format.poleSpacingX, 0.f, float(z) * format.poleSpacingY );

			float rx   = absPos.x - toolOffset.x;
			float rz   = absPos.z - toolOffset.z;
			float rlen = ::sqrtf( rx*rx + rz*rz );		
			float adjustedStrength = (1.0f - rlen*falloff_)*currentStrength_/255.0f;
			if (adjustedStrength > 0.0f)
			{
				MaskPixel maskValue = pMaskImage->get( x, z );
				adjustedStrength *= maskValue.fuzziness_;
				if (adjustedStrength > 0.0f)
				{
					// Perturb the strength by some noise.  This helps prevent
					// banding due to the 8-bit quantization
					adjustedStrength *= bw_random( randomness_, 1.0f );

					// Set the value for this layer:
					uint8 *pPixel    = pLayers[ level ]; 
					uint8 startValue = *pPixel;
					uint8 newValue   = (uint8)std::min( (int)(startValue + adjustedStrength), (int)maskValue.maxValue_ );
					uint8 value		 = std::max( newValue, startValue );
					*pPixel          = value;

					// This is an optimization that can happen quite often: if 
					// this layer is fully saturated then the other layers must
					// be zero:
					if (value == 255)
					{
						for (size_t i = 0; i < level; ++i)
						{
							uint8 *pPixel = pLayers[ i ];
							*pPixel = 0;
						}
						for (size_t i = level + 1; i < numLayers; ++i)
						{
							uint8 *pPixel = pLayers[ i ];
							*pPixel = 0;
						}
					}
					else 
					{			
						// Find the other contributions by other texture
						// layers in the mask:
						BW::vector<bool> const &layerMask = 
							pMaskResult->maskTexLayer_;
						uint32 sumMask = 0;
						for (size_t i = 0; i < numLayers; ++i)
						{
							if (layerMask[i] && i != level)
							{
								sumMask += *pLayers[ i ];
							}
						}

						// Remove the new contribution to the destination pixel
						// from the other layers:
						int sumDestAndMask = value;
						if (sumMask != 0)
						{
							uint8 incr       = value - startValue;
							uint32 maskRatio = (incr << STRENGTH_EXP)/sumMask;
							if (maskRatio > FULL_STRENGTH)
							{
								maskRatio = FULL_STRENGTH;
							}
							for (size_t i = 0; i < numLayers; ++i)
							{									
								if (layerMask[ i ] && i != level)
								{
									uint8 *pPixel = pLayers[ i ];
									uint8 subValue = 
										(uint8)((*pPixel*maskRatio + HALF_STRENGTH) >> STRENGTH_EXP);
									*pPixel = *pPixel - subValue;
									sumDestAndMask += *pPixel;
								}
							}
						}

						// Normalise the remaining layers not yet 
						// considered.
						uint32 sumOther = 0;
						for (size_t i = 0; i < numLayers; ++i)
						{
							if (!layerMask[ i ] && i != level)
							{
								sumOther += *pLayers[ i ];
							}
						}
						if (sumOther != 0)
						{
							if (sumDestAndMask > 255)
							{
								sumDestAndMask = 255;
							}
							int ratio = ((255 - sumDestAndMask) << STRENGTH_EXP)/sumOther;
							for (size_t i = 0; i < numLayers; ++i)
							{
								if (!layerMask[i] && i != level)
								{
									uint8 *pPixel = pLayers[ i ];
									*pPixel = (uint8)((*pPixel*ratio + HALF_STRENGTH) >> STRENGTH_EXP);
								}
							}
						}
					}
				}
			}
			// Increment the layer pointers:
			for (size_t i = 0; i < numLayers; ++i)
			{
				++pLayers[ i ];
			}
		}
	}

	bw_safe_delete_array(pLayers);
}


void TerrainTextureFunctor::stopApplyCommitChanges( Tool & tool,
	bool addUndoBarrier )
{
	BW_GUARD;

	if (applying_)
	{
		TerrainFunctor::stopApplyCommitChanges( tool, addUndoBarrier );

		Flora::floraReset();
		TextureMaskCache::instance().clear();
		EditorChunkTerrainLodCache::enable();
	}
}


/**
 *	Static python factory method
 */
PyObject *TerrainTextureFunctor::pyNew( PyObject * args )
{
	BW_GUARD;

	return new TerrainTextureFunctor();
}
BW_END_NAMESPACE

