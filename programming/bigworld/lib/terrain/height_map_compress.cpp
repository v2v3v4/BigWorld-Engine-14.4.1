#include "pch.hpp"

#include "terrain/height_map_compress.hpp"

#include "moo/png.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	/**
	 *	This is a magic number for checking whether the data was compressed 
	 *	using quantised PNG.
	 */
	const uint32 QUANTISED_PNG_VERSION	= 0x71706e67;	// 'qpng'


	/**
	 *	The following are used to compress/decompress using quantised PNG.
	 *	Here we quantise each floating point height to integer coordinates on
	 *	a millimetre grid.  We then do a 32bpp PNG compression on the result.
	 */
	const float	QUANTISATION_LEVEL	= 0.001f;	// quantise heights to mm

	/**
	 *	This converts a height value to an int32.
	 *
	 *	@param h		The height value.
	 *	@return			The quantised height value.
	 */
	inline int32 quantise(float h)
	{
		return (int32)(h / QUANTISATION_LEVEL + (h > 0 ? 0.5f : -0.5f));
	}


	/**
	 *	This converts a quantised height value to a float.
	 *
	 *	@param q		The quantised height.
	 *	@return			The unquantised height.
	 */
	inline float unquantise(int32 q)
	{
		return q*QUANTISATION_LEVEL;
	}


	/**
	 *	This checks whether the data was compressed with quantised PNG.
	 *
	 *	@param data		The compressed data.
	 *	@return			True if the data was compressed as quantised PNG.
	 */
	bool isQuantisedPNG(BinaryPtr data)
	{
		BW_GUARD;
		if (!data || (size_t)data->len() <= sizeof(uint32))
			return false;

		uint32 magick = *(uint32 const*)data->data();
		return magick == QUANTISED_PNG_VERSION;
	}


	/**
	 *	This decompresses the height map using the quantised PNG.
	 *
	 *	@param data			The data to decompress.
	 *	@param heightMap	The decompressed height map.
	 *	@return			True upon successful decompression, false otherwise
	 */
	bool quantisedPNGDecompress(BinaryPtr data, Moo::Image<float> &heightMap)
	{
		BW_GUARD;
		if (!isQuantisedPNG(data))
			return false;

		// Skip past the header
		uint32 const *udata = (uint32 const *)data->data();
		++udata;

		BinaryPtr subdata = 
			new BinaryBlock
			(
				udata, 
				data->len() - sizeof(uint32), 
				"Terrain/HeightMapCompression/Image",
				data
			);

		// Decompress the quantised heights:
		Moo::PNGImageData pngData;
		if (!Moo::decompressPNG(subdata, pngData))
			return false;

		// Create a dummy image around the integerised height data:	
		Moo::Image<int32> 
			qimage
			(
				pngData.width_, pngData.height_, 
				(int32 *)pngData.data_, 
				false // only wrap the data
			);

		// Unquantise the heights:
		heightMap.resize(pngData.width_, pngData.height_);		
		for (uint32 y = 0; y < pngData.height_; ++y)
		{
			int32 const *src  = qimage   .getRow(y);
			float       *dst  = heightMap.getRow(y);
			float		*dend = dst + pngData.width_;
			for (; dst != dend; ++src, ++dst)
				*dst = unquantise(*src);
		}

		// Cleanup:
		bw_safe_delete_array(pngData.data_); 

		return true;
	}


	/**
	 *	This compresses the height map using quantised PNG.
	 *
	 *	@param heightMap	The height map to compress.
	 *	@return				The compressed height map in binary form.
	 */
	BinaryPtr quantisedPNGCompress(Moo::Image<float> const &heightMap)
	{
		BW_GUARD;
		if (heightMap.isEmpty())
			return NULL;

		uint32 width  = heightMap.width();
		uint32 height = heightMap.height();

		// Convert to a quantised image:
		Moo::Image<int32> qheightMap(width, height);	
		for (uint32 y = 0; y < height; ++y)
		{
			const float *src  = heightMap .getRow(y);
			int32       *dst  = qheightMap.getRow(y);
			int32		*dend = dst + width;
			for (; dst != dend; ++src, ++dst)
				*dst = quantise(*src);
		}

		// Return a PNG compression of the quantised heights:
		Moo::PNGImageData pngData;
		pngData.data_		= (uint8 *)qheightMap.getRow(0);
		pngData.width_		= width;
		pngData.height_		= height;
		pngData.bpp_		= 32;
		pngData.stride_		= width*sizeof(int32);
		pngData.upsideDown_	= false;
		BinaryPtr imgData = Moo::compressPNG(pngData, "Terrain/HeightMapCompression/Image");

		// Now add the header to the front:
		BW::vector<uint8> data(imgData->len() + sizeof(uint32));
		*((uint32 *)&data[0]) = QUANTISED_PNG_VERSION;
		::memcpy(&data[sizeof(uint32)], imgData->data(), imgData->len());

		// Return the result:
		return new BinaryBlock(&data[0], data.size(), "Terrain/HeightMapCompression/BinaryBlock");
	}
}


/**
 *	This compresses the height map into something that can be stored on disk.
 *
 *	@param heightMap	The height map to compress.
 *	@return			The compressed height map in binary form.
 */
BinaryPtr Terrain::compressHeightMap(Moo::Image<float> const &heightMap)
{
	BW_GUARD;
	return quantisedPNGCompress(heightMap);
}


/**
 *	This decompresses the height map.
 *
 *	@param data		The data to decompress.
 *	@param heightMap	The decompressed height map.
 *	@return			True upon successful decompression.
 */
bool Terrain::decompressHeightMap(BinaryPtr data, Moo::Image<float> &heightMap)
{
	BW_GUARD;
	if (isQuantisedPNG(data))
		return quantisedPNGDecompress(data, heightMap); 
	return false; // unknown format
}

//--------------------------------------------------------------------------------------------------
BinaryPtr Terrain::compressHeightMap2(const Moo::Image<float>& heightMap)
{
	BW_GUARD;
	if (heightMap.isEmpty())
		return NULL;

	uint32 width  = heightMap.width();
	uint32 height = heightMap.height();

	// Return a PNG compression of the quantised heights:
	Moo::PNGImageData pngData;
	pngData.data_		= (uint8 *)heightMap.getRow(0);
	pngData.width_		= width;
	pngData.height_		= height;
	pngData.bpp_		= 32;
	pngData.stride_		= width*sizeof(float);
	pngData.upsideDown_	= false;
	BinaryPtr imgData = Moo::compressPNG(pngData, "Terrain/HeightMapCompression/Image");

	// Now add the header to the front:
	BW::vector<uint8> data(imgData->len() + sizeof(uint32));
	*((uint32 *)&data[0]) = QUANTISED_PNG_VERSION;
	::memcpy(&data[sizeof(float)], imgData->data(), imgData->len());

	// Return the result:
	return new BinaryBlock(&data[0], data.size(), "Terrain/HeightMapCompression/BinaryBlock");
}

//--------------------------------------------------------------------------------------------------
bool Terrain::decompressHeightMap2(BinaryPtr data, Moo::Image<float> &heightMap)
{
	// Skip past the header
	const uint32* udata = reinterpret_cast<const uint32*>(data->data());
	++udata;

	BinaryPtr subdata = 
		new BinaryBlock
		(
		udata, 
		data->len() - sizeof(uint32), 
		"Terrain/HeightMapCompression/Image",
		data
		);

	// Decompress the heights:
	Moo::PNGImageData pngData;
	if (!Moo::decompressPNG(subdata, pngData))
		return false;

	heightMap.resize(pngData.width_, pngData.height_);	
	heightMap.copyFrom(reinterpret_cast<const float*>(pngData.data_));

	// Cleanup:
	delete[] pngData.data_; 
	pngData.data_ = NULL;

	return true;
}

BW_END_NAMESPACE

// height_map_compress.cpp
