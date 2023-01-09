#ifndef MATH_EXTRA_HPP
#define MATH_EXTRA_HPP

#include "cstdmf/locale.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "vector2.hpp"
#include "vector3.hpp"
#include "matrix.hpp"
#include "rectt.hpp"
#include <sstream>
#include <iomanip>
#include <float.h>


BW_BEGIN_NAMESPACE
	class BWRandom;
BW_END_NAMESPACE

namespace BW
{

inline
bool watcherStringToValue( const char * valueStr, Rect & rect )
{
	char comma;
	istringstream istr( valueStr );
	istr.imbue( Locale::standardC() );
	istr >> rect.xMin_ >> comma >> rect.yMin_ >> comma >>
			rect.xMax_ >> comma >> rect.yMax_;
	return !istr.fail() && istr.eof();
}


inline
void streamFloat( ostringstream & ostr, float v )
{
	if (isEqual( v, FLT_MAX ))
	{
		ostr << "FLT_MAX";
	}
	else if (isEqual( v, -FLT_MAX ))
	{
		ostr << "-FLT_MAX";
	}
	else
	{
		ostr << v;
	}
}


inline
string watcherValueToString( const BW::Rect & rect )
{
	BW::ostringstream ostr;
	ostr.imbue( Locale::standardC() );
	ostr << std::setprecision( 3 ) << std::fixed;

	streamFloat( ostr, rect.xMin_ );
	ostr << ", ";
	streamFloat( ostr, rect.yMin_ );
	ostr << ", ";
	streamFloat( ostr, rect.xMax_ );
	ostr << ", ";
	streamFloat( ostr, rect.yMax_ );

	return ostr.str();
}


void orthogonalise(const Vector3 &v1, const Vector3 &v2, const Vector3 &v3,
				 Vector3 &e1, Vector3 &e2, Vector3 &e3);


/**
 * @brief Get power of two larger than specified number.
 * @param number Number to find larger power of two of.
 * @return Power of two larger than number (i.e. 3 => 4, 5 => 8, etc)
 */
uint32 largerPow2( uint32 number );

/**
 * @brief Get power of two smaller than specified number.
 * @param number Number to find smaller power of two of.
 * @return Power of two smaller than number (i.e. 3 => 2, 5 => 4, etc)
 */
uint32 smallerPow2( uint32 number );

/**
 * @brief Is value a power of two?
 * @param value Value to test.
 * @return true if value is a power of two, false if not.
 */
bool isPow2( uint32 value );

/**
 * @brief Round up value to a multiple of a power of two.
 * @param value Value to round up.
 * @param multiple Multiple to round up to. Must be power of two.
 * @return Rounded up value.
 */
uint32 roundUpToPow2Multiple( uint32 value, uint32 multiple );

/**
* @brief Calculate the ceiling log2 of an unsigned integer value. 
* @param value Value to get log2 of.
 */
uint32 log2ceil( uint32 x );

Vector3 createRandomVector3( BW_NAMESPACE BWRandom & randomSource,
	float minComponentValue, float maxComponentValue );

Vector3 createRandomNonZeroVector3( BW_NAMESPACE BWRandom & randomSource,
	float minComponentAbsoluteValue, float maxComponentAbsoluteValue );

Matrix createRandomTransform( BW_NAMESPACE BWRandom & randomSource,
	float minComponentValue, float maxComponentValue );

Matrix createRandomRotate( BW_NAMESPACE BWRandom & randomSource );

Matrix createRandomRotateTranslate( BW_NAMESPACE BWRandom & randomSource,
	float translationRange = 10.0f );

Matrix createRandomRotateTranslateScale( BW_NAMESPACE BWRandom & randomSource,
	float translationRange = 10.0f, float maxScale = 5.0f );

} // namespace BW

#endif // MATH_EXTRA_HPP
