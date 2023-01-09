#include "pch.hpp"
#include "horizon_shadow_map2.hpp"

#include "../terrain_settings.hpp"
#include "../terrain_data.hpp"
#include "terrain_block2.hpp"

#include "chunk/chunk_grid_size.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 );


BW_BEGIN_NAMESPACE

using namespace Terrain;

/**
 *  This is the HorizonShadowMap2 constructor.
 *
 *  @param block        The underlying block.
 */
/*explicit*/ HorizonShadowMap2::HorizonShadowMap2(TerrainBlock2* block):
	HorizonShadowMap(block ? block->blockSize() : DEFAULT_GRID_RESOLUTION), // 22_MERGE: can receive NULL block via dummyHorizonShadowMap_
    block_(block),
    texture_(NULL),
	textureLock_( NULL ),
    width_(0),
    height_(0),
    lockCount_(0),
	sizeInBytes_(sizeof(*this))
{
}


//#ifdef EDITOR_ENABLED
/**
 *  This function creates an HorizonShadowMap2 using the settings in the
 *  DataSection.
 *
 *  @param settings         DataSection containing the terrain settings.
 *  @param error		    Optional output parameter that receives an error
 *							message if an error occurs.
 *  @returns                True if successful, false otherwise.
 */
bool HorizonShadowMap2::create( uint32 size, BW::string *error /*= NULL*/ )
{
	BW_GUARD;
	if ( !powerOfTwo( size ) )
	{
		if ( error )
			*error = "Shadow map size is not a power of two.";
		return false;
	}

	texture_ = NULL;

	if ((texture_ = Moo::rc().createTexture( size, size, 1,
		Moo::rc().usingD3DDeviceEx() ? D3DUSAGE_DYNAMIC : 0, 
		D3DFMT_G16R16, D3DPOOL_MANAGED, "Terrain/HorizonShadowMap2/Texture" )))
	{
		D3DLOCKED_RECT rect;
		Moo::TextureLockWrapper textureLock( texture_ );
		if (SUCCEEDED(textureLock.lockRect( 0, &rect, NULL, 0 )))
		{
			HorizonShadowMap::PixelType pixel;
			pixel.east = 0;
			pixel.west = 65535;
            HorizonShadowMap::PixelType* imgBits = 
                (HorizonShadowMap::PixelType*)rect.pBits;
            image_.resize( size, size, imgBits, false, rect.Pitch );
			image_.fill( pixel );
			textureLock.unlockRect( 0 );
            image_.clear();
            width_  = size;
            height_ = size;
			return true;
		}
		else
		{
			if ( error )
				*error = "Failed to lock shadow map texture.";
		}
	}
	else
	{
		if ( error )
			*error = "Failed to create shadow map texture.";
	}

	texture_ = NULL;

	return false;
}
//#endif // EDITOR_ENABLED


/**
 *  This function determines the shadow at the given point.
 *
 *  @param x            The x coordinate to get the shadow at.
 *  @param z            The z coordinate to get the shadow at.
 *  @returns            The shadow at x, z.
 */ 
/*virtual*/ HorizonShadowPixel 
HorizonShadowMap2::shadowAt(int32 x, int32 z) const
{
    BW_GUARD;
	// Cast away the const is okay because we are using read-only access:
    HorizonShadowMap2 *myself = const_cast<HorizonShadowMap2 *>(this);
    if (myself->lock(true))
    {
        HorizonShadowPixel result = image_.get(x, z);
        myself->unlock();
        return result;
    }
    else
    {
        MF_ASSERT(0); // failed to lock for some reason
        return HorizonShadowPixel();
    }
}


/**
 *  This function returns the width of the HorizonShadowMap2.
 *
 *  @returns            The width of the HorizonShadowMap2.
 */
/*virtual*/ uint32 HorizonShadowMap2::width() const
{
    return width_;
}


/**
 *  This function returns the height of the HorizonShadowMap2.
 *
 *  @returns            The height of the HorizonShadowMap2.
 */
/*virtual*/ uint32 HorizonShadowMap2::height() const
{
    return height_;
}


/**
 *  This function locks the HorizonShadowMap2 to memory and enables it for
 *  editing.
 *
 *  @param readOnly     This is a hint to the locking.  If true then
 *                      we are only reading values from the map.
 *  @returns            True if the lock is successful, false 
 *                      otherwise.
 */
/*virtual*/ bool HorizonShadowMap2::lock(bool readOnly)
{
    BW_GUARD;

    if (texture_ && lockCount_++ == 0)
    {
        D3DLOCKED_RECT lockedRect;
		textureLock_.init( texture_ );
        HRESULT hr = 
            textureLock_.lockRect
            (
                0, 
                &lockedRect, 
                NULL, 
                readOnly ? D3DLOCK_READONLY : 0
            );

        if (SUCCEEDED(hr))
		{
			image_.resize(
				width_, height_,
				reinterpret_cast<HorizonShadowPixel *>(lockedRect.pBits),
				false, /* don't delete memory in destructor */
				lockedRect.Pitch );

			return true;
		}
		else
		{
			ERROR_MSG( "HorizonShadowMap2::lock: "
					"failed to lock map with error 0x%lx",
				hr );
		}
    }

    return false;
}


/**
 *  This function accesses the HorizonShadowMap2 as though it is an image.
 *  This function should only be called within a lock/unlock pair.
 *
 *  @returns            The HorizonShadowMap2 as an image.
 */
/*virtual*/ HorizonShadowMap2::ImageType &HorizonShadowMap2::image()
{
    MF_ASSERT(lockCount_ != 0);
    return image_;
}


/**
 *  This function accesses the HorizonShadowMap2 as though it is an image.
 *  This function should only be called within a lock/unlock pair.
 *
 *  @returns            The HorizonShadowMap2 as an image.
 */
/*virtual*/ HorizonShadowMap2::ImageType const &HorizonShadowMap2::image() const
{
    MF_ASSERT(lockCount_ != 0);
    return image_;
}


/**
 *  This function unlocks the HorizonShadowMap2.
 *
 *  @returns            True if the unlocking was successful.
 */
/*virtual*/ bool HorizonShadowMap2::unlock()
{
    BW_GUARD;
	if (!texture_)
        return false;

    if (--lockCount_ == 0)
    {
        HRESULT hr = textureLock_.unlockRect( 0 );
        MF_ASSERT(SUCCEEDED(hr));
		textureLock_.flush();
		textureLock_.fini();
        image_.clear();
    }

    return true;
}


/**
 *  This function saves the underlying data to a DataSection that can
 *  be read back in via load.  You should not call this between
 *  lock/unlock pairs.
 *
 *  @param	pSection		The DataSection to save to.
 */
/*virtual*/ bool HorizonShadowMap2::save( DataSectionPtr pSection ) const
{
	BW_GUARD;
	MF_ASSERT( pSection );

    HorizonShadowMap2 *myself = const_cast<HorizonShadowMap2 *>(this);

	if (myself->lock(true))
	{
		BW::vector<uint8> data;
		data.resize( sizeof(ShadowHeader) + image_.rawDataSize() );			

		ShadowHeader* sh = (ShadowHeader*)(&data.front());
		sh->magic_ = ShadowHeader::MAGIC;		
		sh->width_ = image_.width();
		sh->height_ = image_.height();
		sh->bpp_ = sizeof(HorizonShadowPixel)*8; // 8 bits per byte		
		sh->version_ = 1;

		HorizonShadowPixel* pPixels = (HorizonShadowPixel*)(sh + 1);
		image_.copyTo(pPixels);

		BinaryPtr binaryBlock = 
			new BinaryBlock
			(
				&data.front(), 
				data.size(),
				"BinaryBlock/HorizonShadowMap2"
			);
		BinaryPtr pCompressed = binaryBlock->compress();
		pCompressed->canZip( false ); // don't recompress!
		pSection->setBinary( pCompressed );
		myself->unlock();
		return true;
	}

	ERROR_MSG( "Could not lock image\n" );
    return false;
}


/**
 *  This function loads the HorizonShadowMap2 from a DataSection that was 
 *  saved via HorizonShadowMap2::save.  You should not call this between
 *  lock/unlock pairs.
 *
 *  @param dataSection  The DataSection to load from.
 *  @param error        If not NULL and an error occurs then this will
 *                      be set to a reason why the error occured.
 *  @returns            True if the load was successful.
 */
/*virtual*/ bool 
HorizonShadowMap2::load(DataSectionPtr dataSection, BW::string *error/*= NULL*/)
{
	BW_GUARD;
	bool ret = false;
	texture_ = NULL;
    width_ = height_ = 0;

	BinaryPtr pHorizonData = dataSection->asBinary();
	if (pHorizonData)
	{
		if (pHorizonData->isCompressed())
			pHorizonData = pHorizonData->decompress();

		const ShadowHeader* pHeader = (const ShadowHeader*)pHorizonData->data();

		MF_ASSERT( pHeader->magic_ == ShadowHeader::MAGIC );
		MF_ASSERT( pHeader->bpp_ == sizeof(HorizonShadowPixel)*8 );

#ifdef EDITOR_ENABLED
		DWORD usage = Moo::rc().usingD3DDeviceEx() ? D3DUSAGE_DYNAMIC : 0;
#else
		DWORD usage = 0;
#endif

		if ((texture_ = Moo::rc().createTexture( pHeader->width_,
				pHeader->height_, 1, usage, D3DFMT_G16R16, D3DPOOL_MANAGED,
				"Terrain/HorizonShadowMap2/Texture" )))
		{
			D3DLOCKED_RECT rect;
			Moo::TextureLockWrapper textureLock( texture_ );
			if (SUCCEEDED( textureLock.lockRect( 0, &rect, NULL, 0 ) ))
			{
                HorizonShadowMap::PixelType* srcShadows = 
                    (HorizonShadowMap::PixelType*)(pHeader + 1);
                HorizonShadowMap::PixelType* imgBits = 
                    (HorizonShadowMap::PixelType*)rect.pBits;
                image_.resize(pHeader->width_, pHeader->height_, imgBits, false, rect.Pitch);
                image_.copyFrom(srcShadows);
				textureLock.unlockRect( 0 );
                image_.clear();
                width_  = pHeader->width_;
                height_ = pHeader->height_;
			}
			ret = true;
			// Add the texture to the preload list so that it can get uploaded
			// to video memory
			Moo::rc().addPreloadResource( texture_.pComObject() );
		}
		else
		{
			texture_ = NULL;
		}
	}
	return ret;
}


/**
 *  This function returns the BaseTerrainBlock that the HorizonShadowMap2 is
 *  associated with.
 *
 *  @returns            The BaseTerrainBlock of the HorizonShadowMap2.
 */
/*virtual*/ BaseTerrainBlock &HorizonShadowMap2::baseTerrainBlock()
{
    return *block_;
}


/**
 *  This function returns the BaseTerrainBlock that the HorizonShadowMap2 is
 *  associated with.
 *
 *  @returns            The BaseTerrainBlock of the HorizonShadowMap2.
 */
/*virtual*/ BaseTerrainBlock const &HorizonShadowMap2::baseTerrainBlock() const
{
    return *block_;
}


/**
 *  This function returns the underlying texture of the HorizonShadowMap2.
 */
DX::Texture *HorizonShadowMap2::texture() const
{
    if (!texture_)
        return NULL;
    else
        return texture_.pComObject();
}

BW_END_NAMESPACE

// horizon_shadow_map2.cpp
