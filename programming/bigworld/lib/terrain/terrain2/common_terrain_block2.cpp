#include "pch.hpp"
#include "common_terrain_block2.hpp"

#include "../terrain_settings.hpp"
#include "terrain_height_map2.hpp"
#include "terrain_hole_map2.hpp"
#include "terrain_texture_layer2.hpp"
#include "dominant_texture_map2.hpp"

#include "cstdmf/debug.hpp"
#include "resmgr/bwresource.hpp"
#include "chunk/chunk_grid_size.hpp"


BW_BEGIN_NAMESPACE

namespace
{
	const BW::string BASE_HEIGHT_SECTION_NAME = "heights";
}

namespace Terrain
{

/**
 *	Constructor.
 */
CommonTerrainBlock2::CommonTerrainBlock2(TerrainSettingsPtr pSettings) :
	CommonTerrainBlock2Base( pSettings == NULL ? DEFAULT_GRID_RESOLUTION : pSettings->blockSize() ),
	settings_(pSettings),
    bb_( BoundingBox::s_insideOut_ )
{
}

/**
 *	Destructor.
 */
CommonTerrainBlock2::~CommonTerrainBlock2()
{
	// Note, we explicitly dereference pointers so ensuing destruction can be
	// profiled.
	PROFILER_SCOPED( CommonTerrainBlock2_destruct );

	pHeightMap_				= NULL;
	pHoleMap_				= NULL;
	pDominantTextureMap_	= NULL;
}


/**
 *  This function loads an BaseTerrainBlock from a DataSection.
 *
 *  @param filename         A file to load the terrain block from.
 *  @param worldTransform   The transform matrix to position the terrain block
 *  						in world co-ordinates.
 *	@param cameraPosition	The position of the camera (to work out LoD level).
 *  @param error            If not null and there was an error loading
 *                          the terrain then this will be set to a
 *                          description of the problem.
 *  @returns                True if the load was completely successful,
 *                          false if a problem occured.
 */
/*virtual*/ bool
CommonTerrainBlock2::load(BW::string const &filename,
					Matrix  const &worldTransform,
					Vector3 const &cameraPosition,
					BW::string *error/* = NULL*/)
{
	BW_GUARD;
	PROFILE_FILE_SCOPED( CommonTerrainBlock2_load );
	DataSectionPtr pTerrain = BWResource::openSection( filename );
	if ( !pTerrain )
	{
		if ( error ) *error = "Can't open terrain section.";
		return false;
	}

	return internalLoad(
		filename, pTerrain, worldTransform, cameraPosition, 0, error );
}


/**
 *  This function does the actual loading of the block
 *
 *  @param filename         A file to load the terrain block from.
 *  @param terrainDS        DataSection corresponding to the file 'filename'.
 *  @param worldTransform   The transform matrix to position the terrain block
 *  						in world co-ordinates.
 *	@param cameraPosition	The position of the camera (to work out LoD level).
 *	@param heightMapLod     Index of the height map LoD to load.
 *  @param error            If not null and there was an error loading
 *                          the terrain then this will be set to a
 *                          description of the problem.
 *
 *  @returns                True if the load was completely successful,
 *                          false if a problem occurred.
 */
/*virtual*/ bool
CommonTerrainBlock2::internalLoad(BW::string const &filename,
					DataSectionPtr terrainDS,
					Matrix  const &worldTransform,
					Vector3 const &cameraPosition,
					uint32 heightMapLod,
					BW::string *error/* = NULL*/)
{
	BW_GUARD;
	// Load specified lod for height maps.
	PROFILE_FILE_SCOPED( CommonTerrainBlock2_internalLoad );
#ifdef MF_SERVER
	heightMapLod	 = settings_->serverHeightMapLod();
#endif

	BW::string		heightMapSection = TerrainHeightMap2::getHeightSectionName( 
								BASE_HEIGHT_SECTION_NAME, heightMapLod );

	DataSectionPtr pHeights = terrainDS->openSection( heightMapSection );
	if (!pHeights)
	{
		if ( error ) *error = "Can't open heights section '" + heightMapSection + "'";
		return false;
	}

	// Load height map
	TerrainHeightMap2Ptr pDHM = new TerrainHeightMap2( blockSize(), 0, heightMapLod );

	if ( !pDHM->load( pHeights, error ) )
	{
		if ( error ) *error = "Can't load height map.";
		return false;
	}
	pHeightMap_ = pDHM;

	// Load hole map
    pHoleMap_ = new TerrainHoleMap2(*this);
	DataSectionPtr pHoles = terrainDS->openSection( "holes" );
	if (pHoles)
	{
		pHoleMap_->load( pHoles );
	}
#ifdef EDITOR_ENABLED
	else
	{
		pHoleMap_->create( settings_->holeMapSize() );
	}
#endif // EDITOR_ENABLED

	if ( settings_->loadDominantTextureMaps() )
	{
		DataSectionPtr pDominantTexures = terrainDS->openSection( "dominantTextures" );
		if (pDominantTexures)
		{
			pDominantTextureMap_ = new DominantTextureMap2( blockSize() );
			BW::string dominantTextureMapError;
			if ( !pDominantTextureMap_->load( pDominantTexures, &dominantTextureMapError ) )
			{
				// doesn't matter, keep loading without the dominant texture map.
				pDominantTextureMap_ = NULL;
				WARNING_MSG( "Failed to load dominant texture map for %s: %s\n",
					filename.c_str(), dominantTextureMapError.c_str() );
			}
		}
	}

	return true;
}


/**
 *  This function gets the height map for the terrain.
 *
 *  @returns                The height map for the terrain.
 */
/*virtual*/ TerrainHeightMap &CommonTerrainBlock2::heightMap()
{
    return *pHeightMap_;
}


/**
 *  This function gets the height map for the terrain.
 *
 *  @returns                The height map for the terrain.
 */
/*virtual*/ TerrainHeightMap const &CommonTerrainBlock2::heightMap() const
{
    return *pHeightMap_;
}


/**
 *  This function gets the hole map for the terain.
 *
 *  @returns                The hole map for the terrain.
 */
/*virtual*/ TerrainHoleMap &CommonTerrainBlock2::holeMap()
{
    return *pHoleMap_;
}


/**
 *	This function gets the size of the hole map, raised to the next power of
 *  2.
 *
 *  @returns				The size of the hole map raised to the next power
 *							of 2.
 */
uint32 CommonTerrainBlock2::holesMapSize() const
{
	return pHoleMap_ != NULL ? pHoleMap_->holesMapSize() : 0;
}


/**
 *	This function gets the size (width and height) of the hole map.
 *
 *  @returns				The size of the hole map.
 */
uint32 CommonTerrainBlock2::holesSize() const
{
	return pHoleMap_ != NULL ? pHoleMap_->holesSize() : 0;
}


/**
 *  This function gets the hole map for the terrain.
 *
 *  @returns                The hole map for the terrain.
 */
/*virtual*/ TerrainHoleMap const &CommonTerrainBlock2::holeMap() const
{
    return *pHoleMap_;
}


/**
 *  This function gets the dominant texture map for the terain.
 *
 *  @returns                The dominant texture map for the terrain.
 */
DominantTextureMapPtr CommonTerrainBlock2::dominantTextureMap()
{
	return pDominantTextureMap_.getObject();
}


/**
 *  This function gets the dominant texture map for the terain.
 *
 *  @returns                The dominant texture map for the terrain.
 */
DominantTextureMapPtr const CommonTerrainBlock2::dominantTextureMap() const
{
	return pDominantTextureMap_.getObject();
}


/**
 *  This function gets the containing BoundingBox for the terrain.
 *
 *  @returns The bounding box of the CommonTerrainBlock.
 */
/*virtual*/ BoundingBox const &CommonTerrainBlock2::boundingBox() const
{
	float maxh = pHeightMap_->maxHeight();
	float minh = pHeightMap_->minHeight();

	bb_.setBounds( Vector3( 0.f, minh, 0.f ), 
		Vector3( blockSize(), maxh, blockSize() ) );

    return bb_;
}


/**
 *  This function determines whether the given line segment intersects
 *  the terrain, and if so where.
 *
 *  @param start            The start of the line segment.
 *  @param end              The end of the line segment.
 *  @param callback         The collision callback.
 *  @returns                True if there was a collision, false otherwise.
 */
/*virtual*/ bool
CommonTerrainBlock2::collide
(
    Vector3                 const &start,
    Vector3                 const &end,
    TerrainCollisionCallback *callback
) const
{
    return pHeightMap_->collide(start, end, callback);
}


/**
 *  This function determines whether the given prism intersects
 *  the terrain, and if so where.
 *
 *  @param start            The start face of the prism.
 *  @param end              The end of the line segment.
 *  @param callback         The collision callback.
 *  @returns                True if there was a collision, false otherwise.
 */
/*virtual*/ bool
CommonTerrainBlock2::collide
(
    WorldTriangle           const &start,
    Vector3                 const &end,
    TerrainCollisionCallback *callback
) const
{
    return pHeightMap_->collide(start, end, callback);
}


/**
 *  This function determines the height at the given point, taking into
 *  account holes.
 *
 *  @param x            The x coordinate to get the height at.
 *  @param z            The z coordinate to get the height at.
 *  @returns            The height at x, z.
 */
/*virtual*/ float CommonTerrainBlock2::heightAt(float x, float z) const
{
	float res = NO_TERRAIN;

	if (!holeMap().holeAt( x, z ))
	{
	    res = pHeightMap_->heightAt(x, z);
	}

	return res;
}

/**
 *  This function determines the normal at the given point, taking into
 *  account holes.
 *
 *  @param x            The x coordinate to get the normal at.
 *  @param z            The z coordinate to get the normal at.
 *  @returns            The normal at x, z.
 */
/*virtual*/ Vector3 CommonTerrainBlock2::normalAt(float x, float z) const
{
    // TODO take into account holes.
    return pHeightMap_->normalAt(x, z);
}


/**
 *	This function allows access to the height map.  This can be used by
 *	derived classes.
 *
 *  @param heightMap2			The TerrainHeightMap2.
 */
void CommonTerrainBlock2::heightMap2( TerrainHeightMap2Ptr heightMap2 )
{
	pHeightMap_ = heightMap2;
}


/**
 *	This function allows access to the height map.  This can be used by
 *	derived classes.
 *
 *  @returns			The TerrainHeightMap2.
 */
TerrainHeightMap2Ptr CommonTerrainBlock2::heightMap2() const
{
	BW_GUARD;
	return pHeightMap_;
}


/*static */void CommonTerrainBlock2::stripUnusedHeightSections(
	DataSectionPtr terrainDs, uint32 heightMapLodToPreserve )
{
	BW::string heightMapSectionToPreserve =
		 TerrainHeightMap2::getHeightSectionName(
			BASE_HEIGHT_SECTION_NAME, heightMapLodToPreserve );

	for ( int i = 0; i < terrainDs->countChildren(); )
	{
		BW::string nodeName = terrainDs->childSectionName( i );
		if(nodeName.find( BASE_HEIGHT_SECTION_NAME ) != 0 ||
			nodeName == heightMapSectionToPreserve)
		{
			++i;
			continue;
		}
		terrainDs->deleteSection( nodeName );
	}
}


/**
 *	This function allows access to the hole map.  This can be used by
 *	derived classes.
 *
 *  @param holeMap2			The TerrainHoleMap2.
 */
void CommonTerrainBlock2::holeMap2( TerrainHoleMap2Ptr holeMap2 )
{
	pHoleMap_ = holeMap2;
}


/**
 *	This function allows access to the hole map.  This can be used by
 *	derived classes.
 *
 *  @returns			The TerrainHoleMap2.
 */
TerrainHoleMap2Ptr CommonTerrainBlock2::holeMap2() const
{
	return pHoleMap_;
}


/**
 *	This function allows access to the dominant texture map.  This can be used by
 *	derived classes.
 *
 *  @returns			The DominantTextureMap.
 */
DominantTextureMap2Ptr CommonTerrainBlock2::dominantTextureMap2() const
{
	return pDominantTextureMap_;
}


/**
 *  This function allows the terrain dominant texture map to be set.
 *
 *  @param dominantTextureMap         The terrain dominant texture map to use.
 */
void CommonTerrainBlock2::dominantTextureMap2( DominantTextureMap2Ptr dominantTextureMap )
{
	pDominantTextureMap_ = dominantTextureMap;
}

} // namespace Terrain

BW_END_NAMESPACE

// common_terrain_block2.cpp
