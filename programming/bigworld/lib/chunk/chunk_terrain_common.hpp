#ifndef CHUNK_TERRAIN_COMMON_HPP
#define CHUNK_TERRAIN_COMMON_HPP

#include "math/vector4.hpp"


BW_BEGIN_NAMESPACE

/**
 * Helper to convert a blend value to vector4
 */
inline Vector4 convertBlendToVec4( uint32 blend )
{
	Vector4 v;
	v.x = float( (blend & 0xff) - 0x80 );
	v.y = float( ((blend & 0xff00) >> 8) - 0x80 );
	v.z = float( ((blend & 0xff0000) >> 16) - 0x80 );
	v.w = float( blend >> 25 );
	return v;
}

/**
 * Helper to get the dominant blend index
 */
inline uint32 calcDominantBlend( uint32 blend1, uint32 blend2, uint32 blend3 )
{
	uint32 ind = 0;
	Vector4 v = convertBlendToVec4( blend1 ) + convertBlendToVec4( blend2 ) +
					convertBlendToVec4( blend3 );
	for (uint32 i = 1; i < 4; i++)
	{
		if (v[ind] < v[i])
			ind = i;
	}
	return ind;
}

BW_END_NAMESPACE

#endif /*CHUNK_TERRAIN_COMMON_HPP*/
