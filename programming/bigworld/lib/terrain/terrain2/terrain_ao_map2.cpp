#include "pch.hpp"
#include "terrain_ao_map2.hpp"
#include "moo/png.hpp"
#include "resmgr/bin_section.hpp"

DECLARE_DEBUG_COMPONENT2( "Terrain", 0 );


BW_BEGIN_NAMESPACE

using namespace Terrain;

TerrainAOMap2::TerrainAOMap2()	:	size_ ( 0 )
{
	RESOURCE_COUNTER_ADD( ResourceCounters::DescriptionPool("TerrainAOMap2/Base",
		(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		sizeof(*this), 0 );
}

TerrainAOMap2::~TerrainAOMap2()
{
	RESOURCE_COUNTER_SUB( ResourceCounters::DescriptionPool("TerrainAOMap2/Base",
		(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		sizeof(*this), 0 );
}

/**
 *	This method inits the ao map
 *	@param terrainResource the resource name of the underlying terrain section
 *	@return true if successful
 */
bool TerrainAOMap2::init(const BW::string& terrainResource)
{
	BW_GUARD;
	bool res = false;
	DataSectionPtr pTerrainSection = BWResource::instance().openSection( terrainResource );
	if (pTerrainSection)
	{
		pAOMap_ = loadMap(pTerrainSection->openSection( "ambientOcclusion" ), size_ );
		res = pAOMap_.hasComObject();
	}
	return res;
}

/**
 *	This static method loads a ao map from a datasection
 *	@param pMapSection the data section for the ao map
 *	@param size return value for the size of the ao map
 *	@return the normal map texture
 */
ComObjectWrap<DX::Texture> TerrainAOMap2::loadMap(const DataSectionPtr& pMapSection, uint32& size)
{
	BW_GUARD;
	ComObjectWrap<DX::Texture> pAOMap;

	BinaryPtr pData;
	if (pMapSection && (pData = pMapSection->asBinary()))
	{
		const TerrainAOMapHeader* pHeader = reinterpret_cast<const TerrainAOMapHeader*>(pData->data());

		// Check the header for 8 bit png compression
		if (pHeader->magic_ == TerrainAOMapHeader::MAGIC &&	pHeader->version_ == TerrainAOMapHeader::VERSION_8_BIT_PNG)
		{
			// The compressed image data is immediately after the header
			// in the binary block
			BinaryPtr pImageData = 
				new BinaryBlock
				( 
					pHeader + 1, 
					pData->len() - sizeof( TerrainAOMapHeader ), 
					"Terrain/AOMap2/BinaryBlock",
					pData 
				);

			// Decompress the image, if this succeeds, we are good to go
			Moo::Image<uint8> image;
			if(!decompressImage(pImageData, image))
			{
				ERROR_MSG("TerrainAOMap2::load - the ao map data could not be compressed" );
				return NULL;
			}

#ifdef EDITOR_ENABLED
			DWORD usage = Moo::rc().usingD3DDeviceEx() ? D3DUSAGE_DYNAMIC : 0;
#else
			DWORD usage = 0;
#endif // EDITOR_ENABLED

			// Create the texture for the ao map
			pAOMap = Moo::rc().createTexture( image.width(), image.height(), 
				1, usage, D3DFMT_L8, D3DPOOL_MANAGED, "Terrain/AOMap2/Texture" );
			
			// Fill the ao map texture
			if (pAOMap)
			{
				D3DSURFACE_DESC desc;
				D3DLOCKED_RECT rect;

				Moo::TextureLockWrapper textureLock( pAOMap );

				HRESULT hr = textureLock.lockRect( 0, &rect, NULL, 0 );
				if (SUCCEEDED(hr))
				{
					pAOMap->GetLevelDesc( 0, &desc );
					uint8* pOutData = (uint8*)rect.pBits;
					for (uint32 y = 0; y < desc.Height; y++)
					{
						memcpy( pOutData, image.getRow(y), sizeof(uint8) * image.width() );
						pOutData += rect.Pitch;
					}
					textureLock.unlockRect( 0 );
					size =  image.width();

					// Add the texture to the preload list so that it can get uploaded
					// to video memory
					Moo::rc().addPreloadResource( pAOMap.pComObject() );
				}
				else
				{
					pAOMap = NULL;
					ERROR_MSG( "TerrainAOMap2::load - unable to lock texture - dx error: %lx", hr );
				}
			}
		}
		else
		{
			ERROR_MSG( "TerrainAOMap2::load - ao map format error." );
		}
	}

	return pAOMap;
}

#ifdef EDITOR_ENABLED

/**
 *
 */
bool TerrainAOMap2::import(const Moo::Image<uint8>& srcImg, uint top, uint left, uint size)
{
	BW_GUARD;

	bool success = true;

	//-- 1. blit desired rect of the big source image.
	//--	Note: ao has range [0,1] where 0.5 means no ao influence, 0 - 0.5 values make ambient,
	//--		  darker 0.5 - 1 values make ambient brighter.
	Moo::Image<uint8> dstImg(size, size, 127);
	{
		//-- calculate src rect with 1 pixel border around it. This border needed for doing correct
		//-- bilinear interpolation in pixel shader.
		uint32 outsidesSize = size;
		uint32 insidesSize  = outsidesSize - 1;

		//-- Note: chunks located on the edges of the space have 1 pixel border outside the srcImg
		//--	   texture so they are initialized with the default value 255.
		uint32 srcX = left * insidesSize - (left ? 1 : 0);
		uint32 srcY = top  * insidesSize - (top  ? 1 : 0);
		uint32 srcW = outsidesSize - ((((left + 1) * insidesSize) > srcImg.width() ) ? 1 : 0) - ((left == 0) ? 1 : 0);
		uint32 srcH = outsidesSize - ((((top  + 1) * insidesSize) > srcImg.height()) ? 1 : 0) - ((top  == 0) ? 1 : 0);

		//--
		uint32 dstX = srcX ? 0 : 1;
		uint32 dstY = srcY ? 0 : 1;

		//--
		dstImg.blit(srcImg, srcX, srcY, srcW, srcH, dstX, dstY);
	}

	//-- 2. try to upload extracted image rect into chunk ao map.
	{
		success &= createMap(dstImg, pAOMap_);
		size_ = size;
	}
	
	return success;
}

/**
 *
 */
void TerrainAOMap2::regenerate(uint size)
{
	BW_GUARD;

	size_ = size;
	Moo::Image<uint8> srcImg(size_, size_, 127);
	createMap(srcImg, pAOMap_);
}

/**
 *  This method saves the ao maps to a data section
 *	@param pTerrainSection the parent section of the ao map
 *	@return true if success
 */
bool TerrainAOMap2::save(const DataSectionPtr& pTerrainSection )
{
	BW_GUARD;
	pTerrainSection->deleteSections( "ambientOcclusion" );
	return saveMap( pTerrainSection->newSection( "ambientOcclusion", BinSection::creator() ), pAOMap_ );
}

/**
 *
 */
bool TerrainAOMap2::createMap(const Moo::Image<uint8>& srcImg, ComObjectWrap<DX::Texture>& pMap)
{
	BW_GUARD;
	MF_ASSERT(srcImg.width() == srcImg.height());


#ifdef EDITOR_ENABLED
	DWORD usage = Moo::rc().usingD3DDeviceEx() ? D3DUSAGE_DYNAMIC : 0;
#else
	DWORD usage = 0;
#endif // EDITOR_ENABLED

	// Create the texture for the ao map
	ComObjectWrap<DX::Texture> pAOMap = Moo::rc().createTexture(
		srcImg.width(), srcImg.width(), 1, usage, D3DFMT_L8, D3DPOOL_MANAGED,
		"Terrain/AOMap2/Texture" );

	// Fill the AO map texture
	if (pAOMap)
	{
		D3DSURFACE_DESC desc;
		D3DLOCKED_RECT rect;


		Moo::TextureLockWrapper textureLock( pAOMap );

		HRESULT hr = textureLock.lockRect( 0, &rect, NULL, 0 );
		if (SUCCEEDED(hr))
		{
			pAOMap->GetLevelDesc( 0, &desc );
			uint8* pOutData = (uint8*)rect.pBits;
			for (uint32 y = 0; y < desc.Height; y++)
			{
				memcpy( pOutData, srcImg.getRow(y), sizeof(uint8) * srcImg.width() );
				pOutData += rect.Pitch;
			}
			textureLock.unlockRect( 0 );

			// Add the texture to the preload list so that it can get uploaded
			// to video memory
			Moo::rc().addPreloadResource( pAOMap.pComObject() );

			//-- final step
			pMap = pAOMap;
		}
		else
		{
			pAOMap = NULL;
			ERROR_MSG( "TerrainAOMap2::import - unable to lock texture - dx error: %1x", hr );
			return false;
		}
	}
	else
	{
		ERROR_MSG( "TerrainAOMap2::import - unable to create new texture." );
		return false;
	}

	return true;
}

/**
 *	This method saves a ao map from a texture to a datasection
 *	@param pSection the section to save the ao map to
 *	@param pMap the texture containing the ao map
 *	@return true if successful
 */
bool TerrainAOMap2::saveMap(const DataSectionPtr& pSection, const ComObjectWrap<DX::Texture>& pMap)
{
	BW_GUARD;
	bool ret = false;

	if (!pMap.pComObject() || !pSection.hasObject())
		return ret;

	// Lock the ao map
	
	D3DLOCKED_RECT rect;
	D3DSURFACE_DESC surfaceDesc;
	Moo::TextureLockWrapper textureLock( pMap );
	if (FAILED(textureLock.lockRect( 0, &rect, NULL, D3DLOCK_READONLY )) ||
		FAILED(pMap->GetLevelDesc(0, &surfaceDesc )))
	{
		ERROR_MSG( "TerrainAOMap2::saveMap - Unable to lock ao map" );
		return ret;
	}
	
	// Create the image for the AO map and compress it
	uint8* pRawData = (uint8*)rect.pBits;
	Moo::Image<uint8> aoData(
		surfaceDesc.Width, surfaceDesc.Height, pRawData, false, rect.Pitch, false
		);
	
	BinaryPtr pData = compressImage( aoData );

	// unlock ao map
	textureLock.unlockRect( 0 );

	
	if (pData && pSection)
	{
		// We store the data initially in a vector
		BW::vector<uint8> data;
		data.resize(pData->len() + sizeof(TerrainAOMapHeader));

		// Write out the header
		TerrainAOMapHeader* pHeader = reinterpret_cast<TerrainAOMapHeader*>(&data.front());
		pHeader->magic_		= TerrainAOMapHeader::MAGIC;
		pHeader->version_	= TerrainAOMapHeader::VERSION_8_BIT_PNG;

		// Copy the compressed image data
		memcpy( pHeader + 1, pData->data(), pData->len() );

		// Create data section
		BinaryPtr pSaveData = new BinaryBlock(&data.front(), data.size(), "Terrain/AOMap2/BinaryBlock");
		pSaveData->canZip( false ); // we are already compressed, don't recompress!
	
		// Write the data out to the data section
		pSection->setBinary( pSaveData );
		ret = true;
	}
	
	return ret;
}

#endif // EDITOR_ENABLED

BW_END_NAMESPACE
