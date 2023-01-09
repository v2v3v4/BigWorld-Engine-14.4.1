#ifndef COMPILED_TERRAIN2_TYPES_HPP
#define COMPILED_TERRAIN2_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"
#include "math/vector3.hpp"

#include "string_table_types.hpp"

namespace BW {
namespace CompiledSpace {

	namespace Terrain2Types
	{
		const FourCC FORMAT_MAGIC = FourCC( "BWT2" );
		const uint32 FORMAT_VERSION = 0x00000001;

		const uint32 INVALID_BLOCK_REFERENCE = uint32( ~0 );

		struct Header
		{
			float blockSize_;
			int32 gridMinX_;
			int32 gridMaxX_;
			int32 gridMinZ_;
			int32 gridMaxZ_;
		};

		struct BlockData
		{
			StringTableTypes::Index resourceID_;
			int16 x_;
			int16 z_;
		};

		inline int pointToGrid( float point, float gridSize )
		{
			return static_cast<int>( floorf( point / gridSize ) );
		}

		inline uint32 blockIndexFromPoint( const Vector3& point, 
			Header& header )
		{
			int32 xCoord = pointToGrid( point.x, header.blockSize_ );
			int32 zCoord = pointToGrid( point.z, header.blockSize_ );

			// Check bounds
			if (xCoord < header.gridMinX_ || 
				xCoord > header.gridMaxX_ ||
				zCoord < header.gridMinZ_ ||
				zCoord > header.gridMaxZ_)
			{
				return INVALID_BLOCK_REFERENCE;
			}

			// Normalize the coordinates to the min/max
			xCoord -= header.gridMinX_;
			zCoord -= header.gridMinZ_;
			int32 width = header.gridMaxX_ - header.gridMinX_ + 1;

			uint32 gridIndex = xCoord + zCoord * width;
			return gridIndex;
		}
	}

} // namespace CompiledSpace
} // namespace BW


#endif // SRTING_TABLE_TYPES_HPP
