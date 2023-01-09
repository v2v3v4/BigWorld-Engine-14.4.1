#include "pch.hpp"

#include "cstdmf/stdmf.hpp"
#include <cstring>

BW_BEGIN_NAMESPACE

TEST( FloatIsEqual_test )
{
	CHECK( isEqual( 1.0f, 1.0f ) );

	float x = 2.f;
	float y = x/2.f;

	while ( std::memcmp( &x, &y, sizeof(float) ) != 0 )
	{
		CHECK( !isEqual( x, y ) );

		x = y;
		y = x/2.f;
	}

	CHECK( isEqual( x, y ) );
	CHECK( isZero( x ) );
	CHECK( isZero( y ) );
}

BW_END_NAMESPACE


// test_float.cpp

