#include "pch.hpp"
#include "terrain_hole_map2.hpp"

#include "../terrain_settings.hpp"
#include "../terrain_data.hpp"
#include "terrain_block2.hpp"

#include "cstdmf/debug.hpp"
#include "math/math_extra.hpp"
#include "moo/texture_lock_wrapper.hpp"


BW_BEGIN_NAMESPACE

using namespace Terrain;

namespace
{
    // An empty 4x4 block of a dxt1 texture
    const uint64 emptyBlock = uint64(0x0000ffff);


    /*
     *	This class is a helper class for iterating over the hole data.
     */
    class HoleHelper
    {
    public:
	    HoleHelper( BinaryPtr pHolesData ) :
	      pBlock_( pHolesData )
	    {

		    BW_GUARD;
			pHeader_  = (const HolesHeader*)pHolesData->data();
		    pData_ = (const uint8*)(pHeader_ + 1);
		    srcStride_ = (pHeader_->width_ / 8) +
			    ((pHeader_->width_ & 3) ? 1 : 0);
		    MF_ASSERT( pHeader_->magic_ == HolesHeader::MAGIC );
		    maxX_ = pHeader_->width_ - 1;
		    maxZ_ = pHeader_->height_ - 1;
	    }

        HoleHelper(uint32 w, uint32 h, uint32 stride, uint8 *data) :
          pBlock_(NULL),
          pHeader_(NULL),
          pData_(data),
          srcStride_(stride),
          maxX_(w - 1),
          maxZ_(h - 1)
        {
        }

	    const HolesHeader& header() const { return *pHeader_; }

	    bool hole( uint32 x, uint32 z )
	    {
		    z = std::min(z, maxZ_);
		    x = std::min(x, maxX_);
		    uint8 val =  pData_[z * srcStride_ + (x / 8)];
		    return (val & ( uint8(1) << (x & 0x7 ))) != 0;
	    }

	    uint64 block( uint32 x, uint32 z )
	    {
		    uint64 block = emptyBlock;
		    uint64 mask = uint64(1) << 32;
		    for (uint32 zz = 0; zz < 4; zz++)
		    {
			    for (uint32 xx = 0; xx < 4; xx++)
			    {
				    if (!hole(x + xx, z + zz))
				    {
					    block |= mask;
				    }
				    mask <<= 2;
			    }
		    }
		    return block;
	    }

#ifndef MF_SERVER
        bool updateTexture(ComObjectWrap<DX::Texture> &texture, uint32 w, uint32 h)
        {
            BW_GUARD;
			if (!texture)
                return false;

            D3DLOCKED_RECT rect;
			Moo::TextureLockWrapper textureLock( texture );
			if (SUCCEEDED(textureLock.lockRect( 0, &rect, NULL, 0 )))
			{
				uint64* pMapData = (uint64*)(rect.pBits);
				for (uint32 z = 0; z < h; z += 4)
				{
					for (uint32 x = 0; x < w; x+= 4)
					{
						*(pMapData++) = block( x, z );
					}
				}
				textureLock.unlockRect( 0 );
                return true;
            }
            else
            {
                return false;
            }
        }
#endif

        const uint8 * data() const { return pData_; }

        const uint8 * row(unsigned int y) { return pData_ + y*srcStride_; }

        uint32 width() const { return maxX_ + 1; }

        uint32 height() const { return maxZ_ + 1; }

        uint32 stride() const { return srcStride_; }

    private:
	    BinaryPtr			pBlock_;
	    const HolesHeader*	pHeader_;
	    const uint8*		pData_;
	    uint32				srcStride_;
	    uint32				maxX_;
	    uint32				maxZ_;
    };

}


/**
 *  This is the TerrainHoleMap2 default constructor.  This represents
 *  terrain without holes.
 *
 *  @param block        The block that the holes are for.
 */
/*explicit*/ TerrainHoleMap2::TerrainHoleMap2(CommonTerrainBlock2 &block):
	TerrainHoleMap(block.blockSize()),
    block_(&block),
    width_(0),
    height_(0),
    lockCount_(0),
    readOnly_(true),
    allHoles_(false),
    noHoles_(true),
#ifndef MF_SERVER
    texture_(NULL),
#endif
    holesSize_(0),
    holesMapSize_(0)
{
}


/**
 *  This is the TerrainHoleMap2 destructor.
 */
/*virtual*/ TerrainHoleMap2::~TerrainHoleMap2()
{
}


/**
 *  This function returns true if there are no holes in the hole map.
 *
 *  @returns            True if there are no holes in the hole map.
 */
/*virtual*/ bool TerrainHoleMap2::noHoles() const
{
    return noHoles_;
}


/**
 *  This function returns true if the hole map is completely holes.
 */
/*virtual*/ bool TerrainHoleMap2::allHoles() const
{
    return allHoles_;
}


/**
 *  This function returns the width of the hole map.
 *
 *  @returns            The width of the hole map.
 */
/*virtual*/ uint32 TerrainHoleMap2::width() const
{
    return width_;
}


/**
 *  This function returns the height of the hole map.
 *
 *  @returns            The height of the hole map.
 */
/*virtual*/ uint32 TerrainHoleMap2::height() const
{
    return height_;
}



/**
 *  This function locks the hole map into memory.
 *
 *  @param readOnly     If true then the map is read but not written to.
 */
/*virtual*/ bool TerrainHoleMap2::lock( bool readOnly )
{
    BW_GUARD;
	// Make sure that it wasn't originaly locked for readOnly and
	// trying to nest a non-readOnly lock:
    MF_ASSERT(lockCount_ == 0 ||
		(lockCount_ != 0 &&
			( readOnly_ == readOnly || readOnly_ == false ) ));

    if (lockCount_ == 0 && image_.isEmpty() && width_ != 0 && height_ != 0)
    {
        // If the image was empty because there are no holes or everything
        // was a hole then create the image.
        image_.resize(width_, height_, allHoles_);
    }

	if ( lockCount_ == 0 )
		readOnly_ = readOnly;

    ++lockCount_;
    return true;
}


/**
 *  This function returns the underlying image of the hole map.
 *
 *  @returns            The underlying image of the hole map.
 */
/*virtual*/ TerrainHoleMap2::ImageType &TerrainHoleMap2::image()
{
    BW_GUARD;
	MF_ASSERT(lockCount_ != 0);
    return image_;
}


/**
 *  This function returns the underlying image of the hole map.
 *
 *  @returns            The underlying image of the hole map.
 */
/*virtual*/ TerrainHoleMap2::ImageType const &TerrainHoleMap2::image() const
{
    BW_GUARD;
	MF_ASSERT(lockCount_ != 0);
    return image_;
}


/**
 *  This function unlocks the hole map.
 */
/*virtual*/ bool TerrainHoleMap2::unlock()
{
	BW_GUARD;
	bool ret = true;
    --lockCount_;
    if (lockCount_ == 0)
    {
        if (!readOnly_)
        {
            recalcHoles(); // also clears image if all holes or no holes.
#ifndef MF_SERVER
			if (!allHoles_ && !noHoles_)
			{
				if(texture_.hasComObject() || createTexture())
				{
					updateTexture();
				}
				else
				{
					ERROR_MSG( "TerrainHoleMap2::unlock - unable to create hole texture\n" );
					ret = false;
				}
			}
#endif
        }
        readOnly_ = true;
    }
    return ret;
}


/**
 *  This function saves the hole map to a DataSection.
 *
 *  @param  pSection      The DataSection to save to.
 *
 *  @return true on successful save, false otherwise.
 */
/*virtual*/ bool TerrainHoleMap2::save( DataSectionPtr pSection ) const
{
    BW_GUARD;
	MF_ASSERT( pSection );

    TerrainHoleMap2 *myself = const_cast<TerrainHoleMap2 *>(this);

	if (myself->lock(true))
	{
		BW::vector<uint8> data;
		data.resize( sizeof(HolesHeader) + image_.rawDataSize(), 0 );

		HolesHeader* hh = (HolesHeader*)(&data.front());
		hh->magic_ = HolesHeader::MAGIC;
		hh->width_ = image_.width();
		hh->height_ = image_.height();
		hh->version_ = 1;

		bool* pHoles = (bool*)(hh + 1);
		image_.copyTo(pHoles);

		BinaryPtr binaryBlock = 
			new BinaryBlock(&data.front(), data.size(), "BinaryBlock/TerrainHoleMap2");
		BinaryPtr pCompressed = binaryBlock->compress();
		pCompressed->canZip( false ); // don't recompress!
		pSection->setBinary( pCompressed );

		myself->unlock();
	}

    return true;
}


/**
 *  This function loads a hole map saved with TerrainHoleMap2::Save.
 *
 *  @param dataSection  The DataSection to load from.
 *  @param error    If not null and an error occurs, returns the error string
 *
 *  @returns        true if successful, false otherwise
 */
/*virtual*/ bool TerrainHoleMap2::load(DataSectionPtr dataSection, BW::string *error/* = NULL*/)
{
	BW_GUARD;
	bool ret    = false;

#ifndef MF_SERVER

	texture_        = NULL;

#endif
    width_          = 0;
    height_         = 0;
    allHoles_       = false;
    noHoles_        = true;
    holesSize_      = 0;
    holesMapSize_   = 0;

	BinaryPtr pHolesData = dataSection->asBinary();

	if (pHolesData)
	{
		if (pHolesData->isCompressed())
			pHolesData = pHolesData->decompress();

		HoleHelper helper(pHolesData);

		holesSize_ = helper.header().width_;
		holesMapSize_ = BW::largerPow2( holesSize_ );

#ifndef MF_SERVER
		if (createTexture())
		{
            if (helper.updateTexture(texture_, holesMapSize_, holesMapSize_))
#endif
            {
				image_ = Moo::Image<bool>(helper.header().width_, helper.header().height_);
                uint32 sz = std::min(helper.stride(), (uint32)image_.rawRowSize());
                for (uint32 z = 0; z < helper.header().height_; ++z)
				{
                    uint8 *dest = image_.getRawRow(z);
                    uint8 const *src = helper.row(z);
                    ::memcpy(dest, src, sz);
                }
                width_  = helper.header().width_;
                height_ = helper.header().height_;
                recalcHoles();
				ret = true;
			}
#ifndef MF_SERVER
		}
		else
		{
			holesSize_ = 0;
			holesMapSize_ = 0;
			texture_ = NULL;
            if (error != NULL)
                *error = "Unable to create hole texture";
		}
#endif
	}
    else
    {
        if (error != NULL)
            *error = "NULL DataSection";
    }
	return ret;
}


/**
 *  This function creates an empty (no holes) hole map
 *
 *  @param size     Width and depth of map to create.
 *  @param error    If not null and an error occurs, returns the error string
 *
 *  @returns        true if successful
 */
bool TerrainHoleMap2::create( uint32 size, BW::string *error/* = NULL*/ )
{
	BW_GUARD;
	uint32 hsize = BW::largerPow2( size );

	holesSize_ = size;
	holesMapSize_ = hsize;

	width_  = size;
	height_ = size;

	allHoles_ = false;
	noHoles_ = true;

    image_.clear();
#ifndef MF_SERVER

	texture_ = NULL;
#endif

	return true;
}


/**
 *  This function gets the block that the hole map is on.
 *
 *  @returns            The block that the hole map is on.
 */
/*virtual*/ BaseTerrainBlock &TerrainHoleMap2::baseTerrainBlock()
{
    return *block_;
}


/**
 *  This function gets the block that the hole map is on.
 *
 *  @returns            The block that the hole map is on.
 */
/*virtual*/ BaseTerrainBlock const &TerrainHoleMap2::baseTerrainBlock() const
{
    return *block_;
}


#ifndef MF_SERVER
/**
 *  This function returns the hole map texture.
 *
 *  @returns            The hole map texture.
 */
DX::Texture *TerrainHoleMap2::texture() const
{
    return texture_ != NULL ? texture_.pComObject() : NULL;
}
#endif


/**
 *  This function returns the size of the texture in the hole map.
 *
 *  @returns            The size of the texture in the hole map.
 */
uint32 TerrainHoleMap2::holesMapSize() const
{
    return holesMapSize_;
}


/**
 *  This function returns the size of the in the hole map.
 *
 *  @returns            The size of the in the hole map.
 */
uint32 TerrainHoleMap2::holesSize() const
{
    return holesSize_;
}


#ifndef MF_SERVER
/**
 *  This creates the actual DX texture for the holes map
 *
 *  @returns	true if successful, false otherwise
 */
bool TerrainHoleMap2::createTexture()
{
	BW_GUARD;
	texture_ = NULL; // Make sure any existing texture is released

	texture_ = Moo::rc().createTexture(	holesMapSize_, holesMapSize_,
		1, 0, D3DFMT_DXT1, D3DPOOL_MANAGED, "Terrain/HoleMap2/Texture" );

	return !!texture_;
}


/**
 *  This gets called to synchronise the DirectX hole map texture with the
 *  image.
 */
void TerrainHoleMap2::updateTexture()
{
    BW_GUARD;
	if (!texture_ && !image_.isEmpty())
	{
        MF_ASSERT(0); // TODO: create texture
	}

    if (texture_ != NULL && !image_.isEmpty())
    {
        HoleHelper
            helper
            (
                image_.width(),
                image_.height(),
                image_.rawRowSize(),
                image_.getRawRow(0)
            );
        helper.updateTexture(texture_, holesMapSize_, holesMapSize_);
    }
	// Add the texture to the preload list so that it can get uploaded
	// to video memory
	Moo::rc().addPreloadResource( texture_.pComObject() );
}
#endif


/**
 *  This recalculates allHoles_ and noHoles_ after unlocking and gets rid of
 *  the hole map if it is degenerate.
 */
void TerrainHoleMap2::recalcHoles()
{
    BW_GUARD;
	if (image_.isEmpty())
    {
        allHoles_ = false;
        noHoles_  = true;
    }
    else
    {
        uint32 w = image_.width();
        uint32 h = image_.height();

        allHoles_ = true;
        bool holes = false;

        for (uint32 y = 0; y < h; ++y)
        {
            for (uint32 x = 0; x < w; ++x)
            {
                bool v = image_.get(x, y);
                allHoles_ &= v;
                holes     |= v;
            }
        }
        noHoles_ = !holes;
        if (noHoles_ || allHoles_)
		{
		    image_.clear();
#ifndef MF_SERVER

			texture_ = NULL;
#endif
		}
    }
}

/**
 *  This method checks if there is a hole at the x/z position
 *	@param x x position
 *	@param z z position
 *	@return true if there is a hole
 */
bool TerrainHoleMap2::holeAt(float x, float z) const
{
	if (allHoles_) return true;
	if (noHoles_) return false;

	return image_.get( int32((x / blockSize()) * float(width())),
		int32((z / blockSize()) * float(height())) );
}
/**
 *  This method checks if there is a hole at the x/z rect
 *	@param xs x start position
 *	@param zs z start position
 *	@param xe x end position
 *	@param ze z end position
 *	@return true if the rect intersects a hole
 */
bool TerrainHoleMap2::holeAt( float xs, float zs, float xe, float ze ) const
{
	if (allHoles_) return true;
	if (noHoles_) return false;

	int32 xStart = int32(floorf((xs / blockSize()) * float(width())));
	int32 xEnd = int32(floorf((xe / blockSize()) * float(width())));

	int32 zStart = int32(floorf((zs / blockSize()) * float(height())));
	int32 zEnd = int32(floorf((ze / blockSize()) * float(height())));

	for (int32 z = zStart; z <= zEnd; z++)
	{
		for (int32 x = xStart; x <= xEnd; x++)
		{
			if (image_.get(x, z))
			{
				return true;
			}
		}
	}

	return false;
}

BW_END_NAMESPACE

// terrain_hole_map2.cpp
