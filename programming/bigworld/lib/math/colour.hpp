#ifndef COLOUR_HPP
#define COLOUR_HPP

#include "cstdmf/stdmf.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"

BW_BEGIN_NAMESPACE

namespace Colour
{

	inline
	uint32 getUint32( int r, int g, int b, int a = 0 )
	{
		r = r > 255 ? 255 : r < 0 ? 0 : r;
		g = g > 255 ? 255 : g < 0 ? 0 : g;
		b = b > 255 ? 255 : b < 0 ? 0 : b;
		a = a > 255 ? 255 : a < 0 ? 0 : a;

		return ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
	}

	inline
	uint32 getUint32NoClamp( int r, int g, int b, int a = 0 )
	{
		return ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
	}

	inline
	uint32 getUint32NoClamp( int* c )
	{
		return ( ( c[0] << 16 ) | ( c[1] << 8 ) | c[2] | ( c[3] << 24 ) );
	}

	inline
	uint32 getUint32( const Vector3 &colour, int a )
	{
		int r = int(colour[ 0 ]);
		int g = int(colour[ 1 ]);
		int b = int(colour[ 2 ]);

		r = r > 255 ? 255 : r < 0 ? 0 : r;
		g = g > 255 ? 255 : g < 0 ? 0 : g;
		b = b > 255 ? 255 : b < 0 ? 0 : b;
		a = a > 255 ? 255 : a < 0 ? 0 : a;

		return ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
	}

	inline
	uint32 getUint32NoClamp( const Vector3 &colour, int a )
	{
		return ( a << 24 ) | ( (uint32)colour[ 0 ] << 16 ) | ( (uint32)colour[ 1 ] << 8 ) | ( (uint32)colour[ 2 ] );
	}

	inline
	uint32 getUint32( const Vector3 &colour )
	{
		int r = int(colour[ 0 ]);
		int g = int(colour[ 1 ]);
		int b = int(colour[ 2 ]);

		r = r > 255 ? 255 : r < 0 ? 0 : r;
		g = g > 255 ? 255 : g < 0 ? 0 : g;
		b = b > 255 ? 255 : b < 0 ? 0 : b;

		return ( r << 16 ) | ( g << 8 ) | b;
	}

	inline
	uint32 getUint32NoClamp( const Vector3 &colour )
	{
		return ( (uint32)colour[ 0 ] << 16 ) | ( (uint32)colour[ 1 ] << 8 ) | ( (uint32)colour[ 2 ] );
	}

	inline
	uint32 getUint32FromNormalised( const Vector3 &colour )
	{
		int r = int(colour[ 0 ] * 255.0f);
		int g = int(colour[ 1 ] * 255.0f);
		int b = int(colour[ 2 ] * 255.0f);

		r = r > 255 ? 255 : r < 0 ? 0 : r;
		g = g > 255 ? 255 : g < 0 ? 0 : g;
		b = b > 255 ? 255 : b < 0 ? 0 : b;

		return ( r << 16 ) | ( g << 8 ) | b;
	}

	inline
	uint32 getUint32FromNormalisedNoClamp( const Vector3 &colour )
	{
		return ( (uint32)( colour[ 0 ] * 255 ) << 16 ) | ( (uint32)( colour[ 1 ] * 255 ) << 8 ) | ( (uint32)( colour[ 2 ] * 255 ) );
	}

	inline
	Vector3 getVector3( uint32 colour )
	{
		return Vector3( float((colour >> 16) & 255),
						float((colour >> 8)  & 255),
						float( colour        & 255));
	}

	inline
	Vector3 getVector3Normalised( uint32 colour )
	{
		Vector3 v(	float((colour >> 16) & 255),
					float((colour >>  8) & 255),
					float( colour        & 255));
		v /= 255;
		return v;		
	}

	inline
	uint32 getUint32( const Vector4 &colour )
	{
		int r = int(colour[ 0 ]);
		int g = int(colour[ 1 ]);
		int b = int(colour[ 2 ]);
		int a = int(colour[ 3 ]);

		r = r > 255 ? 255 : r < 0 ? 0 : r;
		g = g > 255 ? 255 : g < 0 ? 0 : g;
		b = b > 255 ? 255 : b < 0 ? 0 : b;
		a = a > 255 ? 255 : a < 0 ? 0 : a;

		return uint32(( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b);
	}

	inline
	uint32 getUint32NoClamp( const Vector4 &colour )
	{
		return ( (uint32)colour[ 3 ] << 24 ) | ( (uint32)colour[ 0 ] << 16 ) | ( (uint32)colour[ 1 ] << 8 ) | ( (uint32)colour[ 2 ] );
	}

	inline
	uint32 getUint32FromNormalised( const Vector4 &colour )
	{
		int r = int(colour[ 0 ] * 255.0);
		int g = int(colour[ 1 ] * 255.0);
		int b = int(colour[ 2 ] * 255.0);
		int a = int(colour[ 3 ] * 255.0);

		r = r > 255 ? 255 : r < 0 ? 0 : r;
		g = g > 255 ? 255 : g < 0 ? 0 : g;
		b = b > 255 ? 255 : b < 0 ? 0 : b;
		a = a > 255 ? 255 : a < 0 ? 0 : a;

		return ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
	}

	inline
	uint32 getUint32FromNormalisedNoClamp( const Vector4 &colour )
	{
		return ( (uint32)( colour[ 0 ] * 255 ) << 16 ) | ( (uint32)( colour[ 1 ] * 255 ) << 8 ) | ( (uint32)( colour[ 2 ] * 255 ) );
	}

	inline
	Vector4 getVector4( uint32 colour )
	{
		return Vector4(
			float( ( colour >> 16 ) & 255),
			float( ( colour >>  8 ) & 255),
			float(   colour         & 255),
			float( ( colour >> 24 ) & 255 ));
	}

	inline
	Vector4 getVector4Normalised( uint32 colour )
	{
		Vector4 v(
			float(( colour >> 16 ) & 255),
			float(( colour >>  8 ) & 255),
			float(  colour         & 255),
			float(( colour >> 24 ) & 255 ));
		v *= 1.f / 255;
		return v;
	}

	inline
	float luminance( const Vector4& colour )
	{
		return (0.3f*colour.x + 0.59f*colour.y + 0.11f*colour.z);
	}
};

BW_END_NAMESPACE

#endif // COLOUR.HPP
/* colour.hpp */
