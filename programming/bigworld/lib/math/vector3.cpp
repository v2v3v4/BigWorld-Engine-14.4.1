#include "pch.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/binary_stream.hpp"
#include "cstdmf/static_array.hpp"
#include "cstdmf/string_builder.hpp"
#include <sstream>

#include "vector3.hpp"

DECLARE_DEBUG_COMPONENT2( "Math", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
    #include "vector3.ipp"
#endif

const Vector3 Vector3::ZERO( 0.f, 0.f, 0.f );
const Vector3 Vector3::I( 1.f, 0.f, 0.f );
const Vector3 Vector3::J( 0.f, 1.f, 0.f );
const Vector3 Vector3::K( 0.f, 0.f, 1.f );


/**
 *	This function gets the angle between this and @a other without requiring
 *	them to be unit vectors.
 *	
 *	@return The minimum angle between this and @a other in radians.
 *		The result if either is a zero vector is undefined.
 */
float Vector3::getAngle( const Vector3 & other ) const
{
	const Vector3 aUnit = unitVector();
	const Vector3 bUnit = other.unitVector();

	return acosf( Math::clamp( -1.0f, aUnit.dotProduct( bUnit ), 1.0f ) );

	//atan2 is more reliable than using acos. It gives better precision
	//(mostly when acos input around +/-1) and we don't need to worry about
	//precision issues giving arguments outside of [-1, 1].
	//const Vector3 aCrossB = aUnit.crossProduct( bUnit );
	//return atan2( aCrossB.length(), aUnit.dotProduct( bUnit ) );
}


/**
 *	@return A description of the vector for debug purposes.
 *
 */
BW::string Vector3::desc() const
{
	StaticArray<char, 135> buf( 135 );
	buf.assign( 0 );

	StringBuilder builder( buf.data(), buf.size() );
	builder.appendf( "(%g, %g, %g)", x, y, z );

	return BW::string( builder.string() );
}


/**
 *	This function implements the output streaming operator for Vector3.
 *
 *	@relates Vector3
 */
std::ostream & operator <<( std::ostream & o, const Vector3 & t )
{
	StaticArray<char, 135> buf( 135 );
	buf.assign( 0 );

	StringBuilder builder( buf.data(), buf.size() );
	builder.appendf( "(%1.1f, %1.1f, %1.1f)", t.x, t.y, t.z );
	o << builder.string();

    return o;
}


/**
 *	This function implements the input streaming operator for Vector3.
 *
 *	@relates Vector3
 */
std::istream& operator >>( std::istream& i, Vector3& t )
{
	char dummy;
    i >> dummy >> t.x >> dummy >> t.y >> dummy >> t.z >>  dummy;

    return i;
}


BinaryIStream& operator>>( BinaryIStream &is, Vector3 &v )
{
	return is >> v.x >> v.y >> v.z;
}


BinaryOStream& operator<<( BinaryOStream &os, const Vector3 &v )
{
	return os << v.x << v.y << v.z;
}

BW_END_NAMESPACE

// vector3.cpp
