#include "pch.hpp"
#include "terrain_lod_texture.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

namespace Terrain
{

const uint32 MAX_TEXTURE_SIZE = 4096;

TerrainLodTexture::TerrainLodTexture() :
	lodItemSize_(),
	textureSize_(),
	rowSize_(),
	lodItemFraction_()
{
	ZeroMemory( &textureFormat_, sizeof(textureFormat_) );
}

TerrainLodTexture::~TerrainLodTexture()
{

}

/**
 *	This method inits the terrain lod texture
 *	@param textureSize the size of the texture
 *	@param lodSize the size of the individual lod cells in the texture
 *	@param textureFormat the texture format of the texture
 *	@return true if the lod texture was created successfully
 */
bool TerrainLodTexture::init( uint32 textureSize, uint32 lodSize, D3DFORMAT textureFormat )
{
	BW_GUARD;
	textureSize_ = std::min( 
		(uint32)Moo::rc().deviceInfo( Moo::rc().deviceIndex() ).caps_.MaxTextureWidth, 
		textureSize );

	lodItemSize_ = lodSize;
	rowSize_ = textureSize_ / lodItemSize_;
	textureFormat_ = textureFormat_;
	lodItemFraction_ = float(lodSize) / float( textureSize_ );

	return true;
}

void TerrainLodTexture::deleteUnmanagedObjects( )
{
	pTexture_ = NULL;
}

void TerrainLodTexture::createUnmanagedObjects( )
{
	BW_GUARD;
	pTexture_ = Moo::rc().createTexture( 
		textureSize_, textureSize_, 1, 
		D3DUSAGE_DYNAMIC,
		textureFormat_, D3DPOOL_DEFAULT, 
		"texture/lod map" );
}

/**
 *	This method adds a texture tile to the lod texture
 *	@param pTexture the texture to add, this texture needs to be of the 
 *		same format and dimensions as all the other lodItems in the
 *		lod texture
 *	@param uTile
 *	@param vTile
 *	@return pointer to the lod texture entry that points to the tile
 *		in the lod texture that is used for rendering the texture
 */
TerrainLodTextureEntryPtr TerrainLodTexture::addTextureTile( 
	DX::Texture* pTexture, uint32 uTile, uint32 vTile )
{
	BW_GUARD;
	TerrainLodTextureEntryPtr pEntry;
	MF_ASSERT( pTexture_.hasComObject() );
	MF_ASSERT( pTexture );

	// Get the surface of the source image
	ComObjectWrap<DX::Surface> pSrcSurface;
	HRESULT hr = pTexture->GetSurfaceLevel( 0, &pSrcSurface );

	// Get the surface of the lod texture
	ComObjectWrap<DX::Surface> pDestSurface;
	HRESULT hr2 = pTexture_->GetSurfaceLevel( 0, &pDestSurface );

	if (SUCCEEDED(hr) && SUCCEEDED(hr2))
	{
		POINT	destPoint;
		destPoint.x = uTile * lodItemSize_;
		destPoint.y = vTile * lodItemSize_;

		RECT	srcRect;
		srcRect.bottom = lodItemSize_;
		srcRect.right = lodItemSize_;

		HRESULT hrUpdate = Moo::rc().device()->UpdateSurface( 
			pSrcSurface.pComObject(), &srcRect,
			pDestSurface.pComObject(), &destPoint );

		if (SUCCEEDED(hrUpdate))
		{
			pEntry = new TerrainLodTextureEntry();
			pEntry->init( this, Vector4( 
				lodItemFraction_, lodItemFraction_,
				lodItemFraction_ * float( uTile ), 
				lodItemFraction_ * float( vTile ) ),
				uTile, vTile );
		}
		else
		{
			ERROR_MSG( "TerrainLodTexture::addTextureTile - updateSurface "
				"failed\n" );
		}
	}
	else
	{
		ERROR_MSG( "TerrainLodTexture::addTextureTile - unable to get surface "
				"for either texture tile (0x%lx) or lod texture (0x%lx)\n",
			hr, hr2 );
	}

	return pEntry;
}

/**
 *	This method inits the TerrainLodTextureEntry
 *
 */
bool TerrainLodTextureEntry::init( TerrainLodTexturePtr pLodTexture, 
		const Vector4& offsetScale, uint32 uTile, uint32 vTile )
{
	BW_GUARD;
	pLodTexture_ = pLodTexture;
	offsetScale_ = offsetScale;
	uTile_ = uTile;
	vTile_ = vTile;

	return true;
}

} // namespace Terrain

BW_END_NAMESPACE

// terrain_lod_texture.cpp
