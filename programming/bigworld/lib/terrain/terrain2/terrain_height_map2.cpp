#include "pch.hpp"
#include "terrain_height_map2.hpp"
#include "../terrain_settings.hpp"
#include "../terrain_data.hpp"
#include "../terrain_collision_callback.hpp"
#include "../height_map_compress.hpp"
#include "physics2/worldtri.hpp"
#include "moo/png.hpp"
#include "cstdmf/concurrency.hpp"

#if BW_MULTITHREADED_TERRAIN_COLLIDE
#include <mutex>
#endif

DECLARE_DEBUG_COMPONENT2("Moo", 0)


BW_BEGIN_NAMESPACE

using namespace Terrain;

MEMTRACKER_DECLARE( Terrain, "Terrain", 0 );


#ifndef MF_SERVER
	#include "terrain_vertex_lod.hpp"
#endif


namespace
{
	THREADLOCAL( int ) s_optimiseCollisionAlongZAxis = 0;

	bool  disableQuadTree = false;

	const float EXTEND_BIAS = 0.001f;

	// This helper function extends a triangle along all it's edges by
	// the amount specified by extend bias.
	void extend( WorldTriangle& wt )
	{
		Vector3 a = wt.v1() - wt.v0();
		Vector3 b = wt.v2() - wt.v1();
		Vector3 c = wt.v0() - wt.v2();
		a *= EXTEND_BIAS;
		b *= EXTEND_BIAS;
		c *= EXTEND_BIAS;

		wt = WorldTriangle( wt.v0() - a + c,
			 wt.v1() - b + a,
			 wt.v2() - c + b, TRIANGLE_TERRAIN );
	}
}

/**
 * Construct a terrain height map.
 *
 * @param	size			the width of the visible part of the height map.
 * @param	lodLevel		the LOD level to apply to this terrain height map.
 * @param	unlockCallback	called when unlock() occurs.
 */
/*explicit*/ TerrainHeightMap2::TerrainHeightMap2( 
		float blockSize,
		uint32 size,
		uint32 lodLevel,
		UnlockCallback* unlockCallback 
	) :
	TerrainHeightMap(blockSize),
#ifdef EDITOR_ENABLED
	lockCount_(0),
	lockReadOnly_(true),
#endif
	heights_("Terrain/HeightMap2/Image"),
	minHeight_(0),
	maxHeight_(0),
	diagonalDistanceX4_(0.0f),
	unlockCallback_( unlockCallback ),
	quadTreeRoot_( NULL ),
	quadTreeData_( NULL ),
	numQuadTreeCells_( 0 ),
	visibleOffset_( lodLevel > 0 ? 0 : TerrainHeightMap2::DEFAULT_VISIBLE_OFFSET ),
	lodLevel_( lodLevel ),
	internalBlocksWidth_( 0 ),
	internalBlocksHeight_( 0 ),
	internalSpacingX_( 0.f ),
	internalSpacingZ_( 0.f ),
	isQuadTreeInUse_( 0 )
{
	BW_GUARD;

	MEMTRACKER_SCOPED(Terrain);
	// resize the image to be the correct size
	heights_.resize( size + visibleOffset_ * 2, size + visibleOffset_ * 2 );

	refreshInternalDimensions();

	sizeInBytes_ = sizeof(*this) + heights_.width()*heights_.height()*sizeof(float);

	// Track memory usage
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("Terrain/HeightMap2", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		(uint)(sizeof(*this)), 0)
}

/**
 *  This is the TerrainHeightMap2 destructor.
 */
/*virtual*/ TerrainHeightMap2::~TerrainHeightMap2()
{
	BW_GUARD;

	freeQuadTreeData();
	// Track memory usage
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("Terrain/HeightMap2", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
						 (uint)(sizeof(*this)), 0)
}

void	TerrainHeightMap2::allocateQuadTreeData() const
{
	MF_ASSERT( quadTreeData_ == NULL );
	// calculate how many quadtree cells do we need
	numQuadTreeCells_ = 1;
	uint32 sz = std::min( this->blocksWidth(), this->blocksHeight() );
	float threshold = this->quadtreeThreshold();

	int level = 1;
	while (sz > threshold)
	{
		sz /= 2;
		numQuadTreeCells_ += (int)pow( 4.0f, (float)level );
		level++;
	}
	// pre-allocate cell array
	quadTreeData_ = new TerrainQuadTreeCell[numQuadTreeCells_];

	// Track memory usage
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("Terrain/HeightMap2/QuadTree",
		(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER), numQuadTreeCells_ * sizeof(TerrainQuadTreeCell), 0 );
}

void	TerrainHeightMap2::freeQuadTreeData() const
{
	MF_ASSERT_DEV( isQuadTreeInUse_ == 0 );
	if (quadTreeData_)
	{
		delete[] quadTreeData_;
		quadTreeData_ = NULL;
		quadTreeRoot_ = NULL;
		numQuadTreeCells_ = 0;
	}

	MF_ASSERT( numQuadTreeCells_ == 0 );
	MF_ASSERT( quadTreeData_ == NULL );
	
	// Track memory usage
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("Terrain/HeightMap2/QuadTree",
		(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER), numQuadTreeCells_ * sizeof(TerrainQuadTreeCell), 0 );
}
/**
 *  This function returns the width of the TerrainMap.
 *
 *  @returns            The width of the TerrainMap.
 */
/*virtual*/ uint32 TerrainHeightMap2::width() const
{
    return heights_.width();
}


/**
 *  This function returns the height of the TerrainMap.
 *
 *  @returns            The height of the TerrainMap.
 */
/*virtual*/ uint32 TerrainHeightMap2::height() const
{
    return heights_.height();
}


#ifdef EDITOR_ENABLED
/**
 *  This function creates an TerrainHeightMap2 using the settings in the
 *  DataSection.
 *
 *  @param settings         DataSection containing the terrain settings.
 *  @param error		    Optional output parameter that receives an error
 *							message if an error occurs.
 *  @returns                True if successful, false otherwise.
 */
bool TerrainHeightMap2::create( uint32 size, BW::string *error /*= NULL*/ )
{
	BW_GUARD;
	if ( !powerOfTwo( size ) )
	{
		if ( error )
			*error = "Height map size is not a power of two.";
		return false;
	}

	// add space for the extra height values in the invisible area.
	size += internalVisibleOffset() * 2 + 1;

	minHeight_ = 0;
	maxHeight_ = 0;
	heights_.resize( size, size, 0 );

	refreshInternalDimensions();

	return true;
}


/**
 *  This function locks the TerrainMap to memory and enables it for
 *  editing.
 *
 *  @param readOnly     This is a hint to the locking.  If true then
 *                      we are only reading values from the map.
 *  @returns            True if the lock is successful, false
 *                      otherwise.
 */
/*virtual*/ bool TerrainHeightMap2::lock(bool readOnly)
{
    BW_GUARD;
	// Make sure that it wasn't originaly locked for readOnly and
	// trying to nest a non-readOnly lock:
    MF_ASSERT(lockCount_ == 0 ||
		(lockCount_ != 0 &&
			( lockReadOnly_ == readOnly || lockReadOnly_ == false ) ));

	if ( lockCount_ == 0 )
	    lockReadOnly_ = readOnly;
    ++lockCount_;
    return true;
}


/**
 *  This function accesses the TerrainMap as though it is an image.
 *  This function should only be called within a lock/unlock pair.
 *
 *  @returns            The TerrainMap as an image.
 */
/*virtual*/ TerrainHeightMap2::ImageType &TerrainHeightMap2::image()
{
    BW_GUARD;
	MF_ASSERT(lockCount_ != 0);
    return heights_;
}

/**
*  This function unlocks the TerrainMap.  It gives derived classes
*  the chance to repack the underlying data and/or reconstruct
*  DirectX objects.
*
*  @returns            True if the unlocking was successful.
*/
/*virtual*/ bool TerrainHeightMap2::unlock()
{
	BW_GUARD;
	--lockCount_;
	if (lockCount_ == 0)
	{
		if (!lockReadOnly_)
		{
			recalcMinMax();
			invalidateQuadTree();
			if ( unlockCallback_ )
				unlockCallback_->notify();
		}
	}
	return true;
}

/**
*  This function saves the underlying data to a DataSection that can
*  be read back in via load.  You should not call this between
*  lock/unlock pairs.
*
*/
bool TerrainHeightMap2::save( DataSectionPtr pSection ) const
{
	BW_GUARD;
	MF_ASSERT( pSection );

	// prior to saving heightmap, recalculate min/max heights based on real
	// data obtained from the heightmap. This will prevent issues when heightmap is saved
	// multiple times with loss of precision due to PNG compression and quantization
	// in compressHeightMap
	float realMinHeight = std::numeric_limits<float>::max();
	float realMaxHeight = -realMinHeight;

	const Moo::Image<float> &img = image();
	for (uint y = 0; y < img.height(); y++)
	{
		for (uint x = 0; x < img.width(); x++)
		{
			float h = img.get(x, y);
			realMinHeight = std::min<float>(realMinHeight, h);
			realMaxHeight = std::max<float>(realMaxHeight, h);
		}
	}

	TerrainHeightMap2 *localThis = const_cast<TerrainHeightMap2*>(this);
	localThis->minHeight_ = realMinHeight;
	localThis->maxHeight_ = realMaxHeight;

	// Create header at front of buffer
	HeightMapHeader hmh;
	::ZeroMemory(&hmh, sizeof(hmh));
	hmh.magic_       = HeightMapHeader::MAGIC;
	hmh.compression_ = COMPRESS_RAW;
	hmh.width_       = heights_.width();
	hmh.height_      = heights_.height();
	hmh.minHeight_   = minHeight_;
	hmh.maxHeight_   = maxHeight_;
	hmh.version_     = HeightMapHeader::VERSION_ABS_QFLOAT;

	BinaryPtr compHeight = compressHeightMap(heights_);

	BW::vector<uint8> data(sizeof(hmh) + compHeight->len());
	::memcpy(&data[          0], &hmh              , sizeof(hmh)      );
	::memcpy(&data[sizeof(hmh)], compHeight->data(), compHeight->len());

	// write to binary block
	BinaryPtr bdata = new BinaryBlock(&data[0], data.size(), "Terrain/HeightMap2/BinaryBlock");
	bdata->canZip( false ); // we are already compressed, don't recompress!
	pSection->setBinary( bdata );

	return true;
}

#endif // EDITOR_ENABLED

/**
*  This function accesses the TerrainMap as though it is an image.
*  This function can be called outside a lock/unlock pair.
*
*  @returns            The TerrainMap as an image.
*/
/*virtual*/ TerrainHeightMap2::ImageType const &TerrainHeightMap2::image() const
{
	return heights_;
}

/**
 *  This function loads the TerrainMap from a DataSection that was
 *  saved via TerrainMap::save.  You should not call this between
 *  lock/unlock pairs.
 *
 *  @param dataSection  The DataSection to load from.
 *  @param error		If not NULL and an error occurs then this will
 *                      be set to a reason why the error occurred.
 *  @returns            True if the load was successful.
 */
/*virtual*/ bool
TerrainHeightMap2::load
(
    DataSectionPtr	dataSection,
    BW::string*	error  /*= NULL*/
)
{
	BW_GUARD;
	BinaryPtr pBin = dataSection->asBinary();
	if (!pBin)
	{
		if ( error ) *error = "dataSection is not binary";
		return false;
	}

	if (pBin->isCompressed())
		pBin = pBin->decompress();

	return loadHeightMap( pBin->data(), pBin->len(), error );
}

bool TerrainHeightMap2::loadHeightMap(	const void*		data, 
										size_t			length,
										BW::string*	error /*= NULL*/)
{
	BW_GUARD;
	PROFILER_SCOPED( loadHeightMap );
	const HeightMapHeader* header = (const HeightMapHeader*) data;

	if ( header->magic_ != HeightMapHeader::MAGIC )
	{
		if ( error ) *error = "dataSection is not for a TerrainHeightMap2";
		return false;
	}

	if ( header->version_ == HeightMapHeader::VERSION_REL_UINT16_PNG )
	{
		// Height map is stored in compressed format, we'll need to convert the
		// heights back to floats.
		maxHeight_ = convertHeight2Float( *((int32 *)&header->maxHeight_) );
		minHeight_ = convertHeight2Float( *((int32 *)&header->minHeight_) );
		if (maxHeight_ < minHeight_)
			std::swap(minHeight_, maxHeight_);

		// The compressed image data is immediately after the header
		// in the binary block
		BinaryPtr pImageData = 
			new BinaryBlock
			( 
				header + 1,
				length - sizeof( HeightMapHeader ),
				"Terrain/HeightMap2/BinaryBlock"
			);

		// Decompress the image, if this succeeds, we are good to go
		Moo::Image<uint16> image("Terrain/HeightMap2/Image");
		if( !decompressImage( pImageData, image ) )
		{
			if (error ) *error = "The height map data could not be decompressed.";
			return false;
		}

		const uint16* pRelHeights = image.getRow(0);
		heights_ = Moo::Image<float>( header->width_, header->height_, "Terrain/HeightMap2/Image" );

		for ( uint32 i = 0; i < header->height_ * header->width_; i++ )
		{
			*( heights_.getRow(0) + i ) =
				convertHeight2Float( *pRelHeights++ + *((int32 *)&header->minHeight_) );
		}
	}
	// support raw float loading if new format or in editor 
	else if ( header->version_ == HeightMapHeader::VERSION_ABS_FLOAT )
	{
		maxHeight_ = header->maxHeight_;
		minHeight_ = header->minHeight_;
		if (maxHeight_ < minHeight_)
			std::swap(minHeight_, maxHeight_);

		const float* pHeights = (const float*) (header + 1);
		heights_ = Moo::Image<float>(header->width_, header->height_, "Terrain/HeightMap2/Image");
		heights_.copyFrom(pHeights);
	}
	else if ( header->version_ == HeightMapHeader::VERSION_ABS_QFLOAT )
	{
		maxHeight_ = header->maxHeight_;
		minHeight_ = header->minHeight_;
		if (maxHeight_ < minHeight_)
			std::swap(minHeight_, maxHeight_);

		BinaryPtr compData = 
			new BinaryBlock((void *)(header + 1), length - sizeof(*header), "Terrain/HeightMap2/BinaryBlock");
		heights_ = Moo::Image<float>("Terrain/HeightMap2/Image");
		if (!decompressHeightMap(compData, heights_))
		{
			if (error ) *error = "The height map data could not be decompressed.";
			return false;
		}
		MF_ASSERT(heights_.width () == header->width_ );
		MF_ASSERT(heights_.height() == header->height_);

//-- debug compression/decompression error.
#if 0
		{
			uint32 width  = header->width_;
			uint32 height = header->height_;

			Moo::Image<float> image1(width, height, "TEST1");
			Moo::Image<float> image2(width, height, "TEST2");
			image1.blit(heights_);

			Moo::Image<float>* images[] = {&image1, &image2};

			const uint32 itersCount = 1000;
			uint32 ping = 0;
			for (uint32 iter = 0; iter < itersCount; ++iter)
			{
				BinaryPtr data = compressHeightMap(*images[ping]);
				MF_ASSERT(data.hasObject());

				ping = !ping;

				bool success = decompressHeightMap(data, *images[ping]);
				MF_ASSERT(success);
			}

			float	delta = 0;
			float	maxDelta = 0;
			Vector2 maxMinVal(-10000000.0f,+10000000.0f);
			for (uint32 i = 0; i < width; ++i)
			{
				for (uint32 j = 0; j < height; ++j)
				{
					float val1 = heights_.get(i, j);
					float val2 = image2.get(i, j);

					//-- find maximum delta.
					delta = abs(val1 - val2);
					if (delta > maxDelta)
						maxDelta = delta;

					//-- find minimum and maximum heights.
					maxMinVal.x = max(maxMinVal.x, max(val1, val2));
					maxMinVal.y = min(maxMinVal.y, min(val1, val2));
				}
			}

			ERROR_MSG("------------------------------------------------------------------------\n");
			ERROR_MSG("Compression<->decompression results:\n");
			ERROR_MSG("max height %f, min height %f", maxMinVal.x, maxMinVal.y);
			ERROR_MSG("Iters count: %d, maximal compress/decompress error: %f \n", itersCount, maxDelta);
			ERROR_MSG("------------------------------------------------------------------------\n");
		}
#endif
	}
	else
	{
		char buffer[256];
		bw_snprintf( buffer, ARRAY_SIZE(buffer),
			"Invalid TerrainHeightMap2 format %u."
			"You must re-export from World Editor.",
			header->version_ );
		if (error) *error = buffer;
		return false;
	}

	refreshInternalDimensions();

	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("Terrain/HeightMap2", 
		(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER), sizeInBytes_, 0);

	sizeInBytes_ = sizeof(*this) + heights_.width()*heights_.height()*sizeof(float);
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("Terrain/HeightMap2", 
		(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER), sizeInBytes_, 0);

	return true;
}

/**
 *  This function returns the spacing between each sample.
 *
 *  @returns            The spacing between each sample in the x-direction.
 */
/*virtual*/ float TerrainHeightMap2::spacingX() const
{
    return internalSpacingX();
}


/**
 *  This function returns the spacing between each sample.
 *
 *  @returns            The spacing between each sample in the z-direction.
 */
/*virtual*/ float TerrainHeightMap2::spacingZ() const
{
    return internalSpacingZ();
}


/**
 *  This function returns the width of the visible part of the
 *  TerrainMap in poles.
 *
 *  @returns            The width of the visible part of the
 *                      TerrainMap.
 */
/*virtual*/ uint32 TerrainHeightMap2::blocksWidth() const
{
    return internalBlocksWidth();
}


/**
 *  This function returns the height of the visible part of the
 *  TerrainMap in poles.
 *
 *  @returns            The height of the visible part of the
 *                      TerrainMap.
 */
/*virtual*/ uint32 TerrainHeightMap2::blocksHeight() const
{
    return internalBlocksHeight();
}


/**
 *  This function returns the number of vertices wide of the visible
 *  part of the TerrainMap.
 *
 *  @returns            The width of the visible part of the
 *                      TerrainMap in terms of vertices.
 */
/*virtual*/ uint32 TerrainHeightMap2::verticesWidth() const
{
    return heights_.width() - ( internalVisibleOffset() * 2 );
}


/**
 *  This function returns the number of vertices high of the visible
 *  part of the TerrainMap.
 *
 *  @returns            The height of the visible part of the
 *                      TerrainMap in terms of vertices.
 */
/*virtual*/ uint32 TerrainHeightMap2::verticesHeight() const
{
    return heights_.height() - ( internalVisibleOffset() * 2 );
}


/**
 *  This function returns the width of the TerrainMap, including
 *  non-visible portions.
 *
 *  @returns            The width of the TerrainMap, including
 *                      non-visible portions.
 */
/*virtual*/ uint32 TerrainHeightMap2::polesWidth() const
{
    return heights_.width();
}


/**
 *  This function returns the height of the TerrainMap, including
 *  non-visible portions.
 *
 *  @returns            The height of the TerrainMap, including
 *                      non-visible portions.
 */
/*virtual*/ uint32 TerrainHeightMap2::polesHeight() const
{
    return heights_.height();
}


/**
 *  This function returns the x-offset of the visible portion of the
 *  HeightMap.
 *
 *  @returns            The x-offset of the first visible column.
 */
/*virtual*/ uint32 TerrainHeightMap2::xVisibleOffset() const
{
    return internalVisibleOffset();
}


/**
 *  This function returns the z-offset of the visible portion of the
 *  HeightMap.
 *
 *  @returns            The z-offset of the first visible row.
 */
/*virtual*/ uint32 TerrainHeightMap2::zVisibleOffset() const
{
    return internalVisibleOffset();
}


/**
 *  This function returns the minimum height in the height map.
 *
 *  @returns            The minimum height in the height map.
 */
/*virtual*/ float TerrainHeightMap2::minHeight() const
{
    return minHeight_;
}


/**
 *  This function returns the maximum height in the height map.
 *
 *  @returns            The maximum height in the height map.
 */
/*virtual*/ float TerrainHeightMap2::maxHeight() const
{
    return maxHeight_;
}


/**
 *  This function gets the height of the terrain at (x, z).
 *
 *  @param x            The x coordinate.
 *  @param z            The z coordinate.
 *
 *  @returns            The height at (x, z).
 */
float TerrainHeightMap2::heightAt(int x, int z) const
{
    return internalHeightAt( x, z );
}


/**
 *  This function determines the height at the given xz position, interpolated
 *	to fit on the lod0 mesh.
 *
 *  @param x            The x coordinate to get the height at.
 *  @param z            The z coordinate to get the height at.
 *  @returns            The height at x, z.
 */
/*virtual*/ float TerrainHeightMap2::heightAt(float x, float z) const
{
	BW_GUARD;
	// calculate the x and z positions on the height map.
	float xs = (x / internalSpacingX()) + internalVisibleOffset();
	float zs = (z / internalSpacingZ()) + internalVisibleOffset();

	// calculate the fractional coverage for the x/z positions
	float xf = xs - floorf(xs);
	float zf = zs - floorf(zs);

	// find the height map start cell of this height
	int32 xOff = int32(floorf(xs));
	int32 zOff = int32(floorf(zs));

	float res = 0;

	// Work out the diagonals for the height map cell, neighbouring cells
	// have opposing diagonals.
	if ((xOff ^ zOff) & 1)
	{
		// the cells diagonal goes from top left to bottom right.
		// Get the heights for the diagonal
		float h01 = heights_.get( xOff, zOff + 1 );
		float h10 = heights_.get( xOff + 1, zOff );

		// Work out which triangle we are in and calculate the interpolated
		// height.
		if ((1.f - xf) > zf)
		{
			float h00 = heights_.get( xOff, zOff );
			res = h00 + (h10 - h00) * xf + (h01 - h00) * zf;
		}
		else
		{
			float h11 = heights_.get( xOff + 1, zOff + 1 );
			res = h11 + (h01 - h11) * (1.f - xf) + (h10 - h11) * (1.f - zf);
		}
	}
	else
	{
		// the cells diagonal goes from top left to bottom right.
		// Get the heights for the diagonal
		float h00 = heights_.get( xOff, zOff );
		float h11 = heights_.get( xOff + 1, zOff + 1 );

		// Work out which triangle we are in and calculate the interpolated
		// height.
		if (xf > zf)
		{
			float h10 = heights_.get( xOff + 1, zOff );
			res = h10 + (h00 - h10) * (1.f - xf) + (h11 - h10) * zf;
		}
		else
		{
			float h01 = heights_.get( xOff, zOff + 1 );
			res = h01 + (h11 - h01) * xf + (h00 - h01) * (1.f - zf);
		}
	}

	return res;
}


/**
 *  This function gets the normal at the given integer coordinates.
 *
 *  @param x            The x coordinate.
 *  @param z            The z coordinate.
 *  @returns            The normal at x, z.
 */
Vector3 TerrainHeightMap2::normalAt(int x, int z) const
{
	BW_GUARD;
	float spaceX = internalSpacingX();
	float spaceZ = internalSpacingZ();

	//This function will not work if the spacing is not
	//equal in the X and Z directions.
	MF_ASSERT( isEqual( spaceX, spaceZ ) );

	const float diagonalMultiplier = 0.7071067811f;// sqrt( 0.5 )

	Vector3 ret;

#if 0
	// Using the cached distance x 4 to avoid doing a sqrtf every time here.
	ret.y = diagonalDistanceX4_ + (spaceX * 2 + spaceZ * 2);
#else
	//-- Note: we use this stuff for the y component of the normal to add more power to the x and
	//--		  z components. By doing this we can make normal maps generated by WorldEditor almost
	//--		  with the same quality and contrast as WorldMachine does. My research points me out
	//--		  that BigWorld uses some fake formula for calculation normal maps for terrain, so
	//--		  there is no solid meaning about y component calculation.
	ret.y = (spaceX * 2 + spaceZ * 2);
#endif

	float val1 = 0.0f;
	float val2 = 0.0f;
	if (x - 1 < 0 || z - 1 < 0 ||
		x + 1 >= int( heights_.width() ) || z + 1 >= int( heights_.height() ))
	{
		// Slower path, need to clamp in one or more axes.
		ret.x = heights_.get( x-1, z ) - heights_.get( x+1, z );
		ret.z = heights_.get( x, z-1 ) - heights_.get( x, z+1 );

		val1 = heights_.get( x-1, z-1 ) - heights_.get( x+1, z+1 );
		val2 = heights_.get( x-1, z+1 ) - heights_.get( x+1, z-1 );
	}
	else
	{
		// Fast path, everything in range, no need to clamp
		const TerrainHeightMap2::PixelType * zMin = heights_.getRow( z - 1 ) + x;
		const TerrainHeightMap2::PixelType * zCen = heights_.getRow( z ) + x;
		const TerrainHeightMap2::PixelType * zMax = heights_.getRow( z + 1 ) + x;

		ret.x = *(zCen - 1) - *(zCen + 1);
		ret.z = *(zMin) - *(zMax);

		val1 = *(zMin - 1) - *(zMax + 1);
		val2 = *(zMax - 1) - *(zMin + 1);
	}

	val1 *= diagonalMultiplier;

	val2 *= diagonalMultiplier;

	ret.x += val1;
	ret.z += val1;

	ret.x += val2;
	ret.z -= val2;
	ret.normalise();

    return ret;
}


/**
 *  This function determines the normal at the given point.
 *
 *  @param x            The x coordinate to get the normal at.
 *  @param z            The z coordinate to get the normal at.
 *  @returns            The normal at x, z.
 */
/*virtual*/ Vector3 TerrainHeightMap2::normalAt(float x, float z) const
{
	BW_GUARD;
	float xf = x/internalSpacingX() + internalVisibleOffset();
	float zf = z/internalSpacingZ() + internalVisibleOffset();

	Vector3 ret;
    float xFrac = xf - ::floorf(xf);
    float zFrac = zf - ::floorf(zf);

	xf -= xFrac;
	zf -= zFrac;

	int xfi = (int)xf;
	int zfi = (int)zf;

	// We do a bilinear interpolation of the normals.  This is a quite
	// expensive operation.  We optimise the case were either the fractional
	// parts of the coordinates are zero since this occurs frequently.
	const float FRAC_EPSILON = 1e-6f;
	if (fabs(xFrac) < FRAC_EPSILON)
	{
		if (fabs(zFrac) < FRAC_EPSILON)
		{
			return normalAt(xfi, zfi);
		}
		else
		{
			Vector3 n1 = normalAt(xfi, zfi    );
			Vector3 n2 = normalAt(xfi, zfi + 1);
			n1 *= (1.0f - zFrac);
			n2 *= zFrac;
			ret = n1;
			ret += n2;
			ret.normalise();
			return ret;
		}
	}
	else if (fabs(zFrac) < FRAC_EPSILON)
	{
		Vector3 n1 = normalAt(xfi    , zfi);
		Vector3 n2 = normalAt(xfi + 1, zfi);
		n1 *= (1.0f - xFrac);
		n2 *= xFrac;
		ret = n1;
		ret += n2;
		ret.normalise();
		return ret;
	}

	Vector3 n1 = normalAt(xfi    , zfi    );
	Vector3 n2 = normalAt(xfi + 1, zfi    );
	Vector3 n3 = normalAt(xfi    , zfi + 1);
	Vector3 n4 = normalAt(xfi + 1, zfi + 1);

	n1 *= (1 - xFrac)*(1 - zFrac);
	n2 *= (    xFrac)*(1 - zFrac);
	n3 *= (1 - xFrac)*(	   zFrac);
	n4 *= (    xFrac)*(	   zFrac);

	ret  = n1;
	ret += n2;
	ret += n3;
	ret += n4;

	ret.normalise();
	return ret;
}


/**
 *  This function does line-segment intersection tests with the terrain.
 *
 *  @param start		The start of the line-segment.
 *  @param end			The end of the line-segment.
 *  @param pCallback    The terrain collision callback.
 */
bool TerrainHeightMap2::collide
(
    Vector3             const &start,
    Vector3             const &end,
    TerrainCollisionCallback *pCallback
) const
{
	BW_GUARD;
	// Check the distance traveled on the x and z axis if they are smaller
	// than the threshold use the straight collision method
	float xDist = fabsf(end.x - start.x);
	float zDist = fabsf(end.z - start.z);

	float quadtreeThreshold = this->quadtreeThreshold();

	if (s_optimiseCollisionAlongZAxis && almostEqual( start.z, end.z ) && pCallback)
	{
		return hmCollideAlongZAxis( start, start, end, pCallback );
	}

	if ((xDist < quadtreeThreshold &&
		zDist < quadtreeThreshold) ||
		disableQuadTree )
	{
		return hmCollide( start, start, end, pCallback );
	}

	// If the quad tree has not been initialised, init it
	ensureQuadTreeValid();
	
	BW_ATOMIC32_INC_AND_FETCH( &isQuadTreeInUse_ );
#if BW_MULTITHREADED_TERRAIN_COLLIDE
	bool result = quadTreeRoot_.load()->collide(start, end, this, pCallback);
#else
	bool result = quadTreeRoot_->collide(start, end, this, pCallback);
#endif
	BW_ATOMIC32_DEC_AND_FETCH( &isQuadTreeInUse_ );
	return result;
}

	
/**
 *  This function does prism intersection tests with the terrain.
 *
 *  @param start		The start of the prism.
 *  @param end			The first point on end triangle of prism.
 *  @param pCallback    The terrain collision callback.
 */
bool TerrainHeightMap2::collide
(
    WorldTriangle       const &start,
    Vector3             const &end,
    TerrainCollisionCallback *pCallback
) const
{
	BW_GUARD;
	Vector3 delta = end - start.v0();

	float quadtreeThreshold = this->quadtreeThreshold();

	// Check the distance covered on the x and z axis if they are smaller
	// than the threshold use the straight collision method
	if ((fabsf(delta.x) < quadtreeThreshold &&
		fabsf(delta.z) < quadtreeThreshold) ||
		disableQuadTree)
	{
		// if the initial check fits within the quadtree create a bounding box
		// that covers the prism and collide with the height map
		BoundingBox bb;
		bb.addBounds(start.v0());
		bb.addBounds(start.v1());
		bb.addBounds(start.v2());

		Vector3 bbSize = bb.maxBounds() - bb.minBounds();

		if ((fabsf(bbSize.x) < quadtreeThreshold &&
			fabsf(bbSize.z) < quadtreeThreshold)
			|| disableQuadTree)
		{
			bb.addBounds(end);
			bb.addBounds(start.v1() + delta);
			bb.addBounds(start.v2() + delta);

			return hmCollide(start, end, bb.minBounds().x, bb.minBounds().z,
				bb.maxBounds().x, bb.maxBounds().z, pCallback);
		}
	}

	// If the quad tree has not been initialised, init it
	ensureQuadTreeValid();

	BW_ATOMIC32_INC_AND_FETCH( &isQuadTreeInUse_ );
#if BW_MULTITHREADED_TERRAIN_COLLIDE
	bool result = quadTreeRoot_.load()->collide(start, end, this, pCallback);
#else
	bool result = quadTreeRoot_->collide(start, end, this, pCallback);
#endif
	BW_ATOMIC32_DEC_AND_FETCH( &isQuadTreeInUse_ );
	return result;
}


namespace // Anonymous
{
	// Epsilon value for comparing collision results
	const float HEIGHTMAP_COLLISION_EPSILON = 0.0001f;
}


/**
 *  This function does line-segment intersection tests with the terrain
 *	using the height map values
 *
 *  @param originalStart	The source of the unclipped line-segment.
 *  @param start			The start of the line-segment.
 *  @param end				The end of the line-segment.
 *  @param pCallback		The terrain collision callback.
 *	@return true if collision should stop
 */
bool TerrainHeightMap2::hmCollide
(
    Vector3             const &originalStart,
    Vector3             const &start,
    Vector3             const &end,
    TerrainCollisionCallback *pCallback
) const
{
	BW_GUARD;
	Vector3 s = start;
	Vector3 e = end;

    const float BOUNDING_BOX_EPSILON = 0.1f;

	BoundingBox bb( Vector3( 0, minHeight(), 0 ),
		Vector3( blockSize(), maxHeight(),
        blockSize() ) );

	const Vector3 gridAlign( float(blocksWidth()) / blockSize(), 1.f,
		float(blocksHeight()) / blockSize() );

	if (bb.clip(s, e, BOUNDING_BOX_EPSILON ))
	{
		s = s * gridAlign;
		e = e * gridAlign;

		Vector3 dir = e - s;
		if (!(isZero(dir.x) && isZero(dir.z)))
		{
			dir *= 1.f / sqrtf(dir.x * dir.x + dir.z * dir.z);
		}

		int32 gridX = int32(floorf(s.x));
		int32 gridZ = int32(floorf(s.z));

		int32 gridEndX = int32(floorf(e.x));
		int32 gridEndZ = int32(floorf(e.z));

		int32 gridDirX = 0;
		int32 gridDirZ = 0;

		if (dir.x < 0) gridDirX = -1; else if (dir.x > 0) gridDirX = 1;
		if (dir.z < 0) gridDirZ = -1; else if (dir.z > 0) gridDirZ = 1;

		while ((gridX != gridEndX &&
			!almostEqual( s.x, e.x, HEIGHTMAP_COLLISION_EPSILON)) ||
			(gridZ != gridEndZ &&
			!almostEqual( s.z, e.z, HEIGHTMAP_COLLISION_EPSILON )))
		{
			if (checkGrid( gridX, gridZ,
				originalStart, end, pCallback))
				return true;

			float xDistNextGrid = gridDirX < 0 ? float(gridX) - s.x : float(gridX + 1) - s.x;
			float xDistEnd = e.x - s.x;

			bool xEnding = fabsf(xDistEnd) < fabsf(xDistNextGrid);

			float xDist = xEnding ? xDistEnd : xDistNextGrid;

			float zDistNextGrid = gridDirZ < 0 ? float(gridZ) - s.z : float(gridZ + 1) - s.z;
			float zDistEnd = e.z - s.z;

			bool zEnding = fabsf(zDistEnd) < fabsf(zDistNextGrid);

			float zDist = zEnding ? zDistEnd : zDistNextGrid;

			float a = xDist / dir.x;
			float b = zDist / dir.z;

			if (gridDirZ == 0 ||
				a < b)
			{
				if (xEnding)
				{
					s = e;
				}
				else
				{
					gridX += gridDirX;
					s.x += a * dir.x;
					s.y += a * dir.y;
					s.z += a * dir.z;
				}
			}
			else
			{
				if (zEnding)
				{
					s = e;
				}
				else
				{
					gridZ += gridDirZ;
					s.x += b * dir.x;
					s.y += b * dir.y;
					s.z += b * dir.z;
				}
			}
		}
		if (checkGrid( gridX, gridZ,
			originalStart, end, pCallback))
			return true;
	}
	return false;
}

namespace  // unnamed
{
	// Helper for collideSegment.
	inline bool collideTerrainGrid(
		float& outT,
		Vector3& outTriNormal,
		const Vector3& segStart, const Vector3& segStop,
		const Vector3& gridAdj1, const Vector3& gridAdj2,
		const Vector3& gridPre, const Vector3& gridPost)
	{
		Vector3 segDelta(segStart - segStop);
		Vector3 adj1ToStart(segStart - gridAdj1);
		Vector3 adjEdge(gridAdj2 - gridAdj1);
		float d;

		Vector3 e;
		e.crossProduct(segDelta, adj1ToStart);
		float v = adjEdge.dotProduct(e);
		if(v >= 0.f)
		{
			Vector3 otherEdge(gridPost - gridAdj1);

			outTriNormal.crossProduct(otherEdge, adjEdge);
			d = segDelta.dotProduct(outTriNormal);
			if(d < 0.f || v > d)
				return false;

			float w = -otherEdge.dotProduct(e);
			if(w < 0.f || w + v > d)
				return false;

			outT = adj1ToStart.dotProduct(outTriNormal);
			if(outT < 0.f || outT > d)
				return false;
		}
		else
		{
			v = -v;
			Vector3 otherEdge(gridPre - gridAdj1);

			outTriNormal.crossProduct(adjEdge, otherEdge);
			d = segDelta.dotProduct(outTriNormal);
			if(d < 0.f || v > d)
				return false;

			float w = otherEdge.dotProduct(e);
			if(w < 0.f || w + v > d)
				return false;

			outT = adj1ToStart.dotProduct(outTriNormal);
			if(outT < 0.f || outT > d)
				return false;
		}

		outT /= d;
		return true;
	}

} // unnamed namespace

/**
 *	Optimized method for colliding short or near vertical segments with terrrain.
 *
 *	@param outCollDist	distance at wich collision occurs
 *	@param outTriNormal	normal of intersected triangle
 *	@param start		start point of segment
 *	@param dir			direction of segment
 *	@param dist			length of the segment, such that: end = start + dist * dir
 *	@return true if collision found
 */
bool TerrainHeightMap2::collideShortSegment(
	float& outCollDist,
	Vector3& outTriNormal,
	const Vector3& start,
	const Vector3& dir,
	float dist ) const
{
#define FAST_HEIGHT_AT(x, z)\
	heights_.getRow((z)+ internalVisibleOffset())[(x) + internalVisibleOffset()]

	Vector3 end = start + dir * dist;

	// TODO: reconsider using hardcoded constant
	const float INV_BLOCK_SIZE = 0.01f; // 1.f / BLOCK_SIZE_METRES
	float alignX = internalBlocksWidth() * INV_BLOCK_SIZE;
	float alignZ = internalBlocksHeight() * INV_BLOCK_SIZE;

	int gridBegX = int(start.x * alignX);
	int gridBegZ = int(start.z * alignZ);
	int gridEndX = int(end.x * alignX);
	int gridEndZ = int(end.z * alignZ);

	int tmp;
	if(gridBegX > gridEndX)
	{
		tmp = gridBegX;
		gridBegX = gridEndX;
		gridEndX = tmp;
	}
	gridBegX = std::max(gridBegX, 0);
	gridEndX = std::min(gridEndX, int(internalBlocksWidth() - 1));

	if(gridBegZ > gridEndZ)
	{
		tmp = gridBegZ;
		gridBegZ = gridEndZ;
		gridEndZ = tmp;
	}
	gridBegZ = std::max(gridBegZ, 0);
	gridEndZ = std::min(gridEndZ, int(internalBlocksHeight() - 1));

	float t = 0;
	Vector3 triNormal;
	bool gotCollision;

	Vector3 bottomLeft;
	Vector3 bottomRight;
	Vector3 topLeft;
	Vector3 topRight;

	for(int gridZ = gridBegZ; gridZ <= gridEndZ; ++gridZ)
	{
		bottomRight.set(
			float(gridBegX) * internalSpacingX(),
			FAST_HEIGHT_AT(gridBegX, gridZ),
			float(gridZ) * internalSpacingZ() );

		topRight.set(
			bottomRight.x,
			FAST_HEIGHT_AT(gridBegX, gridZ + 1),
			bottomRight.z + internalSpacingZ() );

		for(int gridX = gridBegX; gridX <= gridEndX; ++gridX)
		{
			bottomLeft = bottomRight;
			topLeft = topRight;

			bottomRight.x += internalSpacingX();
			bottomRight.y = FAST_HEIGHT_AT(gridX + 1, gridZ);

			topRight.x += internalSpacingX();
			topRight.y =  FAST_HEIGHT_AT(gridX + 1, gridZ + 1);

			// Create the world triangles for the collision intersection tests.
			if ((gridX ^ gridZ) & 1)
				gotCollision = collideTerrainGrid(
									t,
									triNormal,
									start,
									end,
									bottomRight,
									topLeft ,
									topRight,
									bottomLeft );
			else
				gotCollision = collideTerrainGrid(
									t,
									triNormal,
									start,
									end,
									bottomLeft,
									topRight,
									bottomRight,
									topLeft );

			if(gotCollision)
			{
				// TODO: may be search for all collisions, not only first detected.
				outCollDist = t * dist;
				outTriNormal = triNormal;
				return true;
			}
		}
	}

	return false;
#undef FAST_HEIGHT_AT
}

/**
 *	Visit triangles that are _may_ intersects with given oriented box.
 *	Triangles in back of given clipping plane are discarded,
 *	wich may significantly optimize selection in some cases (eg vehicle box above smooth ground).
 *
 *	@param outTris				triangles
 *	@param boxCenter			center of the box
 *	@param boxE1				box X-extent (half-edge)
 *	@param boxE2				box Y-extent
 *	@param boxE3				box Z-extent
 *	@param clippingPlanePt		point of clipping plane
 *	@param clippingPlaneNormal	normal of clipping plane
 */
void TerrainHeightMap2::visitTriangles(
	RealWTriangleSet& outTris,
	const Vector3& boxCenter,
	const Vector3& boxE1,
	const Vector3& boxE2,
	const Vector3& boxE3,
	const Vector3& clippingPlanePt,
	const Vector3& clippingPlaneNormal) const
{

#define FAST_HEIGHT_AT(x, z)\
	heights_.getRow((z) + internalVisibleOffset())[(x) + internalVisibleOffset()]

	const uint STATIC_BUFFER_SIZE = 40; // Seems to be enough.

	float dx = fabsf(boxE1.x) + fabsf(boxE2.x) + fabsf(boxE3.x);
	float dz = fabsf(boxE1.z) + fabsf(boxE2.z) + fabsf(boxE3.z);
	float spacingX = internalSpacingX();
	float spacingZ = internalSpacingZ();

	float invSpacingX = 1.f / spacingX;
	float invSpacingZ = 1.f / spacingZ;

	int xs = std::max(int((boxCenter.x - dx) * invSpacingX), 0);
	int xe = std::min(int((boxCenter.x + dx) * invSpacingX) + 1, int(internalBlocksWidth()));
	int zs = std::max(int((boxCenter.z - dz) * invSpacingZ), 0);
	int ze = std::min(int((boxCenter.z + dz) * invSpacingZ) + 1, int(internalBlocksHeight()));

	if (xs >= xe || zs >= ze)
		return;

	float sBuf[STATIC_BUFFER_SIZE];
	float* dBuf = NULL;
	float* prevDots;
	float* curDots;
	uint rowLength = xe - xs + 1;
	if((rowLength << 1) <= STATIC_BUFFER_SIZE)
	{
		prevDots = sBuf;
	}
	else
	{
		// TODO: reconsider using dynamic buffer
		dBuf = new float[rowLength << 1];
		prevDots = dBuf;
	}
	curDots = prevDots + rowLength;

	Vector3 bottomLeft;
	Vector3 bottomRight;
	Vector3 topLeft;
	Vector3 topRight;

	float clippingPlaneDot = clippingPlaneNormal.dotProduct(clippingPlanePt);
	float initX = float(xs) * spacingX;
	Vector3 v(
		initX,
		0.f,
		float(zs) * spacingZ);

	for (int x = xs; x <= xe; ++x)
	{
		v.y = FAST_HEIGHT_AT(x, zs);
		prevDots[x - xs] = clippingPlaneNormal.dotProduct(v) - clippingPlaneDot;
		v.x += spacingX;
	}

	for (int z = zs; z < ze; ++z)
	{
		v.x = initX;
		v.z += spacingZ;
		v.y = FAST_HEIGHT_AT(xs, z + 1);
		curDots[0] = clippingPlaneNormal.dotProduct(v) - clippingPlaneDot;
		for (int x = xs; x < xe; ++x)
		{
			v.x += spacingX;
			v.y = FAST_HEIGHT_AT(x + 1, z + 1);
			float trd = clippingPlaneNormal.dotProduct(v) - clippingPlaneDot;
			curDots[x + 1 - xs] = trd;

			float tld = curDots[x - xs];
			float brd = prevDots[x + 1 - xs];
			float bld = prevDots[x - xs];

			if((bld > 0.f) | (tld > 0.f) | (brd > 0.f) | (trd > 0.f))
			{
				topRight = v;

				bottomRight.set(
					v.x,
					FAST_HEIGHT_AT(x + 1, z),
					v.z - spacingZ );

				topLeft.set(
					v.x - spacingX,
					FAST_HEIGHT_AT(x, z + 1),
					v.z );

				bottomLeft.set(
					v.x - spacingX,
					FAST_HEIGHT_AT(x, z),
					v.z - spacingZ );


				if ((x ^ z) & 1)
				{
					if(bld > 0.f || tld > 0.f || brd > 0.f)
						outTris.push_back(WorldTriangle(bottomLeft, topLeft, bottomRight));

					if(tld > 0.f || trd > 0.f || brd > 0.f)
						outTris.push_back(WorldTriangle(topLeft, topRight, bottomRight));
				}
				else
				{
					if(bld > 0.f || tld > 0.f || trd > 0.f)
						outTris.push_back(WorldTriangle(bottomLeft, topLeft, topRight));

					if(trd > 0.f || brd > 0.f || bld > 0.f)
						outTris.push_back(WorldTriangle(topRight, bottomRight, bottomLeft));
				}
			}
		}

		float* tmp = prevDots;
		prevDots = curDots;
		curDots = tmp;
	}

	if(dBuf)
		delete[]dBuf;
#undef FAST_HEIGHT_AT
}

void TerrainHeightMap2::visitTriangles(RealWTriangleSet& outTris, const BoundingBox& bb) const
{
#define FAST_HEIGHT_AT(x, z)\
	heights_.getRow((z) + internalVisibleOffset())[(x) + internalVisibleOffset()]

	float spacingX = internalSpacingX();
	float spacingZ = internalSpacingZ();

	float invSpacingX = 1.f / spacingX;
	float invSpacingZ = 1.f / spacingZ;

	int xs = std::max(int((bb.minBounds().x) * invSpacingX), 0);
	int xe = std::min(int((bb.maxBounds().x) * invSpacingX) + 1, int(internalBlocksWidth()));
	int zs = std::max(int((bb.minBounds().z) * invSpacingZ), 0);
	int ze = std::min(int((bb.maxBounds().z) * invSpacingZ) + 1, int(internalBlocksHeight()));

	if (xs >= xe || zs >= ze)
		return;

	Vector3 bottomLeft;
	Vector3 bottomRight;
	Vector3 topLeft;
	Vector3 topRight;

	float initX = float(xs) * spacingX;
	Vector3 v(
		initX,
		0.f,
		float(zs) * spacingZ);

	for (int z = zs; z < ze; ++z)
	{
		v.x = initX;
		v.z += spacingZ;
		v.y = FAST_HEIGHT_AT(xs, z + 1);
		for (int x = xs; x < xe; ++x)
		{
			v.x += spacingX;
			v.y = FAST_HEIGHT_AT(x + 1, z + 1);

			topRight = v;

			bottomRight.set(
				v.x,
				FAST_HEIGHT_AT(x + 1, z),
				v.z - spacingZ );

			topLeft.set(
				v.x - spacingX,
				FAST_HEIGHT_AT(x, z + 1),
				v.z );

			bottomLeft.set(
				v.x - spacingX,
				FAST_HEIGHT_AT(x, z),
				v.z - spacingZ );


			if ((x ^ z) & 1)
			{
				outTris.push_back(WorldTriangle(bottomLeft, topLeft, bottomRight));
				outTris.push_back(WorldTriangle(topLeft, topRight, bottomRight));
			}
			else
			{
				outTris.push_back(WorldTriangle(bottomLeft, topLeft, topRight));
				outTris.push_back(WorldTriangle(topRight, bottomRight, bottomLeft));
			}
		}
	}

#undef FAST_HEIGHT_AT
}

/**
 *  This function does line-segment intersection tests with the terrain
 *	using the height map values along Z axis. We have several optimisations
 *	into it:
 *	a. Remove quad tree searching because it is useless for collision along
 *		Z axis.
 *	b. Check for height range before doing the costly world triangle test.
 *	c. Don't extend the triangle before testing to speed up a bit.
 *
 *  @param originalStart	The source of the unclipped line-segment.
 *  @param start			The start of the line-segment.
 *  @param end				The end of the line-segment.
 *  @param pCallback		The terrain collision callback.
 *	@return true if collision should stop
 */
bool TerrainHeightMap2::hmCollideAlongZAxis
(
    Vector3             const &originalStart,
    Vector3             const &start,
    Vector3             const &end,
    TerrainCollisionCallback *pCallback
) const
{
	BW_GUARD;

	Vector3 s = start;
	Vector3 e = end;

    const float BOUNDING_BOX_EPSILON = 0.1f;

	BoundingBox bb( Vector3( 0, minHeight(), 0 ),
		Vector3( blockSize(), maxHeight(),
        blockSize() ) );

	const Vector3 gridAlign( float(blocksWidth()) / blockSize(), 1.f,
		float(blocksHeight()) / blockSize() );

	if (bb.clip(s, e, BOUNDING_BOX_EPSILON ))
	{
		s = s * gridAlign;
		e = e * gridAlign;

		int32 gridX = int32(floorf(s.x));
		int32 gridEndX = int32(floorf(e.x));
		int32 gridDirX;

		int32 gridZ = int32(floorf(s.z));

		if (e.x - s.x < 0)
		{
			gridDirX = -1;
		}
		else
		{
			gridDirX = 1;
		}

		// We only need one triangle because in terrain shadow calculation
		// we only care if it hits.
		WorldTriangle tri;
		Vector3 dir = end - originalStart;
		float dist = dir.length();

		dir *= 1.f / dist;
		gridEndX += gridDirX;

		const float* rowZ = heights_.getRow( gridZ + internalVisibleOffset() ) + gridX + internalVisibleOffset();
		const float* rowZPlusOne = heights_.getRow( gridZ + 1 + internalVisibleOffset() ) + gridX + internalVisibleOffset();

		// y = ax + b
		float a = ( end.y - start.y ) / ( end.x - start.x );
		float b = ( start.y * end.x - start.x * end.y ) / ( end.x - start.x );
		int adjust = ( a > 0 ) ? 1 : 0;

		for (; gridX != gridEndX; gridX += gridDirX)
		{
			float minY = a * ( float( gridX + adjust ) * internalSpacingX() ) + b;

			float bottomLeftHeight = *rowZ;
			float bottomRightHeight = rowZ[ 1 ];
			float topLeftHeight = *rowZPlusOne;
			float topRightHeight = rowZPlusOne[ 1 ];

			rowZ += gridDirX;
			rowZPlusOne += gridDirX;

			if (bottomLeftHeight < minY && bottomRightHeight < minY &&
				topLeftHeight < minY && topRightHeight < minY)
			{
				continue;
			}

			// Construct our four vertices that make up this grid cell
			Vector3 bottomLeft( float(gridX) * internalSpacingX(),
				bottomLeftHeight,
				float(gridZ) * internalSpacingZ() );

			Vector3 bottomRight = bottomLeft;
			bottomRight.x += internalSpacingX();
			bottomRight.y = bottomRightHeight;

			Vector3 topLeft = bottomLeft;
			topLeft.z += internalSpacingZ();
			topLeft.y = topLeftHeight;

			Vector3 topRight = topLeft;
			topRight.x += internalSpacingX();
			topRight.y = topRightHeight;

			// Create the world triangles for the collision intersection tests
			if ((gridX ^ gridZ) & 1)
			{
				tri = WorldTriangle( bottomLeft, topLeft, bottomRight, 
					TRIANGLE_TERRAIN );
			}
			else
			{
				tri = WorldTriangle( bottomLeft, topLeft, topRight, 
					TRIANGLE_TERRAIN );
			}

			// Check if we intersect with either triangle
			float dist1 = dist;

			if (tri.intersects( originalStart, dir, dist1 ) &&
				pCallback->collide( tri, dist1 ))
			{
				return true;
			}
			else
			{
				dist1 = dist;

				if ((gridX ^ gridZ) & 1)
				{
					tri = WorldTriangle( topLeft, topRight, bottomRight, 
						TRIANGLE_TERRAIN );
				}
				else
				{
					tri = WorldTriangle( topRight, bottomRight, bottomLeft, 
						TRIANGLE_TERRAIN );
				}

				if (tri.intersects( originalStart, dir, dist1 ) &&
					pCallback->collide( tri, dist1 ))
				{
					return true;
				}
			}
		}
	}
	return false;
}


/**
 *  This function collides a prism with a rect in the heightmap
 *
 *	@param triStart the start triangle of the prism
 *	@param triEnd the end location of the prism
 *	@param xStart the x start location of our rect
 *	@param zStart the z start location of our rect
 *	@param xEnd the x end location of our rect
 *	@param zEnd the z end location of our rect
 *	@param pCallback the collision callback
 *
 *	@return true if collision should stop, false otherwise.
 */
bool TerrainHeightMap2::hmCollide
(
	WorldTriangle		const &triStart,
	Vector3             const &triEnd,
	float				xStart,
	float				zStart,
	float				xEnd,
	float				zEnd,
	TerrainCollisionCallback *pCallback
) const
{
	BW_GUARD;
	const float xMul = 1.f / internalSpacingX();
	const float zMul = 1.f / internalSpacingZ();

	int xs = int(xStart * xMul);
	int xe = int(ceilf(xEnd * xMul));
	int zs = int(zStart * zMul);
	int ze = int(ceilf(zEnd * zMul));

	for (int z = zs; z < ze; z++)
	{
		for (int x = xs; x < xe; x++)
		{
			if ( checkGrid( x, z, triStart, triEnd, pCallback ) )
			{
				return true;
			}
		}
	}

	return false;
}


#ifdef EDITOR_ENABLED
/**
 *  This function recalculates the minimum and maximum values of the
 *  height map.
 */
void TerrainHeightMap2::recalcMinMax()
{
	BW_GUARD;
	maxHeight_ = -std::numeric_limits<ImageType::PixelType>::max();
    minHeight_ = +std::numeric_limits<ImageType::PixelType>::max();

    for (uint32 y = 0; y < heights_.height(); ++y)
    {
        const float *p = heights_.getRow(y);
        const float *q = p + heights_.width();
        while (p != q)
        {
            maxHeight_ = std::max(*p, maxHeight_);
            minHeight_ = std::min(*p, minHeight_);
            ++p;
        }
    }
}
#endif

/**
 *  This function recalculates the quad-tree.
 */
void TerrainHeightMap2::recalcQuadTree() const
{
    BW_GUARD;
	MEMTRACKER_SCOPED(Terrain);
	
	MF_ASSERT_DEV( isQuadTreeInUse_ == 0 );
	this->freeQuadTreeData();
	this->allocateQuadTreeData();
	TerrainQuadTreeCell* pEmptyCell = quadTreeData_;
	TerrainQuadTreeCell* pRoot = pEmptyCell++;
	
		// initialise all cells using pre-allocated memory
	pRoot->init
    (
        this,
        0, 0, blocksWidth(), blocksHeight(),
        0.f, 0.f, blockSize(), blockSize(),
		this->quadtreeThreshold(),
		&pEmptyCell, pRoot + numQuadTreeCells_
	);
}

/**
*  This function invalidates the quad-tree.
*/
void TerrainHeightMap2::invalidateQuadTree() const
{
	BW_GUARD;
	this->freeQuadTreeData();
}


/**
*  This function makes sure the quadtree is valid before anything uses it
*/
#if !BW_MULTITHREADED_TERRAIN_COLLIDE
void TerrainHeightMap2::ensureQuadTreeValid() const
{
	MF_ASSERT_DEV( MainThreadTracker::isCurrentThreadMain() );

	if (!quadTreeRoot_)
	{
		static SimpleMutex quadTreeMutex;
		SimpleMutexHolder smh( quadTreeMutex );

		recalcQuadTree();
		quadTreeRoot_ = quadTreeData_;
	}
}

#else

void TerrainHeightMap2::ensureQuadTreeValid() const
{
	TerrainQuadTreeCell* tmp = quadTreeRoot_.load();
	if (tmp == nullptr) 
	{
		static std::mutex quadTreeMutex;
		std::lock_guard<std::mutex> lock( quadTreeMutex );
		tmp = quadTreeRoot_.load();
		if (tmp == nullptr) 
		{
			recalcQuadTree();
			quadTreeRoot_.store(quadTreeData_);
		}
	}
}
#endif

/**
 *  Check to see if the given line-segment goes through any of the triangles
 *  at gridX, gridZ.
 *
 *  @param gridX        The x coordinate of the triangles.
 *  @param gridZ        The z coordinate of the triangles.
 *  @param start		The start of the line-segment.
 *  @param end			The end of the line-segment.
 *  @param pCallback    The collision callback.
 */
bool
TerrainHeightMap2::checkGrid
(
    int					gridX,
    int					gridZ,
    Vector3             const &start,
    Vector3             const &end,
    TerrainCollisionCallback* pCallback
) const
{
	BW_GUARD;
	bool res = false;

	if (gridX >= 0 && gridX < (int)verticesWidth() &&
		gridZ >= 0 && gridZ < (int)verticesHeight() )
	{
		// Construct our four vertices that make up this grid cell
		Vector3 bottomLeft( float(gridX) * internalSpacingX(),
			heightAt(gridX, gridZ),
			float(gridZ) * internalSpacingZ() );

		Vector3 bottomRight = bottomLeft;
		bottomRight.x += internalSpacingX();
		bottomRight.y = heightAt(gridX + 1, gridZ);

		Vector3 topLeft = bottomLeft;
		topLeft.z += spacingZ();
		topLeft.y =  heightAt(gridX, gridZ + 1);

		Vector3 topRight = topLeft;
		topRight.x += internalSpacingX();
		topRight.y =  heightAt(gridX + 1, gridZ + 1);

		// Construct our triangles
		WorldTriangle triA;
		WorldTriangle triB;

		// Create the world triangles for the collision intersection tests
		if ((gridX ^ gridZ) & 1)
		{
			triA = WorldTriangle( bottomLeft, topLeft, bottomRight, 
				TRIANGLE_TERRAIN );
			triB = WorldTriangle( topLeft, topRight, bottomRight, 
				TRIANGLE_TERRAIN );
		}
		else
		{
			triA = WorldTriangle( bottomLeft, topLeft, topRight, 
				TRIANGLE_TERRAIN );
			triB = WorldTriangle( topRight, bottomRight, bottomLeft, 
				TRIANGLE_TERRAIN );
		}

		// Extend the world triangles a bit to make sure edge cases give a 
		// positive result
		extend( triA );
		extend( triB );

		Vector3 dir = end - start;
		float dist = dir.length();
		float dist2 = dist;
		dir *= 1.f / dist;

		// Check if we intersect with either triangle
		bool intersectsA = triA.intersects( start, dir, dist );
		bool intersectsB = triB.intersects( start, dir, dist2 );

		// If we have a callback pass colliding triangles to it
		// We always pass in triangles from closest to furthest away
		// so that we can stop colliding if the callback only wants
		// near collisions.
		if (pCallback)
		{
			if (intersectsA && intersectsB)
			{
				if (dist < dist2)
				{
					res = pCallback->collide( triA, dist );
					if (!res)
					{
						res = pCallback->collide( triB, dist2 );
					}
				}
				else
				{
					res = pCallback->collide( triB, dist2 );
					if (!res)
					{
						res = pCallback->collide( triA, dist );
					}
				}
			}
			else if (intersectsA)
			{
				res = pCallback->collide( triA, dist );
			}
			else if (intersectsB)
			{
				res = pCallback->collide( triB, dist2 );
			}
		}
		else
		{
			res = intersectsA || intersectsB;
		}
	}

	return res;
}

/**
*  Check to see if the given prism goes through any of the triangles
*  at gridX, gridZ.
*
*  @param gridX			The x coordinate of the triangles.
*  @param gridZ			The z coordinate of the triangles.
*  @param start			The start of prism.
*  @param end			The first point on end triangle of prism.
*  @param pCallback		The collision callback.
*/
bool
TerrainHeightMap2::checkGrid
(
	int				gridX,
	int				gridZ,
	WorldTriangle	const &start,
	Vector3			const &end,
	TerrainCollisionCallback* pCallback
) const
{
	BW_GUARD;
	bool res = false;

	if (gridX >= 0 && gridX < (int)blocksWidth() &&
		gridZ >= 0 && gridZ < (int)blocksHeight() )
	{
		// Construct our four vertices that make up this grid cell
		Vector3 bottomLeft( float(gridX) * internalSpacingX(),
			internalHeightAt(gridX, gridZ),
			float(gridZ) * internalSpacingZ() );

		Vector3 bottomRight = bottomLeft;
		bottomRight.x += internalSpacingX();
		bottomRight.y = internalHeightAt(gridX + 1, gridZ);

		Vector3 topLeft = bottomLeft;
		topLeft.z += internalSpacingZ();
		topLeft.y =  internalHeightAt(gridX, gridZ + 1);

		Vector3 topRight = topLeft;
		topRight.x += internalSpacingX();
		topRight.y =  internalHeightAt(gridX + 1, gridZ + 1);

		// Construct our triangles
		WorldTriangle triA;
		WorldTriangle triB;

		if ((gridX ^ gridZ) & 1)
		{
			triA = WorldTriangle( bottomLeft, topLeft,
						bottomRight, TRIANGLE_TERRAIN );
			triB = WorldTriangle( topLeft, topRight,
						bottomRight, TRIANGLE_TERRAIN );
		}
		else
		{
			triA = WorldTriangle( bottomLeft, topLeft,
						topRight, TRIANGLE_TERRAIN );
			triB = WorldTriangle( topRight, bottomRight,
						bottomLeft, TRIANGLE_TERRAIN );
		}

		// Get the offset of other prism end for the intersection
		Vector3 offset = end - start.v0();

		// Check if we intersect with either triangle
		bool intersectsA = triA.intersects( start, offset );
		bool intersectsB = triB.intersects( start, offset );

		// TODO: calculate proper distance
		float dist = 0.f;

		// If we have a callback pass colliding triangles to it
		if (pCallback)
		{
			if (intersectsA && intersectsB)
			{
				res = pCallback->collide( triA, dist );
				if (!res)
				{
					res = pCallback->collide( triB, dist );
				}
			}
			else if (intersectsA)
			{
				res = pCallback->collide( triA, dist );
			}
			else if (intersectsB)
			{
				res = pCallback->collide( triB, dist );
			}
		}
		else
		{
			res = intersectsA || intersectsB;
		}
	}

	return res;
}

/**
 * Returns correct name for a height map, given base name and a lod index.
 */
BW::string TerrainHeightMap2::getHeightSectionName(const BW::string&	base, 
													uint32				lod )
{
	BW_GUARD;
	BW::string name = base;

	if ( lod > 0 )
	{
		char buffer[64];
		bw_snprintf( buffer, ARRAY_SIZE(buffer), "%u", lod );
		name += buffer;
	}

	return name;
}


/**
 * Enable/Disable the optimisation on collisions along zaxis
 */
void TerrainHeightMap2::optimiseCollisionAlongZAxis( bool enable )
{
	if (enable)
	{
		++s_optimiseCollisionAlongZAxis;
	}
	else
	{
		--s_optimiseCollisionAlongZAxis;
	}
}


#ifndef MF_SERVER

HeightMapResource::HeightMapResource(	float blockSize, 
									 const BW::string&	heightsSectionName, 
									 uint32				lodLevel ) :
terrainSectionName_( heightsSectionName ),
lodLevel_( lodLevel ),
blockSize_( blockSize )
{
	BW_GUARD;
}

bool HeightMapResource::load()
{
	BW_GUARD;

	BW::string heightSectionName = TerrainHeightMap2::getHeightSectionName(
											terrainSectionName_ + "/heights", 
											lodLevel_ );
	DataSectionPtr pHeights = 
		BWResource::instance().openSection( heightSectionName );

	if (!pHeights)
	{
		WARNING_MSG("Can't open heights section '%s'.\n", heightSectionName.c_str() );
		return false;
	}

	// Create the height map.
	TerrainHeightMap2* pDHM = new TerrainHeightMap2( blockSize_, 0, lodLevel_ );
	BW::string error;

	// load, then assign once fully loaded.
	if ( !pDHM->load( pHeights, &error ) )
	{
		WARNING_MSG("%s\n", error.c_str() );
		bw_safe_delete(pDHM);
		return false;
	}

	object_ = pDHM;

	return true;
}

#endif // MF_SERVER

BW_END_NAMESPACE

// terrain_height_map2.cpp
