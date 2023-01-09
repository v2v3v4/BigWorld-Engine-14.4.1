#include "pch.hpp"
#include "terrain_normal_map2.hpp"

#include "../terrain_data.hpp"
#include "terrain_height_map2.hpp"
#include "moo/png.hpp"

#include "resmgr/bin_section.hpp"
#include "moo/texture_lock_wrapper.hpp"

DECLARE_DEBUG_COMPONENT2( "Terrain", 0 );


BW_BEGIN_NAMESPACE

using namespace Terrain;

const BW::string TerrainNormalMap2::LOD_NORMALS_SECTION_NAME( "lodNormals" );
const BW::string TerrainNormalMap2::NORMALS_SECTION_NAME( "normals" );

TerrainNormalMap2::TerrainNormalMap2()
: size_ ( 0 ),
  lodSize_( 0 ),
  pTask_(NULL),
  wantQualityMap_(false)
{
	RESOURCE_COUNTER_ADD( ResourceCounters::DescriptionPool("TerrrainNormalMap2/Base",
		(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		sizeof(*this), 0 );
}

TerrainNormalMap2::~TerrainNormalMap2()
{
	RESOURCE_COUNTER_SUB( ResourceCounters::DescriptionPool("TerrrainNormalMap2/Base",
		(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		sizeof(*this), 0 );
}

/**
 *	This method inits the normal map
 *	@param terrainResource the resource name of the underlying terrain section
 *	@return true if successful
 */
bool TerrainNormalMap2::init(const BW::string& terrainResource, DataSectionPtr lodSection)
{
	BW_GUARD;
	bool res = false;
	DataSectionPtr pTerrainSection;
#ifndef EDITOR_ENABLED
	// Editor needs the parent section (pTerrainSection) so that it can load
	// the normals from it at this point.
	if (!lodSection)
#endif
	{
		pTerrainSection = BWResource::instance().openSection( terrainResource );
		if (pTerrainSection)
		{
			lodSection = pTerrainSection->openSection( LOD_NORMALS_SECTION_NAME );
		}
	}

	pLodNormalMap_ = loadMap( lodSection, lodSize_ );
	res = pLodNormalMap_.hasComObject();
	terrainResource_ = terrainResource;
#ifdef EDITOR_ENABLED
	pNormalMap_ = loadMap(pTerrainSection->openSection(
		NORMALS_SECTION_NAME ), size_ );
	res &= pNormalMap_.hasComObject();
#endif

	return res;
}

/**
 *	This static method loads a normal map from a datasection
 *	@param pMapSection the data section for the normal map
 *	@param size return value for the size of the normal map
 *	@return the normal map texture
 */
ComObjectWrap<DX::Texture> TerrainNormalMap2::loadMap( DataSectionPtr pMapSection, uint32& size )
{
	BW_GUARD;
	ComObjectWrap<DX::Texture> pNormalMap;

	BinaryPtr pData;
	if (pMapSection && (pData = pMapSection->asBinary()))
	{
		const TerrainNormalMapHeader* pHeader = 
			reinterpret_cast<const TerrainNormalMapHeader* >(pData->data());

		// Check the header for 16 bit png compression
		if (pHeader->magic_ == TerrainNormalMapHeader::MAGIC &&
			pHeader->version_ == TerrainNormalMapHeader::VERSION_16_BIT_PNG)
		{
			// The compressed image data is immediately after the header
			// in the binary block
			BinaryPtr pImageData = 
				new BinaryBlock
				( 
					pHeader + 1, 
					pData->len() - sizeof( TerrainNormalMapHeader ), 
					"Terrain/NormalMap2/BinaryBlock",
					pData 
				);

			// Decompress the image, if this succeeds, we are good to go
			Moo::Image<uint16> image;
			if(!decompressImage( pImageData, image ))
			{
				ERROR_MSG( "TerrainNormalMap2::load - "
					"the normal map data could not be compressed" );
				return NULL;
			}

#ifdef EDITOR_ENABLED
			DWORD usage = Moo::rc().usingD3DDeviceEx() ? D3DUSAGE_DYNAMIC : 0;
#else
			DWORD usage = 0;
#endif // EDITOR_ENABLED

			// Create the texture for the normal map
			pNormalMap = Moo::rc().createTexture( image.width(), image.height(), 
				1, usage, D3DFMT_A8L8, D3DPOOL_MANAGED, "Terrain/NormalMap2/Texture" );

			// Fill the normal map texture
			if (pNormalMap)
			{
				ComObjectWrap<DX::Texture> map = pNormalMap;

				D3DSURFACE_DESC desc;
				D3DLOCKED_RECT rect;

				Moo::TextureLockWrapper textureLock( map );
				HRESULT hr = textureLock.lockRect( 0, &rect, NULL, 0 );

				if (SUCCEEDED(hr))
				{
					pNormalMap->GetLevelDesc( 0, &desc );
					uint8* pOutData = (uint8*)rect.pBits;
					for (uint32 y = 0; y < desc.Height; y++)
					{
						memcpy( pOutData, image.getRow(y), 
							sizeof(uint16) * image.width() );
						pOutData += rect.Pitch;
					}
					textureLock.unlockRect( 0 );
					size =  image.width();

					// Add the texture to the preload list so that it can get uploaded
					// to video memory
					Moo::rc().addPreloadResource( pNormalMap.pComObject() );
				}
				else
				{
					pNormalMap = NULL;
					ERROR_MSG( "TerrainNormalMap2::load - "
						"unable to lock texture - dx error: %1lx", hr );
				}
			}
		}
		else
		{
			ERROR_MSG( "TerrainNormalMap2::load - "
				"normal map format error." );
		}
	}

	return pNormalMap;
}

/**
 *	This method streams the normal map if needed
 */
void TerrainNormalMap2::stream()
{
	BW_GUARD;	
#ifndef EDITOR_ENABLED
	if (wantQualityMap_)
	{
		if (!pNormalMap_.hasComObject() && !pTask_ && !terrainResource_.empty())
		{
			pTask_ = new NormalMapTask( this, terrainResource_ );
			FileIOTaskManager::instance().addBackgroundTask( pTask_ );
		}
	}
	else if (pNormalMap_.hasComObject())
	{
		pNormalMap_ = NULL;
	}
#endif
}

#ifdef EDITOR_ENABLED

/**
 *  This method saves the normal maps to a data section
 *	@param pTerrainSection the parent section of the normal map
 *	@return true if success
 */
bool TerrainNormalMap2::save( DataSectionPtr pTerrainSection )
{
	BW_GUARD;
	pTerrainSection->deleteSections( NORMALS_SECTION_NAME );
	pTerrainSection->deleteSections( LOD_NORMALS_SECTION_NAME );
	bool ret = true;
	ret &= saveMap( pTerrainSection->newSection( NORMALS_SECTION_NAME, 
									BinSection::creator() ), pNormalMap_ );
	ret &= saveMap( pTerrainSection->newSection( LOD_NORMALS_SECTION_NAME, 
									BinSection::creator() ), pLodNormalMap_ );
	return ret;
}

/**
 *	This method saves a normal map from a texture to a datasection
 *	@param pSection the section to save the normal map to
 *	@param pMap the texture containing the normal map
 *	@return true if successful
 */
bool TerrainNormalMap2::saveMap( DataSectionPtr pSection,
								ComObjectWrap<DX::Texture> pMap)
{
	BW_GUARD;
	bool ret = false;

	if (!pMap.pComObject() || !pSection.hasObject())
		return ret;

	// Lock the normal map

	D3DLOCKED_RECT rect;
	D3DSURFACE_DESC surfaceDesc;

	Moo::TextureLockWrapper textureLock ( pMap );
	if (FAILED(textureLock.lockRect( 0, &rect, NULL, D3DLOCK_READONLY)) ||
		FAILED(pMap->GetLevelDesc(0, &surfaceDesc )))
	{
		ERROR_MSG( "TerrainNormalMap2::saveMap - Unable to lock normal map" );
		return ret;
	}

	// Create the image for the normal map and compress it
	uint16* pRawData = (uint16*)rect.pBits;
	Moo::Image<uint16> normalData( surfaceDesc.Width, surfaceDesc.Height, 
		pRawData, false, rect.Pitch, false );

	BinaryPtr pData = compressImage( normalData );

	// unlock normal map
	textureLock.unlockRect( 0 );
	textureLock.flush();

	if (pData && pSection)
	{
		// We store the data initially in a vector
		BW::vector<uint8> data;
		data.resize(pData->len() + sizeof(TerrainNormalMapHeader));

		// Write out the header
		TerrainNormalMapHeader* pHeader = 
			reinterpret_cast<TerrainNormalMapHeader*>(&data.front());
		pHeader->magic_ = TerrainNormalMapHeader::MAGIC;
		pHeader->version_ = TerrainNormalMapHeader::VERSION_16_BIT_PNG;

		// Copy the compressed image data
		memcpy( pHeader + 1, pData->data(), pData->len() );

		// Create data section
		BinaryPtr pSaveData = 
			new BinaryBlock( &data.front(), data.size(), "Terrain/NormalMap2/BinaryBlock" );

		pSaveData->canZip( false ); // we are already compressed, don't recompress!

		// Write the data out to the data section
		pSection->setBinary( pSaveData );
		ret = true;
	}

	return ret;
}

namespace // anonymous
{

	/*
	*	This method generates the normal map using the interpolated normal from
	*	the height map
	*	@param pNormals the normals to output
	*	@param normalDimensions the dimensions of the normal map
	*	@param stride the stride of the normal map
	*/
	void calculateNormals(	TerrainHeightMap2Ptr pHeightMap,
		uint16* pNormals, 
		uint32	normalDimensions, 
		uint32	stride )
	{
		BW_GUARD;
		// Calculate the iteration step for the height blocks
		const float xStep = pHeightMap->blockSize() / float(normalDimensions - 1);
		const float zStep = pHeightMap->blockSize() / float(normalDimensions - 1);

		// Result normal and scaling
		const Vector3	multiplier( 255.9f / 2.f, 255.9f / 2.f, 255.9f / 2.f );
		const Vector3	adder( 1.f, 1.f, 1.f );
		Vector3			normal;

		float z = 0.f;
		for (uint32 zi = 0; zi < normalDimensions; zi++)
		{
			uint16* pCurNormal = (uint16*)((uint8*)(pNormals) + zi * stride);

			float x = 0.f;
			for (uint32 xi = 0; xi < normalDimensions; xi++)
			{			
				normal = pHeightMap->normalAt( x, z );
				normal += adder;
				normal *= multiplier.x;

				// swizzle the x and z normal component into our a8l8 array
				*(pCurNormal++) = 
					(uint16( normal.x ) << 8) |
					(uint16( normal.z ));

				x += xStep;
			}

			z += zStep;
		}
	}

	/*
	*	This method calculates the normals from the height map in a fast way.
	*	The normals can exhibit a bit of aliasing as they are calculated on a height
	*	granularity from the height map
	*	@param pHeightMap the height map to generate from
	*	@param pNormals the normals to output
	*	@param normalDimensions the dimensions of the normal map
	*	@param stride the stride of the normal map
	*/
	void calculateNormalsFast(	TerrainHeightMap2Ptr pHeightMap,
		uint16* pNormals, 
		uint32	normalDimensions, 
		uint32	stride )
	{
		BW_GUARD;
		// Calculate the iteration step for the height blocks
		const float xStep = pHeightMap->verticesWidth() / float(normalDimensions - 1);
		const float zStep = pHeightMap->verticesHeight() / float(normalDimensions - 1);
		const float xOffset = float( pHeightMap->xVisibleOffset() );
		const float zOffset = float( pHeightMap->zVisibleOffset() );

		// Result normal and scaling
		const Vector3	multiplier( 255.9f / 2.f, 255.9f / 2.f, 255.9f / 2.f );
		const Vector3	adder( 1.f, 1.f, 1.f );
		Vector3			normal;

		float z = zOffset;
		for (uint32 zi = 0; zi < normalDimensions; zi++)
		{
			uint16* pCurNormal = (uint16*)((uint8*)(pNormals) + zi * stride);

			float x = xOffset;
			for (uint32 xi = 0; xi < normalDimensions; xi++)
			{			
				// note normalAt(int,int) is very different to float version, which
				// takes into account visible offset;
				normal = pHeightMap->normalAt( int(x), int(z) );
				normal += adder;
				normal *= multiplier.x;

				// swizzle the x and z normal component into our a8l8 array
				*(pCurNormal++) = 
					(uint16( normal.x ) << 8) |
					(uint16( normal.z ));

				x += xStep;
			}

			z += zStep;
		}
	}

} // namespace anonynous

//#define PROFILE_NORMAL_GENERATION

/**
 *	This method generates the normal map and the lod normal map 
 *	from a height map
 *	@param pHeightMap the source height map
 *	@param quality the normal map quality to use
 *	@param size the size of the highest res normal map
 *	@return true if successful
 */
bool TerrainNormalMap2::generate(	const TerrainHeightMap2Ptr	pHeightMap,
									NormalMapQuality			quality,
									uint32 size )
{
	BW_GUARD;
	pNormalMap_ = generateMap( pHeightMap, quality, size );
	size_ = size;

	const uint32 MIN_MAP_SIZE = 32;

	if (size > MIN_MAP_SIZE && quality == NMQ_NICE)
	{
		lodSize_ = std::max(size >> 2, MIN_MAP_SIZE);
		pLodNormalMap_ = generateMap( pHeightMap, quality, lodSize_ );
	}
	else
	{
		pLodNormalMap_ = pNormalMap_;
		lodSize_ = size_;
	}

	return (pNormalMap_.hasComObject() && pLodNormalMap_.hasComObject());
}

/**
 *	This static method generates a normal map from a height map
 *	@param pHeightMap the source height map
 *	@param quality the normal map quality to use
 *	@param size the size for the generated normal map
 *	@return the normal map texture
 */
ComObjectWrap<DX::Texture> TerrainNormalMap2::generateMap(	const TerrainHeightMap2Ptr	pHeightMap,
									NormalMapQuality			quality,
									uint32 size )
{
	BW_GUARD;	
#ifdef PROFILE_NORMAL_GENERATION 
	LARGE_INTEGER start, end;
	QueryPerformanceCounter( &start );
#endif

	ComObjectWrap<DX::Texture> pNormalMap; 

	// Create 16 bit texture for normal map information as the normals y
	// component is always positive we can get away with only storing the
	// x/z components
	bool ret = false;
#ifdef EDITOR_ENABLED
	DWORD usage = Moo::rc().usingD3DDeviceEx() ? D3DUSAGE_DYNAMIC : 0;
#else
	DWORD usage = 0;
#endif // EDITOR_ENABLED

	if (pNormalMap = Moo::rc().createTexture( 
		size, size, 1, usage, D3DFMT_A8L8, 
		D3DPOOL_MANAGED, "Terrain/NormalMap2/Texture" ))
	{

		D3DLOCKED_RECT rect;
		Moo::TextureLockWrapper textureLock( pNormalMap );

		if (SUCCEEDED(textureLock.lockRect( 0, &rect, NULL, 0 )))
		{

			if ( quality == NMQ_NICE )
			{
				// calculate normals using the normal interpolation, nice quality
				calculateNormals( pHeightMap, (uint16*)rect.pBits, 
					size, rect.Pitch );
			}
			else
			{
				// calculate normals directly from height map, not as good quality
				calculateNormalsFast( pHeightMap, (uint16*)rect.pBits, 
					size, rect.Pitch );
			}
			textureLock.unlockRect( 0 );
		}

	}

#ifdef PROFILE_NORMAL_GENERATION
	QueryPerformanceCounter( &end );
	static LARGE_INTEGER total; total.QuadPart = 0L;
	total.QuadPart += end.QuadPart - start.QuadPart;
	INFO_MSG( "createNormalMap total time=%d\n", ( total.QuadPart ) );
#endif

	return pNormalMap;
}
#endif

TerrainNormalMap2::NormalMapTask::NormalMapTask(
	SmartPointer<TerrainNormalMap2> pMap, const BW::string & terrainResource ) :
	BackgroundTask( "TerrainNormalMap2" ),
	terrainResource_( terrainResource ),
	size_(0)
{
	BW_GUARD;

	MF_ASSERT( !terrainResource_.empty() );

	RESOURCE_COUNTER_ADD( ResourceCounters::DescriptionPool("TerrrainNormalMap2/Task",
		(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		sizeof(*this), 0 );

	// keep a reference to the normal map
	pMap_ = pMap;
}


TerrainNormalMap2::NormalMapTask::~NormalMapTask()
{
	BW_GUARD;
	RESOURCE_COUNTER_SUB( ResourceCounters::DescriptionPool("TerrrainNormalMap2/Task",
		(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		sizeof(*this), 0 );
	if (pMap_)
	{
		// Set map task to null as we are done
		pMap_->pTask_ = NULL;
	}
}

void TerrainNormalMap2::NormalMapTask::doBackgroundTask( TaskManager& mgr )
{
	BW_GUARD;
	DataSectionPtr pTerrainSection = BWResource::openSection( terrainResource_ );
	if (pTerrainSection)
	{
		// Load the normal map
		pNormalMap_ = TerrainNormalMap2::loadMap( pTerrainSection->openSection( NORMALS_SECTION_NAME ), size_ );
		if (pNormalMap_)
			FileIOTaskManager::instance().addMainThreadTask( this );
	}
}

void TerrainNormalMap2::NormalMapTask::doMainThreadTask( TaskManager& mgr )
{
	// Set up the normal map texture
	pMap_->pNormalMap_ = pNormalMap_;
	pMap_->size_ = size_;
}

BW_END_NAMESPACE

// terrain_normal_map2.cpp

