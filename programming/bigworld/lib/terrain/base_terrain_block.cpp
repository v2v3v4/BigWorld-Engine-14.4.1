#include "pch.hpp"
#include "cstdmf/debug.hpp"
#include "base_terrain_block.hpp"
#include "terrain2/terrain_block2.hpp"
#if defined( EDITOR_ENABLED )
#include "terrain2/editor_terrain_block2.hpp"
#endif // defined( EDITOR_ENABLED )
#include "terrain_height_map.hpp"
#include "resmgr/bwresource.hpp"

DECLARE_DEBUG_COMPONENT2("Terrain", 0)


BW_BEGIN_NAMESPACE

using namespace Terrain;


/*static*/ float          BaseTerrainBlock::NO_TERRAIN     = -1000000.f;
/*static*/ TerrainFinder *BaseTerrainBlock::terrainFinder_ = NULL;
/*static*/ bool			  BaseTerrainBlock::s_disableStreaming_ = false;


/*static*/
uint32 BaseTerrainBlock::terrainVersion( BW::string& resource )
{
	BW_GUARD;
	if ( resource.substr( resource.length() - 8 ) == "/terrain" &&
		BWResource::openSection( resource + '2' ) != NULL )
	{
		// it's got a new terrain section, so treat it first as a new terrain,
		// even if a plan '/terrain' was requested. Also, have to add the '2'
		// at the end of the resource name.
		resource += '2';
		return TerrainSettings::TERRAIN2_VERSION;
	}
	else if ( resource.substr( resource.length() - 8 ) == "/terrain" &&
		BWResource::openSection( resource ) != NULL )
	{
		return TerrainSettings::TERRAIN1_VERSION;
	}
	else if ( resource.substr( resource.length() - 9 ) == "/terrain2" &&
		BWResource::openSection( resource ) != NULL )
	{
		return TerrainSettings::TERRAIN2_VERSION;
	}

	return 0;
}


BaseTerrainBlockPtr BaseTerrainBlock::loadBlock( const BW::string& filename, 
	const Matrix& worldTransform, const Vector3& cameraPosition, 
	TerrainSettingsPtr pSettings, BW::string* error /* = NULL*/ )
{
	BW_GUARD;	
#ifdef EDITOR_ENABLED
	EditorBaseTerrainBlockPtr result;
#else
	BaseTerrainBlockPtr result;
#endif // EDITOR_ENABLED
	BW::string verResource = filename;
	uint32 ver = BaseTerrainBlock::terrainVersion( verResource );
	if (ver < TerrainSettings::TERRAIN2_VERSION)
	{
		char msg[80];
		bw_snprintf( msg, sizeof( msg ),
			"BaseTerrainBlock::loadBlock: "
			"Terrain version %d is no longer supported", ver );

		if (error)
		{
			*error = msg;
		}
		else
		{
			ERROR_MSG( "%s\n", msg );
		}
		return NULL;
	}
	else if (ver == TerrainSettings::TERRAIN2_VERSION)
	{
		result = new TERRAINBLOCK2(pSettings);
	}
	else
	{
		const BW::string msg = "BaseTerrainBlock::loadBlock: "
			"Unknown terrain version for block " + filename;
		if (error)
		{
			*error = msg;
		}
		else
		{
			ERROR_MSG( "%s\n", msg.c_str() );
		}
		return NULL;
	}

#if defined(MF_SERVER) || defined(EDITOR_ENABLED)
	// TODO:
	result->resourceName( verResource );
#endif

	bool ok = result->load(verResource, worldTransform, cameraPosition, error);
    if (!ok)
    {
		ERROR_MSG( "BaseTerrainBlock::loadBlock: "
			"Failed to load from %s\n", verResource.c_str() );
        result = NULL;
    }
    return result;
}


BaseTerrainBlock::BaseTerrainBlock( float blockSize ) :
	blockSize_( blockSize )
{
}


/*virtual*/ BaseTerrainBlock::~BaseTerrainBlock()
{
}


/*static*/ TerrainFinder::Details
BaseTerrainBlock::findOutsideBlock(Vector3 const &pos)
{
    BW_GUARD;
	if (terrainFinder_ == NULL)
        return TerrainFinder::Details();
    else
        return terrainFinder_->findOutsideBlock(pos);
}


/*static*/ float BaseTerrainBlock::getHeight(float x, float z)
{
	BW_GUARD;
	Vector3 pos(x, 0.0f, z);
	TerrainFinder::Details findDetails =
        BaseTerrainBlock::findOutsideBlock(pos);

    BaseTerrainBlock *block = findDetails.pBlock_;
	if (block != NULL)
	{
		Vector3 terPos = findDetails.pInvMatrix_->applyPoint(pos);
		float height = block->heightAt(terPos[0], terPos[2]);
		if (!isEqual( height, NO_TERRAIN ))
		{
			return height - findDetails.pInvMatrix_->applyToOrigin()[1];
		}
	}

	return NO_TERRAIN;
}


/*static*/ void BaseTerrainBlock::setTerrainFinder(TerrainFinder & terrainFinder)
{
    terrainFinder_ = &terrainFinder;
}


#ifdef MF_SERVER

const BW::string & BaseTerrainBlock::resourceName() const
{
	return resourceName_;
}


void BaseTerrainBlock::resourceName( const BW::string & name )
{
	resourceName_ = name;
}

#endif // MF_SERVER

BW_END_NAMESPACE

// base_terrain_block.cpp
