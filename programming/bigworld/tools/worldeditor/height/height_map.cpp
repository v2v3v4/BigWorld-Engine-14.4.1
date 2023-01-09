#include "pch.hpp"
#include "worldeditor/height/height_map.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "worldeditor/misc/sync_mode.hpp"
#include "worldeditor/misc/progress_bar_helper.hpp"
#include "worldeditor/import/terrain_utils.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "moo/texture_compressor.hpp"
#include "terrain/base_terrain_block.hpp"
#include "resmgr/string_provider.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "cstdmf/debug.hpp"
#include "resmgr/multi_file_system.hpp"
#include <limits>


DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

BW_INIT_SINGLETON_STORAGE( HeightMap )


namespace
{
    // Extents of the representable range of heights in the texture:
	const float         MIN_HEIGHT                  = TerrainUtils::MIN_TERRAIN_HEIGHT;
	const float         MAX_HEIGHT                  = TerrainUtils::MAX_TERRAIN_HEIGHT;

    // The number of chunks to convert to a height map per frame:
    const uint32  NUM_CHUNKS_PER_FRAME        = 10;

    // The number of chunks after which we show a progress bar when creating
    // the height map texture:
    const uint32  NUM_CHUNKS_FOR_TEX_PROGRESS = 200;

    // Convert from height to uint16 as used in the height map:
    inline uint16 heightToUint16(float height)
    {
        return 
            Math::lerp
            (
                Math::clamp(MIN_HEIGHT, height, MAX_HEIGHT),
                MIN_HEIGHT, 
                MAX_HEIGHT, 
                (uint16)0, 
                (uint16)65535
            );
    }

    // Covnert a uint16 as used in the texture to a height:
    float uint16ToHeight(uint16 theight)
    {
        return 
            Math::lerp
            (
                theight, 
                (uint16)0, 
                (uint16)65535, 
                MIN_HEIGHT, 
                MAX_HEIGHT
            );
    }

    // Convert a height delta to an int16 for use in the texture
    inline int16 heightDeltaToInt16(float heightDelta)
    {
        return (int16)(65535.0f*heightDelta/(MAX_HEIGHT - MIN_HEIGHT));
    }

    // Convert a height to a normalised float (in the range [0.0f, 1.0f]):
    float heightToFloat(float height)
    {
        float cheight = Math::clamp(MIN_HEIGHT, height, MAX_HEIGHT);
        return (cheight - MIN_HEIGHT)/(MAX_HEIGHT - MIN_HEIGHT);
    }

    // Test whether two rectangles intersect, and if they do return the
    // common area.  The code assumes that left < right and bottom < top.
    bool
    rectanglesIntersect
    (
        int32  left1, int32  top1, int32  right1, int32  bottom1,
        int32  left2, int32  top2, int32  right2, int32  bottom2,
        int32 &ileft, int32 &itop, int32 &iright, int32 &ibottom
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
        itop    = std::min(top1   , top2   ); // Note that bottom < top so
        iright  = std::min(right1 , right2 ); 
        ibottom = std::max(bottom1, bottom2);
        return true;
    }
}


/**
 *  This function should be called by the client of the height map to give it
 *  information about the current world.
 *
 *  @param spaceName        The current space's folder.
 *  @param localToWorldX    The function that converts chunk numbers to 
 *                          chunk numbers in the x-direction.
 *  @param localToWorldY    The function that converts chunk numbers to 
 *                          chunk numbers in the y-direction.
 *  @param width            The width of the space in chunks.
 *  @param height           The height of the space in chunks.
 *  @param createAndValidateTexture Generate a valid texture for the height?
 */
void HeightMap::spaceInformation
(
    BW::string         const &spaceName,
    int32               localToWorldX,
    int32               localToWorldY,
    uint16              width,
    uint16              height,
    bool                createAndValidateTexture    /*= true*/
)
{
	BW_GUARD;

    // Do we support the more advanced rendering method or only the fixed 
    // function pipeline?:
    isPS2_ = 
        Moo::rc().psVersion() >= 0x200 
        && 
        Moo::rc().supportsTextureFormat(D3DFMT_L16);

    gridWidth_      = width;
    gridHeight_     = height;
    spaceName_      = spaceName;
    localToWorldX_  = localToWorldX;
    localToWorldY_  = localToWorldY;

    // Work out the number of poles:
    const TerrainUtils::TerrainFormat& ti = WorldManager::instance().getTerrainInfo();
	numPolesX_   = ti.polesWidth;
	numPolesY_   = ti.polesHeight;
	blockWidth_  = ti.blockWidth;
	blockHeight_ = ti.blockHeight;
	visOffX_     = ti.visOffsX;
	visOffY_     = ti.visOffsY;

    // Base the size of the texture on the number of poles and block sizes:
    uint32 recommendWidth  = recommendHeightMapSize(numPolesX_*gridWidth_);
    uint32 recommendHeight = recommendHeightMapSize(numPolesY_*gridHeight_);
    recommendSize_ = std::max(recommendWidth, recommendHeight);
    if (isPS2_)
    {
		heightMap_.resize(recommendSize_, recommendSize_);
    }

	madeValidOnce_ = false;

    if (createAndValidateTexture)
        createTexture();

    updateMinMax();
}



/**
 *  This is called to save the HeightMap to the cached file.  It also makes
 *  sure that the HeightMap is up to date.
 */
void HeightMap::save()
{
	BW_GUARD;

    if (hasTexture() && aborted_ == false)
    {
        // Draw missing chunks:
        SyncMode chunkStopper; 
        CWaitCursor waitCursor;
        while (drawSomeChunks(true)) 
            ;
        saveCachedHeightMap();
    }
}


/**
 *  This is called when the HeightMap is no longer needed.
 */
void HeightMap::onStop()
{
	BW_GUARD;

    deleteTexture();
}


/**
 *  This function sets the HeightMap's texture as the active texture in Moo's
 *  rendering context.
 *
 *  @param textureStage     The stage to place the texture.
 */
void HeightMap::setTexture(uint8 textureStage)
{
	BW_GUARD;

    ensureValidTexture();
    Moo::rc().setTexture(textureStage, texture_.pComObject());
}


/**
 *  This is called to update the HeightMap.  We use the opportunity to make
 *  sure that the HeightMap is up to date.
 *
 *  @param dtime            The time since this function was last called.
 */
void HeightMap::update(float /*dtime*/)
{
	BW_GUARD;

	drawSomeChunks(false); // do not update if there is imported terrain
}


/**
 *  This function gets the HeightMap's texture.
 *
 *  @return                 The HeightMap's texture.
 */
DX::Texture *HeightMap::texture() const 
{
	BW_GUARD;

    ensureValidTexture();
	return texture_.pComObject(); 
}


/**
 *  This function gets the minimum height of the terrain, including any
 *  not yet placed imported terrain.
 *
 *  @return                 The mininum height of the terrain, taking into
 *                          account terrain which is being imported.
 */
float HeightMap::minHeight() const
{
    return minHeight_;
}


/**
 *  This function gets the maximum height of the terrain, including any
 *  not yet placed imported terrain.
 *
 *  @return                 The maximum height of the terrain, taking into
 *                          account terrain which is being imported.
 */
float HeightMap::maxHeight() const
{
    return maxHeight_;
}


/**
 *  This function makes sure that the min/max heights are up to date.
 */
void HeightMap::updateMinMax()
{
    minHeight_ = std::min(minHeightTerrain_, minHeightImpTerrain_);
    maxHeight_ = std::max(maxHeightTerrain_, maxHeightImpTerrain_);
    invalidTexture_ = true;
}


/**
 *  This function converts from a height to a normalised height (0..1) where
 *  0 represents -2048 and 1 represents 2048.  Values outside +/-2048 are 
 *  clipped.
 *
 *  @param height           The height in metres.
 *
 *  @return                 The normalised height.                
 */
/*static*/ float HeightMap::normaliseHeight(float height)
{
    return heightToFloat(height);
}


/**
 *  This function 'draws' imported terrain.
 *
 *  @param left         The left coordinate in grid units.
 *  @param top          The top coordinate in grid units.
 *  @param right        The right coordinate in grid units.
 *  @param bottom       The bottom coordinate in grid units.
 *  @param mode         How is the terrain data placed - replace existing 
 *                      terrain etc.
 *  @param minv         The height to scale the minimum of the imported
 *                      terrain.
 *  @param maxv         The height to scale the maximum of the imported 
 *                      terrain.
 *  @param alpha        The strength of the import (0...1).
 */
void HeightMap::drawImportedTerrain
(
    float               left,
    float               top,
    float               right,
    float               bottom,
    ElevationBlit::Mode mode,
    float               minv,
    float               maxv,
    float               alpha,
    ImportImage			const &image
)
{
	BW_GUARD;

    restoreSubimage
    (
        impTerrLeft_,
        impTerrTop_,
        impTerrRight_,
        impTerrBottom_,
        impTerrSubImg_
    );

    impTerrLeft_    = left;
    impTerrTop_     = top;
    impTerrRight_   = right;
    impTerrBottom_  = bottom;

    extractSubimage
    (
        impTerrLeft_,
        impTerrTop_,
        impTerrRight_,
        impTerrBottom_,
        impTerrSubImg_
    );

    addTerrain
    (
        impTerrLeft_,
        impTerrTop_,
        impTerrRight_,
        impTerrBottom_,
        mode,
        minv,
        maxv,
        alpha,
        image
    );
}


/**
 *  This function restores the HeightMap after drawing some terrain.
 */
void HeightMap::undrawImportedTerrain()
{
	BW_GUARD;

    restoreSubimage
    (
        impTerrLeft_,
        impTerrTop_,
        impTerrRight_,
        impTerrBottom_,
        impTerrSubImg_
    );
}


/**
 *  This function is called when importing terrain is cancelled.
 */
void HeightMap::clearImpTerrain()
{
	BW_GUARD;

    restoreSubimage
    (
        impTerrLeft_,
        impTerrTop_,
        impTerrRight_,
        impTerrBottom_,
        impTerrSubImg_
    );

    impTerrLeft_   = impTerrTop_ = impTerrRight_ = impTerrBottom_ = 0.0f;
    impTerrSubImg_.clear();

    minHeightImpTerrain_ = +std::numeric_limits<float>::max();
    maxHeightImpTerrain_ = -std::numeric_limits<float>::max();
}


/**
 *  This function invalidates the area that the imported terrain would go
 *  into.
 */
void HeightMap::invalidateImportedTerrain()
{
	BW_GUARD;

    // Invalidate the imported terrain:
    int32 left   = (int32)impTerrLeft_   - 1;
    int32 top    = (int32)impTerrTop_    + 1;
    int32 right  = (int32)impTerrRight_  + 1;    
    int32 bottom = (int32)impTerrBottom_ - 1;
    if (right < left  ) std::swap(left, right );
    if (top   < bottom) std::swap(top , bottom);
    for (int32 y = bottom; y <= top; ++y)
    {
        for (int32 x = left; x <= right; ++x)
        {
            dirtyThumbnail(x + localToWorldX_, y + localToWorldY_);
        }
    }

    // The saved backbuffer is no longer needed:
    impTerrSubImg_.clear();

    // Draw the chunks:
    while (drawSomeChunks(true)) 
        ;
}


/**
 *  This function is called when a chunk is dirty and needs updating in the
 *  HeightMap.  
 *
 *  @param chunk            The dirty chunk.
 *	@param justLoaded		Has the chunk just been loaded?
 */
void HeightMap::dirtyThumbnail(Chunk *chunk, bool justLoaded)
{
	BW_GUARD;

    int16 gridX, gridY;
	WorldManager::instance().geometryMapping()->gridFromChunkName(chunk->identifier(), gridX, gridY);
    dirtyThumbnail(gridX, gridY, justLoaded);
}


/**
 *  This function is called to add a dirty chunk by coordinates.  We check
 *  that the chunk is not already dirty.  Note that the coordinates are in the
 *  range (localToWorldX_, localToWorldY_) to 
 *  (localToWorldX_ + gridWidth, localToWorldY_ + gridHeight).
 *
 *  @param gridX            The x coordinate of the dirty chunk.
 *  @param gridY            The y coordinate of the dirty chunk.
 *	@param justLoaded		Has the chunk just been loaded?
 */
void HeightMap::dirtyThumbnail(int32 gridX, int32 gridY, bool justLoaded)
{
	BW_GUARD;

	Chunk* chunk = ChunkManager::instance().findChunkByGrid( gridX, gridY, WorldManager::instance().geometryMapping() );
	ChunkTerrain* terrain = NULL;

	if (chunk && chunk->loaded())
	{
		terrain = ChunkTerrainCache::instance( *chunk ).pTerrain();
	}

	if (!terrain)
	{
		return;
	}

	// If the chunk was just loaded and not edited and we have succesfully
	// made a valid height map then the chunk cannot be really dirty.
	if (justLoaded && madeValidOnce_)
		return;

    gridX -= localToWorldX_; // convert to (0..gridWidth - 1).
    gridY -= localToWorldY_; // convert to (0..gridHeight - 1).

    // See if undrawnChunks_ already has the chunk at x, y
    bool alreadyContains = false;
    for
    (
        BW::list<GridPos>::iterator it = undrawnChunks_.begin();
        it != undrawnChunks_.end();
        ++it
    )
    {
        if (it->first == (int)gridX && it->second == (int)gridY)
        {
            alreadyContains = true;
            break;
        }
    }
    if (!alreadyContains)
        undrawnChunks_.push_back(GridPos(gridX, gridY));
}


/**
 *  This function gets an approximate height at the given position.  This 
 *  height includes currently imported terrain.
 *
 *  @param pos              The position to query, in grid units.
 *
 *  @return                 The height a the given position.
 */
float HeightMap::heightAtGridPos(Vector2 const &pos) const
{
	BW_GUARD;

    if 
    (
        pos.x < 0 || pos.x >= (float)gridWidth_ 
        ||
        pos.y < 0 || pos.y >= (float)gridHeight_
        ||
        !hasTexture()
    )
    {
        return std::numeric_limits<float>::max();
    }
    else
    {
        int32 twidth  = textureWidth();
        int32 theight = textureHeight();
        int32 tx = (int32)(pos.x/gridWidth_ *twidth );
        int32 ty = (int32)(pos.y/gridHeight_*theight);
        uint16 *bits;
        uint32 pitch;
        beginTexture( &bits, &pitch, true );
        uint16 iheight = *(bits + ty*pitch + tx);
        endTexture();
        return uint16ToHeight(iheight);
    }
}


/**
 *  This function returns the minimum and maximum heights in the given range.
 *  We do not assume anything about the grid coordinates, though if the 
 *  rectangle formed by them does not intersect the terrain then 
 *  +/-std::numeric_limits<float>::max() (in minv/maxv resp) will be returned.
 *  The height returned includes currently imported terrain.
 *
 *  @param pos1         Coordinate 1 in grid units.
 *  @param pos2         Coordinate 2 in grid units.
 *  @param minv         The minimum height.
 *  @param maxv         The maximum height.
 */
void 
HeightMap::heightRange
(
    Vector2             const &pos1, 
    Vector2             const &pos2,
    float               &minv,
    float               &maxv
) const
{
	BW_GUARD;

    minv = +std::numeric_limits<float>::max();
    maxv = -std::numeric_limits<float>::max();

    int32 twidth  = textureWidth ();
    int32 theight = textureHeight();

    // Convert the coordinates to texture coordinates:
    int32 tx1 = (int32)(pos1.x/gridWidth_ *twidth );
    int32 ty1 = (int32)(pos1.y/gridHeight_*theight);
    int32 tx2 = (int32)(pos2.x/gridWidth_ *twidth );
    int32 ty2 = (int32)(pos2.y/gridHeight_*theight);

    // Normalise:
    if (tx1 > tx2) std::swap(tx1, tx2);
    if (ty1 > ty2) std::swap(ty1, ty2);

    // Increase by one (if clicked at a single point):
    ++tx2; ++ty2;

    // Clip:
    if
    (
        !rectanglesIntersect
        (
            tx1, ty2, tx2, ty1,
            0, (int)twidth-1, (int)theight-1, 0,
            tx1, ty2, tx2, ty1
        )
    )
    {
        return;
    }

    // Handle degenerate cases:
    if (tx1 == tx2 || ty1 == ty2)
        return;

    // The result as an uint16:
    uint16 imin = 65535;
    uint16 imax =     0;

    uint16 *bits;
    uint32 pitch;
    beginTexture( &bits, &pitch, true);
    for (int32 y = ty1; y <= ty2; ++y)
    {
        uint16 const *p = bits + y*pitch + tx1;
        for (int32 x = tx1; x <= tx2; ++x, ++p)
        {
            imin = std::min(imin, *p);
            imax = std::max(imax, *p);
        }
    }
    endTexture();

    // Convert the result to floating point:
    minv = uint16ToHeight(imin);
    maxv = uint16ToHeight(imax);
}


/**
 *  This makes sure that the HeightMap is valid by drawing all dirty chunks.
 *  Optionally a progress bar is shown.
 *
 *  @param showProgress     If true then a progress screen is shown if there
 *                          are more than NUM_CHUNKS_FOR_TEX_PROGRESS dirty
 *                          chunks to process.
 */
void HeightMap::makeValid(bool showProgress /*= true*/)
{
	BW_GUARD;

    SyncMode chunkStopper; // exclusive read access to chunks

	
	ProgressBarTask       *genHMTask = NULL;
    
    try
    { 
        if 
        (
            showProgress 
            && 
            undrawnChunks_.size() >= NUM_CHUNKS_FOR_TEX_PROGRESS
        )
        {
			genHMTask = new ProgressBarTask(
				LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/HEIGHT/HEIGHT_MAP/GENERATING_TEXTURE"),
				(float)(undrawnChunks_.size()/NUM_CHUNKS_PER_FRAME), false );
        }

        while (drawSomeChunks(true)) 
        {
            if (genHMTask != NULL)
                genHMTask->step();
        }

        bw_safe_delete(genHMTask);

		madeValidOnce_ = true;
    }
    catch (...)
    {
        bw_safe_delete(genHMTask);
        throw;
    }
}


/**
 *  This is called to delete the saved height map and prevent it from being 
 *  saved.  This is because the user is closing this space and not saving it,
 *  and so the HeightMap is completely invalid.
 */
void HeightMap::doNotSaveHeightMap()
{
	BW_GUARD;

    // Delete the texture:
    texture_ = NULL;
	textureSubstitute_ = NULL;

    // Delete the cached file:
    BW::string filename = 
         BWResource::resolveFilename(cachedHeightMapName());
    ::unlink(filename.c_str());

    // Clear the height map:
    if (!isPS2_)
        heightMap_.clear();
}


/**
 *  This function returns true if we are using the more advanced rendering 
 *  path.
 */
bool HeightMap::isPS2() const
{
    return isPS2_;
}


/**
 *  This extracts a sub-image of the terrain into the given image for later
 *  retrieval.  The image is resized as necessary.  The coordinates are clipped
 *  if necessary.
 *
 *  @param leftf        The left coordinate in grid units.
 *  @param topf         The top coordinate in grid units.
 *  @param rightf       The right coordinate in grid units.
 *  @param bottomf      The bottom coordinate in grid units.
 *  @param subImage     The image to save to.
 */
void HeightMap::extractSubimage
(
    float               leftf,
    float               topf,
    float               rightf,
    float               bottomf,
    Moo::Image<uint16>    &subImage
) const
{
	BW_GUARD;

    subImage.clear();

    int32 left, top, right, bottom;
    bool ok = 
        toTexCoords(leftf, topf, rightf, bottomf, left, top, right, bottom);
    if (!ok)
        return;

    // Blit into the subimage:
    subImage.resize(right - left + 1, top - bottom + 1);
    uint16 *bits;
    uint32 pitch;
    beginTexture( &bits, &pitch, true );
    for (int32 y = bottom; y <= top; ++y)
    {
        uint16 *p       = subImage.getRow(y - bottom);
        uint16 const *q = bits + y*pitch + left;
		memcpy( p, q, sizeof( uint16 ) * ( right - left + 1 ) );
    }
    endTexture();

    invalidTexture_ = true;
}


/**
 *  This restores a sub-image of the terrain from the given image.  The image 
 *  is resized as necessary.  The coordinates are clipped if necessary.
 *
 *  @param leftf        The left coordinate in grid units.
 *  @param topf         The top coordinate in grid units.
 *  @param rightf       The right coordinate in grid units.
 *  @param bottomf      The bottom coordinate in grid units.
 *  @param subImage     The image to restore from.
 */
void HeightMap::restoreSubimage
(
    float               leftf,
    float               topf,
    float               rightf,
    float               bottomf,
    Moo::Image<uint16>    const &subImage
)
{
	BW_GUARD;

    if (subImage.isEmpty())
        return;

    int32 left, top, right, bottom;
    bool ok = 
        toTexCoords(leftf, topf, rightf, bottomf, left, top, right, bottom);
    if (!ok)
        return;

    // Blit into the subimage:
    uint16 *bits;
    uint32 pitch;
    beginTexture( &bits, &pitch, false );
    for (int32 y = bottom; y <= top; ++y)
    {
        uint16 *p       = bits + y*pitch + left;;
        uint16 const *q = subImage.getRow(y - bottom);            
        for (int32 x = left; x <= right; ++x, ++p, ++q)
        {
            *p = *q;
        }
    }
    endTexture();

    invalidTexture_ = true;
}


/**
 *  This function is used to 'blit' imported terrain into the HeightMap 
 *  using the appropriate mode.
 *
 *  @param leftf        The left coordinate in grid units.
 *  @param topf         The top coordinate in grid units.
 *  @param rightf       The right coordinate in grid units.
 *  @param bottomf      The bottom coordinate in grid units.
 *  @param mode         How is the terrain data placed - replace existing 
 *                      terrain etc.
 *  @param minv         The height to scale the minimum of the imported
 *                      terrain.
 *  @param maxv         The height to scale the maximum of the imported 
 *                      terrain.
 *  @param alpha        The strength of the import (0...1).
 *  @param image        The imported terrain.
 */
void HeightMap::addTerrain
(
    float               leftf,
    float               topf,
    float               rightf,
    float               bottomf,
    ElevationBlit::Mode mode,
    float               minv,
    float               maxv,
    float               alpha, 
    ImportImage			const &image
)
{
	BW_GUARD;

    if (minv > maxv) 
        std::swap(minv, maxv);

    float offset = 0.0f;
    float scale  = 0.0f;
    switch (mode)
    {
    case ElevationBlit::ADDITIVE:       // fall through deliberate
    case ElevationBlit::SUBTRACTIVE:   
        offset = 0.0f;
        scale  = alpha*(maxv - minv);
        break;
    case ElevationBlit::OVERLAY:
        offset = -0.5f*alpha*(maxv - minv);
        scale  = alpha*(maxv - minv);
        break;
    case ElevationBlit::MAX:            // fall through deliberate
    case ElevationBlit::MIN:            // fall through deliberate
    case ElevationBlit::REPLACE:
        offset = minv;
        scale  = alpha*(maxv - minv);
        break;
    }

    int32 ileft   = 0;
    int32 itop    = image.height() - 1;
    int32 iright  = image.width () - 1;
    int32 ibottom = 0;

    int32 left, top, right, bottom;
    bool ok = 
        toTexCoords
        (
            leftf , topf , rightf , bottomf, 
            left  , top  , right  , bottom ,
            &ileft, &itop, &iright, &ibottom
        );
    if (!ok)
        return;

    switch (mode)
    {
    case ElevationBlit::OVERLAY:       // fall through
    case ElevationBlit::ADDITIVE:
        blitAdditive
        (
            image, 
            left , top , right , bottom , 
            ileft, itop, iright, ibottom,
            scale, offset
        );
        break;
    case ElevationBlit::SUBTRACTIVE:
        blitSubtractive
        (
            image, 
            left , top , right , bottom , 
            ileft, itop, iright, ibottom,
            scale, offset
        );
        break;
    case ElevationBlit::MIN:
        blitMin
        (
            image, 
            left , top , right , bottom , 
            ileft, itop, iright, ibottom,
            scale, offset
        );
        break;
    case ElevationBlit::MAX:
        blitMax
        (
            image, 
            left , top , right , bottom , 
            ileft, itop, iright, ibottom,
            scale, offset
        );
        break;
    case ElevationBlit::REPLACE:
        blitReplace
        (
            image, 
            left , top , right , bottom , 
            ileft, itop, iright, ibottom,
            scale, offset
        );
        break;
    }
}


/**
 *  This is the HeightMap constructor.
 */
HeightMap::HeightMap() :
    gridWidth_(0),
    gridHeight_(0),
    minHeight_(+std::numeric_limits<float>::max()),
    maxHeight_(-std::numeric_limits<float>::max()),
    minHeightImpTerrain_(+std::numeric_limits<float>::max()),
    maxHeightImpTerrain_(-std::numeric_limits<float>::max()),
    minHeightTerrain_(+std::numeric_limits<float>::max()),
    maxHeightTerrain_(-std::numeric_limits<float>::max()),
    numPolesX_(0),
    numPolesY_(0),
    blockWidth_(0),
    blockHeight_(0),
    visOffX_(0),
    visOffY_(0),
    localToWorldX_(0),
    localToWorldY_(0),
    impTerrLeft_(0.0f),
    impTerrTop_(0.0f),
    impTerrRight_(0.0f),
    impTerrBottom_(0.0f),
    initialised_(false),
    invalidTexture_(true),
    isEightBitTex_(true),
    isPS2_(false),
    recommendSize_(0),
	madeValidOnce_(false),
	aborted_(false),
	textureSubstitute_( NULL ),
	accessReadOnly_( true )
{
}


/**
 *  This is the HeightMap destructor.
 */
HeightMap::~HeightMap()
{
}


/**
 *  This function creates the texture used to draw the height map.  We try and
 *  load the height map from disk, and if that fails we recreate it.
 */
void HeightMap::createTexture()
{
	BW_GUARD;

    texture_ = NULL;
	textureSubstitute_ = NULL;

    // Try loading off disk
    if (aborted_ == false && loadCachedHeightMap())
    {
        // Draw missing chunks:        
        CWaitCursor waitCursor;
        makeValid();
    }
    // Recreate from scratch:
    else
    {
        SyncMode chunkStopper; // exclusive read access to chunks
        CWaitCursor waitCursor;
        redrawAllChunks();
    }
}


/**
 *  This function deletes the texture used to draw the height map.  We save
 *  it to disk for later retrieval.
 */
void HeightMap::deleteTexture()
{
	BW_GUARD;

    texture_ = NULL;
	textureSubstitute_ = NULL;
    invalidTexture_ = true;
}


/**
 *  This function takes the chunk at gx, gy and adds it to our height map.
 *
 *  @param gx           The x coordinate of the chunk in grid coordiantes.
 *  @param gy           The y coordinate of the chunk in grid coordiantes.
 *	@param force		Force update even if there is an import texture?
 *  @returns			True if the update was successful.
 */
bool HeightMap::chunkToHeightMap(int32 gx, int32 gy, bool force)
{
	BW_GUARD;

    if (gx < 0 || gx >= (int32)gridWidth_ || gy < 0 || gy >= (int32)gridHeight_)
        return true; // Invalid coordinates so update is trivially successful.

	// Should not update if there is an imported image:
	if (!force && !impTerrSubImg_.isEmpty())
		return false;

    uint32 twidth  = textureWidth ();
    uint32 theight = textureHeight();

    // The coordinates of this tile withn our texture:
    uint32 dx1 = Math::lerp(gx    , (int32)0, (int32)gridWidth_ , (uint32)0U, twidth  - 1U);
    uint32 dy1 = Math::lerp(gy    , (int32)0, (int32)gridHeight_, (uint32)0U, theight - 1U);
    uint32 dx2 = Math::lerp(gx + 1, (int32)0, (int32)gridWidth_ , (uint32)0U, twidth  - 1U);
    uint32 dy2 = Math::lerp(gy + 1, (int32)0, (int32)gridHeight_, (uint32)0U, theight - 1U);

    // Get the chunk's data:
    Terrain::TerrainHeightMap::ImageType terrainImage;
    gx += localToWorldX_;
    gy += localToWorldY_;
    TerrainUtils::getTerrain(gx, gy, terrainImage, false);

    if (!terrainImage.isEmpty())
    {
        // Iterate over the pixels for this chunk in the texture, bicubic 
        // sampling from the terrain data:
        uint16 *bits;
        uint32 pitch;
        beginTexture( &bits, &pitch, false );
        uint32 mx = numPolesX_ - blockWidth_;
        uint32 my = numPolesY_ - blockHeight_;
        for (uint32 dy = dy1; dy <= dy2; ++dy)
        {
            uint16 *p = bits + dy*pitch + dx1;
            float  sy = Math::lerp(dy, dy1, dy2, 0.0f, (float)numPolesX_ - mx);
            for (uint32 dx = dx1; dx <= dx2; ++dx, ++p)
            {
                float sx     = Math::lerp(dx, dx1, dx2, 0.0f, (float)numPolesY_ - my);
                float height = terrainImage.getBicubic(sx, sy);
                minHeightTerrain_ = std::min(minHeightTerrain_, height);
                maxHeightTerrain_ = std::max(maxHeightTerrain_, height);
                *p = heightToUint16(height);
            }
        }
        endTexture();
    }
    // In case the chunk could not be read then fill this part of the texture
    // with the height 0.0:
    else
    {
        uint16 fillValue = heightToUint16(0.0f);
        uint16 *bits;
        uint32 pitch;
        beginTexture( &bits, &pitch, false );
        for (uint32 dy = dy1; dy <= dy2; ++dy)
        {
            uint16 *p = bits + dy*pitch + dx1;
            for (uint32 dx = dx1; dx <= dx2; ++dx, ++p)
            {
                *p = fillValue;
            }
        }
        endTexture();
    }

    invalidTexture_ = true;

	return true;
}


/**
 *  This function draws up to NUM_CHUNKS_PER_FRAME dirty chunks into the
 *  height map.
 *
 *	@param force		Force drawing even if there is an imported 
 *						terrain/mask.
 *  @returns            True if there are still chunks needed to be drawn.
 */
bool HeightMap::drawSomeChunks(bool force)
{
	BW_GUARD;

    if (undrawnChunks_.empty())
        return false;

    for 
    (
        uint32 count = 0; 
        count < NUM_CHUNKS_PER_FRAME && !undrawnChunks_.empty();
        ++count
    )
    {
        GridPos gp = undrawnChunks_.front();
        if (chunkToHeightMap(gp.first, gp.second, force))
			undrawnChunks_.pop_front();
		else
			break;
    }

    return !undrawnChunks_.empty();
}


/**
 *  This function creates the recommended size of the height map in the.
 *
 *  @param x            The coordinate that we want the size of.
 *
 *  @return             The recommended size of this coordinate.
 */
uint32 HeightMap::recommendHeightMapSize(uint32 x) const
{
    // Find the next power of 2, or clamp to 2048.
    uint32 result = 1;
    while (result < x && result < 2048)
        result *= 2;
    return result;
}


/**
 *  This function converts from grid coordinates to height map texture 
 *  coordinates.  It clips the coordinates and optionally returns the 
 *  clipped area.
 *
 *  @param leftf        The left coordinate in grid units.
 *  @param topf         The top coordinate in grid units.
 *  @param rightf       The right coordinate in grid units.
 *  @param bottomf      The bottom coordinate in grid units.
 *  @param texLeft      The left texture coordinate.
 *  @param texTop       The top texture coordinate.
 *  @param texRight     The right texture coordinate.
 *  @param texBottom    The bottom texture coordinate.
 *  @param ileft        The left clipped coordinate.
 *  @param itop         The top clipped coordinate.
 *  @param iright       The right clipped coordinate.
 *  @param ibottom      The bottom clipped coordinate.
 *
 *  @return             True if the rectangle intersects the texture rectangle,
 *                      false if the rectangle did not intersect.
 */
bool HeightMap::toTexCoords
(
    float               leftf,
    float               topf,
    float               rightf,
    float               bottomf,
    int32               &texLeft,
    int32               &texTop,
    int32               &texRight,
    int32               &texBottom,
    int32               *ileft      /*= NULL*/,
    int32               *itop       /*= NULL*/,
    int32               *iright     /*= NULL*/,
    int32               *ibottom    /*= NULL*/
) const
{
	BW_GUARD;

    // Normalise the rectangle:
    if (leftf > rightf)
        std::swap(leftf, rightf);
    if (bottomf > topf)
        std::swap(bottomf, topf);

    uint32 twidth  = textureWidth();
    uint32 theight = textureHeight();

    // Convert to texture coordinates:
    texLeft   = (int)(leftf  *twidth /gridWidth_ );
    texTop    = (int)(topf   *theight/gridHeight_);
    texRight  = (int)(rightf *twidth /gridWidth_ );
    texBottom = (int)(bottomf*theight/gridHeight_);

    // Save these coordinates:
    int32 otexLeft   = texLeft;
    int32 otexTop    = texTop;
    int32 otexRight  = texRight;
    int32 otexBottom = texBottom;
 
    if (texLeft == texRight || texTop == texBottom)
        return false;

    // Clip to the texture coordinates and return true if there is some
    // overlap:
    bool result =
        rectanglesIntersect
        (
            texLeft, texTop           , texRight          , texBottom,
            0      , (int32)twidth - 1, (int32)theight - 1, 0        ,
            texLeft, texTop           , texRight          , texBottom
        );
    if (result && ileft != NULL)
    {
        int32 width  = *iright - *ileft   + 1;
        int32 height = *itop   - *ibottom + 1;
        // Have to cast to int64 to avoid overflow
        *ileft   = (int32)Math::lerp< int64, int64 >(
							texLeft  , otexLeft  , otexRight , 0, width  - 1 );
        *itop    = (int32)Math::lerp< int64, int64 >(
							texTop   , otexTop   , otexBottom, 0, height - 1 );
        *iright  = (int32)Math::lerp< int64, int64 >(
							texRight , otexLeft  , otexRight , 0, width  - 1 );
        *ibottom = (int32)Math::lerp< int64, int64 >(
							texBottom, otexTop   , otexBottom, 0, height - 1 );
		// don't allow degenerate results
		result = (*ileft != *iright) && (*itop != *ibottom);
    }
    return result;
}


/**
 *  This function blits in imported terrain using the additive mode.
 *
 *  @param image        The terrain to import.
 *  @param left         The left coordinate in grid units.
 *  @param top          The top coordinate in grid units.
 *  @param right        The right coordinate in grid units.
 *  @param bottom       The bottom coordinate in grid units..
 *  @param ileft        The left clipped corodinate.
 *  @param itop         The top clipped corodinate.
 *  @param iright       The right clipped corodinate.
 *  @param ibottom      The bottom clipped corodinate.
 *  @param scale        The scale to apply to the imported terrain.
 *  @param offset       The offset to apply to the imported terrain.
 */
void 
HeightMap::blitAdditive
(
    ImportImage			const &image,
    int32               left,
    int32               top,
    int32               right,
    int32               bottom,
    int32               ileft,
    int32               itop,
    int32               iright,
    int32               ibottom,
    float               scale,
    float               offset
)
{
	BW_GUARD;

    // Blit into the subimage:
    uint16 mint   = 65535;
    uint16 maxt   =     0;
    uint16 result =     0;

    minHeightImpTerrain_ = +std::numeric_limits<float>::max();
    maxHeightImpTerrain_ = +std::numeric_limits<float>::max();

    uint16 *bits;
    uint32 pitch;
    beginTexture( &bits, &pitch, false );
    for (int32 y = bottom; y <= top; ++y)
    {
        uint16 *p   = bits + y*pitch + left;
        int32  sy   = (int32)Math::lerp< int64, int64 >(
											y, top, bottom, itop, ibottom );  
        const uint16 *row = image.getRow(sy);
        for (int32 x = left; x <= right; ++x, ++p)
        {
            int32 sx = (int32)Math::lerp< int64, int64 >(
											x, left, right, ileft, iright );
            float height = image.toHeight(*(row + sx))*scale + offset;
			height = uint16ToHeight( *p ) - MIN_HEIGHT + height;
			height = Math::clamp( 0.f, height, MAX_HEIGHT - MIN_HEIGHT );
			result = uint16( height * 65535.f / ( MAX_HEIGHT - MIN_HEIGHT ) );
            mint = std::min(mint, result);
            maxt = std::max(maxt, result);
            *p = result;
        }
    }
    endTexture();

    minHeightImpTerrain_ = uint16ToHeight(mint);
    maxHeightImpTerrain_ = uint16ToHeight(maxt);
    invalidTexture_      = true;
}


/**
 *  This function blits in imported terrain using the subtractive mode.
 *
 *  @param image        The terrain to import.
 *  @param left         The left coordinate in grid units.
 *  @param top          The top coordinate in grid units.
 *  @param right        The right coordinate in grid units.
 *  @param bottom       The bottom coordinate in grid units..
 *  @param ileft        The left clipped corodinate.
 *  @param itop         The top clipped corodinate.
 *  @param iright       The right clipped corodinate.
 *  @param ibottom      The bottom clipped corodinate.
 *  @param scale        The scale to apply to the imported terrain.
 *  @param offset       The offset to apply to the imported terrain.
 */
void 
HeightMap::blitSubtractive
(
    ImportImage		const &image,
    int32           left,
    int32           top,
    int32           right,
    int32           bottom,
    int32           ileft,
    int32           itop,
    int32           iright,
    int32           ibottom,
    float           scale,
    float           offset
)
{
	BW_GUARD;

    // Blit into the subimage:
    uint16 mint   = 65535;
    uint16 maxt   =     0;
    uint16 result =     0;

    minHeightImpTerrain_ = +std::numeric_limits<float>::max();
    maxHeightImpTerrain_ = +std::numeric_limits<float>::max();

    uint16 *bits;
    uint32 pitch;
    beginTexture( &bits, &pitch, false );
    for (int32 y = bottom; y <= top; ++y)
    {
        uint16 *p   = bits + y*pitch + left;
        int32  sy   = (int32)Math::lerp< int64, int64 >(
											y, top, bottom, itop, ibottom );  
        const uint16 *row = image.getRow(sy);
        for (int32 x = left; x <= right; ++x, ++p)
        {
            int32 sx = (int32)Math::lerp< int64, int64 >(
											x, left, right, ileft, iright );
            float height = image.toHeight(*(row + sx))*scale + offset;
			height = uint16ToHeight( *p ) - MIN_HEIGHT - height;
			height = Math::clamp( 0.f, height, MAX_HEIGHT - MIN_HEIGHT );
			result = uint16( height * 65535.f / ( MAX_HEIGHT - MIN_HEIGHT ) );
            mint = std::min(mint, result);
            maxt = std::max(maxt, result);
            *p = result;
        }
    }
    endTexture();

    minHeightImpTerrain_ = uint16ToHeight(mint);
    maxHeightImpTerrain_ = uint16ToHeight(maxt);
    invalidTexture_      = true;
}


/**
 *  This function blits in imported terrain using the minimum mode.
 *
 *  @param image        The terrain to import.
 *  @param left         The left coordinate in grid units.
 *  @param top          The top coordinate in grid units.
 *  @param right        The right coordinate in grid units.
 *  @param bottom       The bottom coordinate in grid units..
 *  @param ileft        The left clipped corodinate.
 *  @param itop         The top clipped corodinate.
 *  @param iright       The right clipped corodinate.
 *  @param ibottom      The bottom clipped corodinate.
 *  @param scale        The scale to apply to the imported terrain.
 *  @param offset       The offset to apply to the imported terrain.
 */
void 
HeightMap::blitMin
(
    ImportImage		const &image,
    int32           left,
    int32           top,
    int32           right,
    int32           bottom,
    int32           ileft,
    int32           itop,
    int32           iright,
    int32           ibottom,
    float           scale,
    float           offset
)
{
	BW_GUARD;

    // Blit into the subimage:
    uint16 mint   = 65535;
    uint16 maxt   =     0;
    uint16 result =     0;

    minHeightImpTerrain_ = +std::numeric_limits<float>::max();
    maxHeightImpTerrain_ = +std::numeric_limits<float>::max();

    uint16 *bits;
    uint32 pitch;
    beginTexture( &bits, &pitch, false );
    for (int32 y = bottom; y <= top; ++y)
    {
        uint16 *p   = bits + y*pitch + left;
        int32  sy   = (int32)Math::lerp< int64, int64 >(
											y, top, bottom, itop, ibottom );  
        const uint16 *row = image.getRow(sy);
        for (int32 x = left; x <= right; ++x, ++p)
        {
            int32 sx = (int32)Math::lerp< int64, int64 >(
											x, left, right, ileft, iright );
            float height = image.toHeight(*(row + sx))*scale + offset;
            result = std::min(*p, heightToUint16(height));
            mint = std::min(mint, result);
            maxt = std::max(maxt, result);
            *p = result;
        }
    }
    endTexture();

    minHeightImpTerrain_ = uint16ToHeight(mint);
    maxHeightImpTerrain_ = uint16ToHeight(maxt);
    invalidTexture_      = true;
}


/**
 *  This function blits in imported terrain using the maximum mode.
 *
 *  @param image        The terrain to import.
 *  @param left         The left coordinate in grid units.
 *  @param top          The top coordinate in grid units.
 *  @param right        The right coordinate in grid units.
 *  @param bottom       The bottom coordinate in grid units..
 *  @param ileft        The left clipped corodinate.
 *  @param itop         The top clipped corodinate.
 *  @param iright       The right clipped corodinate.
 *  @param ibottom      The bottom clipped corodinate.
 *  @param scale        The scale to apply to the imported terrain.
 *  @param offset       The offset to apply to the imported terrain.
 */
void 
HeightMap::blitMax
(
    ImportImage		const &image,
    int32           left,
    int32           top,
    int32           right,
    int32           bottom,
    int32           ileft,
    int32           itop,
    int32           iright,
    int32           ibottom,
    float           scale,
    float           offset
)
{
	BW_GUARD;

    // Blit into the subimage:
    uint16 mint   = 65535;
    uint16 maxt   =     0;
    uint16 result =     0;

    minHeightImpTerrain_ = +std::numeric_limits<float>::max();
    maxHeightImpTerrain_ = +std::numeric_limits<float>::max();

    uint16 *bits;
    uint32 pitch;
    beginTexture( &bits, &pitch, false );
    for (int32 y = bottom; y <= top; ++y)
    {
        uint16 *p   = bits + y*pitch + left;
        int32  sy   = (int32)Math::lerp< int64, int64 >(
											y, top, bottom, itop, ibottom );  
        const uint16 *row = image.getRow(sy);
        for (int32 x = left; x <= right; ++x, ++p)
        {
            int32 sx = (int32)Math::lerp< int64, int64 >(
											x, left, right, ileft, iright );
            float height = image.toHeight(*(row + sx))*scale + offset;
            result = std::max(*p, heightToUint16(height));
            mint = std::min(mint, result);
            maxt = std::max(maxt, result);
            *p = result;
        }
    }
    endTexture();

    minHeightImpTerrain_ = uint16ToHeight(mint);
    maxHeightImpTerrain_ = uint16ToHeight(maxt);
    invalidTexture_      = true;
}


/**
 *  This function blits in imported terrain using the replace mode.
 *
 *  @param image        The terrain to import.
 *  @param left         The left coordinate in grid units.
 *  @param top          The top coordinate in grid units.
 *  @param right        The right coordinate in grid units.
 *  @param bottom       The bottom coordinate in grid units..
 *  @param ileft        The left clipped corodinate.
 *  @param itop         The top clipped corodinate.
 *  @param iright       The right clipped corodinate.
 *  @param ibottom      The bottom clipped corodinate.
 *  @param scale        The scale to apply to the imported terrain.
 *  @param offset       The offset to apply to the imported terrain.
 */
void 
HeightMap::blitReplace
(
    ImportImage		const &image,
    int32           left,
    int32           top,
    int32           right,
    int32           bottom,
    int32           ileft,
    int32           itop,
    int32           iright,
    int32           ibottom,
    float           scale,
    float           offset
)
{
	BW_GUARD;

    // Blit into the subimage:
    uint16 mint   = 65535;
    uint16 maxt   =     0;
    uint16 result =     0;

    minHeightImpTerrain_ = +std::numeric_limits<float>::max();
    maxHeightImpTerrain_ = +std::numeric_limits<float>::max();

    uint16 *bits;
    uint32 pitch;
    beginTexture( &bits, &pitch, false );
	const float multiplier = 65535 / (MAX_HEIGHT- MIN_HEIGHT);
    for (int32 y = bottom; y <= top; ++y)
    {
        uint16 *p   = bits + y*pitch + left;
        int32  sy   = (int32)Math::lerp< int64, int64 >(
											y, top, bottom, itop, ibottom );  
        const uint16 *row = image.getRow(sy);
        for (int32 x = left; x <= right; ++x, ++p)
        {
            int32 sx = (int32)Math::lerp< int64, int64 >(
											x, left, right, ileft, iright );
            float height = image.toHeight(*(row + sx))*scale + offset;
			result = ( uint16 ) ( multiplier * (Math::clamp( MIN_HEIGHT, height, MAX_HEIGHT ) - MIN_HEIGHT ) );
            mint = std::min(mint, result);
            maxt = std::max(maxt, result);
            *p = result;
        }
    }

    endTexture();

    minHeightImpTerrain_ = uint16ToHeight(mint);
    maxHeightImpTerrain_ = uint16ToHeight(maxt);
    invalidTexture_      = true;
}


/**
 *  This is the file name of the cached height map.
 */
BW::string HeightMap::cachedHeightMapName() const
{
	BW_GUARD;

    return spaceName_ + "/space.height.raw";
}


/**
 *  This function tries to load the cached height map.
 *
 *  @return         True if the load was successful, false otherwise.
 */
bool HeightMap::loadCachedHeightMap()
{
	BW_GUARD;

    BW::string textureName = 
        BWResource::resolveFilename(cachedHeightMapName());
    FILE *file = NULL;
    try
    {
        file = BWResource::instance().fileSystem()->posixFileOpen(textureName, "rb");
        if (file == NULL)
            return false;
        MF_VERIFY( fseek(file, 0, SEEK_END) == 0 );
        long sz = ftell(file);
        MF_VERIFY( fseek(file, 0, SEEK_SET) == 0 );
        long width = (long)std::sqrt((float)(sz/sizeof(uint16)));
        long height = width;
        if (isPS2_)
        {
            texture_ =  
                Moo::rc().createTexture
                (
                    width,
                    height,
                    1,                  // no mipmap levels
                    0,					// usage
                    D3DFMT_L16,
                    D3DPOOL_MANAGED
                );
			textureSubstitute_ = 
				Moo::rc().createTexture
				( 
					width,
					height,
					1,
					0,
					D3DFMT_L16,
					D3DPOOL_SYSTEMMEM
				);
        }
        else
        {
            heightMap_.resize(width, height);
        }
        uint16 minv = 65535;
        uint16 maxv =     0;
        uint16 *bits;
        uint32 pitch;
        beginTexture( &bits, &pitch, false );
        for (long y = 0; y < height; ++y)
        {
            uint16 *row = bits + y*pitch;
            fread(row, sizeof(uint16), width, file);
            uint16 *p = row;
            for (long x = 0; x < width; ++x)
            {
                minv = std::min(*p, minv);
                maxv = std::max(*p, maxv);
                ++p;
            }
        }
        endTexture();
        minHeightTerrain_ = uint16ToHeight(minv);
        maxHeightTerrain_ = uint16ToHeight(maxv);
        fclose(file);
        file = NULL;
        invalidTexture_ = true;
		madeValidOnce_ = true;
        return true;
    }
    catch (...)
    {
        if (file != NULL)
        {
            fclose(file);
            file = NULL;
        }
        throw;
    }
}


/**
 *  This function saves the cached height map.
 */
void HeightMap::saveCachedHeightMap()
{
	BW_GUARD;

    if (!heightMap_.isEmpty())
    {
        clearImpTerrain();
        BW::string textureName = 
            BWResource::resolveFilename(cachedHeightMapName());
        FILE *file = NULL;
        try
        {
            file = BWResource::instance().fileSystem()->posixFileOpen(textureName, "wb");
            if (file == NULL)
                return;
            uint32 twidth  = textureWidth ();
            uint32 theight = textureHeight();
            uint16 *bits;
            uint32 pitch;
            beginTexture( &bits, &pitch, true );
            for (size_t y = 0; y < theight; ++y)
            {
                uint16 *row = bits + y*pitch;
                fwrite(row, sizeof(uint16), twidth, file);
            }
            endTexture();
            fclose(file);
            file = NULL;
        }
        catch (...)
        {
            if (file != NULL)
            {
                fclose(file);
                file = NULL;
            }
            throw;
        }
    }
}


/**
 *  This function redraws all the chunks, whether they are dirty or not.
 */
void HeightMap::redrawAllChunks()
{
	BW_GUARD;

    if (isPS2_)
    {
        texture_ =
			Moo::rc().createTexture
            (
                recommendSize_,
                recommendSize_,
                1,                  // no mipmap levels
                0,              // usage
                D3DFMT_L16,
                D3DPOOL_MANAGED
            );
		MF_ASSERT(texture_);
		textureSubstitute_ = 
			Moo::rc().createTexture
			( 
				recommendSize_,
				recommendSize_,
				1,
				0,
				D3DFMT_L16,
				D3DPOOL_SYSTEMMEM
			);
		MF_ASSERT( textureSubstitute_ );
    }
    else
    {
        heightMap_.resize(recommendSize_, recommendSize_);
    }

    minHeightTerrain_ = +std::numeric_limits<float>::max();
    maxHeightTerrain_ = -std::numeric_limits<float>::max();

	ProgressBarTask		*genHMTask = NULL;
    
    try
    { 
        if (gridHeight_*gridWidth_ >= NUM_CHUNKS_FOR_TEX_PROGRESS)
        {
            genHMTask =
                new ProgressBarTask
                (
                    LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/HEIGHT/HEIGHT_MAP/GENERATING_TEXTURE"),
                    (float)(gridWidth_*gridHeight_), true
                );
        }

        // Redraw all chunks:
		aborted_ = false;

        for (uint32 y = 0; !aborted_ && y < gridHeight_; ++y)
        {
            for (uint32 x = 0; !aborted_ && x < gridWidth_; ++x)
            {
				if (genHMTask != NULL)
					genHMTask->step();
                chunkToHeightMap(x, y, true);
				if( !aborted_ && genHMTask != NULL )
					aborted_ = genHMTask->isCancelled();
            }
        }
        bw_safe_delete(genHMTask);
		
		madeValidOnce_ = true;
    }
    catch (...)
    {
        bw_safe_delete(genHMTask);
        throw;
    }

    // Everything is drawn:
    undrawnChunks_.clear();
}


/**
 *  This function makes sure that we have a texture that matches the height 
 *  map.
 */
void HeightMap::ensureValidTexture() const
{
	BW_GUARD;

    // We don't need to update this in PS2.0 systems:
    if (isPS2_)
        return;
    
    // Is the texture currently valid:
    if (!invalidTexture_ && texture_ )
        return;

    // Create the texture:
    if (!texture_)
    {
        isEightBitTex_ = Moo::rc().supportsTextureFormat(D3DFMT_L8);
        if (isEightBitTex_)
        {
            texture_ =
                Moo::rc().createTexture
                (
                    heightMap_.width(),
                    heightMap_.height(),
                    1,                  // no mipmap levels
                    0,					// usage
                    D3DFMT_L8,
                    D3DPOOL_MANAGED
                );
			MF_ASSERT(texture_);
        }
        if (!texture_)
        {
            texture_ =
                Moo::rc().createTexture
                (
                    heightMap_.width(),
                    heightMap_.height(),
                    1,                  // no mipmap levels
                    0,					// usage
                    D3DFMT_X8R8G8B8,
                    D3DPOOL_MANAGED
                );
			MF_ASSERT(texture_);
            isEightBitTex_ = false;
        }
    }

	Moo::TextureLockWrapper	textureLock;
    D3DLOCKED_RECT rect;
	textureLock.init( texture_ );
    HRESULT hr = textureLock.lockRect(0, &rect, NULL, 0);
    if (FAILED(hr))
    {
        ERROR_MSG("Unable to lock height map texture");
		textureLock.flush();
		textureLock.fini();
		return;
    }

    uint16 minh = heightToUint16(minHeight());
    uint16 maxh = heightToUint16(maxHeight());

    if (isEightBitTex_)
    {
        uint8 *textureBits = static_cast<uint8 *>(rect.pBits);
        if (minh == maxh)
        {
            for (uint32 y = 0; y < heightMap_.height(); ++y)
            {
                uint8 *dest = textureBits + y*rect.Pitch;
                for (uint32 x = 0; x < heightMap_.width(); ++x)
                {
                    *dest++ = 128;
                }
            }
        }
        else
        {
            for (uint32 y = 0; y < heightMap_.height(); ++y)
            {
                uint8  *dest      = textureBits + y*rect.Pitch;
                uint16 const *src = heightMap_.getRow(y);
                for (uint32 x = 0; x < heightMap_.width(); ++x)
                {
                    *dest = Math::clamp(0, Math::lerp(*src, minh, maxh, 0, 255), 255);
                    ++dest;
                    ++src;
                }
            }
        }
    }
    else
    {
        uint8 *textureBits = static_cast<uint8 *>(rect.pBits);
        if (minh == maxh)
        {
            for (uint32 y = 0; y < heightMap_.height(); ++y)
            {
                uint8 *dest = textureBits + y*rect.Pitch;
                for (uint32 x = 0; x < heightMap_.width(); ++x)
                {
                    uint8 val = 128;
                    *dest++ = val;
                    *dest++ = val;
                    *dest++ = val;
                    ++dest;
                }
            }
        }
        else
        {
            for (uint32 y = 0; y < heightMap_.height(); ++y)
            {
                uint8  *dest      = textureBits + y*rect.Pitch;
                uint16 const *src = heightMap_.getRow(y);
                for (uint32 x = 0; x < heightMap_.width(); ++x)
                {
                    uint8 val = Math::clamp(0, Math::lerp(*src, minh, maxh, 0, 255), 255);
                    *dest++ = val;
                    *dest++ = val;
                    *dest++ = val;
                    ++dest;
                    ++src;
                }
            }
        }
    }

    // Cleanup:
    textureLock.unlockRect(0);
	textureLock.flush();
	textureLock.fini();

    invalidTexture_ = false; // the texture is now valid
}


/**
 *  This function is used to begin to access textures for the ps2.0 path. 
 *
 *  @param base         A pointer to the first pixel.
 *  @param pitch        The difference between scanlines in uint16s.
 *	@param readOnly		If this texture is only used for read purpose in this access
 */
void HeightMap::beginTexture( uint16 **base, uint32 *pitch, bool readOnly ) const
{
	BW_GUARD;

	if (isPS2_)
	{
		// source texture should have at least one level
		MF_ASSERT ( texture_->GetLevelCount() > 0 );
		accessReadOnly_ = readOnly;
		HRESULT hr;
		D3DLOCKED_RECT  lockedRect;
		if (textureSubstitute_.hasComObject())
		{
			hr =  textureSubstitute_->LockRect( 0, &lockedRect, NULL, 0 );
		}
		else
		{
			hr =  texture_->LockRect( 0, &lockedRect, NULL, 0 );
		}
		MF_ASSERT(SUCCEEDED(hr));
		*base  = reinterpret_cast<uint16 *>(lockedRect.pBits);
		*pitch = static_cast<uint32>(lockedRect.Pitch/sizeof(uint16));
	}
    else
    {
        *base  = const_cast< uint16* > ( heightMap_.getRow(0) ); // TODO fix this const_cast<>, see BWT-26710
        *pitch = heightMap_.width();
    }
}


/**
 *  This function is used to end modify the texture for the ps2.0 path.
 */
void HeightMap::endTexture() const
{
	BW_GUARD;

	if (isPS2_)
	{
		HRESULT hr;
		if (textureSubstitute_.hasComObject())
		{
			hr =  textureSubstitute_->UnlockRect( 0 );
			MF_ASSERT(SUCCEEDED(hr));
			// update from textureSubstitute_ to texture_
			if (!accessReadOnly_)
			{
				hr = Moo::rc().device()->UpdateTexture(
						textureSubstitute_.pComObject(), texture_.pComObject() );
				MF_ASSERT( SUCCEEDED( hr ) );
			}
		}
		else
		{
			hr = texture_->UnlockRect( 0 );
			MF_ASSERT(SUCCEEDED(hr));
		}
	}
}


/**
 *  This returns the width of the texture.
 */
uint32 HeightMap::textureWidth() const
{
	BW_GUARD;

    if (isPS2_)
    {
        D3DSURFACE_DESC desc;
        HRESULT hr = texture_->GetLevelDesc(0, &desc);
        MF_ASSERT(SUCCEEDED(hr));
        return desc.Width;
    }
    else
    {
        return heightMap_.width();
    }
}


/**
 *  This returns the height of the texture.
 */
uint32 HeightMap::textureHeight() const
{
	BW_GUARD;

    if (isPS2_)
    {
        D3DSURFACE_DESC desc;
        HRESULT hr = texture_->GetLevelDesc(0, &desc);
        MF_ASSERT(SUCCEEDED(hr));
        return desc.Height;
    }
    else
    {
        return heightMap_.width();
    }
}


/**
 *  This returns true if we have a texture ready to be used.
 */ 
bool HeightMap::hasTexture() const
{
	BW_GUARD;

    return (isPS2_ && texture_ ) || (!isPS2_ && !heightMap_.isEmpty());
}
BW_END_NAMESPACE

