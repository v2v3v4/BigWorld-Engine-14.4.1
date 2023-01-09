#ifndef VECTOR3_HPP
#define VECTOR3_HPP

#include <iostream>

#include "xp_math.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"
#include "math/mathdef.hpp"
#include <math.h>
#ifdef _WIN32
#include <xmmintrin.h>
#endif


BW_BEGIN_NAMESPACE

/**
 *	This class implements a vector of three floats.
 *
 *	@ingroup Math
 */
class BWENTITY_API Vector3 : public Vector3Base
{

public:
	Vector3();
	Vector3( float a, float b, float c );
	explicit Vector3( const Vector3Base & v );
#ifdef _WIN32
	Vector3( __m128 v4 );
#endif

	// redefining operator float* to fix an error in d3dx math
	// d3dx math returns &x instead of returning this
	// according to c++ standard internal class members aren't guarantee to follow each other
	// in one case Intel compiler inlined entire Vector3 implementation and d3dx math caused bug
	// returning this as a float* tells ICC to properly unwrap class members
	// MS fixed this bug in the new DirectX Math
	// ideally we need to define x,y,z as union float[3] and returns float[3] array when we asked for float*
	// can't do this now cause we stuck with d3d9x math
	operator float* ();
	operator const float* () const;


	// Use the default compiler implementation for these methods. It is faster.
	//	Vector3( const Vector3& v);
	//	Vector3& operator  = ( const Vector3& v );

	void setZero();
	void set( float a, float b, float c );
//	void scale( const Vector3& v, float s );
	void setPitchYaw( float pitchInRadians, float yawInRadians );

	float dotProduct( const Vector3& v ) const;
	void crossProduct( const Vector3& v1, const Vector3& v2 );
	Vector3 crossProduct( const Vector3 & v ) const;
	void lerp( const Vector3 & a, const Vector3 & b, float t );
	void clamp( const Vector3 & lower, const Vector3 & upper );

	void projectOnto( const Vector3& v1, const Vector3& v2 );
	Vector3 projectOnto( const Vector3 & v ) const;
	Vector3 projectOntoPlane( const Vector3 & vPlaneNorm ) const;

	INLINE float length() const;
	INLINE float lengthSquared() const;
	INLINE float distance( const Vector3 & otherPoint ) const;
	INLINE float distanceSquared( const Vector3 & otherPoint ) const;
	INLINE void normalise();
	INLINE Vector3 unitVector() const;

	float getAngle( const Vector3 & other ) const;
	INLINE float getUnitVectorAngle( const Vector3 & other ) const;

	INLINE void operator += ( const Vector3& v );
	INLINE void operator -= ( const Vector3& v );
	INLINE void operator *= ( float s );
	INLINE void operator /= ( float s );
	INLINE Vector3 operator-() const;

	float yaw() const;
	float pitch() const;
	BW::string desc() const;

	static const Vector3 & zero()		{ return ZERO; }

	///Zero vector.
	static const Vector3 ZERO;
	///(1, 0, 0)
	static const Vector3 I;
	///(0, 1, 0)
	static const Vector3 J;
	///(0, 0, 1)
	static const Vector3 K;

private:
    friend std::ostream& operator <<( std::ostream&, const Vector3& );
    friend std::istream& operator >>( std::istream&, Vector3& );

	// This is to prevent construction like:
	//	Vector3( 0 );
	// It would interpret this as a float * and later crash.
	Vector3( int value );
};

BWENTITY_API INLINE Vector3 operator +( const Vector3& v1, const Vector3& v2 );
BWENTITY_API INLINE Vector3 operator -( const Vector3& v1, const Vector3& v2 );
BWENTITY_API INLINE Vector3 operator *( const Vector3& v, float s );
BWENTITY_API INLINE Vector3 operator *( float s, const Vector3& v );
BWENTITY_API INLINE Vector3 operator *( const Vector3& v1, const Vector3& v2 );
BWENTITY_API INLINE Vector3 operator /( const Vector3& v, float s );
BWENTITY_API INLINE bool operator   ==( const Vector3& v1, const Vector3& v2 );
BWENTITY_API INLINE bool operator   !=( const Vector3& v1, const Vector3& v2 );
BWENTITY_API INLINE bool operator   < ( const Vector3& v1, const Vector3& v2 );

BWENTITY_API inline
bool operator   > ( const Vector3& v1, const Vector3& v2 ) { return v2 < v1; }
BWENTITY_API inline
bool operator   >=( const Vector3& v1, const Vector3& v2 ) { return !(v1<v2); }
BWENTITY_API inline
bool operator   <=( const Vector3& v1, const Vector3& v2 ) { return !(v2<v1); }


class BinaryIStream;
class BinaryOStream;
BWENTITY_API BinaryIStream& operator>>( BinaryIStream &is, Vector3 &v );
BWENTITY_API BinaryOStream& operator<<( BinaryOStream &is, const Vector3 &v );


BWENTITY_API inline
bool almostEqual( const Vector3& v1, const Vector3& v2, const float epsilon = 0.0004f )
{
	return almostEqual( v1.x, v2.x, epsilon ) &&
		almostEqual( v1.y, v2.y, epsilon ) &&
		almostEqual( v1.z, v2.z, epsilon );
}

// Vector3 pitchYawToVector3( float pitchInRadians, float yawInRadians );

#ifdef CODE_INLINE
	#include "vector3.ipp"
#endif

BW_END_NAMESPACE

#endif // VECTOR3_HPP
