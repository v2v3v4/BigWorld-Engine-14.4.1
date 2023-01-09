/**
 *	@file
 *
 *	@ingroup Math
 */

#ifndef VECTOR4_HPP
#define VECTOR4_HPP

#include <iostream>

#include "mathdef.hpp"
#include "vector3.hpp"
#include "xp_math.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"

#include <math.h>

BW_BEGIN_NAMESPACE

/**
 *	This class implements a vector of four floats.
 *
 *	@ingroup Math
 */
class BWENTITY_API Vector4 : public Vector4Base
{
public:
	Vector4();
	Vector4( float a, float b, float c, float d );
	explicit Vector4( const Vector4Base & v );
	Vector4( const Vector3 & v, float w );
#ifdef _WIN32
	Vector4( __m128 v4 );
#endif

	// Use the default compiler implementation for these methods. It is faster.
	//	Vector4( const Vector4& v);
	//	Vector4& operator = ( const Vector4& v );

	void setZero();
	void set( float a, float b, float c, float d ) ;
	void scale( const Vector4& v, float s );
	void scale( float s );
	void parallelMultiply( const Vector4& v );
	float length() const;
	float lengthSquared() const;
	void normalise();
	Vector4 unitVector() const;
	Outcode calculateOutcode() const;

	float dotProduct( const Vector4& v ) const;

	INLINE void operator += ( const Vector4& v);
	INLINE void operator -= ( const Vector4& v);
	INLINE void operator *= ( const Vector4& v);
	INLINE void operator /= ( const Vector4& v);
	INLINE void operator *= ( float s);

	BW::string desc() const;

	static const Vector4 & zero()	{ return ZERO; }

	///Zero vector.
	static const Vector4 ZERO;
	///(1, 0, 0, 0)
	static const Vector4 I;
	///(0, 1, 0, 0)
	static const Vector4 J;
	///(0, 0, 1, 0)
	static const Vector4 K;
	///(0, 0, 0, 1)
	static const Vector4 L;

private:
    friend std::ostream& operator <<( std::ostream&, const Vector4& );
    friend std::istream& operator >>( std::istream&, Vector4& );

	// This is to prevent construction like:
	//	Vector4( 0 );
	// It would interpret this as a float * and later crash.
	Vector4( int value );
};

BWENTITY_API INLINE Vector4 operator +( const Vector4& v1, const Vector4& v2 );
BWENTITY_API INLINE Vector4 operator -( const Vector4& v1, const Vector4& v2 );
BWENTITY_API INLINE Vector4 operator *( const Vector4& v1, const Vector4& v2 );
BWENTITY_API INLINE Vector4 operator /( const Vector4& v1, const Vector4& v2 );
BWENTITY_API INLINE Vector4 operator *( const Vector4& v, float s );
BWENTITY_API INLINE Vector4 operator *( float s, const Vector4& v );
BWENTITY_API INLINE Vector4 operator /( const Vector4& v, float s );
BWENTITY_API INLINE bool operator   ==( const Vector4& v1, const Vector4& v2 );
BWENTITY_API INLINE bool operator   !=( const Vector4& v1, const Vector4& v2 );
BWENTITY_API INLINE bool operator   < ( const Vector4& v1, const Vector4& v2 );

BWENTITY_API inline
bool operator   > ( const Vector4& v1, const Vector4& v2 ) { return v2 < v1; }
BWENTITY_API inline
bool operator   >=( const Vector4& v1, const Vector4& v2 ) { return !(v1<v2); }
BWENTITY_API inline
bool operator   <=( const Vector4& v1, const Vector4& v2 ) { return !(v2<v1); }

BWENTITY_API INLINE bool almostEqual( const Vector4& v1, const Vector4& v2, float epsilon );

#ifdef CODE_INLINE
	#include "vector4.ipp"
#endif

BW_END_NAMESPACE

#endif // VECTOR4_HPP

// vector4.hpp
