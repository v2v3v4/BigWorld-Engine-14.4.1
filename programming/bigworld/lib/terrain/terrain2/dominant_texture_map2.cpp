#include "pch.hpp"
#include "dominant_texture_map2.hpp"

#if defined( EDITOR_ENABLED )
#include "../editor_base_terrain_block.hpp"
#endif // defined( EDITOR_ENABLED )
#include "../terrain_data.hpp"
#include "../terrain_texture_layer.hpp"


BW_BEGIN_NAMESPACE

using namespace Terrain;

/**
 *  This is the DominantTextureMap2 default constructor. Use this
 *	constructor if you want to use the load() interface afterwards.
 */
DominantTextureMap2::DominantTextureMap2( float blockSize ) :
	DominantTextureMap( blockSize )
{
}


/**
 *  This is the DominantTextureMap2 default constructor.  This represents
 *  terrain without material information.
 *
 *  @param layers          Texture layers to extract texture info.
 *  @param sizeMultiplier  Multiplier to affect the size of the dominant texture map
 *                         relative to the size of the biggest layer.
 */
/*explicit*/ DominantTextureMap2::DominantTextureMap2(
	float blockSize, TextureLayers& layers, float sizeMultiplier /*= 0.5f*/) :
		DominantTextureMap( blockSize, layers, sizeMultiplier )
{
}


#ifdef EDITOR_ENABLED

/**
*  This function saves the dominant texture map to a DataSection.
*
*  @param pSection     A DataSection that can be used to restore the
*                      dominant texture map with DominantTextureMap2::Load.
*  @returns            True if successful, false otherwise.
*/
/*virtual*/ bool DominantTextureMap2::save( DataSectionPtr pSection ) const
{
	DominantTextureMapHolder holder( (DominantTextureMap*)this, true );

	// calculate data size from the num of textures and image size.
	uint32 numTextures = static_cast<uint32>(textureNames().size());
	uint32 textureNameSize = 0;
	for( uint32 i = 0; i < numTextures; ++i )
	{
		uint32 nameLength = static_cast<uint32>(textureNames()[i].length());
		if ( nameLength > textureNameSize )
			textureNameSize = nameLength;
	}
	uint32 textureDataSize = textureNameSize * numTextures;

	// checks
	if ( textureDataSize == 0 || image().rawDataSize() == 0 )
	{
		ERROR_MSG( "Failed to save dominant texture map: empty texture names and/or image\n" );
		return false;
	}

	// create header
	BW::vector<uint8> hdrData(
		sizeof(DominantTextureMapHeader) + textureDataSize, 0 );

	DominantTextureMapHeader* pHeader = 
		reinterpret_cast<DominantTextureMapHeader*>(&hdrData.front());

	pHeader->magic_ = DominantTextureMapHeader::MAGIC;
	pHeader->version_ = DominantTextureMapHeader::VERSION_ZIP;

	// save texture names
	pHeader->numTextures_ = numTextures;
	pHeader->textureNameSize_ = textureNameSize;
	char *p = (char*)( pHeader + 1 );
	for( uint32 i = 0; i < numTextures; ++i )
	{
		strncpy( p, textureNames()[i].c_str(), textureNameSize );
		p += textureNameSize;
	}

	// save image and compress it
	pHeader->width_ = width();
	pHeader->height_ = height();

	BW::vector<uint8> imgData( image().rawDataSize() );
	image().copyTo( &imgData.front() );

	BinaryPtr pImageBlock = new BinaryBlock(
		&imgData.front(), imgData.size(), "Terrain/DominantTextureMap2/BinaryBlock" );
	pImageBlock = pImageBlock->compress();

	// Write the header and image data out to the data section
	BW::vector<uint8> data( hdrData.size() + pImageBlock->len() );
	memcpy( &data.front(), &hdrData.front(), hdrData.size() );
	memcpy( &data.front() + hdrData.size(), pImageBlock->data(), pImageBlock->len() );

	BinaryPtr pSaveData = 
		new BinaryBlock( &data.front(), data.size(), "Terrain/DominantTextureMap2/BinaryBlock" );

	pSaveData->canZip( false ); // don't recompress
	pSection->setBinary( pSaveData );

	return true;
}

#endif // EDITOR_ENABLED

/**
 *  This function loads a dominant texture map saved with DominantTextureMap2::save.
 *
 *  @param dataSection   The DataSection to load from.
 *  @param error         If not null and an error occurs
 */
/*virtual*/ bool DominantTextureMap2::load(DataSectionPtr dataSection, BW::string *error/* = NULL*/)
{
	BW_GUARD;
	PROFILE_FILE_SCOPED( dominantTextureMap2_load );
	BinaryPtr pData = dataSection->asBinary();
	if ( pData == NULL )
	{
		if ( error )
			*error = "Couldn't load dominant texture map: dataSection not binary.";
		return false;
	}

	// create and check the header
	const DominantTextureMapHeader* pHeader = 
		reinterpret_cast<const DominantTextureMapHeader* >(pData->data());

	if ( pHeader->magic_ != DominantTextureMapHeader::MAGIC ||
		pHeader->version_ != DominantTextureMapHeader::VERSION_ZIP )
	{
		if ( error )
			*error = "Wrong map type when loading dominant texture map.";
		return false;
	}

	if ( pHeader->width_ == 0 ||
		 pHeader->height_ == 0 )
	{
		if ( error )
			*error = "Failed to load dominant texture map: image has zero width/height.";
		return false;
	}

	// load texture names
	uint32 textureDataSize = pHeader->textureNameSize_ * pHeader->numTextures_;
	BW::vector<BW::string> texNames;
	char *buf = new char[pHeader->textureNameSize_ + 1];
	char *p = (char*)( pHeader + 1 );
	for( uint32 i = 0; i < pHeader->numTextures_; ++i )
	{
		strncpy( buf, p, pHeader->textureNameSize_);
		buf[ std::min( uint32(strlen(p)), pHeader->textureNameSize_ ) ] = '\0';
		texNames.push_back( buf );
		p += pHeader->textureNameSize_;
	}
	bw_safe_delete_array(buf);
	textureNames( texNames );

	// load image
	BinaryPtr pImageData = new BinaryBlock( 
		(uint8*)(pHeader + 1) + textureDataSize, 
		pData->len() - sizeof( DominantTextureMapHeader ) - textureDataSize,
		"Terrain/DominantTextureMap2/BinaryBlock",
		pData );

	if (pImageData->isCompressed())
		pImageData = pImageData->decompress();

	DominantTextureMap::ImageType newImage(pHeader->width_, pHeader->height_, 
		"Terrain/DominantTextureMap2" );
	if ( newImage.rawDataSize() > (size_t)pImageData->len() )
	{
		if ( error )
			*error = "Image data size overflow when loading dominant texture map.";
		return false;
	}
	newImage.copyFrom( (uint8*)pImageData->data() );
	image( newImage );

    return true;
}

BW_END_NAMESPACE

// dominant_texture_map2.cpp
