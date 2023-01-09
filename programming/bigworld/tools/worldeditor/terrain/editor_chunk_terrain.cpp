#include "pch.hpp"
#include "editor_chunk_terrain.hpp"
#include "worldeditor/undo_redo/terrain_height_map_undo.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/gui/pages/panel_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/project/project_module.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "worldeditor/project/forced_lod_map.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/misc/sync_mode.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "appmgr/module_manager.hpp"
#include "appmgr/options.hpp"
#include "chunk/base_chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_light.hpp"
#include "terrain/base_terrain_renderer.hpp"
#include "terrain/terrain2/editor_terrain_block2.hpp"
#include "terrain/terrain_data.hpp"
#include "terrain/terrain_height_map.hpp"
#include "terrain/terrain_hole_map.hpp"
#include "terrain/terrain_settings.hpp"
#include "terrain/terrain_texture_layer.hpp"
#include "chunk/chunk_water.hpp"

DECLARE_DEBUG_COMPONENT2( "EditorChunkTerrain", 0 );

BW_BEGIN_NAMESPACE

EditorChunkTerrain::EditorChunkTerrain():
    transform_(Matrix::identity),
    pOwnSect_(NULL),
	brushTextureExist_( false )
{
	BW_GUARD;

	const float blockSize = WorldManager::instance().pTerrainSettings()->blockSize();
	// We want to pretend the origin is near (but not at) the centre so
	// everything rotates correctly when we have an even and an odd amount of 
	// terrain blocks selected
	transform_.setTranslate( 0.5f*blockSize - 0.5f, 0.f, 0.5f*blockSize - 0.5f );

	maxLayersWarning_ = new MaxLayersOverlay( this );
	forcedLodNotice_ = new ForcedLodOverlay( this );
}


EditorChunkTerrain::~EditorChunkTerrain()
{
	BW_GUARD;

	// removes the rendereable, just in case it's still hanging around.
	WorldManager::instance().removeRenderable( maxLayersWarning_ );
	WorldManager::instance().removeRenderable( forcedLodNotice_ );
}


/**
 *  This method should be called if you have changed the heights in a chunk
 *  terrain.  It makes the neighbouring changes borders match and it 
 *  recalculates the chunk's bounding box/collision scene.
 */
void EditorChunkTerrain::onEditHeights()
{
	BW_GUARD;

    // Lock the terrain height map for writing:
    Terrain::TerrainHeightMap &thm = block().heightMap();    
    { // scope to lock terrain height map
        Terrain::TerrainHeightMapHolder holder(&thm, false);
        // Coordinates of first height pole, first visible height pole, the 
        // last visible height pole, one past last height pole in x-direction.
		// It effectivelly copies one extra pole from the right chunk,
		// stitching both chunks to avoid seams
        /*uint32 x1  = 0;*/
        uint32 x2  = thm.xVisibleOffset();
        uint32 x3  = x2 + thm.verticesWidth() - 1;
        uint32 x4  = thm.polesWidth();

        // Coordinates of first height pole, first visible height pole, the 
        // last visible height pole, one past last height pole in z-direction.
		// It effectivelly copies one extra pole from the below chunk,
		// stitching both chunks to avoid seams
        /*uint32 z1  = 0;*/
        uint32 z2  = thm.zVisibleOffset();
        uint32 z3  = z2 + thm.verticesHeight() - 1;
        uint32 z4  = thm.polesHeight();

        // Copy heights from the left, right, above and below blocks:
        copyHeights(-1,  0, x3 - x2, 0      , x2     , z4     , 0  , 0 ); // left
        copyHeights(+1,  0, x2     , 0      , x4 - x3, z4     , x3 , 0 ); // right
        copyHeights( 0, -1, 0      , z3 - z2, x4     , z2     , 0  , 0 ); // above
        copyHeights( 0, +1, 0      , z2     , x4     , z4 - z3, 0  , z3); // below

        // Copy heights from the diagonals:
        copyHeights(-1, -1, x3 - x2, z3 - z2, x2     , z2     , 0 , 0 ); // left & above
        copyHeights(+1, -1, x2     , z3 - z2, x4 - x3, z2     , x3, 0 ); // right & above
        copyHeights(-1, +1, x3 - x2, z2     , x2     , z4 - z3, 0 , z3); // left & below
        copyHeights(+1, +1, x2     , z2     , x4 - x3, z4 - z3, x3, z3); // right & below
    }

	// Finally fix our bounding box, the collision scene etc.
    this->onTerrainChanged();
}


/**
 *	This method is exclusive to editorChunkTerrian.  It calculates an image
 *	of relative elevation values for each pole.
 *
 *	@param relHeights   	An image with the relative height information.
 */
void 
EditorChunkTerrain::relativeElevations
(
    Terrain::TerrainHeightMap::ImageType &relHeights
) const
{
	BW_GUARD;

	Terrain::EditorBaseTerrainBlock const &b    = block();
    Terrain::TerrainHeightMap       const &cthm = b.heightMap();
    // Cast away const - we are only reading the terrain height anyway
    Terrain::TerrainHeightMap       &thm        = const_cast<Terrain::TerrainHeightMap &>(cthm);

    Terrain::TerrainHeightMapHolder holder(&thm, true); // lock/unlock height map
    relHeights = thm.image();

    // Find the average height:
    float average = 0.0f;
    for (uint32 h = 0; h < relHeights.height(); ++h)
    {
        for (uint32 w = 0; w < relHeights.width(); ++w)
        {
            average += relHeights.get(w, h);
        }
    }
    uint32 samples = relHeights.width()*relHeights.height();
    MF_ASSERT(samples != 0);
    average /= samples;

    // Subtract the average:
    for (uint32 h = 0; h < relHeights.height(); ++h)
    {
        for (uint32 w = 0; w < relHeights.width(); ++w)
        {
            float v = relHeights.get(w, h);
            relHeights.set(w, h, v - average);
        }
    }
}


/**
 *	This method calculates the slope at a height pole, and returns the
 *	angle in degrees.
 *
 *  @param xIdx     The x coordinate of the sample to get the slope at.
 *  @param zIdx     The z coordinate of the sample to get the slope at.
 *  @returns        The slope at the given height pole in degrees.
 */
float EditorChunkTerrain::slope(int xIdx, int zIdx) const
{
	BW_GUARD;

    Terrain::EditorBaseTerrainBlock const &b   = block();
    Terrain::TerrainHeightMap       const &thm = b.heightMap();
    float x = (float)xIdx/(float)thm.verticesWidth ();
    float z = (float)zIdx/(float)thm.verticesHeight();
    Vector3 normal = b.normalAt(x, z);
	return RAD_TO_DEG(acosf(Math::clamp(-1.0f, normal.y, +1.0f)));
}


/** 
 *  This calculates the dominant blend value at the given x, z.
 *
 *	@param x		The x coordinate to get the blend at.
 *	@param z		The z coordinate to get the blend at.
 *	@param strength	If not NULL then this is set to the strength of the 
 *					dominant blend.
 */
size_t EditorChunkTerrain::dominantBlend(float x, float z, uint8 *strength) const
{
	BW_GUARD;

    size_t result  = NO_DOMINANT_BLEND;
    uint8 maxValue = 0;

    Terrain::EditorBaseTerrainBlock const &b = block();
    for (size_t i = 0; i < b.numberTextureLayers(); ++i)
    {
        Terrain::TerrainTextureLayer const &ttl = b.textureLayer(i);
        Terrain::TerrainTextureLayerHolder 
            holder(const_cast<Terrain::TerrainTextureLayer *>(&ttl), true);
        Terrain::TerrainTextureLayer::ImageType const &image = ttl.image();
		int sx = (int)(x*image.width ()/b.blockSize() + 0.5f);
        int sz = (int)(z*image.height()/b.blockSize() + 0.5f);
        uint8 v = image.get(sx, sz);
        if (v > maxValue)
        {
            result   = i;
            maxValue = v;
			if (strength != NULL) *strength = v;
        }
    }

    return result;
}


/**
 *  This function gets the transformation of the EditorChunkTerrain.
 *
 *  @returns        The EditorChunkTerrain's transformation.
 */
/*virtual*/ const Matrix & EditorChunkTerrain::edTransform()	
{ 
    return transform_; 
}


/**
 *  This function applies a transformation to the EditorChunkTerrain.
 *
 *  @param m            The transformation to apply.
 *  @param transient    Is the transformation only temporary?
 *  @returns            True if successful.
 */
bool EditorChunkTerrain::edTransform(const Matrix & m, bool transient)
{
	BW_GUARD;

	Matrix newWorldPos;
	newWorldPos.multiply( m, pChunk_->transform() );

	// If this is only a temporary change, do nothing:
	if (transient)
	{
		transform_ = m;
		this->syncInit();
		return true;
	}

	// It's permanent, so find out which chunk we belong to now:
	Chunk *pOldChunk = pChunk_;
	Chunk *pNewChunk = 
        EditorChunk::findOutsideChunk( newWorldPos.applyToOrigin() );
	if (pNewChunk == NULL)
		return false;

	EditorChunkTerrain *newChunkTerrain = 
        (EditorChunkTerrain*)ChunkTerrainCache::instance( *pNewChunk ).pTerrain();

	// Don't allow two terrain blocks in a single chunk
	// We allow an exception for ourself to support tossing into our current chunk
	// We allow an exception for a selected item as it implies we're moving a group of
	// terrain items in a direction, and the current one likely won't be there any longer.
	if 
    (
        newChunkTerrain 
        && 
        newChunkTerrain != this 
        &&
		!WorldManager::instance().isItemSelected( newChunkTerrain ) 
    )
    {
		return false;
    }


	// Make sure that the chunks aren't read only:
	if 
    (
        !EditorChunkCache::instance( *pOldChunk ).edIsWriteable() 
		|| 
        !EditorChunkCache::instance( *pNewChunk ).edIsWriteable()
    )
    {
		return false;
    }

	float gridSize = pOldChunk->space()->gridSize();
	transform_ = Matrix::identity;
	transform_.setTranslate( 0.5f*gridSize - 0.5f, 0.f, 0.5f*gridSize - 0.5f );

	// Note that both affected chunks have seen changes
	InvalidateFlags flags(InvalidateFlags::FLAG_SHADOW_MAP |
		InvalidateFlags::FLAG_NAV_MESH|
		InvalidateFlags::FLAG_THUMBNAIL );
	WorldManager::instance().changedChunk( pOldChunk, flags );
	if (pNewChunk != pOldChunk )
	{
		WorldManager::instance().changedChunk( pNewChunk, flags );
	}

	edMove( pOldChunk, pNewChunk );

	// Ok, now apply any rotation

	// Calculate rotation angle
	Vector3 newPt = m.applyVector( Vector3( 1.f, 0.f, 0.f ) );
	newPt.y = 0.f;

	float angle = acosf( Math::clamp(-1.0f, newPt.dotProduct( Vector3( 1.f, 0.f, 0.f ) ), +1.0f) );

	if (!almostZero( angle, 0.0001f) )
	{
		// dp will only give an angle up to 180 degs, make it from 0..2PI
		if (newPt.crossProduct(Vector3( 1.f, 0.f, 0.f )).y > 0.f)
			angle = (2 * MATH_PI) - angle;
	}	

	// Turn the rotation angle into an amount of 90 degree rotations
	int rotateAmount = 0;

	if (angle > (DEG_TO_RAD(90.f) / 2.f))
		++rotateAmount;
	if (angle > (DEG_TO_RAD(90.f) + (DEG_TO_RAD(90.f) / 2.f)))
		++rotateAmount;
	if (angle > (DEG_TO_RAD(180.f) + (DEG_TO_RAD(90.f) / 2.f)))
		++rotateAmount;
	if (angle > (DEG_TO_RAD(270.f) + (DEG_TO_RAD(90.f) / 2.f)))
		rotateAmount = 0;

	if (rotateAmount > 0)
	{
		MF_ASSERT( rotateAmount < 4 );

	    UndoRedo::instance().add(new TerrainHeightMapUndo(&block(), pChunk_));

		// Rotate terrain 90 degress rotateAmount amount of times

        Terrain::TerrainHeightMap &thm = block().heightMap();        
        {
            Terrain::TerrainHeightMapHolder holder(&thm, false);
            Terrain::TerrainHeightMap::ImageType &heights = thm.image();
            MF_ASSERT(heights.width() == heights.height());
            for (int r = 0; r < rotateAmount; ++r)
            {                
                // A slow but sure way of rotating:
                Terrain::TerrainHeightMap::ImageType heightsCopy(heights);
                MF_ASSERT(0); // TODO: I'm not sure if the rotate direction is ok.
                heightsCopy.rotate(true);
                heights.blit(heightsCopy);
            }
        } // rebuild DX objects for the terrain
    	WorldManager::instance().changedTerrainBlock( pChunk_ );
	}
	this->syncInit();
	return true;
}


/**
 *  This method gets the bounds of the terrain.
 */
void EditorChunkTerrain::edBounds(BoundingBox &bbRet) const
{
	BW_GUARD;

    // I removed the hard-coded numbers, but I'm not sure why we add/subtract
    // 0.5 or why 0.25 was chosen as the expansion number in the y-directions.
	const float gridSize = chunk()->space()->gridSize();
    Vector3 minb = 
        bb_.minBounds() + 
            Vector3
            ( 
                -0.5f*gridSize + 0.5f, 
                -0.25f,
                -0.5f*gridSize + 0.5f
            );
    Vector3 maxb =
        bb_.maxBounds() + 
            Vector3
            ( 
                -0.5f*gridSize - 0.5f,  
                0.25f, 
                -0.5f*gridSize - 0.5f 
            );
	bbRet.setBounds(minb, maxb);
}


bool EditorChunkTerrain::edSave( DataSectionPtr pSection )
{
	BW_GUARD;

	edCommonSave( pSection );
	pSection->writeString( "resource", pChunk_->identifier() + ".cdata/" + block().dataSectionName() );
	return true;
}


DataSectionPtr EditorChunkTerrain::pOwnSect()
{ 
    return pOwnSect_; 
}


const DataSectionPtr EditorChunkTerrain::pOwnSect()	const 
{ 
    return pOwnSect_; 
}


void EditorChunkTerrain::edPreDelete()
{
	EditorChunkItem::edPreDelete();
	WorldManager::instance().changedTerrainBlock( this->chunk() );
}


void EditorChunkTerrain::edPostCreate()
{
	WorldManager::instance().changedTerrainBlock( this->chunk() );
	EditorChunkItem::edPostCreate();
}


void EditorChunkTerrain::edPostClone(EditorChunkItem* srcItem)
{
	BW_GUARD;

	EditorChunkItem::edPostClone( srcItem );
	WorldManager::instance().changedTerrainBlock( this->chunk() );
}


BinaryPtr EditorChunkTerrain::edExportBinaryData()
{
	BW_GUARD;

	// Return a copy of the terrain block
	BW::string terrainName = "export_temp." + this->block().dataSectionName();

	BWResource::instance().purge( terrainName, true );
	BW::wstring fname;
	bw_utf8tow( BWResource::resolveFilename( terrainName ), fname );
	::DeleteFile( fname.c_str() );

	block().save( terrainName );

	BinaryPtr bp;
	{
		DataSectionPtr ds = BWResource::openSection( terrainName );
		bp = ds->asBinary();
	}

	::DeleteFile( fname.c_str() );

	return bp;
}


bool EditorChunkTerrain::edShouldDraw()
{
	BW_GUARD;

	if( !ChunkTerrain::edShouldDraw() )
		return false;

	return OptionsTerrain::visible();
}


/**
 *	The 20000 y value is pretty much just for prefabs, so that when placing it,
 *	it'll most likely end up at y = 0.f, if it's anything else items will not
 *	be sitting on the ground.
 */
Vector3 EditorChunkTerrain::edMovementDeltaSnaps()
{
	const float gridSize = chunk()->space()->gridSize();
	return Vector3( gridSize, 20000.f, gridSize );
}


float EditorChunkTerrain::edAngleSnaps()
{
	return 90.f;
}

/*virtual */bool EditorChunkTerrain::edEdit( class GeneralEditor & editor )
{
	BW_GUARD;

	if ( this->edFrozen() )
	{
		return false;
	}

	if ( edCommonEdit( editor ) == false )
	{
		return false;
	}

	Terrain::TerrainSettingsPtr pSettings = WorldManager::instance().pTerrainSettings();

	if (pSettings->version() >= Terrain::TerrainSettings::TERRAIN2_VERSION)
	{
		STATIC_LOCALISE_NAME( s_forcedLod, "WORLDEDITOR/WORLDEDITOR/TERRAIN/EDITOR_CHUNK_TERRAIN/FORCEDLOD" );

		editor.addProperty( new GenIntProperty( s_forcedLod,
			new AccessorDataProxy< EditorChunkTerrain, IntProxy >(
			this, "forcedLod", 
			&EditorChunkTerrain::getForcedLod, 
			&EditorChunkTerrain::setForcedLod ) ) );
	}

	return true;
}

void EditorChunkTerrain::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if( !edShouldDraw() )
		return;
	if (block().holeMap().allHoles())
		return;

	const float gridSize = block().blockSize();
	// Draw using our transform, to show when the user is dragging the block around
	Matrix m = transform_;
	//m.setTranslate( m[3] - Vector3( gridSize, 0.f, gridSize ) );
	m.setTranslate( m[3] - Vector3( 0.5f*gridSize - 0.5f, 0.f, 0.5f*gridSize - 0.5f ) );

	m.preMultiply( Moo::rc().world() );

	bool isWriteable = EditorChunkCache::instance( *chunk() ).edIsWriteable();

	bool shadeReadOnlyBlocks = OptionsMisc::readOnlyVisible();
	bool projectModule = ProjectModule::currentInstance() == ModuleManager::instance().currentModule();
	if( !isWriteable && WorldManager::instance().drawSelection() )
		return;

	if (WorldManager::instance().drawSelection())
	{
		WorldManager::instance().registerDrawSelectionItem( this );
	}

	if (!isWriteable && shadeReadOnlyBlocks && !projectModule )
	{
        WorldManager::instance().addReadOnlyBlock( m, ChunkTerrain::block() );
	}
	else
	{
		if (OptionsTerrain::numLayersWarningVisible() &&
			PanelManager::pInstance() &&
			PanelManager::instance().currentTool() == L"TerrainTexture")
		{
			// Show/hide too-many-textures warning overlay
			int maxNumLayers = OptionsTerrain::numLayersWarning();
			if ( maxNumLayers > 0 &&
				(int)block().numberTextureLayers() >= maxNumLayers )
			{
				// add the "maxLayersWarning_" rendereable, which will be rendered
				// next frame, and will then remove itself from WorldEditor's
				// rendereables list.
				WorldManager::instance().addRenderable( maxLayersWarning_ );
			}
		}

		if (OptionsTerrain::lodLocksOverlayVisible())
		{
			// Show/hide the LOD Locks overlay
			if (getForcedLod() != -1)
			{
				WorldManager::instance().addRenderable( forcedLodNotice_ );
			}
		}

		// Add the terrain block to the terrain's drawlist.
		Terrain::BaseTerrainRenderer::instance()->addBlock( &block(), m );
	}
}


/**
 *	Handle ourselves being added to or removed from a chunk
 */
void EditorChunkTerrain::toss( Chunk * pChunk )
{
	BW_GUARD;

	Chunk* oldChunk = pChunk_;


	if (pChunk_ != NULL)
	{
		if (pOwnSect_)
		{
			EditorChunkCache::instance( *pChunk_ ).
				pChunkSection()->delChild( pOwnSect_ );
			pOwnSect_ = NULL;
		}
	}

	EditorChunkTerrainBase::toss( pChunk );

	if (pChunk_ != NULL)
	{
		BW::string newResourceID = pChunk_->mapping()->path() + 
			pChunk_->identifier() + ".cdata/" + block().dataSectionName();

		if (block().resourceName() != newResourceID)
		{
			block().resourceName(newResourceID);
			WorldManager::instance().changedTerrainBlock( pChunk_ );
			WorldManager::instance().changedChunk( pChunk_ );
		}

		if (!pOwnSect_)
		{
			pOwnSect_ = EditorChunkCache::instance( *pChunk_ ).
				pChunkSection()->newSection( "terrain" );
		}

		this->edSave( pOwnSect_ );
	}

	// If there are any other terrain blocks in our old chunk, let them be the
	// one the terrain item cache knows about
	if (oldChunk)
	{
		EditorChunkCache::instance( *oldChunk ).fixTerrainBlocks();
	}
}


/**
 *	This gets the neighbouring terrain.
 *
 *  @param n			The neighbour to query for.
 *  @param forceLoad	Load the neighbour if not in memory?
 *	@param writable 	Does the neighbour need to be writable?
 *	@param isValid		If non-null then this is set to a member of 
 *						NeighbourError signifying whether the neighbour exists,
 *						does not exist or is locked.
 *  @returns			The neighbouring block.  This can be NULL at the edge 
 *						of the space, or if not loaded and forceLoad is false.
 */
EditorChunkTerrain *
EditorChunkTerrain::neighbour
(
	Neighbour			n, 
	bool				forceLoad		/*= false*/,
	bool				writable		/*= true*/,
	NeighbourError		*isValid		/*= NULL*/
) const
{
	BW_GUARD;

	// Calculate deltas:
	int dx = 0;
	int dz = 0;
	switch (n)
	{
	case NORTH_WEST:
		dx = -1; dz = +1;
		break;
	case NORTH:
		dx =  0; dz = +1;
		break;
	case NORTH_EAST:
		dx = +1; dz = +1;
		break;
	case EAST:
		dx = +1; dz =  0;
		break;
	case SOUTH_EAST:
		dx = +1; dz = -1;
		break;
	case SOUTH:
		dx =  0; dz = -1;
		break;
	case SOUTH_WEST:
		dx = -1; dz = -1;
		break;
	case WEST:
		dx = -1; dz =  0;
		break;
	}

	// Calculate the center of the neighbour:
    Vector3 cen  = (bb_.maxBounds() + bb_.minBounds()) * 0.5f;
	Vector3 delt = bb_.maxBounds() - bb_.minBounds();
    Vector3 lpos = cen + Vector3(dx*delt.x, 0.0f, dz*delt.z);
    Vector3 wpos = pChunk_->transform().applyPoint(lpos);

	// Find the neighbouring chunk:
	GeometryMapping *mapping = WorldManager::instance().geometryMapping();
	BW::string chunkName = mapping->outsideChunkIdentifier(wpos);
	if (chunkName.empty())
	{
		if (isValid != NULL) 
			*isValid = NEIGHBOUR_NONEXISTANT;
		return NULL;
	}
	Chunk *chunk = 
		ChunkManager::instance().findChunkByName(chunkName, mapping, false);
	if (chunk == NULL)
	{
		if (isValid != NULL) 
			*isValid = NEIGHBOUR_NONEXISTANT;
		return NULL;
	}
	if (writable && !EditorChunk::chunkWriteable(*chunk, false))
	{
		if (isValid != NULL) 
			*isValid = NEIGHBOUR_LOCKED;
		return NULL;
	}
	// Handle the case where the chunk is not loaded:
	if (!chunk->loaded())
	{
		// Chunk not loaded and we are not forcing into memory?
		if (!forceLoad)
		{
			if (isValid != NULL) 
				*isValid = NEIGHBOUR_NOTLOADED;
			return NULL;
		}
		// Force the chunk into memory:
		SyncMode chunkStopper;
		ChunkManager::instance().loadChunkNow(chunkName, mapping);
		ChunkManager::instance().checkLoadingChunks();
	}

	// Finally, get the terrain:
	if (isValid != NULL) 
		*isValid = NEIGHBOUR_OK;	
	return 
		static_cast<EditorChunkTerrain*>
        (
            ChunkTerrainCache::instance(*chunk).pTerrain()
        );
}


/*static*/ Terrain::EditorBaseTerrainBlockPtr 
EditorChunkTerrain::loadBlock
(
    BW::string         const &	resource,
    EditorChunkTerrain*			ect,
	const Vector3&				worldPosition,
	BW::string * errorString
)
{
	BW_GUARD;

	Terrain::EditorBaseTerrainBlockPtr result;

	BW::string verResource = resource;
	uint32 ver = Terrain::BaseTerrainBlock::terrainVersion( verResource );

	Terrain::TerrainSettingsPtr pSettings = WorldManager::instance().pTerrainSettings();

	if (pSettings->pRenderer() == NULL)
	{
		ERROR_MSG( "Failed to initialise terrain. "
			"Please make sure Shader Model 2.0 or higher is used.\n" );
		return NULL;
	}

	if (ver == Terrain::TerrainSettings::TERRAIN2_VERSION)
	{
		result = new Terrain::TERRAINBLOCK2(pSettings);
	}
	else
	{
		// It's not a normal terrain block (i.e. from a .cdata file). Find
		// out if it's a prefab or a cloned block, and find out which version
		// is needed.
		verResource = resource;
		uint32 prefabVer = prefabVersion( verResource );

		uint32 currentTerrainVer = pSettings->version();
		if (prefabVer != currentTerrainVer)
		{
			ERROR_MSG( "EditorChunkTerrain::loadBlock: Failed to load %s, "
				"prefab is version %d while the current terrain is version %d.\n",
				resource.c_str(), prefabVer, currentTerrainVer );
			return NULL;
		}

		if (prefabVer == Terrain::TerrainSettings::TERRAIN2_VERSION)
		{
			result = new Terrain::TERRAINBLOCK2(pSettings);
		}
		else
		{
			ERROR_MSG( "EditorChunkTerrain::loadBlock: Failed to load %s, unknown terrain version.\n", resource.c_str() );
			return NULL;
		}
	}

	result->resourceName( verResource );

	Matrix worldTransform;
	worldTransform.setTranslate( worldPosition );

	const bool ok = result->load(
		verResource,
		worldTransform,
		ChunkManager::instance().cameraTrans().applyToOrigin(),
		errorString );

	return ok ? result : NULL;
}


EditorChunkTerrainPtr EditorChunkTerrain::findChunkFromPoint( const Vector3 &pos )
{
	BW_GUARD;

	ChunkSpacePtr chunkSpace = ChunkManager::instance().cameraSpace();
	if (chunkSpace == NULL)
		return NULL;

	Chunk *chunk = chunkSpace->findChunkFromPoint(pos);
	if (chunk == NULL)
		return NULL;

	EditorChunkTerrain *ect = 
		static_cast<EditorChunkTerrain *>
		( 
			ChunkTerrainCache::instance(*chunk).pTerrain() 
		);
	return ect;
}


size_t EditorChunkTerrain::findChunksWithinBox( BoundingBox const &bb,
	EditorChunkTerrainPtrs  &outVector )
{
	BW_GUARD;


	const float gridSize = ChunkManager::instance().cameraSpace()->gridSize();

	int32 startX = (int32) floorf(bb.minBounds().x/gridSize);
	int32 endX   = (int32) floorf(bb.maxBounds().x/gridSize);

	int32 startZ = (int32) floorf(bb.minBounds().z/gridSize);
	int32 endZ   = (int32) floorf(bb.maxBounds().z/gridSize);

	for ( int32 x = startX; x <= endX; ++x )
	{
		for ( int32 z = startZ; z <= endZ; ++z )
		{
			if ( ChunkManager::instance().cameraSpace() )
			{
				ChunkSpace::Column* pColumn =
					ChunkManager::instance().cameraSpace()->column
                    ( 
                        Vector3
                        ( 
                            x * gridSize + 0.5f*gridSize, 
                            bb.minBounds().y, 
                            z * gridSize + 0.5f*gridSize 
                        ), 
                        false 
                    );

				if ( pColumn )
				{
					if ( pColumn->pOutsideChunk() )
					{
                        EditorChunkTerrain *ect = 
                            static_cast<EditorChunkTerrain *>
                            ( 
                                ChunkTerrainCache::instance
                                ( 
                                    *pColumn->pOutsideChunk() 
                                ).pTerrain() 
                             );
						outVector.push_back( ect );
					}
				}
			}
		}
	}

	return outVector.size();
}


/**
 *  Used when loading from a prefab, to get the terrain data.
 */ 
extern DataSectionPtr prefabBinarySection;


/**
 *  Validates that a prefab's terrain resource section is of the same version
 *  as the current terrain version, and returns the appropriate terrain section
 *  name in the 'resource' param
 *  
 *  @param resource       resource sub-section from the prefab's terrain
 *                        section, and contains the returned terrain section
 *                        name.
 *  @returns              true if the prefab's terrain is compatible with the
 *                        current terrain.
 */
bool EditorChunkTerrain::validatePrefabVersion( BW::string& resource )
{
	BW_GUARD;

	BW::string::size_type lastSlash = resource.find_last_of( '/' );
	if ( lastSlash != BW::string::npos )
		resource = resource.substr( lastSlash + 1 );

	uint32 prefabVer;
	if ((resource.length() >= 8) &&
		(resource.substr( resource.length() - 8 ) == "terrain2"))
	{
		prefabVer = Terrain::TerrainSettings::TERRAIN2_VERSION;
	}
	else
	{
		return false;
	}

	uint32 currentVer = WorldManager::instance().pTerrainSettings()->version();
	if ( prefabVer != currentVer )
		return false;

	return true;
}


/**
 *	This method replaces ChunkTerrain::load
 */
bool 
EditorChunkTerrain::load
( 
    DataSectionPtr      pSection, 
    Chunk               *pChunk, 
    BW::string         *errorString 
)
{
	BW_GUARD;

	if (!Terrain::BaseTerrainRenderer::instance()->version())
	{
		if (errorString)
		{
			*errorString = "Terrain setting initialisation failure\n";
		}
		return false;
	}

	if (!pChunk->isOutsideChunk())
	{
		BW::string err = "Can't load terrain block into an indoor chunk";
		if ( errorString )
		{
			*errorString = err;
		}
		ERROR_MSG( "%s\n", err.c_str() );
		return false;
	}

	edCommonLoad( pSection );

	pOwnSect_ = pSection;

	BW::string prefabBinKey = pOwnSect_->readString( "prefabBin" );
	if (!prefabBinKey.empty())
	{
		// Loading from a prefab, source our terrain block from there
		if (!prefabBinarySection)
		{
			BW::string err = "Unable to load prefab. Section prefabBin found, but no current prefab binary section";
			if ( errorString )
			{
				*errorString = err;
			}
			ERROR_MSG( "%s\n", err.c_str() );
			return false;
		}

		BinaryPtr bp = prefabBinarySection->readBinary( prefabBinKey );

		if (!bp)
		{
			BW::string err = "Unable to load " + prefabBinKey + "  from current prefab binary section";
			if ( errorString )
			{
				*errorString = err;
			}
			ERROR_MSG( "%s\n", err.c_str() );
			return false;
		}

		// Write the binary out to a temp file and read the block in from there
		BW::string terrainRes = pSection->readString( "resource", "" );
		if ( !validatePrefabVersion( terrainRes ) )
		{
			BW::string err =
				"Prefab's terrain version different than the current terrain version "
				"('" + terrainRes + "')";
			if ( errorString )
			{
				*errorString = err;
			}
			ERROR_MSG( "%s\n", err.c_str() );
			return false;
		}

		BW::string::size_type lastSlash = terrainRes.find_last_of( '/' );
		if ( lastSlash != BW::string::npos )
			terrainRes = terrainRes.substr( lastSlash + 1 );

		BW::string terrainName = "prefab_temp." + terrainRes;

		BWResource::instance().purge( terrainName, true );

		BW::wstring fname;
		bw_utf8tow( BWResource::resolveFilename( terrainName ), fname );

		FILE* pFile = _wfopen( fname.c_str(), L"wb" );

		if ( pFile )
		{
			fwrite( bp->data(), bp->len(), 1, pFile );	
			fclose( pFile );

			block_ = EditorChunkTerrain::loadBlock( terrainName, this, 
						pChunk->transform().applyToOrigin() );

			BWResource::instance().purge( terrainName, true );
			::DeleteFile( fname.c_str() );

			pOwnSect_->delChild( "prefabBin" );
		}

		if ( !block_ )
		{
			BW::string err = "Could not load prefab's terrain block. "
				"The prefab terrain is corrupted, or has different map sizes "
				"than the current space.";
			if ( errorString )
			{
				*errorString = err;
			}
			ERROR_MSG( "%s\n", err.c_str() );
			return false;
		}
	}
	else
	{
		// Loading as normal or cloning, check

		EditorChunkTerrain* currentTerrain = (EditorChunkTerrain*)
			ChunkTerrainCache::instance( *pChunk ).pTerrain();

		// If there's already a terrain block there, that implies we're cloning
		// from it.
		if (currentTerrain)
		{
			// Poxy hack to copy to terrain block
			BW::string terrainRes = pSection->readString( "resource", "" );
			if ( !validatePrefabVersion( terrainRes ) )
			{
				// this should never happen, but still need to do the validate
				// to get the correct resource name in terrainRes
				BW::string err =
					"Cloned terrain's version different than the current terrain version "
					"('" + terrainRes + "')";
				if ( errorString )
				{
					*errorString = err;
				}
				ERROR_MSG( "%s\n", err.c_str() );
				return false;
			}

			BW::string terrainName = "clone_temp." + terrainRes;

			BWResource::instance().purge( terrainName );

			currentTerrain->block().rebuildLodTexture( pChunk->transform() );
			currentTerrain->block().save( terrainName );
			block_ = EditorChunkTerrain::loadBlock( terrainName, this, Vector3(0,0,0)  );

			BW::wstring fname;
			bw_utf8tow( BWResource::resolveFilename( terrainName ), fname );
			::DeleteFile( fname.c_str() );

			if (!block_)
			{
				BW::string err = "Could not clone terrain block ";
				if ( errorString )
				{
					*errorString = err;
				}
				ERROR_MSG( "%s\n", err.c_str() );
				return false;
			}
		}
		else
		{
			BW::string resName = pChunk->mapping()->path() + 
										pSection->readString( "resource" );
			// Allocate the terrainblock.
			block_ = EditorChunkTerrain::loadBlock( resName, this, 
										pChunk->transform().applyToOrigin(),
										errorString );

			if (!block_)
			{
				BW::string err = "Could not load terrain block '" 
					+ resName + "': ";

				if ((errorString != NULL) && !errorString->empty())
				{
					err = err + *errorString;
				}
				else
				{
					err = err + "unknown";
				}

				if (errorString != NULL)
				{
					*errorString = err;
				}
				return false;
			}
		}
	}

	union
	{
		DWORD  key;
		void * ptr;
	} cast;
	cast.ptr = this;
	static_cast<Terrain::EditorBaseTerrainBlock*>
		(block_.getObject())->setSelectionKey( cast.key );
    calculateBB();
	return true;
}


/**
 *  This function finds the chunk terrain (chunkDx, chunkDy) relative to this 
 *  one, and if it exists, copies the given source rectangle of its height data 
 *  to the destination location.
 *
 *  @param chunkDx              How many chunks across?
 *  @param chunkDy              How many chunks down/up?
 *  @param srcX                 The source rectangle's left coordinate.
 *  @param srcY                 The source rectangle's top coordinate.
 *  @param srcW                 The source rectangle's width.
 *  @param srcH                 The source rectangle's height.
 *  @param dstX                 The destination rectangle's left coordinate.
 *  @param dstY                 The destination rectangle's right coordinate.
 */
void EditorChunkTerrain::copyHeights
(
    int32                       chunkDx, 
    int32                       chunkDz, 
    uint32                      srcX,
    uint32                      srcZ,
    uint32                      srcW,
    uint32                      srcH,
    uint32                      dstX,
    uint32                      dstZ
)
{
	BW_GUARD;

	// Find the chunk/terrain (chunkDx, chunkDz) coordinates away.
	GeometryMapping *mapping = WorldManager::instance().geometryMapping();
	int16 cx, cz;
	if (!mapping->gridFromChunkName(pChunk_->identifier(), cx, cz))
	{
		return;
	}
	cx += (int16)chunkDx;
	cz += (int16)chunkDz;
	BW::string nchunkId;
	chunkID(nchunkId, cx, cz);
	Chunk *chunk = 
		ChunkManager::instance().findChunkByName
		(
			nchunkId,
			WorldManager::instance().geometryMapping()
		);
	if (chunk == NULL || !chunk->loaded())
		return;
	ChunkTerrain *chunkTerrain = 
		ChunkTerrainCache::instance(*chunk).pTerrain();
	if (chunkTerrain == NULL)
		return;
	Terrain::BaseTerrainBlockPtr other = 
		static_cast<Terrain::EditorBaseTerrainBlock *>(chunkTerrain->block().getObject());

    // Lock this chunk's height map for writing:
    Terrain::TerrainHeightMap            &thm     = block().heightMap();
    Terrain::TerrainHeightMapHolder      dstHolder(&thm, false);
    Terrain::TerrainHeightMap::ImageType &heights = thm.image();

    // Lock the other chunk's height for reading only:
    Terrain::TerrainHeightMap            &otherTHM     = other->heightMap();
    Terrain::TerrainHeightMapHolder      srcHolder(&otherTHM, true);
    Terrain::TerrainHeightMap::ImageType &otherHeights = otherTHM.image();
	
	if (thm.blocksWidth() != otherTHM.blocksWidth())
	{
		BW::string errorText;
		bw_wtoutf8(
			Localise(L"WORLDEDITOR/WORLDEDITOR/HEIGHT/HEIGHT_MAP/INCOMPATIBLE"),
			errorText );

		WorldManager::instance().addCommentaryMsg( errorText, 2 );

		if (&MsgHandler::instance())
			MsgHandler::instance().addAssetErrorMessage( errorText );

		return;
	}

    heights.blit(otherHeights, srcX, srcZ, srcW, srcH, dstX, dstZ);
}


class TerrainItemMatrix : public ChunkItemMatrix
{
public:
	TerrainItemMatrix( ChunkItemPtr pItem ) : ChunkItemMatrix( pItem )
	{
		// Remove snaps, we don't want them as we're pretending the origin is
		// at 50,0,50
		movementSnaps( Vector3( 0.f, 0.f, 0.f ) );
	}

	/**
	 *	Overriding base class to not check the bounding box is < 100m, we just
	 *	don't care for terrain items
	 */
	bool setMatrix( const Matrix & m )
	{
		BW_GUARD;

		pItem_->edTransform( m, true );
		return true;
	}
};


/*static*/ uint32 EditorChunkTerrain::prefabVersion( BW::string& resource )
{
	BW_GUARD;

	if (resource.substr( resource.length() - 9 ) == ".terrain2" != NULL)
	{
		resource += "/terrain2";
		return Terrain::TerrainSettings::TERRAIN2_VERSION;
	}

	return 0;
}


void EditorChunkTerrain::syncInit()
{
	BW_GUARD;

	ChunkTerrain::syncInit();
}


/* Regenerate the bounding box and umbra model */
void EditorChunkTerrain::onTerrainChanged()
{
	BW_GUARD;

	this->calculateBB();
	this->syncInit();

	const Chunk::Items & items  = chunk()->staticItems();
	for(Chunk::Items::const_iterator it = items.begin();
		it != items.end(); ++it)
	{
		ChunkItemPtr item = *it;
		if(item->edIsVLO() == false)
		{
			continue;
		}
		ChunkVLO *vlo = dynamic_cast< ChunkVLO* >( item.getObject() );
		if(vlo != NULL)
		{
			ChunkWater * chunkWater =
				dynamic_cast< ChunkWater * >( vlo->object().getObject() );
			if(chunkWater)
			{
				chunkWater->shouldRebuild( true );
			}
		}
	}
}


/**
 *	Disable hide/freeze operations on terrain
 */
void EditorChunkTerrain::edHidden( bool value )
{
	WARNING_MSG( "Failed to hide terrain. Terrain cannot be hidden.\n" );
}


/**
 *	Disable hide/freeze operations on terrain
 */
bool EditorChunkTerrain::edHidden() const
{
	return false;
}


/**
 *	Disable hide/freeze operations on terrain
 */
void EditorChunkTerrain::edFrozen( bool value )
{
	WARNING_MSG( "Failed to freeze terrain. Terrain cannot be frozen.\n" );
}


/**
 *	Disable hide/freeze operations on terrain
 */
bool EditorChunkTerrain::edFrozen() const
{
	return false;
}

int EditorChunkTerrain::getForcedLod() const
{
	const Terrain::EditorBaseTerrainBlock * terrainBlock = &block();
	if( terrainBlock != NULL )
	{
		return terrainBlock->getForcedLod();
	}
	return -1;
}

bool EditorChunkTerrain::setForcedLod( const int & forcedLod )
{
	Terrain::EditorBaseTerrainBlock * terrainBlock = &block();
	if( terrainBlock != NULL )
	{
		terrainBlock->setForcedLod( forcedLod );
		BW::Chunk* chunk = this->chunk();
		WorldManager::instance().changedTerrainBlock( chunk, false );
		WorldManager::instance().
			forcedLodMap().updateLODElement(chunk->x(), chunk->z(), forcedLod);
		return true;
	}
	return false;
}

/**
 *	Returns a light container used to visualise dynamic lights on this object.
 */
Moo::LightContainerPtr EditorChunkTerrain::edVisualiseLightContainer()
{
	if (!chunk() || !block_)
		return NULL;

	Moo::LightContainerPtr lc = new Moo::LightContainer;

	BoundingBox bb = this->block_->boundingBox();
	bb.transformBy( this->chunk()->transform() );
	lc->init( ChunkLightCache::instance( *chunk() ).pAllLights(), bb, false );

	return lc;
}


// Use macros to write EditorChunkTerrain's static create method
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk, &errorString)
IMPLEMENT_CHUNK_ITEM( EditorChunkTerrain, terrain, 2 )
BW_END_NAMESPACE

