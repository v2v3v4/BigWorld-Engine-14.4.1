#include "pch.hpp"
#include "vector4.hpp"

#include <iostream>

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Math", 0 )

#include <iomanip>


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
    #include "vector4.ipp"
#endif

const Vector4 Vector4::ZERO( 0.f, 0.f, 0.f, 0.f );
const Vector4 Vector4::I( 1.f, 0.f, 0.f, 0.f );
const Vector4 Vector4::J( 0.f, 1.f, 0.f, 0.f );
const Vector4 Vector4::K( 0.f, 0.f, 1.f, 0.f );
const Vector4 Vector4::L( 0.f, 0.f, 0.f, 1.f );

/**
 *	This function returns a description of the vector
 *
 */
BW::string Vector4::desc() const
{
	char buf[128];
	bw_snprintf( buf, sizeof(buf), "(%g, %g, %g, %g)", x, y, z, w );

	return buf;
}

/**
 *	This function implements the output streaming operator for Vector4.
 *
 *	@relates Vector4
 */
std::ostream& operator <<( std::ostream& o, const Vector4& t )
{
	const int fieldWidth = 8;

	o.put('(');
	o.width( fieldWidth );
	o << t[0];
	o.put(',');
	o.width( fieldWidth );
	o << t[1];
	o.put(',');
	o.width( fieldWidth );
	o << t[2];
	o.put(',');
	o.width( fieldWidth );
	o << t[3];
	o.put(')');

    return o;
}


/**
 *	This function implements the input streaming operator for Vector2.
 *
 *	@relates Vector4
 */
std::istream& operator >>( std::istream& i, Vector4& t )
{
	char dummy;
    i >>	dummy >> t[0] >>
			dummy >> t[1] >>
			dummy >> t[2] >>
			dummy >> t[3] >> dummy;

    return i;
}

BW_END_NAMESPACE

// vector4.cpp
