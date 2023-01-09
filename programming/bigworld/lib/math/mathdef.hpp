/**
 *	@file
 *	This file contains a number of definitions used by the Math module.
 *
 *	@ingroup Math
 */


#ifndef _MF3DDEF_HPP_
#define _MF3DDEF_HPP_

#include <stdlib.h>

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bwrandom.hpp"
#include <math.h>

BW_BEGIN_NAMESPACE

/// This value defines pi.
const float MATH_PI = 3.14159265359f;
const float MATH_2PI = 2.f * 3.14159265359f;

/// This value specifies the minimum value that a float should be.
/// @ingroup Math
const float MF_MIN_FLOAT = 0.00000001f;

/**
 *	This function converts an angle in degrees to an angle in radians.
 *
 * 	@param angle	The angle in degrees that is to be converted.
 *
 *	@return The angle in radians.
 *	@ingroup Math
 */
#ifndef DEG_TO_RAD
inline float DEG_TO_RAD( float angle )	{ return angle * MATH_PI / 180.f; }
#endif

/**
 *	This function converts an angle in radians to an angle in degrees.
 *
 * 	@param angle	The angle in radians that is to be converted.
 *
 *	@return The angle in degrees.
 *	@ingroup Math
 */
#ifndef RAD_TO_DEG
inline float RAD_TO_DEG( float angle )	{ return angle * 180.f / MATH_PI; }
#endif


/**
 *	This function rounds the input value to the nearest integer and returns
 *	it as type <code>float</code>.
 */
inline float BW_ROUNDF( float value )
{
	return floorf( value + 0.5f );
}

/**
 *	This function rounds the input value to the nearest integer and returns
 *	it as type <code>int</code>.
 */
inline int BW_ROUNDF_TO_INT( float value )
{
	return int( BW_ROUNDF( value ) );
}

/**
 *	This function rounds the input value to the nearest integer and returns
 *	it as type <code>double</code>.
 */
inline double BW_ROUND( double value )
{
	return floor( value + 0.5 );
}

/**
 *	This function rounds the input value to the nearest integer and returns
 *	it as type <code>int</code>.
 */
inline int BW_ROUND_TO_INT( double value )
{
	return int( BW_ROUND( value ) );
}

/**
 *	This type represents an outcode that is used for clipping.
 *	@ingroup Math
 */
typedef uint32 Outcode;
/**
 *	This type is used to index a vertex in a vertex array.
 *	@ingroup Math
 */
typedef uint32 VertexIndex;

const Outcode  OUTCODE_LEFT		= 0x1;	///< Beyond left plane.
const Outcode  OUTCODE_RIGHT	= 0x2;	///< Beyond right plane.
const Outcode  OUTCODE_BOTTOM	= 0x4;	///< Beyond bottom plane.
const Outcode  OUTCODE_TOP		= 0x8;	///< Beyond top plane.
const Outcode  OUTCODE_NEAR		= 0x10;	///< Beyond near plane.
const Outcode  OUTCODE_FAR		= 0x20;	///< Beyond far plane.
const Outcode  OUTCODE_MASK		= OUTCODE_LEFT | OUTCODE_RIGHT | OUTCODE_BOTTOM | OUTCODE_TOP | OUTCODE_NEAR | OUTCODE_FAR;	///< Combination of all outcodes.

/// This value is used to indicate an invalid index.
const VertexIndex NO_VERTEX_INDEX = 0xffffffff;

//const float FLOAT_EPSILON = 10e-5f;

/// This macro results in the fractional part of a floating point number.
#define ffrac( number )\
		(number-(float)((int)number))

/// This function returns a uniform random number in the range [0, 1].
inline float unitRand()
{
	return ((float)bw_random())/((float)(uint32)-1);
}


/**
 *	This function returns a uniform random number in the input range.
 */
inline float randInRange( float min, float max )
{
	return min + unitRand() * (max - min);
}

/**
 *	This function returns true if a the number is a power of 2, false otherwise
 */
inline bool powerOfTwo( uint32 num )
{
	return num ? ((num & -int32(num)) == num) : false;
}


/**
 *	These can be used for the indexes into vectors etc.
 */
enum BWAxisEnumeration
{
	X_AXIS = 0,		///<	Specifies the x-axis.
	Y_AXIS = 1,		///<	Specifies the y-axis.
	Z_AXIS = 2,		///<	Specifies the z-axis.
	X_COORD = 0,	///<	Specifies the x-coordinate.
	Y_COORD = 1,	///<	Specifies the y-coordinate.
	Z_COORD = 2		///<	Specifies the z-coordinate.
};


/**
 *	This namespace contains some math related functions.
 *
 *	@ingroup Math
 */
namespace Math
{
	/**
	 *	This function accepts a source and destination value and returns a value
	 *	somewhere between the two, approaching from source to destination with
	 *	increasing time.
	 *
	 *	@param srcValue		The value when deltaTime is 0.
	 *	@param dstValue		The value approached as deltaTime gets large.
	 *	@param halfLife		Controls how quickly the value approaches the
	 *						destination. It is the time taken for the value to
	 *						cover half the remaining distance.
	 *	@param deltaTime	Indicates how much time has passed since the last
	 *						update.
	 *
	 *	@return The decayed value between srcValue and dstValue.
	 */
	inline double decay( double srcValue,
			double dstValue,
			double halfLife,
			double deltaTime )
	{
		if ( halfLife <= 0.0 )
		{
			return dstValue;
		}
		else
		{
			return dstValue + pow( 0.5, deltaTime/halfLife ) *
				   ( srcValue - dstValue );
		}
	}

	/**
	 *	@see decay
	 */
	inline float decay( float srcValue,
			float dstValue,
			float halfLife,
			float deltaTime )
	{
		if ( halfLife <= 0.0f )
		{
			return dstValue;
		}
		else
		{
			return dstValue + powf( 0.5f, deltaTime/halfLife ) *
				   ( srcValue - dstValue );
		}
	}

	/**
	 *	This function returns the value clamped in between the minimum and
	 *	maximum values supplied.
	 *
	 *	@param minValue	The minimum the result can be.
	 *	@param value	The value to clamp.
	 *	@param maxValue	The maximum the result can be.
	 *
	 *	@return The value clamped between the minimum and maximum values.
	 */
	template <class TYPE>
	inline TYPE clamp( TYPE minValue, TYPE value, TYPE maxValue )
	{
		return value < minValue ? minValue :
		( value > maxValue ? maxValue : value);
	}


	/**
	 *	This function returns the value clamped in between the negative and
	 *	positive of the magnitude supplied.
	 *
	 *	@param magnitude	Specifies the magnitude of the range to clamp to.
	 *	@param value		The value to clamp.
	 *
	 *	@return The value clamped in the range [-magnitude, magnitude].
	 */
	template <class TYPE>
	inline TYPE clamp( TYPE magnitude, TYPE value )
	{
		return clamp( -magnitude, value, magnitude );
	}


    /**
     *  This function linearly interpolates from a source range into a
     *  destination range.
     *
     *  @param x            The value to interpolate.
     *  @param src_a        The source range minimum.
     *  @param src_b        The source range maximum.
     *  @param dst_a        The destination range minimum.
     *  @param dst_b        The destination range maximum.
     *
     *  @returns            The interpolated value of x in the source range
     *                      in the destination range.  For example if 
     *                      x = src_a then the result is dst_b, if x is the
     *                      mid point of [src_a, src_b] then the result
     *                      is the mid point of [dst_a, dst_b].
     */
    template<typename SRC, typename DST>
    inline DST 
    lerp
    (
        SRC                 const &x, 
        SRC                 const &src_a, 
        SRC                 const &src_b, 
        DST                 const &dst_a,
        DST                 const &dst_b    
    )
    {
        return (DST)((dst_b - dst_a)*(x - src_a)/(src_b - src_a)) + dst_a;
    }


	/**
     *  This function also linearly interpolates from a source range into a
     *  destination range.  It is slightly slower than lerp because it checks
	 *	for the degenerate case in the source in which case it returns dsa_t.
	 *
     *  @param x            The value to interpolate.
     *  @param src_a        The source range minimum.
     *  @param src_b        The source range maximum.
     *  @param dst_a        The destination range minimum.
     *  @param dst_b        The destination range maximum.
     *
     *  @returns            The interpolated value of x in the source range
     *                      in the destination range.  For example if 
     *                      x = src_a then the result is dst_b, if x is the
     *                      mid point of [src_a, src_b] then the result
     *                      is the mid point of [dst_a, dst_b].
	 */
    template<typename SRC, typename DST>
    inline DST 
    safeLerp
    (
        SRC                 const &x, 
        SRC                 const &src_a, 
        SRC                 const &src_b, 
        DST                 const &dst_a,
        DST                 const &dst_b    
    )
    {
        if (src_a == src_b)
            return dst_a;
        else
            return lerp(x, src_a, src_b, dst_a, dst_b);
    }


    /**
     *  Linearly interpolate over an interval.
     *
     *  @param t            The t-value to interpolate with (typically in
     *                      [0, 1]).
     *  @param a            The lower value of the range.
     *  @param b            The upper value of the range.
     *
     *  @returns            a + t(b - a).
     */
    template<typename TYPE>
    inline TYPE
    lerp
    (
        TYPE                const &t,
        TYPE                const &a,
        TYPE                const &b
    )
    {
        return a + t*(b - a);
    }


	/**
	 *	@return (value * value)
	 */
	template<typename T>
	inline T squared( const T & value )
	{
		return value * value;
	}
} // namespace Math

BW_END_NAMESPACE

#endif // _MF3DDEF_HPP_

