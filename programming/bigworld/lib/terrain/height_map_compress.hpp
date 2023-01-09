#ifndef HEIGHT_MAP_COMPRESS_HPP
#define HEIGHT_MAP_COMPRESS_HPP

#include "moo/image.hpp"

BW_BEGIN_NAMESPACE

namespace Terrain
{
	/*
	 *	These methods compress and decompress an image of floats to and from a 
	 *	BinaryBlock.  The heights can be quantised, and so there is a little 
	 *	loss of	precision.  Repeated compressions and decompressions however is 
	 *	stable.  The decompression method should be able to handle older 
	 *	formats but the	compression method always compresses using the best 
	 *	known compression method.  Currently the compression is done by 
	 *	quantising the heights to 1mm intervals and compressing the resultant 
	 *	integerised height map using PNG.  Typically this gives a compression 
	 *	ratio of 8:1.
	 */

	BinaryPtr compressHeightMap(Moo::Image<float> const &heightMap);
	bool decompressHeightMap(BinaryPtr data, Moo::Image<float> &heightMap);

	//-- compression/decompression functions use png compression algo for the direct compress
	//--	 a float value as a 32 bit RGBA color without quantization at all.
	//--	 For now it wasn't used because we found out the error in the quantization function in
	//--	 the above methods. After correction the above methods work also well.
	//--	 So this compression algo exists just to note that there are another good approaches. 		
	//----------------------------------------------------------------------------------------------
	BinaryPtr compressHeightMap2(const Moo::Image<float>& heightMap);
	bool decompressHeightMap2(BinaryPtr data, Moo::Image<float> &heightMap);
}

BW_END_NAMESPACE

#endif // HEIGHT_MAP_COMPRESS_HPP
