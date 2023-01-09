#ifndef TERRAIN_LOD_TEXTURE_HPP
#define TERRAIN_LOD_TEXTURE_HPP

BW_BEGIN_NAMESPACE

namespace Terrain
{

class TerrainLodTextureEntry;
typedef SmartPointer<TerrainLodTextureEntry> TerrainLodTextureEntryPtr;

/**
 *	This class implements a lod texture which is a texture made up of
 *	many different textures. Unlike a texture atlas, this texture is
 *	meant to have textures bleeding into eachother so that we can render
 *	the whole texture as one. This texture is used for the normal maps
 *	and low detail levels of the terrain texturing.
 */
class TerrainLodTexture : public ReferenceCount, public Moo::DeviceCallback
{
public:
	TerrainLodTexture();
	~TerrainLodTexture();

	bool init( uint32 textureSize, uint32 lodSize, D3DFORMAT textureFormat );

	void deleteUnmanagedObjects( );
	void createUnmanagedObjects( );

	TerrainLodTextureEntryPtr addTextureTile( DX::Texture* pTexture, 
		uint32 uTile, uint32 vTile );

	DX::Texture* pTexture() { return pTexture_.pComObject(); }

private:

	uint32	lodItemSize_;
	uint32	textureSize_;
	uint32	rowSize_;

	float	lodItemFraction_;

	D3DFORMAT textureFormat_;

	ComObjectWrap<DX::Texture> pTexture_;
};

typedef SmartPointer<TerrainLodTexture> TerrainLodTexturePtr;

class TerrainLodTextureEntry : public ReferenceCount
{
public:
	~TerrainLodTextureEntry();
private:
	bool init( TerrainLodTexturePtr pTexture, const Vector4& offsetScale,
		uint32 uTile, uint32 vTile );
	TerrainLodTextureEntry();

	TerrainLodTexturePtr	pLodTexture_;
	Vector4					offsetScale_;
	uint32					uTile_;
	uint32					vTile_;
	friend class TerrainLodTexture;
};

} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_LOD_TEXTURE_HPP
