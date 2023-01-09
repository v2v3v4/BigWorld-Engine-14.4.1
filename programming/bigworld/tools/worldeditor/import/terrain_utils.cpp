#include "pch.hpp"
#include "common/bwlockd_connection.hpp"
#include "worldeditor/import/terrain_utils.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/misc/sync_mode.hpp"
#include "chunk/chunk_grid_size.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_clean_flags.hpp"
#include "resmgr/auto_config.hpp"
#include "terrain/terrain_data.hpp"
#include "terrain/terrain_texture_layer.hpp"
#include "terrain/terrain2/editor_terrain_block2.hpp"

BW_BEGIN_NAMESPACE

static AutoConfigString s_terrainUtilsBlankTexture(
	"system/emptyTerrainTextureBmp", "helpers/maps/aid_builder.tga" );

/**
 *  TerrainUtils::TerrainFormat constructor
 */
TerrainUtils::TerrainFormat::TerrainFormat() :
	poleSpacingX( 0.0f ),
	poleSpacingY( 0.0f ),
	widthM( 0.0f ),
	heightM( 0.0f ),
	polesWidth( 0 ),
	polesHeight( 0 ),
	visOffsX( 0 ),
	visOffsY( 0 ),
	blockWidth( 0 ),
	blockHeight( 0 )
{
}


TerrainUtils::TerrainFormatStorage::TerrainFormatStorage() :
	dirty_( true ),
	terrainInfo_()
{
}


/**
 *	Check if a terrain block has been loaded to get settings from.
 *	@return true if a terrain block has been loaded to get settings from.
 */
bool TerrainUtils::TerrainFormatStorage::hasTerrainInfo() const
{
	BW_GUARD;
	return !dirty_;
}


/**
 *	Get terrain format settings.
 *	
 *	If a terrain block has not been loaded to read the settings, force load one
 *	and then use it to get the settings. This will fail if there is no terrain
 *	in the space. There should be terrain sections in every *.cdata (even if
 *	there is none in the *.chunk) because of this. Ideally this info should be
 *	stored in the space's terrain settings and not per block because it only
 *	gets info from one block anyway.
 *	
 *	@param pMapping the geometry mapping to load the terrain block from
 *		(if needed).
 *	@param terrainSettings the terrain settings to load the terrain block with
 *		(if needed).
 *	@return terrain format info read from a terrain block.
 */
const TerrainUtils::TerrainFormat&
	TerrainUtils::TerrainFormatStorage::getTerrainInfo(
	const GeometryMapping* pMapping,
	Terrain::TerrainSettingsPtr& terrainSettings )
{
	BW_GUARD;
	if (dirty_)
	{
		Terrain::BaseTerrainBlockPtr block =
			this->forceLoadSomeTerrainBlock( pMapping, terrainSettings );
		MF_ASSERT( block.hasObject() );
		this->setTerrainInfo( block );
	}
	return terrainInfo_;
}


/**
 *	Get the terrain format settings from a terrain block.
 *	@param block terrain block which has settings.
 */
void TerrainUtils::TerrainFormatStorage::setTerrainInfo(
	Terrain::BaseTerrainBlockPtr& block )
{
	BW_GUARD;
	MF_ASSERT( block.hasObject() );
	MF_ASSERT( !this->hasTerrainInfo() );

	dirty_ = false;

	const Terrain::TerrainHeightMap &thm = block->heightMap();
	terrainInfo_.poleSpacingX = thm.spacingX();
	terrainInfo_.poleSpacingY = thm.spacingZ();
	terrainInfo_.widthM = block->blockSize();
	terrainInfo_.heightM = block->blockSize();
	terrainInfo_.polesWidth = thm.polesWidth();
	terrainInfo_.polesHeight = thm.polesHeight();
	terrainInfo_.visOffsX = thm.xVisibleOffset();
	terrainInfo_.visOffsY = thm.zVisibleOffset();
	terrainInfo_.blockWidth = thm.blocksWidth();
	terrainInfo_.blockHeight = thm.blocksHeight();
}


/**
 *	This resets any cached terrain information.  It should be reset when
 *	the space is changed, or there is a conversion to newer terrain versions.
 */
void TerrainUtils::TerrainFormatStorage::clear()
{
	dirty_ = true;
}


/**
 *  Returns the a block that has terrain in the space.
 *	Various editor systems that use getTerrainInfo depend on the space
 *	containing at least one terrain block. eg. the terrain height map panel.
 */
const Terrain::BaseTerrainBlockPtr
	TerrainUtils::TerrainFormatStorage::forceLoadSomeTerrainBlock(
	const GeometryMapping* pMapping,
	Terrain::TerrainSettingsPtr& terrainSettings ) const
{
	BW_GUARD;
	MF_ASSERT( pMapping != NULL );
	MF_ASSERT( terrainSettings.hasObject() );

	Terrain::BaseTerrainBlockPtr block( NULL );
	const GeometryMapping* dirMap = pMapping;
	for (int i = dirMap->minGridX(); i <= dirMap->maxGridX(); ++i)
	{
		for (int j = dirMap->minGridY(); j <= dirMap->maxGridY(); ++j)
		{
			const BW::string resName = dirMap->path() +
				dirMap->outsideChunkIdentifier( i, j ) +
				".cdata/terrain";
			block = Terrain::BaseTerrainBlock::loadBlock( resName,
				Matrix::identity, Vector3::zero(),
				terrainSettings );
			if (block.hasObject())
			{
				break;
			}
		}
		if (block.hasObject())
		{
			break;
		}
	}
	
	return block;
}


/**
 *	Create a default terrain 2 block.
 *	@param terrainSettings settings to create the block with.
 *	@param terrainSettingsSection settings to create the block with.
 *	@return a new terrain 2 block.
 */
Terrain::EditorBaseTerrainBlockPtr TerrainUtils::createDefaultTerrain2Block(
	Terrain::TerrainSettingsPtr& terrainSettings,
	DataSectionPtr& terrainSettingsSection )
{
	BW_GUARD;

	SmartPointer<Terrain::EditorTerrainBlock2> defaultTerrainBlock = 
		new Terrain::EditorTerrainBlock2( terrainSettings );
	BW::string error;
	const bool ok =
		defaultTerrainBlock->create( terrainSettingsSection, &error );
	if (!ok || !error.empty())
	{
		ERROR_MSG( "Failed to create default terrain block: %s\n",
			error.empty() ? "unknown" : error.c_str() );
	}
	else
	{
		// add a default texture layer
		const uint32 blendSize = terrainSettings->blendMapSize();

		const size_t idx =
			defaultTerrainBlock->insertTextureLayer( blendSize, blendSize );
		if (idx == -1)
		{
			ERROR_MSG( "Couldn't create default texture for new space.\n" );
			return NULL;
		}

		Terrain::TerrainTextureLayer& layer =
			defaultTerrainBlock->textureLayer( idx );
		layer.textureName( s_terrainUtilsBlankTexture.value() );

		{
			// set the blends to full white
			Terrain::TerrainTextureLayerHolder holder( &layer, false );
			layer.image().fill( std::numeric_limits<
				Terrain::TerrainTextureLayer::PixelType >::max() );
		}

		// update and save
		defaultTerrainBlock->rebuildCombinedLayers();
		if (!defaultTerrainBlock->rebuildLodTexture( Matrix::identity ))
		{
			return NULL;
		}
	}

	return defaultTerrainBlock;
}


/**
 *  This function returns the size of the terrain in grid units.
 *
 *  @param space            The space to get.  if this is empty then the 
 *                          current space is used.
 *  @param gridMinX         The minimum grid x unit.
 *  @param gridMinY         The minimum grid y unit.
 *  @param gridMaxX         The maximum grid x unit.
 *  @param gridMaxY         The maximum grid y unit.
 */
void TerrainUtils::terrainSize
(
    BW::string             const &space_,
	float					&gridSize,
    int                     &gridMinX,
    int                     &gridMinY,
    int                     &gridMaxX,
    int                     &gridMaxY
)
{
	BW_GUARD;

    BW::string space = space_;
    if (space.empty())
        space = WorldManager::instance().getCurrentSpace();

    // work out grid size
    DataSectionPtr pDS = BWResource::openSection(space + "/" + SPACE_SETTING_FILE_NAME );
    if (pDS)
    {
        gridMinX = pDS->readInt("bounds/minX",  1);
        gridMinY = pDS->readInt("bounds/minY",  1);
        gridMaxX = pDS->readInt("bounds/maxX", -1);
        gridMaxY = pDS->readInt("bounds/maxY", -1);
		gridSize = pDS->readFloat("chunkSize", DEFAULT_GRID_RESOLUTION);
    }
}


TerrainUtils::TerrainGetInfo::TerrainGetInfo():
    chunk_(NULL),
    chunkTerrain_(NULL)
{
}


TerrainUtils::TerrainGetInfo::~TerrainGetInfo()
{
    chunk_        = NULL;
    chunkTerrain_ = NULL;
}

bool TerrainUtils::sourceTextureHasAlpha( const BW::string fileName )
{
	BW::string fileExtension = BWResource::getExtension( fileName ).to_string();
	// TODO: compare_lower
	std::transform( fileExtension.begin(), fileExtension.end(), 
		fileExtension.begin(), tolower );

	if ( fileExtension == "bmp" ||
		fileExtension == "jpg" ||
		fileExtension == "jpeg")
	{
		return false;
	}
	return true;
}

/**
 *  This gets the raw terrain height information for the given block.
 *
 *  @param gx               The grid x position.
 *  @param gy               The grid y position.
 *  @param terrainImage     The terrain image data.
 *  @param forceToMemory    Force loading the chunk to memory.
 *  @param getInfo          If you later set the terrain data then you need to
 *                          pass one of these here and one to setTerrain.
 *  @return                 True if the terrain was succesfully read.
 */
bool TerrainUtils::getTerrain
(
    int										gx,
    int										gy,
    Terrain::TerrainHeightMap::ImageType    &terrainImage,
    bool									forceToMemory,
    TerrainGetInfo							*getInfo        /*= NULL*/
)
{
	BW_GUARD;

    Terrain::EditorBaseTerrainBlockPtr terrainBlock = NULL;

    BinaryPtr result;

    // Get the name of the chunk:
    BW::string chunkName;
    int16 wgx = (int16)gx;
    int16 wgy = (int16)gy;
    chunkID(chunkName, wgx, wgy);
    if (chunkName.empty())
        return false;
    if (getInfo != NULL)
        getInfo->chunkName_ = chunkName;

    // Force the chunk into memory if requested:
    if (forceToMemory)
    {
        ChunkManager::instance().loadChunkNow
        (
            chunkName,
            WorldManager::instance().geometryMapping()
        );
		ChunkManager::instance().checkLoadingChunks();
    }
    
    // Is the chunk in memory?    
    Chunk *chunk = 
        ChunkManager::instance().findChunkByName
        (
            chunkName,
            WorldManager::instance().geometryMapping()
        );
    if (chunk != NULL && getInfo != NULL)
    {
		getInfo->chunk_ = chunk;
	}
	if (chunk != NULL && chunk->loaded())
	{
		ChunkTerrain *chunkTerrain = 
            ChunkTerrainCache::instance(*chunk).pTerrain();
        if (chunkTerrain != NULL)
        {
            terrainBlock = 
                static_cast<Terrain::EditorBaseTerrainBlock *>(chunkTerrain->block().getObject());
            if (getInfo != NULL)
            {
                
                getInfo->block_        = terrainBlock;
                getInfo->chunkTerrain_ = static_cast<EditorChunkTerrain *>(chunkTerrain);
            }
        }
    }

    // Load the terrain block ourselves if not already in memory:
    if (terrainBlock == NULL)
    {
		BW::string spaceName = WorldManager::instance().getCurrentSpace();
		terrainBlock = EditorChunkTerrain::loadBlock(spaceName + '/' + chunkName + ".cdata/terrain", 
			NULL, chunk->transform().applyToOrigin() );
        if (getInfo != NULL)
            getInfo->block_ = terrainBlock;
    }

    // If the chunk is in memory, get the terrain data:
    if (terrainBlock != NULL)
    {
        if (getInfo != NULL)
            getInfo->block_ = terrainBlock;

        Terrain::TerrainHeightMap &thm = terrainBlock->heightMap();
        Terrain::TerrainHeightMapHolder holder(&thm, true);
        terrainImage = thm.image();
        return true;
    }
    else
    {
        return false;
    }
}


/**
 *  This function sets raw terrain data.
 *
 *  @param gx               The grid x position.
 *  @param gy               The grid y position.
 *  @param terrainImage     The terrain image data.  This needs to be the
 *                          data passed back via getTerrain.
 *  @param getInfo          If you later set the terrain data then you need to
 *                          pass an empty one of these here.  This needs to be 
 *                          the data passed back via getTerrain.
 *  @param forceToDisk      Force saving the terrain to disk if it was not
 *							in memory.
 */
void TerrainUtils::setTerrain
(
    int                                 gx,
    int                                 gy,
    Terrain::TerrainHeightMap::ImageType    const &terrainImage,
    TerrainGetInfo                      const &getInfo,
    bool                                forceToDisk
)
{
	BW_GUARD;

    // Set the terrain:
    {
        Terrain::TerrainHeightMap &thm = getInfo.block_->heightMap();
        Terrain::TerrainHeightMapHolder holder(&thm, false);
        thm.image().blit(terrainImage);
    }
	getInfo.block_->rebuildNormalMap( Terrain::NMQ_NICE );

    if (forceToDisk && getInfo.chunkTerrain_ == NULL)
    {
		BW::string space = WorldManager::instance().getCurrentSpace();
		BW::string cdataName = space + '/' + getInfo.chunkName_ + ".cdata";
		Matrix xform = Matrix::identity;
		DataSectionPtr cData = BWResource::openSection( cdataName );

		xform.setTranslate(gx*getInfo.block_->blockSize(), 0.0f, 
							gy*getInfo.block_->blockSize());
		getInfo.block_->rebuildLodTexture(xform);

		ChunkCleanFlags ChunkCleanFlags( cData );

		ChunkCleanFlags.shadow_ = 0;
		ChunkCleanFlags.thumbnail_= 0;
		ChunkCleanFlags.terrainLOD_ = !getInfo.block_->isLodTextureDirty();
		ChunkCleanFlags.save();

		cData->deleteSection( "thumbnail.dds" );
		if (!getInfo.block_->saveToCData( cData ))
        {
        	ERROR_MSG( "Couldn't set terrain for chunk '%s'\n", getInfo.chunkName_.c_str() );
        }
    }

    if (getInfo.chunk_ != NULL)
    {
    	if (getInfo.chunkTerrain_ != NULL)
			getInfo.chunkTerrain_->onTerrainChanged();
		if (!forceToDisk || getInfo.chunkTerrain_ != NULL)
		{
			WorldManager::instance().changedTerrainBlock( getInfo.chunk_ );
		}
    }
}


/**
 *	This function can be called before setTerrain to patch heights along the
 *	border of the space in regions that are not visible, but which can 
 *	contribute to, for example, normals.  It does this by extending the edge
 *	values into the non-visible regions.
 *
 *	@param gx				The x-position of the chunk.
 *	@param gy				The y-position of the chunk.
 *	@param gridMaxX			The width of the space.
 *	@param gridMaxY			The height of the space.
 *	@param getInfo			Information returned by getTerrain.
 *	@param terrainImage		The new terrain image that needs extending.
 */
void TerrainUtils::patchEdges
(
	int32								gx,
	int32								gy,
	uint32								gridMaxX,
	uint32								gridMaxY,
	TerrainGetInfo                      &getInfo,
	Terrain::TerrainHeightMap::ImageType	&terrainImage
)
{
	BW_GUARD;

	gx += gridMaxX/2;
	gy += gridMaxY/2;

	Terrain::TerrainHeightMap &thm = getInfo.block_->heightMap();

	uint32 x0 = 0;
	uint32 x1 = thm.xVisibleOffset();
	uint32 x2 = x1 + thm.blocksWidth();
	uint32 x3 = thm.polesWidth();

	uint32 y0 = 0;
	uint32 y1 = thm.zVisibleOffset();
	uint32 y2 = y1 + thm.blocksHeight();
	uint32 y3 = thm.polesHeight();

	// top
	if (gy == 0)
	{
		for (uint32 x = x0; x < x3; ++x)		
		{
			float height = terrainImage.get(x, y1);
			for (uint32 y = y0; y < y1; ++y)
			{
				terrainImage.set(x, y, height);
			}
		}	
	}

	// bottom
	if (gy == gridMaxY - 1)
	{
		for (uint32 x = x0; x < x3; ++x)		
		{
			float height = terrainImage.get(x, y2 - 1);
			for (uint32 y = y2; y < y3; ++y)
			{
				terrainImage.set(x, y, height);
			}
		}	
	}

	// left
	if (gx == 0)
	{
		for (uint32 y = y0; y < y3; ++y)				
		{
			float height = terrainImage.get(x1, y);
			for (uint32 x = x0; x < x1; ++x)
			{
				terrainImage.set(x, y, height);
			}
		}	
	}

	// right
	if (gx == gridMaxX - 1)
	{
		for (uint32 y = y0; y < y3; ++y)				
		{
			float height = terrainImage.get(x2 - 1, y);
			for (uint32 x = x2; x < x3; ++x)
			{
				terrainImage.set(x, y, height);
			}
		}	
	}

	// top-left
	if (gx == 0 && gy == 0)
	{
		float height = terrainImage.get(x1, y1);
		for (uint32 y = y0; y < y1; ++y)
		{
			for (uint32 x = x0; x < x1; ++x)
			{
				terrainImage.set(x, y, height);
			}
		}	
	}

	// top-right
	if (gx == gridMaxX - 1 && gy == 0)
	{
		float height = terrainImage.get(x2 - 1, y1);
		for (uint32 y = y0; y < y1; ++y)
		{
			for (uint32 x = x2; x < x3; ++x)
			{
				terrainImage.set(x, y, height);
			}
		}
	}

	// bottom-left
	if (gx == 0 && gy == gridMaxY - 1)
	{
		float height = terrainImage.get(x1, y2 - 1);
		for (uint32 y = y2; y < y3; ++y)
		{
			for (uint32 x = x0; x < x1; ++x)
			{
				terrainImage.set(x, y, height);
			}
		}
	}

	// bottom-right
	if (gx == gridMaxX - 1 && gy == gridMaxY - 1)
	{
		float height = terrainImage.get(x2 - 1, y2 - 1);
		for (uint32 y = y2; y < y3; ++y)
		{
			for (uint32 x = x2; x < x3; ++x)
			{
				terrainImage.set(x, y, height);
			}
		}
	}
}


/**
 *  This function returns true if the terrain chunk at (gx, gy) is editable.
 *
 *  @param gx               The x grid coordinate.
 *  @param gy               The y grid coordinate.
 *  @returns                True if the chunk at (gx, gy) is locked, false
 *                          if it isn't locked.  The chunk does not have be to
 *							loaded yet.
 */
bool TerrainUtils::isEditable
(
    int                     gx,
    int                     gy
)
{
	BW_GUARD;

#ifdef BIGWORLD_CLIENT_ONLY

    return true;

#else
    // See if we are in a multi-user environment and connected:    
	return EditorChunk::outsideChunkWriteable(gx, gy, false);
#endif
}


/**
 *  This function returns the height at the given position.  It includes items
 *  on the ground.
 *
 *  @param gx               The x grid coordinate.
 *  @param gy               The y grid coordinate.
 *  @param forceLoad        Force loading the chunk and getting focus at this
 *                          point to do collisions.  The camera should be moved
 *                          near here.
 *  @returns                The height at the given position.
 */
float TerrainUtils::heightAtPos
(
    float                   gx,
    float                   gy,
    bool                    forceLoad   /*= false*/
)
{
	BW_GUARD;

    ChunkManager &chunkManager = ChunkManager::instance();

    if (forceLoad)
    {
        Vector3 chunkPos = Vector3(gx, 0.0f, gy);
        chunkManager.cameraSpace()->focus(chunkPos);
        BW::string chunkName;
		const float gridSize = ChunkManager::instance().cameraSpace()->gridSize();
		int16 wgx = (int16)floor(gx/gridSize);
        int16 wgy = (int16)floor(gy/gridSize);
        chunkID(chunkName, wgx, wgy);
        if (!chunkName.empty())
        {
            chunkManager.loadChunkNow
            (
                chunkName,
                WorldManager::instance().geometryMapping()
            );
            chunkManager.checkLoadingChunks();
        }
		Chunk* chunk = chunkManager.findChunkByName( chunkName, 
			WorldManager::instance().geometryMapping() );

		if (chunk)
		{
			while (!chunk->loaded())
			{
				chunkManager.checkLoadingChunks();
			}
		}

        chunkManager.cameraSpace()->focus(chunkPos);
    }

    const float MAX_SEARCH_HEIGHT = 1000000.0f;
    Vector3 srcPos(gx, +MAX_SEARCH_HEIGHT, gy);
    Vector3 dstPos(gx, -MAX_SEARCH_HEIGHT, gy);
    
    float dist = 
        chunkManager.cameraSpace()->collide
        (
            srcPos, 
            dstPos, 
            ClosestObstacle::s_default
        );
    float result = 0.0;
    if (dist > 0.0)
        result = MAX_SEARCH_HEIGHT - dist;

    return result;
}


/**
 *	Test whether two rectangles intersect, and if they do return the
 *	common area.  The code assumes that left < right and bottom < top.
 *
 *  @param left1	Left coordinate of rectangle 1.
 *  @param top1  	Top coordinate of rectangle 1.
 *  @param right1	Right coordinate of rectangle 1.
 *  @param bottom1	Bottom coordinate of rectangle 1.
 *  @param left2	Left coordinate of rectangle 2.
 *  @param top2  	Top coordinate of rectangle 2.
 *  @param right2	Right coordinate of rectangle 2.
 *  @param bottom2	Bottom coordinate of rectangle 2.
 *  @param ileft	Left coordinate of the intersection rectangle.
 *  @param itop  	Top coordinate of the intersection rectangle.
 *  @param iright 	Right coordinate of the intersection rectangle.
 *  @param ibottom	Bottom coordinate of the intersection rectangle.
 *	@return			True if there was an intersection, false otherwise.
 */
bool
TerrainUtils::rectanglesIntersect
(
    int  left1, int  top1, int  right1, int  bottom1,
    int  left2, int  top2, int  right2, int  bottom2,
    int &ileft, int &itop, int &iright, int &ibottom
)
{
    if 
    (
        right1  < left2
        ||                     
        left1   > right2
        ||                     
        top1    < bottom2
        ||                     
        bottom1 > top2
    )
    {
        return false;
    }
    ileft   = std::max(left1  , left2  );
    itop    = std::min(top1   , top2   ); // Note that bottom < top
    iright  = std::min(right1 , right2 ); 
    ibottom = std::max(bottom1, bottom2);
    return true;
}


/**
 *	This gets the settings of the current space.
 *
 *  @param space		The name of the space to get.
 *  @param gridWidth	The width in chunks of the space.
 *  @param gridHeight	The height in chunks of the space.
 *  @param localToWorld	Coordinates that can be used to convert from grid
 *						coordinates to world coordinates.
 */
bool TerrainUtils::spaceSettings
(
    BW::string const &space,
	float		&gridSize,
    uint16      &gridWidth,
    uint16      &gridHeight,
    GridCoord   &localToWorld
)
{
	BW_GUARD;

    DataSectionPtr pDS = BWResource::openSection(space + "/" + SPACE_SETTING_FILE_NAME );
    if (pDS)
    {
	    int minX = pDS->readInt("bounds/minX",  1);
	    int minY = pDS->readInt("bounds/minY",  1);
	    int maxX = pDS->readInt("bounds/maxX", -1);
	    int maxY = pDS->readInt("bounds/maxY", -1);
		gridSize = pDS->readFloat("chunkSize", DEFAULT_GRID_RESOLUTION);

	    gridWidth  = maxX - minX + 1;
	    gridHeight = maxY - minY + 1;
	    localToWorld = GridCoord(minX, minY);

        return true;
    }
    else
    {
	    return false;
    }
}
BW_END_NAMESPACE

