#include "pch.hpp"
#include "common/grid_coord.hpp"
#include "worldeditor/import/elevation_blit.hpp"
#include "worldeditor/import/import_image.hpp"
#include "worldeditor/import/terrain_utils.hpp"
#include "worldeditor/undo_redo/elevation_undo.hpp"
#include "worldeditor/project/chunk_walker.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/misc/sync_mode.hpp"
#include "worldeditor/misc/progress_bar_helper.hpp"
#include "resmgr/string_provider.hpp"
#include "math/mathdef.hpp"
#include <limits>

BW_BEGIN_NAMESPACE


namespace
{
	/**
	 *	The number of digits to snap imported terrain to.
	 */
	const double IMPORT_SNAP	= 0.01; // snap to 1cm.

    /**
     *	The position of the gridX'th, gridY'th terrain tile as if the terrain
     *	were one giant bitmap.  Note that edge tiles can have coordinates that
     *	outside the giant bitmap because some poles are set to zero.
     */
    void 
    tilePosition
    (
        TerrainUtils::TerrainFormat &format,
        unsigned int                gridX,
        unsigned int                gridY,
        int                         &left,
        int                         &top,
        int                         &right,
        int                         &bottom
    )
    {
        left   = gridX*format.blockWidth  - format.visOffsX;
        bottom = gridY*format.blockHeight - format.visOffsY;
        right  = left   + format.polesWidth  - 1;
        top    = bottom + format.polesHeight - 1;
    }
}


/**
 *  This function computes the memory (in bytes) required for a terrain import
 *  over the given grid coordinates.  The coordinates do not need be clipped
 *  or normalised (left < right etc).
 *
 *  @param leftf        The left coordinate.
 *  @param topf         The top coordinate.
 *  @param rightf       The right coordinate.
 *  @param bottomf      The bottom coordinate.
 *  @returns            The size of the undo buffer if an import is done.
 */
size_t ElevationBlit::importUndoSize
(
    float               leftf,
    float               topf,
    float               rightf,
    float               bottomf
)
{
	BW_GUARD;

    size_t numChunks = 0;
    int    nPolesX   = 0;
    int    nPolesY   = 0;

    { // force scope of modifyTerrain
        SyncMode chunkStopper;

        if (!chunkStopper)
            return 0;

		BW::string space = WorldManager::instance().getCurrentSpace();

		float gridSize;
	    uint16 gridWidth, gridHeight;
	    GridCoord localToWorld(0, 0);
		if (!TerrainUtils::spaceSettings(space, gridSize, gridWidth, gridHeight, localToWorld))
        {
            return 0;
        }

	    // Retrieve destination terrain format, by opening an existing
	    // terrain file and reading its header:
	    TerrainUtils::TerrainFormat format = WorldManager::instance().getTerrainInfo();
        
        nPolesX = format.polesWidth;
        nPolesY = format.polesHeight;

	    
        // Pretend the terrain is one giant bitmap of size 
        // widthPixels x heightPixels:
        unsigned int widthPixels  = gridWidth *format.blockWidth;
        unsigned int heightPixels = gridHeight*format.blockHeight;

		// Normalise the coordinates:
		if (leftf > rightf ) std::swap(leftf, rightf );
		if (topf  < bottomf) std::swap(topf , bottomf);

        // Convert the selection into the giant bitmap coordinates:
        int left   = (int)(leftf  *widthPixels /gridWidth );
        int top    = (int)(topf   *heightPixels/gridHeight);
        int right  = (int)(rightf *widthPixels /gridWidth );
        int bottom = (int)(bottomf*heightPixels/gridHeight);

        // Handle some degenerate cases:
        if (left == right || top == bottom)
            return false;
        
		// Find the grid boundaries
		int16 gridLeft = (int16)  floor( leftf );
		int16 gridRight = (int16) ceil ( rightf );
		int16 gridTop = (int16) ceil( topf );
		int16 gridBottom = (int16) floor( bottomf );

		BoundedChunkWalker bcw( gridLeft, gridBottom, gridRight, gridTop);
	    bcw.spaceInformation(SpaceInformation(space, localToWorld, gridWidth, gridHeight));

	    BW::string chunkName;
	    int16 gridX, gridZ;               
	    while (bcw.nextTile(chunkName, gridX, gridZ))
	    {
            if (TerrainUtils::isEditable(gridX, gridZ))
            {
                // The coordinates of this chunk in the giant bitmap system:
                int tileLeft, tileRight, tileBottom, tileTop;
                tilePosition
                (
                    format, 
                    gridX - localToWorld.x, 
                    gridZ - localToWorld.y, 
                    tileLeft, 
                    tileTop, 
                    tileRight, 
                    tileBottom
                ); 

                // Only process chunks that intersect the selection:
                int ileft, itop, iright, ibottom;
                if 
                (
                    TerrainUtils::rectanglesIntersect
                    (
                        tileLeft, tileTop, tileRight, tileBottom,
                        left    , top    , right    , bottom    ,
                        ileft   , itop   , iright   , ibottom   
                    )
                )
                {
                    ++numChunks;
                }
            }
        }
    } 

    return numChunks*sizeof(float)*nPolesX*nPolesY;
}


/**
 *  This function sets up an undo/redo operation for terrain import.  The 
 *  coordinates do not need be clipped or normalised (left < right etc).
 *
 *  @param leftf        The left coordinate.
 *  @param topf         The top coordinate.
 *  @param rightf       The right coordinate.
 *  @param bottomf      The bottom coordinate.
 *  @param description  The description of the undo/redo operation.
 *  @param showProgress Show a progress bar while generating undo/redo data.
 */
bool ElevationBlit::saveUndoForImport
(
    float           leftf,
    float           topf,
    float           rightf,
    float           bottomf,
    BW::string     const &description,
    bool            showProgress
)
{
	BW_GUARD;

    BW::list<ElevationUndoPos> modifiedChunksPos; // list of intersecting chunks

    BW::string space = WorldManager::instance().getCurrentSpace();

	float gridSize;
	uint16 gridWidth, gridHeight;
	GridCoord localToWorld(0, 0);
    if (!TerrainUtils::spaceSettings(space, gridSize, gridWidth, gridHeight, localToWorld))
    {
        return false;
    }

	// Retrieve destination terrain format, by opening an existing
	// terrain file and reading its header:
    TerrainUtils::TerrainFormat format = WorldManager::instance().getTerrainInfo();

	BW::string chunkName;
	int16 gridX, gridZ;

    // Pretend the terrain is one giant bitmap of size 
    // widthPixels x heightPixels:
    unsigned int widthPixels  = gridWidth *format.blockWidth;
    unsigned int heightPixels = gridHeight*format.blockHeight;
	
    // Convert the selection into the giant bitmap coordinates:
	// Normalise the coordinates:
	if (leftf > rightf ) std::swap(leftf, rightf );
	if (topf  < bottomf) std::swap(topf , bottomf); 

    int left   = (int)(leftf  *(widthPixels  - 1)/gridWidth );
    int top    = (int)(topf   *(heightPixels - 1)/gridHeight);
    int right  = (int)(rightf *(widthPixels  - 1)/gridWidth );
    int bottom = (int)(bottomf*(heightPixels - 1)/gridHeight);

    // Handle some degenerate cases:
    if (left == right || top == bottom)
        return false;
	// Find the grid boundaries
	int16 gridLeft = (int16)  floor( leftf );
	int16 gridRight = (int16) ceil ( rightf );
	int16 gridTop = (int16) ceil( topf );
	int16 gridBottom = (int16) floor( bottomf );

	BoundedChunkWalker bcw( gridLeft, gridBottom, gridRight, gridTop);
    bcw.spaceInformation(SpaceInformation(space, localToWorld, gridWidth, gridHeight));

	ProgressBarTask        *importTask = NULL;
    int                 progressCnt = 0;
    if (showProgress)
    {
		float length = float((gridRight - gridLeft) * (gridTop - gridBottom));
        importTask  = 
            new ProgressBarTask
            (
                LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/IMPORT/ELEVATION_BLIT/SAVE_TERRAIN_DATA"), 
                length, false
            );
    }

   
	while (bcw.nextTile(chunkName, gridX, gridZ))
	{
		if (importTask != NULL)
			importTask->step();

        // The coordinates of this chunk in the giant bitmap system:
        int tileLeft, tileRight, tileBottom, tileTop;
        tilePosition
        (
            format, 
            gridX - localToWorld.x, 
            gridZ - localToWorld.y, 
            tileLeft, 
            tileTop, 
            tileRight, 
            tileBottom
        );  

        // Only process chunks that intersect the selection:
        int ileft, itop, iright, ibottom;
        if 
        (
            TerrainUtils::rectanglesIntersect
            (
                tileLeft, tileTop, tileRight, tileBottom,
                left    , top    , right    , bottom    ,
                ileft   , itop   , iright   , ibottom   
            )
        )
        {
            modifiedChunksPos.push_back(ElevationUndoPos(gridX, gridZ));
        }
    }

    bw_safe_delete(importTask);

    if (modifiedChunksPos.empty())
        return false;

    UndoRedo::instance().add(new ElevationUndo(modifiedChunksPos));
    UndoRedo::instance().barrier(description, false);

    return true;
}


/**
 *  This function does a terrain import.  The coordinates do not need be 
 *  clipped or normalised (left < right etc).
 *
 *  @param image        The image to import.
 *  @param leftf        The left coordinate of the import area.
 *  @param topf         The top coordinate of the import area.
 *  @param rightf       The right coordinate of the import area.
 *  @param bottomf      The bottom coordinate of the import area. 
 *  @param mode         The import mode (replace, additive etc).
 *  @param min          The minimum height of the import area.
 *  @param max          The maximum height of the import area.
 *  @param alpha        The strength (0..1) of the import.  0.0 leaves the
 *                      area intact, 1.0 is a full import.
 *  @param showProgress If true then show progress while importing.
 *  @param forceToMemory Force the result to memory.
 */
bool ElevationBlit::import
(
    ImportImage			const &image,
    float               leftf,
    float               topf,
    float               rightf,
    float               bottomf,
    Mode                mode,
    float               minv,
    float               maxv,
    float               alpha,
    bool                showProgress,
    bool                forceToMemory
)
{
	BW_GUARD;

    // Note that the code below uses double precision arithmetic.  This is
    // because floating point sometimes gives results that are innacurate
    // enough to be visible as very tiny cracks between chunks.

    if (minv > maxv) 
        std::swap(minv, maxv);

    double offset = 0.0;
    double scale  = 0.0;
    switch (mode)
    {
    case ElevationBlit::ADDITIVE:      // fall through deliberate
    case ElevationBlit::SUBTRACTIVE:   
        offset = 0.0;
        scale  = alpha*(maxv - minv);
        break;
    case ElevationBlit::OVERLAY:
        offset = -0.5*alpha*(maxv - minv);
        scale  = alpha*(maxv - minv);
        break;
    case ElevationBlit::MAX:           // fall through deliberate
    case ElevationBlit::MIN:           // fall through deliberate
    case ElevationBlit::REPLACE:
        offset = minv;
        scale  = alpha*(maxv - minv);
        break;
    }

    SyncMode modifyTerrain;
    if (!modifyTerrain)
        return false;

    BW::string space = WorldManager::instance().getCurrentSpace();
	float gridSize;
	uint16 gridWidth, gridHeight;
	GridCoord localToWorld(0, 0);
    if (!TerrainUtils::spaceSettings(space, gridSize, gridWidth, gridHeight, localToWorld))
        return false;

	// Retrieve destination terrain format, by opening an existing
	// terrain file and reading its header:
	TerrainUtils::TerrainFormat format = WorldManager::instance().getTerrainInfo();

    // Pretend the terrain is one giant bitmap of size 
    // widthPixels x heightPixels:
    unsigned int widthPixels  = gridWidth *format.blockWidth;
    unsigned int heightPixels = gridHeight*format.blockHeight;

	// Normalise the coordinates:
	if (leftf > rightf ) std::swap(leftf, rightf );
	if (topf  < bottomf) std::swap(topf , bottomf);

    // Convert the selection into the giant bitmap coordinates:
    int left   = (int)(leftf  *(widthPixels  - 1)/gridWidth );
    int top    = (int)(topf   *(heightPixels - 1)/gridHeight);
    int right  = (int)(rightf *(widthPixels  - 1)/gridWidth );
    int bottom = (int)(bottomf*(heightPixels - 1)/gridHeight);

    // Handle some degenerate cases:
    if (left == right || top == bottom)
        return false;

	// Go one past the edges:
	++top; 
	++right;
	--left; 
	--bottom;

    // Find the grid boundaries + 1 past the edge
	int16 gridLeft = (int16)  floor( leftf-1 );
	int16 gridRight = (int16) ceil ( rightf+1 );
	int16 gridTop = (int16) ceil( topf+1 );
	int16 gridBottom = (int16) floor( bottomf-1 );

	BoundedChunkWalker bcw( gridLeft, gridBottom, gridRight, gridTop);
	bcw.spaceInformation(SpaceInformation(space, localToWorld, gridWidth, gridHeight));
	ProgressBarTask        *importTask = NULL;
    if (showProgress)
    {
		float length = float((gridRight - gridLeft) * (gridTop - gridBottom));
        importTask  = 
            new ProgressBarTask
            (
                LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/IMPORT/ELEVATION_BLIT/IMPORT_TERRAIN_DATA"),
				length, true
            );
    }

	BW::string chunkName;
	int16 gridX, gridZ;

	while (bcw.nextTile(chunkName, gridX, gridZ) )
	{
		if (importTask != NULL)
		{
			if ( importTask->isCancelled() )
			{
				break;
			}
			importTask->step();
		}

        // The coordinates of this chunk in the giant bitmap system:
        int tileLeft, tileRight, tileBottom, tileTop;
        tilePosition
        (
            format, 
            gridX - localToWorld.x, 
            gridZ - localToWorld.y, 
            tileLeft, 
            tileTop, 
            tileRight, 
            tileBottom
        );   

        // Only process chunks that intersect the selection:
        int ileft, itop, iright, ibottom;
        if 
        (
            TerrainUtils::rectanglesIntersect
            (
                tileLeft, tileTop, tileRight, tileBottom,
                left    , top    , right    , bottom    ,
                ileft   , itop   , iright   , ibottom   
            )
        )
        {
            if (TerrainUtils::isEditable(gridX, gridZ))
            {
                Terrain::TerrainHeightMap::ImageType terrainImage;
                TerrainUtils::TerrainGetInfo getInfo;
                TerrainUtils::getTerrain
                (
                    gridX, gridZ,
                    terrainImage,
                    forceToMemory,
                    &getInfo
                );

			    if (!terrainImage.isEmpty())
                {
                    // Destination coordinates within this chunk:
                    int dleft   = Math::lerp(ileft  , tileLeft  , tileRight, 0, (int)format.polesWidth  - 1);
                    int dtop    = Math::lerp(itop   , tileBottom, tileTop  , 0, (int)format.polesHeight - 1);
                    int dright  = Math::lerp(iright , tileLeft  , tileRight, 0, (int)format.polesWidth  - 1);
                    int dbottom = Math::lerp(ibottom, tileBottom, tileTop  , 0, (int)format.polesHeight - 1);                 

                    // Source coordinates within the imported image:
                    double sleft   = Math::lerp(ileft  , left, right , 0.0, (float)image.width () - 1.0);
                    double stop    = Math::lerp(itop   , top , bottom, 0.0, (float)image.height() - 1.0);
                    double sright  = Math::lerp(iright , left, right , 0.0, (float)image.width () - 1.0);
                    double sbottom = Math::lerp(ibottom, top , bottom, 0.0, (float)image.height() - 1.0);

					double roundUp    = 1.0/IMPORT_SNAP;
					double invRoundUp = IMPORT_SNAP;

                    // Blit the terrain onto this chunk using bicubic 
                    // interpolation.  dx, dy iterate over the chunk,
                    // sx, sy are interpolated coordinates within the imported
                    // image.
                    for (int dy = dbottom; dy <= dtop; ++dy)
                    {
                        float *dst = terrainImage.getRow(dy) + dleft;
                        double sy   = Math::safeLerp((double)dy, (double)dbottom, (double)dtop, (double)sbottom, (double)stop);
                        for (int dx = dleft; dx <= dright; ++dx, ++dst)
                        {
                            double sx		= Math::safeLerp((double)dx, (double)dleft, (double)dright, (double)sleft, (double)sright);
                            double rawVal	= image.getBicubic(sx, sy);
							double imgVal	= image.toHeight(rawVal)*scale + offset;
							double terVal   = *dst;
                            switch (mode)
                            {
                            case ElevationBlit::OVERLAY: // fall thru deliberate
                            case ElevationBlit::ADDITIVE:
                                terVal += imgVal;
                                break;
                            case ElevationBlit::SUBTRACTIVE:
                                terVal -= imgVal;
                                break;                            
                            case ElevationBlit::MIN:
                                terVal = std::min(terVal, imgVal);
                                break;
                            case ElevationBlit::MAX:
                                terVal = std::max(terVal, imgVal);
                                break;
                            case ElevationBlit::REPLACE:
                                terVal = imgVal;
                                break;
                            }
							terVal = (int)(terVal*roundUp + 0.5)*invRoundUp;
							*dst = Math::clamp( TerrainUtils::MIN_TERRAIN_HEIGHT,
								(float)terVal, TerrainUtils::MAX_TERRAIN_HEIGHT );
                        }
                    }
					TerrainUtils::patchEdges
					(
						gridX, gridZ,
						gridWidth, gridHeight,
						getInfo,
						terrainImage
					);
                    TerrainUtils::setTerrain
                    (
                        gridX, gridZ, 
                        terrainImage, 
                        getInfo,
                        !forceToMemory
                    );
                }
            }
        }
	}

    bw_safe_delete(importTask);

    return true;
}


/**
 *  This function does a terrain export.  The coordinates do not need be 
 *  clipped or normalised (left < right etc).
 *
 *  @param image        The exported area.  The image is resized as 
 *                      appropriate.
 *  @param leftf        The left coordinate of the export area.
 *  @param topf         The top coordinate of the export area.
 *  @param rightf       The right coordinate of the export area.
 *  @param bottomf      The bottom coordinate of the export area. 
 *	@param minHeight	The minimum height of the exported area.
 *	@param maxHeight	The maximum height of the exported area.
 *	@param allHeightsOk	This is set to true if all of the exported heights
 *						fitted into the minHeight, maxHeight range.
 *  @param showProgress Show a progress indicator.
 */
bool ElevationBlit::exportTo
(
    ImportImage		    &image,
    float               leftf,
    float               topf,
    float               rightf,
    float               bottomf,
	float				minHeight,
	float				maxHeight,
	bool				&allHeightsOk,
    bool                showProgress
)
{
	BW_GUARD;

    SyncMode chunkStopper;

	BW::string space = WorldManager::instance().getCurrentSpace();
	// Get size and format of terrain
	float gridSize;
	uint16 gridWidth, gridHeight;	
	GridCoord localToWorld(0, 0);
    if (!TerrainUtils::spaceSettings(space, gridSize, gridWidth, gridHeight, localToWorld))
        return false;

	//retrieve terrain format, by opening an existing
	//terrain file and reading its header.
    TerrainUtils::TerrainFormat format = WorldManager::instance().getTerrainInfo();

    // Pretend the terrain is one giant bitmap of size 
    // widthPixels x heightPixels:
    unsigned int widthPixels  = gridWidth *format.blockWidth;
    unsigned int heightPixels = gridHeight*format.blockHeight;
	
	// Normalise the coordinates:
    if (leftf > rightf ) std::swap(leftf, rightf );
    if (topf  < bottomf) std::swap(topf , bottomf);

    // Convert the selection into the giant bitmap coordinates:
    int left   = (int)(leftf  *(widthPixels  - 1)/gridWidth );
    int top    = (int)(topf   *(heightPixels - 1)/gridHeight);
    int right  = (int)(rightf *(widthPixels  - 1)/gridWidth );
    int bottom = (int)(bottomf*(heightPixels - 1)/gridHeight);   
	
	// We should clip left, right against the size of the space.

	// Find the grid boundaries
	int16 gridLeft = (int16)  floor( leftf );
	int16 gridRight = (int16) ceil ( rightf );
	int16 gridTop = (int16) ceil( topf );
	int16 gridBottom = (int16) floor( bottomf );
    // Clip the selected area against the giant bitmap coords:
    if 
    (
        !TerrainUtils::rectanglesIntersect
        (
            0   , heightPixels, widthPixels, 0,
            left, top         , right      , bottom,
            left, top         , right      , bottom
        )
    )
    {
        return false; // outside of range!
    }

    // Handle some degenerate cases:
    if (left == right || top == bottom)
        return false;

	// create storage for it
    int dwidth  = right - left   + 1;
    int dheight = top   - bottom + 1;
    image.resize(dwidth, dheight, 0);

	// All of the heights are ok so far:
	allHeightsOk = true;

	BoundedChunkWalker bcw( gridLeft, gridBottom, gridRight, gridTop);
	bcw.spaceInformation(SpaceInformation(space, localToWorld, gridWidth, gridHeight));

	ProgressBarTask        *importTask = NULL;
    int                 progressCnt = 0;
    if (showProgress)
    {
		float length = float((gridRight - gridLeft) * (gridTop - gridBottom));
        importTask  = 
            new ProgressBarTask
            (
                LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/IMPORT/ELEVATION_BLIT/EXPORT_TERRAIN_DATA"), 
				length, false
            );
    }    

	BW::string chunkName;
	int16 gridX, gridZ;   

	while (bcw.nextTile(chunkName, gridX, gridZ))
	{
		if (importTask != NULL)
			importTask->step();

        // The coordinates of this chunk in the giant bitmap system:
        int tileLeft, tileRight, tileBottom, tileTop;
        tilePosition
        (
            format, 
            gridX - localToWorld.x, 
            gridZ - localToWorld.y, 
            tileLeft, 
            tileTop, 
            tileRight, 
            tileBottom
        );     

        // Only process chunks that intersect the selection:
        int ileft, itop, iright, ibottom;
        if 
        (
            TerrainUtils::rectanglesIntersect
            (
                tileLeft, tileTop, tileRight, tileBottom,
                left    , top    , right    , bottom    ,
                ileft   , itop   , iright   , ibottom   
            )
        )
        {
            if (TerrainUtils::isEditable(gridX, gridZ))
            {
                Terrain::TerrainHeightMap::ImageType terrainImage;
                TerrainUtils::getTerrain
                (
                    gridX, gridZ,
                    terrainImage,
                    false
                );                

			    if (!terrainImage.isEmpty())
                {
                    // Source coordinates within this chunk:
                    int sleft   = Math::lerp(ileft  , tileLeft  , tileRight, 0, (int)format.polesWidth  - 1);
                    int stop    = Math::lerp(itop   , tileBottom, tileTop  , 0, (int)format.polesHeight - 1);
                    int sright  = Math::lerp(iright , tileLeft  , tileRight, 0, (int)format.polesWidth  - 1);
                    int sbottom = Math::lerp(ibottom, tileBottom, tileTop  , 0, (int)format.polesHeight - 1);                 

                    // Destination coordinates within the exported image:
                    int dleft   = Math::lerp(ileft  , left  , right , 0, dwidth  - 1);
                    int dtop    = Math::lerp(itop   , bottom, top   , 0, dheight - 1);
                    int dright  = Math::lerp(iright , left  , right , 0, dwidth  - 1);
                    int dbottom = Math::lerp(ibottom, bottom, top   , 0, dheight - 1);

                    int dy = dbottom;
                    int sy = sbottom;
                    for (; sy <= stop; ++sy, ++dy)
                    {
                        float const *src = terrainImage.getRow(sy) + sleft;
                        uint16 *dst      = image.getRow(dy) + dleft;
                        int   sx         = sleft;
                        int   dx         = dleft;
                        for (; sx <= sright; ++sx, ++dx, ++src, ++dst)
                        {
                            float srcv = *src;
							if (srcv < minHeight)
							{
								srcv = minHeight;
								allHeightsOk = false;
							}
							else if (srcv > maxHeight)
							{
								srcv = maxHeight;
								allHeightsOk = false;
							}
							*dst = 
								(uint16)Math::lerp
								(
									srcv, 
									minHeight, 
									maxHeight, 
									(uint16)0, 
									std::numeric_limits<uint16>::max()
								);
                        }
                    }
		        }
            }
	    }
    }

	// Set the scale of the image based on the min/max heights found.
	image.setScale(minHeight, maxHeight);

    bw_safe_delete(importTask);

    return true;
}


/**
 *  This function determines the range of heights in the given area.
 *
 *  @param leftf        The left coordinate of the export area.
 *  @param topf         The top coordinate of the export area.
 *  @param rightf       The right coordinate of the export area.
 *	@param minHeight	Set to the minimum height of the exported area.
 *	@param maxHeight	Set to the maximum height of the exported area.
 *	@param showProgress	If true then show progress as the heights are 
 *						calculated.
 *	@returns			True if the max/min heights could be found.
 */
bool ElevationBlit::heightRange
(
	float               leftf,
    float               topf,
    float               rightf,
    float               bottomf,
	float				&minHeight,
	float				&maxHeight,
	bool                showProgress
)
{
	BW_GUARD;

	minHeight = +std::numeric_limits<float>::max();
	maxHeight = -std::numeric_limits<float>::max();

    SyncMode chunkStopper;

	BW::string space = WorldManager::instance().getCurrentSpace();
	// Get size and format of terrain
	float gridSize;
	uint16 gridWidth, gridHeight;	
	GridCoord localToWorld(0, 0);
    if (!TerrainUtils::spaceSettings(space, gridSize, gridWidth, gridHeight, localToWorld))
        return false;

	//retrieve terrain format, by opening an existing
	//terrain file and reading its header.
    TerrainUtils::TerrainFormat format = WorldManager::instance().getTerrainInfo();

    // Pretend the terrain is one giant bitmap of size 
    // widthPixels x heightPixels:
    unsigned int widthPixels  = gridWidth *format.blockWidth;
    unsigned int heightPixels = gridHeight*format.blockHeight;
	// Normalise the coordinates:
    if (leftf > rightf ) std::swap(leftf, rightf );
    if (topf  < bottomf) std::swap(topf , bottomf);

    // Convert the selection into the giant bitmap coordinates:
    int left   = (int)(leftf  *(widthPixels  - 1)/gridWidth );
    int top    = (int)(topf   *(heightPixels - 1)/gridHeight);
    int right  = (int)(rightf *(widthPixels  - 1)/gridWidth );
    int bottom = (int)(bottomf*(heightPixels - 1)/gridHeight);

	// Find the grid boundaries
	int16 gridLeft = (int16) floor( leftf );
	int16 gridRight = (int16) ceil ( rightf );
	int16 gridTop = (int16) ceil( topf );
	int16 gridBottom = (int16) floor( bottomf );
	// Convert the selection into Grid Coordinates
    // Clip the selected area against the giant bitmap coords:
    if 
    (
        !TerrainUtils::rectanglesIntersect
        (
            0   , heightPixels, widthPixels, 0,
            left, top         , right      , bottom,
            left, top         , right      , bottom
        )
    )
    {
        return false; // outside of range!
    }

    // Handle some degenerate cases:
    if (left == right || top == bottom)
        return false;

	BoundedChunkWalker bcw( gridLeft, gridBottom, gridRight, gridTop);
	bcw.spaceInformation(SpaceInformation(space, localToWorld, gridWidth, gridHeight));

	ProgressBarTask        *importTask = NULL;
    int                 progressCnt = 0;
    if (showProgress)
    {
		float length = float((gridRight - gridLeft) * (gridTop - gridBottom));
        importTask  = 
            new ProgressBarTask
            ( 
                LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/IMPORT/ELEVATION_BLIT/EXPORT_CALC_HEIGHTS"), 
				length, false
            );
    }    

	BW::string chunkName;
	int16 gridX, gridZ;   

	while (bcw.nextTile(chunkName, gridX, gridZ))
	{
		if (importTask != NULL)
			importTask->step();

        // The coordinates of this chunk in the giant bitmap system:
        int tileLeft, tileRight, tileBottom, tileTop;
        tilePosition
        (
            format, 
            gridX - localToWorld.x, 
            gridZ - localToWorld.y, 
            tileLeft, 
            tileTop, 
            tileRight, 
            tileBottom
        );     

        // Only process chunks that intersect the selection:
        int ileft, itop, iright, ibottom;
        if 
        (
            TerrainUtils::rectanglesIntersect
            (
                tileLeft, tileTop, tileRight, tileBottom,
                left    , top    , right    , bottom    ,
                ileft   , itop   , iright   , ibottom   
            )
        )
        {
            if (TerrainUtils::isEditable(gridX, gridZ))
            {
                Terrain::TerrainHeightMap::ImageType terrainImage;
                TerrainUtils::getTerrain
                (
                    gridX, gridZ,
                    terrainImage,
                    false
                );                

			    if (!terrainImage.isEmpty())
                {
                    // Source coordinates within this chunk:
                    int sleft   = Math::lerp(ileft  , tileLeft  , tileRight, 0, (int)format.polesWidth  - 1);
                    int stop    = Math::lerp(itop   , tileBottom, tileTop  , 0, (int)format.polesHeight - 1);
                    int sright  = Math::lerp(iright , tileLeft  , tileRight, 0, (int)format.polesWidth  - 1);
                    int sbottom = Math::lerp(ibottom, tileBottom, tileTop  , 0, (int)format.polesHeight - 1);                 
                   
                    for (int sy = sbottom; sy <= stop; ++sy)
                    {
                        float const *src  = terrainImage.getRow(sy) + sleft;
						float const *send = src + (sright - sleft + 1);
                        for (; src != send; ++src)
                        {
							minHeight = std::min(minHeight, *src);
							maxHeight = std::max(maxHeight, *src);
                        }
                    }
		        }
            }
	    }
    }

    bw_safe_delete(importTask);

	return true;
}

BW_END_NAMESPACE

