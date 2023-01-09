#ifndef VECTOR2_HPP
#define VECTOR2_HPP

#include <iostream>
#include "xp_math.hpp"
#include "cstdmf/debug.hpp"
#include <math.h>


BW_BEGIN_NAMESPACE

/**
 *	This class implements a vector of two floats.
 *
 *	@ingroup Math
 */
class BWENTITY_API Vector2 : public Vector2Base
{

public:
	Vector2();
	Vector2( float a, float b );
	explicit Vector2( const Vector2Base & v2 );

	// Use the default compiler implementation for these methods. It is faster.
	// Vector2( const Vector2& v );
	// Vector2& operator  = ( const Vector2& v );

	void setZero( );
	void set( float a, float b ) ;
	void scale( const Vector2 &v, float s );
	float length() const;
	float lengthSquared() const;
	void normalise();
	Vector2 unitVector() const;

	float dotProduct( const Vector2& v ) const;
	float crossProduct( const Vector2& v ) const;

	void projectOnto( const Vector2& v1, const Vector2& v2 );
	Vector2 projectOnto( const Vector2 & v ) const;

	INLINE void operator += ( const Vector2& v );
	INLINE void operator -= ( const Vector2& v );
	INLINE void operator *= ( float s );
	INLINE void operator /= ( float s );

	BW::string desc() const;

	static const Vector2 & zero()			{ return ZERO; }

	///Zero vector.
	static const Vector2 ZERO;
	///(1, 0)
	static const Vector2 I;
	///(0, 1)
	static const Vector2 J;

private:
    friend std::ostream& operator <<( std::ostream&, const Vector2& );
    friend std::istream& operator >>( std::istream&, Vector2& );

	// This is to prevent construction like:
	//	Vector2( 0 );
	// It would interpret this as a float * and later crash.
	Vector2( int value );
};

BWENTITY_API INLINE Vector2 operator +( const Vector2& v1, const Vector2& v2 );
BWENTITY_API INLINE Vector2 operator -( const Vector2& v1, const Vector2& v2 );
BWENTITY_API INLINE Vector2 operator *( const Vector2& v, float s );
BWENTITY_API INLINE Vector2 operator *( float s, const Vector2& v );
BWENTITY_API INLINE Vector2 operator *( const Vector2& v1, const Vector2& v2 );
BWENTITY_API INLINE Vector2 operator /( const Vector2& v, float s );
BWENTITY_API INLINE bool operator   ==( const Vector2& v1, const Vector2& v2 );
BWENTITY_API INLINE bool operator   !=( const Vector2& v1, const Vector2& v2 );
BWENTITY_API INLINE bool operator   < ( const Vector2& v1, const Vector2& v2 );

BWENTITY_API inline
bool operator   > ( const Vector2& v1, const Vector2& v2 ) { return v2 < v1; }
BWENTITY_API inline
bool operator   >=( const Vector2& v1, const Vector2& v2 ) { return !(v1<v2); }
BWENTITY_API inline
bool operator   <=( const Vector2& v1, const Vector2& v2 ) { return !(v2<v1); }

BWENTITY_API INLINE
bool almostEqual( const Vector2& v1, const Vector2& v2, const float epsilon = 0.0004f );

#ifdef CODE_INLINE
	#include "vector2.ipp"
#endif

BW_END_NAMESPACE

#endif // VECTOR2_HPP

