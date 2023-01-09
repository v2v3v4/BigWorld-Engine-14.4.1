#include "pch.hpp"
#include "vector2.hpp"

#include "cstdmf/debug.hpp"

#include <iomanip>

DECLARE_DEBUG_COMPONENT2( "Math", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
    #include "vector2.ipp"
#endif

const Vector2 Vector2::ZERO( 0.f, 0.f );
const Vector2 Vector2::I( 1.f, 0.f );
const Vector2 Vector2::J( 0.f, 1.f );

/**
 *	This function returns a description of the vector
 *
 */
BW::string Vector2::desc() const
{
	char buf[128];
	bw_snprintf( buf, sizeof(buf), "(%g, %g)", x, y );

	return buf;
}

/**
 *	This function implements the output streaming operator for Vector2.
 *
 *	@relates Vector2
 */
std::ostream& operator <<( std::ostream& o, const Vector2& t )
{
	const int fieldWidth = 8;

	o.put('(');
	o.width( fieldWidth );
	o << t[0];
	o.put(',');
	o.width( fieldWidth );
	o << t[1];
	o.put(')');

    return o;
}

/**
 *	This function implements the input streaming operator for Vector2.
 *
 *	@relates Vector2
 */
std::istream& operator >>( std::istream& i, Vector2& t )
{
	char dummy;
    i >> dummy >> t[0] >> dummy >> t[1] >> dummy;

    return i;
}

BW_END_NAMESPACE

// vector2.cpp
