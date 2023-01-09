#include "pch.hpp"

#include "png.hpp"

#include "png/png.h"
#include "png/pngstruct.h"
#include "png/pnginfo.h"

#include "cstdmf/concurrency.hpp"
#include "cstdmf/bw_memory.hpp"
#include "cstdmf/bw_vector.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE



namespace
{
	SimpleMutex			s_pngMutex;
}


namespace
{
/**
 *	This is used when reading from an in-memory PNG buffer to make sure that
 *	we don't overrun the end.
 */
struct PNGReadBuffer
{
	uint8				*begin_;
	uint8				*current_;
	uint8				*end_;
};

/**
 *	Wrap libPNG memory allocations with our memory tracking
 */
MEMTRACKER_DECLARE( ThirdParty_libPNG, "ThirdParty - libPNG", 0 );

/** 
 *	This is a libpng callback for allocating memory
 */
png_voidp pngMalloc( png_structp /*png_ptr*/, png_size_t size )
{
	MEMTRACKER_SCOPED( ThirdParty_libPNG );
	return bw_malloc( size );
}

/** 
 *	This is a libpng callback for freeing memory
 */
void pngFree( png_structp /*png_ptr*/, png_voidp ptr )
{
	MEMTRACKER_SCOPED( ThirdParty_libPNG );
	bw_free( ptr );
}

/**
 *	This is a libpng callback for writing out some compressed data.
 *
 *  @param pngPtr		Information about the PNG being compressed.
 *  @param bytes		A pointer to some compressed data.
 *  @param size			The size of the compressed data.
 */
void PNGAPI
pngWriteBuffer
(
	png_structp			pngPtr,
	png_bytep			bytes,
	png_size_t			size
)
{
	BW::vector<BinaryPtr> *buffers =
		(BW::vector<BinaryPtr> *)pngPtr->io_ptr;
	BinaryPtr buffer = new BinaryBlock(bytes, size, "BinaryBlock/PNGWriter");
	buffers->push_back(buffer);
}

/**
 *	This is a libpng callback to signify the end of writing compressed
 *  data.  We do nothing, but we still need this function to use the
 *	user call-backs in libpng.
 */
void PNGAPI	pngWriteFlush(png_structp /*pngPtr*/)
{
}

/**
 *	This is a libpng callback used during decompression.  It gets the
 *	decompresseor some raw data.  We just move forward through an
 *	in-memory block of memory.
 *
 *  @param pngPtr		Information about the PNG being decompressed.
 *  @param bytes		A pointer to a buffer to place compressed data.
 *  @param size			The requested size of the compressed data.
 */
void PNGAPI
pngReadBuffer
(
	png_structp			pngPtr,
	png_bytep			bytes,
	png_size_t			size
)
{
	BW_GUARD;
	PNGReadBuffer *buffer = (PNGReadBuffer *)pngPtr->io_ptr;
	IF_NOT_MF_ASSERT_DEV(buffer->current_ <= buffer->end_)
	{
		MF_EXIT( "pointer past end of buffer" );
	}
	::memcpy(bytes, buffer->current_, size);
	buffer->current_ += size;
}

/**
 *	This concatinates a collection of BinaryBlocks into one big
 *	BinaryBlock.  We do this because when compressing to a PNG the
 *	call-backs give back multiple small buffers.
 *
 *  @param buffers		A list of buffers to concatinate.
 *	@param resCntStr	A string for resource tracking.
 *  @returns			The concatinated buffers.
 */
BinaryPtr concatenate
(
	BW::vector<BinaryPtr>		const &buffers,
	BW::string					const &resCntStr
)
{
	BW_GUARD;
	uint32 totalSize = 0;
	for (size_t i = 0; i < buffers.size(); ++i)
		totalSize += buffers[i]->len();

	if (totalSize == 0)
		return NULL;

	BinaryPtr result = new BinaryBlock( NULL, totalSize, resCntStr.c_str() );
	uint8 *data = (uint8*)result->data();
	uint8 *p = data;
	for (size_t i = 0; i < buffers.size(); ++i)
	{
		::memcpy(p, buffers[i]->data(), buffers[i]->len());
		p += buffers[i]->len();
	}

	return result;
}

/**
 *	This returns true if x is a power of two.
 *
 *  @param x			The number to test.
 *  @returns			True if the number is a power of two, false
 *						otherwise.  Zero is not a power of two.
 */
bool isPower2(uint32 x)
{
	BW_GUARD;
	while (x != 0)
	{
		if (((x & 1) != 0) && ((x & ~(uint32)1) == 0))
			return true;
		else if ((x & 1) != 0)
			return false;
		x = x >> 1;
	}
	return false;
}


/**
 *  Error callback for png_error().  We have to throw an exception here because
 *  you aren't allowed to return from a libpng error function (i.e. expect a
 *  segfault very shortly if you attempt to continue execution after this).
 */
void onPNGError( png_structp png_ptr, png_const_charp err_msg )
{
	ERROR_MSG( "Got libpng error: %s\n", err_msg );
	throw int( 0 );
}


/**
 *  Warning callback for png_warning().
 */
void onPNGWarning( png_structp png_ptr, png_const_charp err_msg )
{
	WARNING_MSG( "Got libpng warning: %s\n", err_msg );
}

} // anonymous namespace

namespace Moo
{

/**
 *	This gives some sensible values to a PNGImageData.
 */
PNGImageData::PNGImageData():
	data_(NULL),
	width_(0),
	height_(0),
	bpp_(0),
	stride_(0),
	upsideDown_(false)
{
}


/**
 *	This compresses an image.
 *
 *  @param data			The image to compress.
 *	@param resCounter	A string used for resource tracking.
 *  @returns			The image as a BinaryBlock.
 */
BinaryPtr compressPNG(PNGImageData const &data,
						   BW::string const &resCounter)
{
	BW_GUARD;
	SimpleMutexHolder	holder(s_pngMutex);

	if (data.data_ == NULL || data.width_ == 0 || data.height_ == 0)
		return NULL;

	IF_NOT_MF_ASSERT_DEV(isPower2(data.bpp_) && data.bpp_ <= 32)
	{
		return NULL;
	}

	// Create the write and info structs:
	png_structp pngPtr = png_create_write_struct_2(PNG_LIBPNG_VER_STRING, 
		0, 0, 0, 0, pngMalloc, pngFree);
	if (pngPtr == NULL)
		return NULL;
	png_infop pngInfo = png_create_info_struct(pngPtr);
	if (pngInfo == NULL)
	{
		png_destroy_write_struct(&pngPtr, NULL);
		return NULL;
	}

	// Setup writing callbacks:
	BW::vector<BinaryPtr> outputBuffers;
	png_set_write_fn(pngPtr, &outputBuffers, pngWriteBuffer, pngWriteFlush);

	// Set error and warning callbacks
	png_voidp errorPtr = png_get_error_ptr( pngPtr );
	png_set_error_fn( pngPtr, errorPtr, onPNGError, onPNGWarning );

	// Let libpng know about the image format:
	int colourType = PNG_COLOR_TYPE_GRAY;
	int bpp        = data.bpp_;
	if (bpp == 32)
	{
		colourType = PNG_COLOR_TYPE_RGBA;
		bpp = 8; // per channel
	}
	png_set_IHDR
	(
		pngPtr,
		pngInfo,
		data.width_, data.height_, bpp,
		colourType,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT
	);
	png_set_pHYs
	(
		pngPtr,
		pngInfo,
		data.width_,
		data.height_,
		PNG_RESOLUTION_UNKNOWN
	);

	// Set a compression level of three.  This gives reasonable compression 
	// levels with good compression and decompression times.
	png_set_compression_level(pngPtr, 3);

	// Create the row pointers:
	png_byte **rowPointers = new png_byte*[data.height_];
	for (uint32 y = 0; y < data.height_; ++y)
	{
		if (data.upsideDown_)
			rowPointers[y] = data.data_ + data.stride_*(data.height_ - y - 1);
		else
			rowPointers[y] = data.data_ + data.stride_*y;
	}

	// Write the image:
	png_write_info(pngPtr, pngInfo);
	png_write_image(pngPtr, rowPointers);
	png_write_end(pngPtr, pngInfo);

	// Generate the result:
	BinaryPtr result = concatenate(outputBuffers, resCounter);

	// Cleanup:
	png_destroy_write_struct(&pngPtr, &pngInfo);
	bw_safe_delete_array(rowPointers);

	return result;
}


/**
 *	This decompresses an image.
 *
 *  @param pngData		The data to decompress.
 *  @param imgData		This is filled out with the decompressed image.
 *  @returns			True if succesfull, false if not successful.
 */
bool decompressPNG(BinaryPtr pngData, PNGImageData &imgData)
{
	BW_GUARD;
	PROFILE_FILE_SCOPED( decompressPNG );
	SimpleMutexHolder	holder(s_pngMutex);

	if (pngData == NULL)
	{
		imgData = PNGImageData();
		return false;
	}

	// Create the write and info structs:
	png_structp pngPtr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING,
		0, 0, 0, 0, pngMalloc, pngFree);
	if (pngPtr == NULL)
	{
		imgData = PNGImageData();
		return false;
	}
	png_infop pngInfo = png_create_info_struct(pngPtr);
	if (pngInfo == NULL)
	{
		png_destroy_read_struct(&pngPtr, NULL, NULL);
		imgData = PNGImageData();
		return false;
	}

	// Set the read call back:
	PNGReadBuffer readBuffer;
	readBuffer.begin_ = readBuffer.current_ = (uint8 *)pngData->data();
	readBuffer.end_ = readBuffer.begin_ + pngData->len();
	png_set_read_fn(pngPtr, (void *)&readBuffer, pngReadBuffer);

	// Set error and warning callbacks
	png_voidp errorPtr = png_get_error_ptr( pngPtr );
	png_set_error_fn( pngPtr, errorPtr, onPNGError, onPNGWarning );

	// Fill out imgData:
	try
	{
		png_read_info( pngPtr, pngInfo );

		imgData.width_		= imgData.stride_ = pngInfo->width;
		imgData.height_		= pngInfo->height;
		imgData.bpp_		= pngInfo->bit_depth*pngInfo->channels;

		uint32 bytesPP		= imgData.bpp_ / 8;

		imgData.data_		= new uint8[imgData.width_*imgData.height_*bytesPP];
		if (!imgData.data_)
		{
			png_destroy_read_struct( &pngPtr, &pngInfo, NULL );
			return false;
		}

		imgData.upsideDown_ = false;

		// Do the decompression:
		for (uint32 i = 0; i < pngInfo->height; ++i)
			png_read_row(pngPtr, imgData.data_ + i*imgData.width_*bytesPP, NULL);

		// Cleanup:
		png_destroy_read_struct(&pngPtr, &pngInfo, NULL);
	}
	catch (...)
	{
		png_destroy_read_struct( &pngPtr, &pngInfo, NULL );
		return false;
	}
	return true;
}

} // namespace Moo

BW_END_NAMESPACE


// png.cpp
