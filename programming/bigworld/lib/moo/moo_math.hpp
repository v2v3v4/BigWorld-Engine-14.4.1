#ifndef MOO_MATH_HPP
#define MOO_MATH_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"

#ifndef MF_SERVER
#include "d3dx9math.h"
#endif

#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "math/matrix.hpp"
#include "math/quat.hpp"

#include "math/angle.hpp"

#ifndef MF_SERVER

BW_BEGIN_NAMESPACE

class Quaternion;

namespace Moo
{
	typedef D3DXCOLOR				Colour;
	typedef D3DCOLOR				PackedColour;

	namespace PackedColours
	{
		static const PackedColour RED = 0xffff0000;
		static const PackedColour GREEN = 0xff00ff00;
		static const PackedColour BLUE = 0xff0000ff;
		static const PackedColour CYAN = 0xff00ffff;
		static const PackedColour MAGENTA = 0xffff00ff;
		static const PackedColour YELLOW = 0xffffff00;
		static const PackedColour WHITE = 0xffffffff;
		static const PackedColour BLACK = 0xff000000;
	}


	//-------------------------------------------------------------------------
	// Packing/Unpacking   float[3] <-> uint32  (version 1)
	//-------------------------------------------------------------------------

	/**
	 * Unpack compressed normal into a three float vector.
	 */
	inline Vector3 unpackNormal( uint32 packed )
	{
		int32 z = int32(packed) >> 22;
		int32 y = int32( packed << 10 ) >> 21;
		int32 x = int32( packed << 21 ) >> 21;

		return Vector3( float( x ) / 1023.f, float( y ) / 1023.f, float( z ) / 511.f );
	}

	/**
	 * Pack three float normal (each component clamped to [-1,1]) into a single
	 * unsigned 32bit word ( 11 bits x, 11 bits y, 10 bits z )
	 */
	inline uint32 packNormal( const Vector3& nn )
	{
		Vector3 n = nn;
		n.normalise();

		n.x = Math::clamp(-1.f, n.x, 1.f);
		n.y = Math::clamp(-1.f, n.y, 1.f);
		n.z = Math::clamp(-1.f, n.z, 1.f);


		return	( ( ( (uint32)(n.z * 511.0f) )  & 0x3ff ) << 22L ) |
				( ( ( (uint32)(n.y * 1023.0f) ) & 0x7ff ) << 11L ) |
				( ( ( (uint32)(n.x * 1023.0f) ) & 0x7ff ) <<  0L );
	}

	inline bool validateUnitLengthNormal( const Vector3 & norm )
	{
		float lengthSquared = norm.lengthSquared();
		if (almostZero(lengthSquared))
		{
			WARNING_MSG( 
				"Moo::validateUnitLengthNormal: zero-length normal data: "
				"(%.2f, %.2f, %.2f) length=%.2f, which will be normalised when packing.\n", 
				norm.x, norm.y, norm.z, sqrtf(lengthSquared) );
			return false;
		}
		else if (!almostEqual( lengthSquared, 1.0f ))
		{
			WARNING_MSG( 
				"Moo::validateUnitLengthNormal: potential data loss on non-unit length normal data: "
				"(%.2f, %.2f, %.2f) length=%.2f, which will be normalised when packing.\n", 
				norm.x, norm.y, norm.z, sqrtf(lengthSquared) );
			return false;
		}
		return true;
	}

	//-------------------------------------------------------------------------
	// Packing/Unpacking   float <-> int16
	//-------------------------------------------------------------------------

	//-- Used fixed point arithmetic. Layout [1 bit - 4 bits - 11 bits]
	//-- 1 sign bit, 4 bit for the int part, and 11 bits for the fractional part.
	//-- for more information see
	//-- http://users.livejournal.com/_winnie/272618.html?thread=3455722#t3455722
	inline int16 packFloatToInt16( float src )
	{
		return int16(src * 2047 + 0.5f);
	}

	inline bool validatePackingRangeFloatToInt16( float src )
	{
		//-- Check pack range.
		// Valid input range is [-16.0,16.0]. This is since (2^15)/2047 is 16. 
		// i.e. 2047 is (2^11)-1, so we shift src << 11, meaning any initial 
		// value beyond 4 bits in magnitude (+/- 16) will be truncated. 
		// (1 bit signed, 4 bits for int part, 11 bits for decimal part).

		if (fabs(src) > 16.0f)
		{
			WARNING_MSG( 
				"Moo::validatePackingRangeFloatToInt16: potential data loss of texcoord data. "
				"cannot pack %.2f to int16, outside [-16.0 to 16.0] range\n", src );
			return false;
		}
		return true;
	}

	//-- Packed float2 vector into 2 signed short values.
	//-- Used fixed point arithmetic. Layout [1 bit - 4 bits - 11 bits]
	//-- 1 sign bit, 4 bit for the int part, and 11 bits for the fractional part.
	inline float unpackInt16ToFloat(int16 src)
	{
		return src / 2047.f;
	}


	//-------------------------------------------------------------------------
	// Packing/Unpacking   float[3] <-> uint32  (version 2)
	//-------------------------------------------------------------------------

	//-- Unpack compressed ubyte4 normal into a three float vector.
	inline Vector3 unpackUByte4Normal888toFloat3(uint32 packed)
	{
		int32 z = (packed >> 16) & 0xFF;
		int32 y = (packed >>  8) & 0xFF;
		int32 x = (packed >>  0) & 0xFF;

		return Vector3((x - 127) / 127.0f, (y - 127) / 127.0f, (z - 127) / 127.0f);
	}

	//-- pack three float normal (each component clamped to [-1,1]) into
	//-- uint32( 8 bits unused, 8 bits x, 8 bits y, 8 bits z )
	inline uint32 packFloat3toUByte4Normal888(const Vector3 & unpacked)
	{
		Vector3 n = unpacked;
		n.normalise();

		n.x = Math::clamp(-1.f, n.x, 1.f);
		n.y = Math::clamp(-1.f, n.y, 1.f);
		n.z = Math::clamp(-1.f, n.z, 1.f);

		return	(((127 + (uint32)(n.z * 127)) & 0xFF) <<  16) |
			(((127 + (uint32)(n.y * 127)) & 0xFF) <<  8 ) |
			(((127 + (uint32)(n.x * 127)) & 0xFF) <<  0 );
	}


	//-------------------------------------------------------------------------
	// Conversion between packing schemes
	//-------------------------------------------------------------------------

	//-- convert normals packing scheme for uint32 from (8, 8, 8, X) to (11, 11, 10)
	inline uint32 convertUByte4Normal_8_8_8_to_11_11_10( uint32 src )
	{
		return packNormal( unpackUByte4Normal888toFloat3( src ) );
	}

	//-- convert normals packing scheme for uint32 from (11, 11, 10) to (8, 8, 8, X)
	inline uint32 convertUbyte4Normal_11_11_10_to_8_8_8( uint32 src )
	{
		return packFloat3toUByte4Normal888( unpackNormal( src ) );
	}


	//-------------------------------------------------------------------------
	// GPU Packed Vector types
	//-------------------------------------------------------------------------
	
	/**
	 * This class implements a packed float3 vector suitable for
	 * representing normals data. The data is stored using a
	 * uint32 ( 8 bits unused, 8 bits x, 8 bits y, 8 bits z)
	 * with each component clamped to [-1,1].
	 */
	class PackedVector3NormalUByte4_8_8_8
	{
	public:
		inline PackedVector3NormalUByte4_8_8_8()
		{
			m_data = 0;
		}

		inline PackedVector3NormalUByte4_8_8_8( float x, float y, float z )
		{
			m_data = packFloat3toUByte4Normal888( Vector3(x, y, z) );
		}

		inline PackedVector3NormalUByte4_8_8_8( float* data )
		{
			MF_ASSERT(data != 0);
			m_data = packFloat3toUByte4Normal888( Vector3(data[0], data[1], data[2]) );
		}

		inline PackedVector3NormalUByte4_8_8_8 & operator=( const Vector3& vec3 )
		{
			m_data = packFloat3toUByte4Normal888(vec3);
			return *this;
		}

		inline PackedVector3NormalUByte4_8_8_8( const Vector3 & in )
		{
			*this = in;
		}

		inline operator Vector3() const
		{
			return unpackUByte4Normal888toFloat3( m_data );
		}

		inline void setZero()
		{
			m_data = packFloat3toUByte4Normal888( Vector3(0, 0, 0) );
		}

	private:
		uint32 m_data;
	};

	/**
	 * This class implements a packed float2 vector suitable for
	 * representing 2D texture coordinates. The data is stored using 
	 * 2 signed short values. Each short uses fixed point arithmetic 
	 * with a layout of [1 bit - 4 bits - 11 bits].
	 *
	 *  - 1 sign bit.
	 *  - 4 bits for the int part.
	 *	- 11 bits for the fractional part.
	 */
	class PackedVector2TexcoordInt16_X2
	{
	public:
		inline PackedVector2TexcoordInt16_X2 & operator=( const Vector2 & in )
		{
			m_data[0] = packFloatToInt16( in.x );
			m_data[1] = packFloatToInt16( in.y );
			return *this;
		}

		inline PackedVector2TexcoordInt16_X2( const Vector2 & in )
		{
			*this = in;
		}

		inline PackedVector2TexcoordInt16_X2()
		{
		}

		inline operator Vector2() const
		{
			Vector2 v2;
			v2.x = unpackInt16ToFloat( m_data[0] );
			v2.y = unpackInt16ToFloat( m_data[1] );
			return v2;
		}

	private:
		int16 m_data[2];
	};

	// Half precision Vector 3
	class PackedVector3NormalFloat16_X4 : public D3DXVECTOR4_16F
	{
	public:
		inline PackedVector3NormalFloat16_X4& operator = (const Vector3& rt)
		{
			D3DXFloat32To16Array(&x, rt, 3);
			return *this;
		}

		inline operator Vector3() const
		{
			Vector3 v3;
			D3DXFloat16To32Array(v3, &x, 3);
			return v3;
		}
	};

	// Half precision Vector 2
	class PackedVector2TexcoordFloat16_X2 : public D3DXVECTOR2_16F
	{
	public:
		inline PackedVector2TexcoordFloat16_X2& operator = (const Vector2& rt)
		{
			D3DXFloat32To16Array(&x, rt, 2);
			return *this;
		}
		inline operator Vector2() const
		{
			Vector2 v2;
			D3DXFloat16To32Array(v2, &x, 2);
			return v2;
		}
	};


} // namespace Moo

BW_END_NAMESPACE

#endif



#endif
