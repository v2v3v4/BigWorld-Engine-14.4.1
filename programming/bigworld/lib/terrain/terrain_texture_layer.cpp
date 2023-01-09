#include "pch.hpp"

#include "terrain_texture_layer.hpp"

#include "cstdmf/watcher.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/multi_file_system.hpp"
#include "moo/render_context.hpp"
#include "moo/texture_lock_wrapper.hpp"
#ifdef EDITOR_ENABLED
#include "moo/png.hpp"
#endif

DECLARE_DEBUG_COMPONENT2("Moo", 0)


BW_BEGIN_NAMESPACE

using namespace Terrain;

static BasicAutoConfig< float  > s_terrainTextureSpacing( "environment/terrainTextureSpacing", 10.0f );


namespace
{
	enum BlendCompressionMethod
	{
		BCM_UNCOMPRESSED = 0,
		BCM_16BIT = 1,
		BCM_DOWNSAMPLED = 2
	};

	BlendCompressionMethod compressBlends = BCM_UNCOMPRESSED;

	bool firstTime = true;
}


#ifdef EDITOR_ENABLED


/**
 *	This function compares this layer with another to see if they use the same
 *	texture and same projection.
 *
 *	@param other			The TerrainTextureLayer to compare with.
 *	@param epsilon			The floating point epsilon to compare texture projections
 *							with.
 */
bool TerrainTextureLayer::sameTexture
(
	TerrainTextureLayer		const &other, 
	float					epsilon
) const
{
	BW_GUARD;
	return
		sameTexture
		(
			other.textureName(),
			other.bumpTextureName(),
			other.hasUVProjections() ? other.uProjection() : Vector4::zero(),
			other.hasUVProjections() ? other.vProjection() : Vector4::zero(),
			epsilon
		);
}


/**
 *	This function determines with the layer uses the given texture at the given
 *	u-v projections.
 *
 *	@param texture			The name of the texture to compare against.
 *	@param uProj			The u-projection to compare against.
 *	@param vProj			The v-projection to compare against.
 *	@returns				True if the layer uses the given texture and projections.
 *							The (x, y, z) components of the projections must be within
 *							epsilon to be considered as the same.
 */
bool TerrainTextureLayer::sameTexture
(
	BW::string				const &texture, 
	BW::string				const &bumpTexture,
	Vector4					const &uProj, 
	Vector4					const &vProj, 
	float					epsilon
) const
{
	BW_GUARD;
	if (bw_stricmp(textureName().c_str(), texture.c_str()) != 0)
		return false;

	if (_stricmp(bumpTextureName().c_str(), bumpTexture.c_str()) != 0)
		return false;

	if (hasUVProjections())
	{
		//since there is no way of modifying the w component, zeroing
		//this component out may be the best solution. 
		Vector3 my, other;
		my = Vector3(uProjection().x, uProjection().y, uProjection().z);
		other = Vector3(uProj.x, uProj.y, uProj.z);
		if (!almostEqual(my, other, epsilon))
			return false;

		my = Vector3(vProjection().x, vProjection().y, vProjection().z);
		other = Vector3(vProj.x, vProj.y, vProj.z);
		if (!almostEqual(my, other, epsilon))
			return false;
	}

	return true;
}


/**
 *  This function saves a layer to a PNG file for debugging purposes.
 *
 *	@param filename		The name of the file to save to.
 *  @returns			True if saved successfully.
 */
bool TerrainTextureLayer::saveToPNG(const BW::string &filename) const
{
	BW_GUARD;
	TerrainTextureLayer *myself = const_cast<TerrainTextureLayer *>(this);
	myself->lock(true);
	ImageType const &img = image();
	BinaryPtr binaryBlock = Moo::compressImage(img);
	FILE *file = BWResource::instance().fileSystem()->posixFileOpen(filename, "wb");
	size_t sz = 0;
	if (file != NULL)
	{
		sz = fwrite(binaryBlock->data(), 1, binaryBlock->len(), file);
		fclose(file);
	}
	myself->unlock();
	return sz == binaryBlock->len();
}


#endif // EDITOR_ENABLED


/**
 *	This function takes up to four layers of the same size and 'compresses' 
 *	them into a single D3DFMT_A8R8G8B8 texture.  Note that there has to be
 *	a pLayer0 and that NULL layers have to come after the non-NULL layers.
 *
 *  @param pLayer0			The first layer.
 *  @param pLayer1			The second layer.
 *  @param pLayer2			The third layer.
 *  @param pLayer3			The fourth layer.
 *	@param allowSmallBlend	If true then allow 8 or 16 bit textures.
 *	@param compressTexture	If true then compress the texture to 4 bits per
 *							channel.
 *  @param smallBlended		If not NULL then this is set to true if not
 *							compressed and 8/16bit textures are allowed.
 *	@returns				The blended texture.
 */
/*static*/ ComObjectWrap<DX::Texture>
TerrainTextureLayer::createBlendTexture
(
	TerrainTextureLayerPtr 	pLayer0, 
    TerrainTextureLayerPtr 	pLayer1,
	TerrainTextureLayerPtr 	pLayer2, 
    TerrainTextureLayerPtr 	pLayer3,
	bool					allowSmallBlend,
	bool					compressTexture,
	bool					*smallBlended
)
{
	BW_GUARD;
	if (firstTime)
	{
		firstTime = false;
		MF_WATCH( "Render/Terrain/Terrain2/compressBlends", (int&)compressBlends, 
			Watcher::WT_READ_WRITE, "Compress blend textures"
			" 0 = no compression, 1 = 16bit, 2 = half size blend texture" );
	}

	// There are only three levels of compression, limit the index
	compressBlends =
		std::max( BCM_UNCOMPRESSED,
			std::min( BCM_DOWNSAMPLED, (BlendCompressionMethod)compressBlends ) );

	bool isCompressed = (compressBlends != BCM_UNCOMPRESSED) && compressTexture;

	ComObjectWrap<DX::Texture> pTexture;

	if (!pLayer0)
	{
		WARNING_MSG("TerrainTextureLayer::createBlendTexture - "
			"No layer0 to create texture from\n");
		return NULL;
	}

	uint32 width = pLayer0->width();
	uint32 height = pLayer0->height();

	// If we are going to compress or scale the blends after creation
	// create the initial texture in the scratch pool, if we are going
	// to use the texture create it in the managed pool.
	D3DPOOL initialPool = isCompressed ? 
		D3DPOOL_SCRATCH : D3DPOOL_MANAGED;

	uint32 numLayers = 0;
	if (pLayer0) ++numLayers;
	if (pLayer1) ++numLayers;
	if (pLayer2) ++numLayers;
	if (pLayer3) ++numLayers;

	D3DFORMAT format = D3DFMT_A8R8G8B8;
	uint32 pixelSize = 4;
	if (allowSmallBlend && !isCompressed)
	{
		if (smallBlended != NULL)
			*smallBlended = true;
		switch (numLayers)
		{
		case 1: format = D3DFMT_L8      ; pixelSize = 1; break;
		case 2: format = D3DFMT_A8L8    ; pixelSize = 2; break;
		case 3: format = D3DFMT_A8R8G8B8; pixelSize = 4; break;
		case 4: format = D3DFMT_A8R8G8B8; pixelSize = 4; break;
		}
	}
	else
	{
		if (smallBlended != NULL)
			*smallBlended = false;
	}

	pTexture = Moo::rc().createTexture( width, height, 1, 0, format,
		initialPool, "Terrain/TextureLayer/BlendTexture" );

	if (!pTexture)
		return NULL;

#ifdef EDITOR_ENABLED
    if (pLayer0) pLayer0->lock(true);
    if (pLayer1) pLayer1->lock(true);
    if (pLayer2) pLayer2->lock(true);
    if (pLayer3) pLayer3->lock(true);
#endif

	D3DLOCKED_RECT rect;
	Moo::TextureLockWrapper textureLock( pTexture );

	HRESULT hr = textureLock.lockRect( 0, &rect, NULL, 0 );
	if (!SUCCEEDED(hr))
	{
		ERROR_MSG("TerrainTextureLayer::createBlendTexture - "
			"Unable to lock texture for writing. DX error: %s\n", 
			DX::errorAsString( hr ).c_str() );
	}
	else
	{
		bool l0Valid = pLayer0 && !pLayer0->image().isEmpty();
		bool l1Valid = pLayer1 && !pLayer1->image().isEmpty();
		bool l2Valid = pLayer2 && !pLayer2->image().isEmpty();
		bool l3Valid = pLayer3 && !pLayer3->image().isEmpty();

		if (numLayers == 1 && l0Valid)
		{

            for (uint32 y = 0; y < height; ++y)
            {
				uint8 const *src0 = pLayer0->image().getRow(y);
				uint8 const *end0 = src0 + width;
				uint8       *dst  = (uint8*)rect.pBits + y*rect.Pitch;
				while (src0 != end0)
				{
					*dst = *src0++;
					dst += pixelSize;
				}
			}
		}
		else if (numLayers == 2 && l0Valid && l1Valid)
		{
			uint8 *dst = (uint8*)rect.pBits;
            for (uint32 y = 0; y < height; ++y)
            {
				uint8 const *src0 = pLayer0->image().getRow(y);
				uint8 const *src1 = pLayer1->image().getRow(y);
				uint8 const *end0 = src0 + width;				
				uint8       *dst  = (uint8*)rect.pBits + y*rect.Pitch;
				while (src0 != end0)
				{
					*(dst    ) = *src0++;
					*(dst + 1) = *src1++;
					dst += pixelSize;
				}
			}
		}
		else if (numLayers == 3 && l0Valid && l1Valid && l2Valid)
		{
			uint8 *dst = (uint8*)rect.pBits;
            for (uint32 y = 0; y < height; ++y)
            {
				uint8 const *src0 = pLayer0->image().getRow(y);				
				uint8 const *src1 = pLayer1->image().getRow(y);
				uint8 const *src2 = pLayer2->image().getRow(y);
				uint8 const *end0 = src0 + width;
				uint8       *dst  = (uint8*)rect.pBits + y*rect.Pitch;
				while (src0 != end0)
				{
					*(dst    ) = *src0++;
					*(dst + 1) = *src1++;
					*(dst + 2) = *src2++;									
					dst += pixelSize;
				}
			}
		}
		else if (numLayers == 4 && l0Valid && l1Valid && l2Valid && l3Valid)
		{
			uint8 *dst = (uint8*)rect.pBits;
            for (uint32 y = 0; y < height; ++y)
            {
				uint8 const *src0 = pLayer0->image().getRow(y);				
				uint8 const *src1 = pLayer1->image().getRow(y);
				uint8 const *src2 = pLayer2->image().getRow(y);
				uint8 const *src3 = pLayer3->image().getRow(y);
				uint8 const *end0 = src0 + width;
				uint8       *dst  = (uint8*)rect.pBits + y*rect.Pitch;
				while (src0 != end0)
				{
					*(dst    ) = *src0++;
					*(dst + 1) = *src1++;
					*(dst + 2) = *src2++;
					*(dst + 3) = *src3++;
					dst += pixelSize;
				}
			}
		}

		textureLock.unlockRect( 0 );
	}

	textureLock.flush();
	textureLock.fini();

#ifdef EDITOR_ENABLED
    if (pLayer0) pLayer0->unlock();
    if (pLayer1) pLayer1->unlock();
    if (pLayer2) pLayer2->unlock();
    if (pLayer3) pLayer3->unlock();
#endif // EDITOR_ENABLED

	if (isCompressed)
	{
		ComObjectWrap<DX::Texture> pCompressed;

		D3DFORMAT fmt = D3DFMT_A4R4G4B4;
		uint32 divisor = 1;
		uint32 filter = D3DX_FILTER_POINT;

		if (compressBlends == BCM_DOWNSAMPLED)
		{
			filter = D3DX_FILTER_BOX;
			fmt = D3DFMT_A8R8G8B8;
			divisor = 2;
		}

		pCompressed = Moo::rc().createTexture( width / divisor, height / divisor, 1,
			0, fmt, D3DPOOL_MANAGED, "Terrain/TextureLayer/BlendTexture" );

		if ( pCompressed )
		{
			ComObjectWrap<DX::Surface> dest;
			pCompressed->GetSurfaceLevel( 0, &dest );
			ComObjectWrap<DX::Surface> src;
			pTexture->GetSurfaceLevel( 0, &src );
			HRESULT hr = D3DXLoadSurfaceFromSurface( dest.pComObject(), NULL, NULL, 
				src.pComObject(), NULL,	NULL, filter, 0 );

			if (FAILED(hr))
			{
				ERROR_MSG( "TerrainTextureLayer::createBlendTexture - "
					"unable to compress blend texture. DX error: %s\n",
					DX::errorAsString(hr).c_str() );
				pCompressed = NULL;
			}

			pTexture = pCompressed;
		}
	}

	if (pTexture.hasComObject())
	{
		// Add the texture to the preload list so that it can get uploaded
		// to video memory
		Moo::rc().addPreloadResource( pTexture.pComObject() );
	}
	return pTexture;
}


/**
 *	This function gets the default U-V projections.
 *
 *	@param u		This is set to the default u-projection.
 *	@param v		This is set to the default v-projection.
 */
/*static*/ void TerrainTextureLayer::defaultUVProjections(Vector4 &u, Vector4 &v)
{
    u = Vector4( 1.f/s_terrainTextureSpacing, 0.f, 0.f, 0.f );
	v = Vector4( 0.f, 0.f, 1.f/s_terrainTextureSpacing, 0.f );
}

BW_END_NAMESPACE

// terrain_texture_layer.cpp
