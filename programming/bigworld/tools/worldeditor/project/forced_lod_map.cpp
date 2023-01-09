#include "pch.hpp"
#include "common/bwlockd_connection.hpp"
#include "worldeditor/project/forced_lod_map.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "moo/render_context.hpp"
#include "math/colour.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/string_builder.hpp"
#include "moo/texture_lock_wrapper.hpp"
#include "chunk_walker.hpp"
#include "chunk/geometry_mapping.hpp"
#include "terrain/terrain2/terrain_block2.hpp"
#include "misc/progress_bar_helper.hpp"

namespace
{
	const int UNFORCED_LOD = -1;
	const int MAX_FORCED_LOD_WE_CARE_ABOUT = 9;
	const int INVALID_LOD = static_cast<int>(1u << 31); // Since -1 and 0 are valid Forced LOD values

	const BW::uint32 FORCED_LOD_COLOURS[] = {
		BW::Colour::getUint32(   0,   0, 255, 128),
		BW::Colour::getUint32(   0, 128, 255, 128),
		BW::Colour::getUint32(   0, 255, 255, 128),
		BW::Colour::getUint32(   0, 255, 128, 128),
		BW::Colour::getUint32(   0, 255,   0, 128),
		BW::Colour::getUint32( 128, 255,   0, 128),
		BW::Colour::getUint32( 255, 255,   0, 128),
		BW::Colour::getUint32( 255, 128,   0, 128),
		BW::Colour::getUint32( 255,   0,   0, 128),
		BW::Colour::getUint32( 128,   0,   0, 128)
	};

	const BW::uint32 UNFORCED_LOD_COLOUR = BW::Colour::getUint32(   0,   0,   0,   0);
}

DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

inline BW::string errorAsString(HRESULT hr)
{
	char s[1024];
	//TODO : find out the DX9 way of getting the error string
	//D3DXGetErrorString( hr, s, 1000 );
	bw_snprintf( s, sizeof(s), "Error %lx\0", hr );
	return BW::string( s );
}

BW_BEGIN_NAMESPACE


ForcedLODMap::ForcedLODMap() :
	gridWidth_(0),
	gridHeight_(0),
	colourUnlocked_(UNFORCED_LOD_COLOUR),
	requiresFileLoad_(true)
{
	std::copy_n(FORCED_LOD_COLOURS, ARRAY_SIZE(colourLODLockLevel_), colourLODLockLevel_);
}


ForcedLODMap::~ForcedLODMap()
{
	BW_GUARD;

	releaseLockTexture();
}


void ForcedLODMap::setTexture(uint8 textureStage)
{
	BW_GUARD;

	Moo::rc().setTexture(textureStage, lockTexture_.pComObject());
}


void ForcedLODMap::setSpaceInformation(const SpaceInformation& info)
{
	BW_GUARD;

	spaceInformation_ = info;
	setGridSize(info.gridWidth_, info.gridHeight_);
}

void ForcedLODMap::setGridSize(uint32 width, uint32 height)
{
	BW_GUARD;

	if (width == gridWidth_ && height == gridHeight_)
	{
		return;
	}

	releaseLockTexture();

	gridWidth_ = width;
	gridHeight_ = height;

	clearLODData();

	createLockTexture();
}

void ForcedLODMap::updateTexture()
{
	BW_GUARD;

	if (requiresFileLoad_)
	{
		loadLODDataFromFiles();
	}

	D3DLOCKED_RECT lockedRect;
	Moo::TextureLockWrapper textureLock(lockTexture_);
	HRESULT hr = textureLock.lockRect(0, &lockedRect, NULL, 0);
	if (FAILED(hr))
	{
		ERROR_MSG( "ForcedLODMap::updateLockData - unable to lock texture  D3D error %s\n", errorAsString(hr).c_str() );
		return;
	}
	
	uint32* pixels = static_cast<uint32*>(lockedRect.pBits);
	uint32* pixelPtr = pixels;

	BW::vector<int>::const_iterator fileIter = fileLodValues_.cbegin();
	BW::vector<int>::const_iterator dynamicIter = dynamicLodValues_.cbegin();

	// Merge the file LOD values with the dynamically updated LOD values and 
	// render the result to the overlay texture image
	for (uint32 i = 0; i < gridWidth_ * gridHeight_; ++i)
	{
		int forcedLod = *fileIter;

		const int dynamicLod = *dynamicIter;
		if (dynamicLod != INVALID_LOD)
		{
			forcedLod = dynamicLod;
		}

		if (forcedLod == UNFORCED_LOD)
		{
			*pixelPtr = colourUnlocked_;
		}
		else
		{
			forcedLod = min(forcedLod, MAX_FORCED_LOD_WE_CARE_ABOUT);
			*pixelPtr = colourLODLockLevel_[forcedLod];
		}

		++pixelPtr;
		++fileIter;
		++dynamicIter;
	}

	hr = textureLock.unlockRect(0);
	if (FAILED(hr))
	{
		ERROR_MSG( "ForcedLODMap::updateLockData - unable to unlock texture.  D3D error %s\n", errorAsString(hr).c_str() );
	}
}

void ForcedLODMap::updateLODElement(int16 offsetX, int16 offsetY, int forcedLod)
{
	BW_GUARD;

	uint16 biasX, biasY;
	BW::biasGrid(spaceInformation_.localToWorld_, offsetX, offsetY, biasX, biasY);

	dynamicLodValues_[biasY * gridWidth_ + biasX] = forcedLod;
}

void ForcedLODMap::clearLODData()
{
	BW_GUARD;

	const uint32 elementCount = gridWidth_ * gridHeight_;
	fileLodValues_ = BW::vector<int>(elementCount, UNFORCED_LOD);
	dynamicLodValues_ = BW::vector<int>(elementCount, INVALID_LOD);

	requiresFileLoad_ = true;
}

void ForcedLODMap::loadLODDataFromFiles()
{
	BW_GUARD;

	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	BW::string pathName = dirMap->path();

	LinearChunkWalker chunkWalker;
	chunkWalker.spaceInformation(spaceInformation_);

	BW::vector<int>::iterator fileIter = fileLodValues_.begin();

	ProgressBarTask progressTask(
		LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/LOAD_ALL_CHUNK_LODS"), 
		static_cast<float>(fileLodValues_.size()), true);

	BW::string chunkName;
	int16 gridX, gridZ;
	while (chunkWalker.nextTile(chunkName, gridX, gridZ))
	{
		BW::string fileName = pathName + chunkName;
		fileName += ".cdata/terrain2";
		DataSectionPtr section = BWResource::openSection(fileName);
		if (section == NULL)
		{
			ERROR_MSG( "ForcedLODMap::loadLODDataFromFiles - unable to open section %s\n", fileName.c_str());
			break;
		}

		int forcedLod = UNFORCED_LOD;
		DataSectionPtr metadataSection = 
			section->openSection(BW::Terrain::TerrainBlock2::TERRAIN_BLOCK_META_DATA_SECTION_NAME);
		if (metadataSection != NULL)
		{
			forcedLod = metadataSection->readInt(Terrain::TerrainBlock2::FORCED_LOD_ATTRIBUTE_NAME, UNFORCED_LOD);
		}

		*fileIter++ = forcedLod;

		progressTask.step();
	}

	requiresFileLoad_ = false;
}


void ForcedLODMap::releaseLockTexture()
{
	BW_GUARD;

	lockTexture_ = NULL;
}


void ForcedLODMap::createLockTexture()
{
	BW_GUARD;

	if (gridWidth_ == 0 || gridHeight_ == 0)
	{
		ERROR_MSG( "ForcedLODMap: Invalid dimensions: %d x %d\n", gridWidth_, gridHeight_ );
		return;
	}

	lockTexture_ = NULL; // Release any lock texture

	lockTexture_ = Moo::rc().createTexture(gridWidth_, gridHeight_ , 1,
		0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, "texture/lock terrain");
}

BW_END_NAMESPACE
