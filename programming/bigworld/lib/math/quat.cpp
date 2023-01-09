/**
 *	@file
 *
 *	@ingroup Math
 */

#include "pch.hpp"
#include <stdio.h>

#include <math.h>

#include "matrix.hpp"

#include "quat.hpp"

BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "quat.ipp"
#endif

#ifndef EXT_MATH
/**
 *	This method slerps from qStart when t = 0 to qEnd when t = 1. It is safe for
 *	the result quaternion (i.e. 'this') to be one of the parameters.
 *
 *	@param qStart	The quaternion to start from.
 *	@param qEnd		The quaternion to end at from.
 *	@param t		A value in the range [0, 1] that indicates the fraction to
 *					slerp between to to input quaternions.
 */
void Quaternion::slerp( const Quaternion &qStart, const Quaternion &qEnd,
		float t )
{
    // Compute dot product (equal to cosine of the angle between quaternions)
	float cosTheta = qStart.dotProduct( qEnd );

	bool invert = false;

    if( cosTheta < 0.0f )
    {
        // If so, flip one of the quaterions
        cosTheta = -cosTheta;

		invert = true;
	}

    // Set factors to do linear interpolation, as a special case where the
    // quaternions are close together.
    float  t2 = 1.0f - t;

    // If the quaternions aren't close, proceed with spherical interpolation
    if( 1.0f - cosTheta > 0.001f )
    {
        float theta = acosf( Math::clamp(-1.0f, cosTheta, +1.0f) );

        t2  = sinf( theta * t2 ) / sinf( theta);
        t = sinf( theta * t ) / sinf( theta);
    }

	if( invert )
		t = -t;

    // Do the interpolation
	Vector4 qs( qStart.x, qStart.y, qStart.z, qStart.w );
	Vector4 qe( qEnd.x,   qEnd.y,   qEnd.z,   qEnd.w );
    Vector4 r = qs * t2 + qe * t;
    
	this->set( r.x, r.y, r.z, r.w );
}


/**
 *	This method initialises this quaternion base on the input angle and axis.
 *	The input parameters describe a rotation in 3 dimensions.
 *
 *	@param angle	The angle in radians to rotate around the input axis.
 *	@param axis		The axis to rotate around.
 */
void Quaternion::fromAngleAxis( float angle, const Vector3 &axis )
{
	// precalculate
	const float theta	 = angle * 0.5f;
    const float sinTheta = sinf( theta );

	// normalise x,y,z
	Vector3 v = axis;
	v.normalise();
    v *= sinTheta;

	// assign to me 
	x = v.x;
	y = v.y;
	z = v.z;
	w = cosf( theta );
}

/**
 *	This method initialises this quaternion based on the rotation in the input
 *	matrix. The translation present in the matrix is not used.
 *
 *	@param m	The matrix to set this quaternion from.
 */
void Quaternion::fromMatrix( const Matrix &m )
{
	float tr = m[0][0] + m[1][1] + m[2][2];

	if( tr > 0 )
	{
		float s = sqrtf( tr + 1.f );
		w = s * 0.5f;

		s = 0.5f / s;

		(*this)[0] = ( m[1][2] - m[2][1] ) * s;
		(*this)[1] = ( m[2][0] - m[0][2] ) * s;
		(*this)[2] = ( m[0][1] - m[1][0] ) * s;
	}
	else
	{
		int i = 0;

		if( m[1][1] > m[0][0] )
			i = 1;
		if( m[2][2] > m[i][i] )
			i = 2;

		int j = ( i + 1 ) % 3;
		int k = ( j + 1 ) % 3;

		float s = sqrtf( (m[i][i] - ( m[j][j] + m[k][k] ) ) + 1.f );

		(*this)[i] = s * 0.5f;

		if (!isZero( s ))
			s = 0.5f / s;

		w		   = ( m[j][k] - m[k][j] ) * s;
		(*this)[j] = ( m[i][j] + m[j][i] ) * s;
		(*this)[k] = ( m[i][k] + m[k][i] ) * s;

	}

}


/**
 *	This method sets this quaternion to be the rotation that would result by
 *	applying the rotation of q1 and then the rotation of q2.
 */
void Quaternion::multiply( const Quaternion& q1, const Quaternion& q2 )
{
	const Vector3 q1v( q1.x, q1.y, q1.z );
	const Vector3 q2v( q2.x, q2.y, q2.z );
	
	Vector3 v;
	v.crossProduct( q2v, q1v );
	
	const Vector3 result  = ( q2.w * q1v ) + ( q1.w * q2v ) + v;
	const float	  resultw = q2.w * q1.w - q2v.dotProduct( q1v );

	x = result.x;
	y = result.y;
	z = result.z;
	w = resultw;
}


/**
 *	This method normalises this quaternion. That is, it makes sure that the
 *	quaternion has a length of 1.
 */
void Quaternion::normalise()
{
	float invd = 1.f / this->length();

	x *= invd;
	y *= invd;
	z *= invd;
	w *= invd;
}

/**
 *	This method inverts this quaternion.
 */
void Quaternion::invert()
{
	float oodivisor = 1.f / ( this->lengthSquared() );
	
	x *= -1 * oodivisor;
	y *= -1 * oodivisor;
	z *= -1 * oodivisor;
	w *= oodivisor;
}

#endif // !EXT_MATH

BW_END_NAMESPACE

/*quat.cpp*/
