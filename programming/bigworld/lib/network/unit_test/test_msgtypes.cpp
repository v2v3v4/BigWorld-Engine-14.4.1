#include "pch.hpp"

#include "network/msgtypes.hpp"

#include "cstdmf/memory_stream.hpp"


BW_BEGIN_NAMESPACE

#define SIZETEST( TYPE, BITS, BYTES )				\
	{												\
	/* sizeof is needed by struct-message packing */	\
	CHECK_EQUAL( (size_t)BYTES, sizeof( TYPE ) );	\
	TYPE test;										\
	stream.reset();									\
	stream << test;									\
	/* We need to stream out sizeof bytes */		\
	CHECK_EQUAL( BYTES, stream.size() );			\
	}


TEST( PackedYaw_sizeof )
{
	MemoryOStream stream;

	// To get around commas-in-macro-parameters issues
#define PY( X ) PackedYaw< X >

	SIZETEST( PY( 1 ), 1, 1 );
	SIZETEST( PY( 1 ), 2, 1 );
	SIZETEST( PY( 1 ), 3, 1 );
	SIZETEST( PY( 4 ), 4, 1 );
	SIZETEST( PY( 5 ), 5, 1 );
	SIZETEST( PY( 6 ), 6, 1 );
	SIZETEST( PY( 7 ), 7, 1 );
	SIZETEST( PY( 8 ), 8, 1 );		// Equivalent to old angleToInt8
	SIZETEST( PY( 9 ), 9, 2 );
	SIZETEST( PY( 10 ), 10, 2 );
	SIZETEST( PY( 11 ), 11, 2 );
	SIZETEST( PY( 12 ), 12, 2 );
	SIZETEST( PY( 13 ), 13, 2 );
	SIZETEST( PY( 14 ), 14, 2 );
	SIZETEST( PY( 15 ), 15, 2 );
	SIZETEST( PY( 16 ), 16, 2 );
#undef PY
}


TEST( PackedYawPitch_sizeof )
{
	MemoryOStream stream;

	// To get around commas-in-macro-parameters issues
#define PYP( X, Y ) PackedYawPitch< false, X, Y >

	SIZETEST( PYP( 1, 1 ), 2, 1 );

	SIZETEST( PYP( 1, 2 ), 3, 1 );
	SIZETEST( PYP( 2, 1 ), 3, 1 );

	SIZETEST( PYP( 1, 3 ), 4, 1 );
	SIZETEST( PYP( 2, 2 ), 4, 1 );
	SIZETEST( PYP( 3, 1 ), 4, 1 );

	SIZETEST( PYP( 1, 4 ), 5, 1 );
	SIZETEST( PYP( 2, 3 ), 5, 1 );
	SIZETEST( PYP( 3, 2 ), 5, 1 );
	SIZETEST( PYP( 4, 1 ), 5, 1 );

	SIZETEST( PYP( 1, 5 ), 6, 1 );
	SIZETEST( PYP( 2, 4 ), 6, 1 );
	SIZETEST( PYP( 3, 3 ), 6, 1 );
	SIZETEST( PYP( 4, 2 ), 6, 1 );
	SIZETEST( PYP( 5, 1 ), 6, 1 );

	SIZETEST( PYP( 1, 6 ), 7, 1 );
	SIZETEST( PYP( 2, 5 ), 7, 1 );
	SIZETEST( PYP( 3, 4 ), 7, 1 );
	SIZETEST( PYP( 4, 3 ), 7, 1 );
	SIZETEST( PYP( 5, 2 ), 7, 1 );
	SIZETEST( PYP( 6, 1 ), 7, 1 );

	SIZETEST( PYP( 1, 7 ), 8, 1 );
	SIZETEST( PYP( 2, 6 ), 8, 1 );
	SIZETEST( PYP( 3, 5 ), 8, 1 );
	SIZETEST( PYP( 4, 4 ), 8, 1 );
	SIZETEST( PYP( 5, 3 ), 8, 1 );
	SIZETEST( PYP( 6, 2 ), 8, 1 );
	SIZETEST( PYP( 7, 1 ), 8, 1 );

	SIZETEST( PYP( 1, 8 ), 9, 2 );
	SIZETEST( PYP( 2, 7 ), 9, 2 );
	SIZETEST( PYP( 3, 6 ), 9, 2 );
	SIZETEST( PYP( 4, 5 ), 9, 2 );
	SIZETEST( PYP( 5, 4 ), 9, 2 );
	SIZETEST( PYP( 6, 3 ), 9, 2 );
	SIZETEST( PYP( 7, 2 ), 9, 2 );
	SIZETEST( PYP( 8, 1 ), 9, 2 );

	SIZETEST( PYP( 1, 9 ), 10, 2 );
	SIZETEST( PYP( 2, 8 ), 10, 2 );
	SIZETEST( PYP( 3, 7 ), 10, 2 );
	SIZETEST( PYP( 4, 6 ), 10, 2 );
	SIZETEST( PYP( 5, 5 ), 10, 2 );
	SIZETEST( PYP( 6, 4 ), 10, 2 );
	SIZETEST( PYP( 7, 3 ), 10, 2 );
	SIZETEST( PYP( 8, 2 ), 10, 2 );
	SIZETEST( PYP( 9, 1 ), 10, 2 );

	SIZETEST( PYP( 1, 10 ), 11, 2 );
	SIZETEST( PYP( 2, 9 ), 11, 2 );
	SIZETEST( PYP( 3, 8 ), 11, 2 );
	SIZETEST( PYP( 4, 7 ), 11, 2 );
	SIZETEST( PYP( 5, 6 ), 11, 2 );
	SIZETEST( PYP( 6, 5 ), 11, 2 );
	SIZETEST( PYP( 7, 4 ), 11, 2 );
	SIZETEST( PYP( 8, 3 ), 11, 2 );
	SIZETEST( PYP( 9, 2 ), 11, 2 );
	SIZETEST( PYP( 10, 1 ), 11, 2 );

	SIZETEST( PYP( 1, 11 ), 12, 2 );
	SIZETEST( PYP( 2, 10 ), 12, 2 );
	SIZETEST( PYP( 3, 9 ), 12, 2 );
	SIZETEST( PYP( 4, 8 ), 12, 2 );
	SIZETEST( PYP( 5, 7 ), 12, 2 );
	SIZETEST( PYP( 6, 6 ), 12, 2 );
	SIZETEST( PYP( 7, 5 ), 12, 2 );
	SIZETEST( PYP( 8, 4 ), 12, 2 );
	SIZETEST( PYP( 9, 3 ), 12, 2 );
	SIZETEST( PYP( 10, 2 ), 12, 2 );
	SIZETEST( PYP( 11, 1 ), 12, 2 );

	// Skip a few...

	SIZETEST( PYP( 1, 15 ), 16, 2 );
	SIZETEST( PYP( 2, 14 ), 16, 2 );
	SIZETEST( PYP( 3, 13 ), 16, 2 );
	SIZETEST( PYP( 4, 12 ), 16, 2 );
	SIZETEST( PYP( 5, 11 ), 16, 2 );
	SIZETEST( PYP( 6, 10 ), 16, 2 );
	SIZETEST( PYP( 7, 9 ), 16, 2 );
	SIZETEST( PYP( 8, 8 ), 16, 2 );		// Equivalent to old YawPitch class
	SIZETEST( PYP( 9, 7 ), 16, 2 );
	SIZETEST( PYP( 10, 6 ), 16, 2 );
	SIZETEST( PYP( 11, 5 ), 16, 2 );
	SIZETEST( PYP( 12, 4 ), 16, 2 );
	SIZETEST( PYP( 13, 3 ), 16, 2 );
	SIZETEST( PYP( 14, 2 ), 16, 2 );
	SIZETEST( PYP( 15, 1 ), 16, 2 );

#undef PYP
}


TEST( PackedYawPitchRoll_sizeof )
{
	MemoryOStream stream;

	// To get around commas-in-macro-parameters issues
#define PYPR( X, Y, Z ) PackedYawPitchRoll< true, X, Y, Z >

	SIZETEST( PYPR( 1, 1, 1 ), 3, 1 );

	SIZETEST( PYPR( 2, 1, 1 ), 4, 1 );
	SIZETEST( PYPR( 1, 2, 1 ), 4, 1 );
	SIZETEST( PYPR( 1, 1, 2 ), 4, 1 );

	SIZETEST( PYPR( 3, 1, 1 ), 5, 1 );
	SIZETEST( PYPR( 2, 2, 1 ), 5, 1 );
	SIZETEST( PYPR( 2, 1, 2 ), 5, 1 );
	SIZETEST( PYPR( 1, 3, 1 ), 5, 1 );
	SIZETEST( PYPR( 1, 2, 2 ), 5, 1 );
	SIZETEST( PYPR( 1, 1, 3 ), 5, 1 );

	SIZETEST( PYPR( 4, 1, 1 ), 6, 1 );
	SIZETEST( PYPR( 3, 2, 1 ), 6, 1 );
	SIZETEST( PYPR( 3, 1, 2 ), 6, 1 );
	SIZETEST( PYPR( 2, 3, 1 ), 6, 1 );
	SIZETEST( PYPR( 2, 2, 2 ), 6, 1 );
	SIZETEST( PYPR( 2, 1, 3 ), 6, 1 );
	SIZETEST( PYPR( 1, 4, 1 ), 6, 1 );
	SIZETEST( PYPR( 1, 3, 2 ), 6, 1 );
	SIZETEST( PYPR( 1, 2, 3 ), 6, 1 );
	SIZETEST( PYPR( 1, 1, 4 ), 6, 1 );

	// Skip a few...

	SIZETEST( PYPR( 6, 1, 1 ), 8, 1 );
	SIZETEST( PYPR( 5, 2, 1 ), 8, 1 );
	SIZETEST( PYPR( 5, 1, 2 ), 8, 1 );
	SIZETEST( PYPR( 4, 3, 1 ), 8, 1 );
	SIZETEST( PYPR( 4, 2, 2 ), 8, 1 );
	SIZETEST( PYPR( 4, 1, 3 ), 8, 1 );
	SIZETEST( PYPR( 3, 4, 1 ), 8, 1 );
	SIZETEST( PYPR( 3, 3, 2 ), 8, 1 );
	SIZETEST( PYPR( 3, 2, 3 ), 8, 1 );
	SIZETEST( PYPR( 3, 1, 4 ), 8, 1 );
	SIZETEST( PYPR( 2, 5, 1 ), 8, 1 );
	SIZETEST( PYPR( 2, 4, 2 ), 8, 1 );
	SIZETEST( PYPR( 2, 3, 3 ), 8, 1 );
	SIZETEST( PYPR( 2, 2, 4 ), 8, 1 );
	SIZETEST( PYPR( 2, 1, 5 ), 8, 1 );
	SIZETEST( PYPR( 1, 6, 1 ), 8, 1 );
	SIZETEST( PYPR( 1, 5, 2 ), 8, 1 );
	SIZETEST( PYPR( 1, 4, 3 ), 8, 1 );
	SIZETEST( PYPR( 1, 3, 4 ), 8, 1 );
	SIZETEST( PYPR( 1, 2, 5 ), 8, 1 );
	SIZETEST( PYPR( 1, 1, 6 ), 8, 1 );

	SIZETEST( PYPR( 7, 1, 1 ), 9, 2 );
	SIZETEST( PYPR( 6, 2, 1 ), 9, 2 );
	SIZETEST( PYPR( 6, 1, 2 ), 9, 2 );
	SIZETEST( PYPR( 5, 3, 1 ), 9, 2 );
	SIZETEST( PYPR( 5, 2, 2 ), 9, 2 );
	SIZETEST( PYPR( 5, 1, 3 ), 9, 2 );
	SIZETEST( PYPR( 4, 4, 1 ), 9, 2 );
	SIZETEST( PYPR( 4, 3, 2 ), 9, 2 );
	SIZETEST( PYPR( 4, 2, 3 ), 9, 2 );
	SIZETEST( PYPR( 4, 1, 4 ), 9, 2 );
	SIZETEST( PYPR( 3, 5, 1 ), 9, 2 );
	SIZETEST( PYPR( 3, 4, 2 ), 9, 2 );
	SIZETEST( PYPR( 3, 3, 3 ), 9, 2 );
	SIZETEST( PYPR( 3, 2, 4 ), 9, 2 );
	SIZETEST( PYPR( 3, 1, 5 ), 9, 2 );
	SIZETEST( PYPR( 2, 6, 1 ), 9, 2 );
	SIZETEST( PYPR( 2, 5, 2 ), 9, 2 );
	SIZETEST( PYPR( 2, 4, 3 ), 9, 2 );
	SIZETEST( PYPR( 2, 3, 4 ), 9, 2 );
	SIZETEST( PYPR( 2, 2, 5 ), 9, 2 );
	SIZETEST( PYPR( 2, 1, 6 ), 9, 2 );
	SIZETEST( PYPR( 1, 7, 1 ), 9, 2 );
	SIZETEST( PYPR( 1, 6, 2 ), 9, 2 );
	SIZETEST( PYPR( 1, 5, 3 ), 9, 2 );
	SIZETEST( PYPR( 1, 4, 4 ), 9, 2 );
	SIZETEST( PYPR( 1, 3, 5 ), 9, 2 );
	SIZETEST( PYPR( 1, 2, 6 ), 9, 2 );
	SIZETEST( PYPR( 1, 1, 7 ), 9, 2 );

	// This one's important for backwards compatibility

	SIZETEST( PYPR( 8, 8, 8 ), 24, 3 );	// Equivalent to old YawPitchRoll class

#undef PYPR
}


TEST( PackedGroundPos_sizeof )
{
	MemoryOStream stream;

	// To get around commas-in-macro-parameters issues
#define PGP( EXP, MAN ) PackedGroundPos< EXP, MAN >

	SIZETEST( PGP( 0, 1 ), 4, 1 );

	SIZETEST( PGP( 0, 2 ), 6, 1 );
	SIZETEST( PGP( 1, 1 ), 6, 1 );

	SIZETEST( PGP( 0, 3 ), 8, 1 );
	SIZETEST( PGP( 1, 2 ), 8, 1 );
	SIZETEST( PGP( 2, 1 ), 8, 1 );

	SIZETEST( PGP( 0, 4 ), 10, 2 );
	SIZETEST( PGP( 1, 3 ), 10, 2 );
	SIZETEST( PGP( 2, 2 ), 10, 2 );
	SIZETEST( PGP( 3, 1 ), 10, 2 );

	SIZETEST( PGP( 0, 5 ), 12, 2 );
	SIZETEST( PGP( 1, 4 ), 12, 2 );
	SIZETEST( PGP( 2, 3 ), 12, 2 );
	SIZETEST( PGP( 3, 2 ), 12, 2 );
	SIZETEST( PGP( 4, 1 ), 12, 2 );

	SIZETEST( PGP( 0, 6 ), 14, 2 );
	SIZETEST( PGP( 1, 5 ), 14, 2 );
	SIZETEST( PGP( 2, 4 ), 14, 2 );
	SIZETEST( PGP( 3, 3 ), 14, 2 );
	SIZETEST( PGP( 4, 2 ), 14, 2 );
	SIZETEST( PGP( 5, 1 ), 14, 2 );

	SIZETEST( PGP( 0, 7 ), 16, 2 );
	SIZETEST( PGP( 1, 6 ), 16, 2 );
	SIZETEST( PGP( 2, 5 ), 16, 2 );
	SIZETEST( PGP( 3, 4 ), 16, 2 );
	SIZETEST( PGP( 4, 3 ), 16, 2 );
	SIZETEST( PGP( 5, 2 ), 16, 2 );
	SIZETEST( PGP( 6, 1 ), 16, 2 );

	SIZETEST( PGP( 0, 8 ), 18, 3 );
	SIZETEST( PGP( 1, 7 ), 18, 3 );
	SIZETEST( PGP( 2, 6 ), 18, 3 );
	SIZETEST( PGP( 3, 5 ), 18, 3 );
	SIZETEST( PGP( 4, 4 ), 18, 3 );
	SIZETEST( PGP( 5, 3 ), 18, 3 );
	SIZETEST( PGP( 6, 2 ), 18, 3 );
	SIZETEST( PGP( 7, 1 ), 18, 3 );

	SIZETEST( PGP( 0, 9 ), 20, 3 );
	SIZETEST( PGP( 1, 8 ), 20, 3 );
	SIZETEST( PGP( 2, 7 ), 20, 3 );
	SIZETEST( PGP( 3, 6 ), 20, 3 );
	SIZETEST( PGP( 4, 5 ), 20, 3 );
	SIZETEST( PGP( 5, 4 ), 20, 3 );
	SIZETEST( PGP( 6, 3 ), 20, 3 );
	SIZETEST( PGP( 7, 2 ), 20, 3 );
	SIZETEST( PGP( 8, 1 ), 20, 3 );

	SIZETEST( PGP( 0, 10 ), 22, 3 );
	SIZETEST( PGP( 1, 9 ), 22, 3 );
	SIZETEST( PGP( 2, 8 ), 22, 3 );
	SIZETEST( PGP( 3, 7 ), 22, 3 );
	SIZETEST( PGP( 4, 6 ), 22, 3 );
	SIZETEST( PGP( 5, 5 ), 22, 3 );
	SIZETEST( PGP( 6, 4 ), 22, 3 );
	SIZETEST( PGP( 7, 3 ), 22, 3 );
	SIZETEST( PGP( 8, 2 ), 22, 3 );
	SIZETEST( PGP( 9, 1 ), 22, 3 );

	SIZETEST( PGP( 0, 11 ), 24, 3 );
	SIZETEST( PGP( 1, 10 ), 24, 3 );
	SIZETEST( PGP( 2, 9 ), 24, 3 );
	SIZETEST( PGP( 3, 8 ), 24, 3 );		// Equivalent to old PackedXZ class
	SIZETEST( PGP( 4, 7 ), 24, 3 );
	SIZETEST( PGP( 5, 6 ), 24, 3 );
	SIZETEST( PGP( 6, 5 ), 24, 3 );
	SIZETEST( PGP( 7, 4 ), 24, 3 );
	SIZETEST( PGP( 8, 3 ), 24, 3 );
	SIZETEST( PGP( 9, 2 ), 24, 3 );
	SIZETEST( PGP( 10, 1 ), 24, 3 );

	SIZETEST( PGP( 0, 12 ), 26, 4 );
	SIZETEST( PGP( 1, 11 ), 26, 4 );
	SIZETEST( PGP( 2, 10 ), 26, 4 );
	SIZETEST( PGP( 3, 9 ), 26, 4 );
	SIZETEST( PGP( 4, 8 ), 26, 4 );
	SIZETEST( PGP( 5, 7 ), 26, 4 );
	SIZETEST( PGP( 6, 6 ), 26, 4 );
	SIZETEST( PGP( 7, 5 ), 26, 4 );
	SIZETEST( PGP( 8, 4 ), 26, 4 );
	SIZETEST( PGP( 9, 3 ), 26, 4 );
	SIZETEST( PGP( 10, 2 ), 26, 4 );
	SIZETEST( PGP( 11, 1 ), 26, 4 );

	// Skip a few...

	SIZETEST( PGP( 0, 15 ), 32, 4 );
	SIZETEST( PGP( 1, 14 ), 32, 4 );
	SIZETEST( PGP( 2, 13 ), 32, 4 );
	SIZETEST( PGP( 3, 12 ), 32, 4 );
	SIZETEST( PGP( 4, 11 ), 32, 4 );
	SIZETEST( PGP( 5, 10 ), 32, 4 );
	SIZETEST( PGP( 6, 9 ), 32, 4 );
	SIZETEST( PGP( 7, 8 ), 32, 4 );
	SIZETEST( PGP( 8, 7 ), 32, 4 );
	SIZETEST( PGP( 9, 6 ), 32, 4 );
	SIZETEST( PGP( 10, 5 ), 32, 4 );
	SIZETEST( PGP( 11, 4 ), 32, 4 );
	SIZETEST( PGP( 12, 3 ), 32, 4 );
	SIZETEST( PGP( 13, 2 ), 32, 4 );
	SIZETEST( PGP( 14, 1 ), 32, 4 );

	SIZETEST( PGP( 0, 16 ), 34, 5 );
	SIZETEST( PGP( 1, 15 ), 34, 5 );
	SIZETEST( PGP( 2, 14 ), 34, 5 );
	SIZETEST( PGP( 3, 13 ), 34, 5 );
	SIZETEST( PGP( 4, 12 ), 34, 5 );
	SIZETEST( PGP( 5, 11 ), 34, 5 );
	SIZETEST( PGP( 6, 10 ), 34, 5 );
	SIZETEST( PGP( 7, 9 ), 34, 5 );
	SIZETEST( PGP( 8, 8 ), 34, 5 );
	SIZETEST( PGP( 9, 7 ), 34, 5 );
	SIZETEST( PGP( 10, 6 ), 34, 5 );
	SIZETEST( PGP( 11, 5 ), 34, 5 );
	SIZETEST( PGP( 12, 4 ), 34, 5 );
	SIZETEST( PGP( 13, 3 ), 34, 5 );
	SIZETEST( PGP( 14, 2 ), 34, 5 );
	SIZETEST( PGP( 15, 1 ), 34, 5 );

#undef PGP
}


TEST( PackedFullPos_sizeof )
{
	MemoryOStream stream;

	// To get around commas-in-macro-parameters issues
#define PFP( X, Y, Z, W ) PackedFullPos< X, Y, Z, W >

	SIZETEST( PFP( 0, 11, 4, 11 ), 40, 5 );

	// This one's important for backwards compatibility

	// Equivalent to old PackedXYZ class
	SIZETEST( PFP( 3, 8, 4, 11 ), 40, 5 );

#undef PFP
}

#undef SIZETEST


TEST( PackedYaw_4_testRounding )
{
	const float step = MATH_PI / 256.f;
	// 4 bits gives an error epsilon of MATH_PI / 16.f due to rounding
	const float error = MATH_PI / 16.f;

	float inputs[512];
	float results[512];

	for (int i = -256; i < 256; ++i)
	{
		inputs[ i + 256 ] = i * step;
		PackedYaw< 4 > result( inputs[ i + 256 ] );
		result.get( results[ i + 256 ] );
	}

	for (int i = 0; i < 512; ++i)
	{
		float input = inputs[ i ];
		float prev = ( i == 0 ) ? results[ 511 ] : results[ i - 1 ];
		float curr = results[ i ];
		float next = ( i == 511 ) ? results[ 0 ] : results[ i + 1 ];

		// Fixups for wrapping around the circle
		while (fabs(input - curr) > MATH_PI)
		{
			if (input > curr)
			{
				curr += MATH_2PI;
			}
			else
			{
				curr -= MATH_2PI;
			}
		}

		if (prev > curr)
		{
			prev -= MATH_2PI;
		}

		if (next < curr)
		{
			next += MATH_2PI;
		}

		CHECK( prev <= curr );
		CHECK( curr <= next );

		// TODO: These next two tests fail a lot, suggesting we're picking
		// a non-optimum option for boundary conditions due to floating point
		// error. They also vary by compiler and build configuration

		/*
		CHECK( fabsf( input - curr ) <= fabsf( input - prev ) );
		CHECK( fabsf( input - curr ) <= fabsf( input - next ) );
		*/

		if (i == 48 || i == 80 || i == 144 || i == 176 || i == 304 ||
			i == 400 || i == 464 || i == 496)
		{
			// The above loop iterators indicate a possible mis-selection of
			// the less-optimal value, or a floating-point error which means
			// that while the two options are MATH_PI / 8.f apart, in single
			// precision, they are slightly further apart than that, and we've
			// fed in a value that's exactly halfway between, and hence more
			// than MATH_PI / 16.f (the expected error epsilon) from either.

			if (curr == next)
			{
				// If we're at the lower-bound of a new output value, we've
				// probably mis-selected the value.
				CHECK( curr > input );
				CHECK( (curr - input) > error );
				continue;
			}
			else if ((input - curr) > error)
			{
				// Anywhere else we're outside our error range, we've probably
				// hit a floating-point error range, which means our two
				// possible values are more than MATH_PI / 8.f apart, and
				// so the error of the next value up is also outside our error
				// range.
				CHECK( (next - curr) > MATH_PI / 8.f );
				CHECK( (next - input) > error );
				continue;
			}
		}

		CHECK_CLOSE( input, curr, error );
	}
}


TEST( PackedYaw_8_testRounding )
{
	const float step = MATH_PI / 256.f;
	// 8 bits gives an error epsilon of MATH_PI / 256.f due to rounding
	// This is however wrong, as we have floating-point rounding issues which
	// can produce successive values separated by more than 'error'.
	// So we take our error by observation
	// TODO: Determine the actual error. Does that depend on the value?
	float largestKnownError;

	float inputs[512];
	float results[512];

	for (int i = -256; i < 256; ++i)
	{
		inputs[ i + 256 ] = i * step;
		PackedYaw< 8 > result( inputs[ i + 256 ] );
		result.get( results[ i + 256 ] );
		if (i != -256)
		{
			largestKnownError = std::max( largestKnownError,
				results[ i + 256 ] - results[ i + 255 ] );
		}
	}

	// Error is half the largest gap between two results, assuming we have
	// a result for every possible encoding...
	largestKnownError /= 2.f;

	for (int i = 0; i < 512; ++i)
	{
		float input = inputs[ i ];
		float prev = ( i == 0 ) ? results[ 511 ] : results[ i - 1 ];
		float curr = results[ i ];
		float next = ( i == 511 ) ? results[ 0 ] : results[ i + 1 ];

		// Fixups for wrapping around the circle
		while (fabs(input - curr) > MATH_PI)
		{
			if (input > curr)
			{
				curr += MATH_2PI;
			}
			else
			{
				curr -= MATH_2PI;
			}
		}

		if (prev > curr)
		{
			prev -= MATH_2PI;
		}

		if (next < curr)
		{
			next += MATH_2PI;
		}

		CHECK( prev <= curr );
		CHECK( curr <= next );

		// TODO: These next two tests fail a lot, suggesting we're picking
		// a non-optimum option for boundary conditions due to floating point
		// error. They also vary by compiler and build configuration

		/*
		CHECK( fabsf( input - curr ) <= fabsf( input - prev ) );
		CHECK( fabsf( input - curr ) <= fabsf( input - next ) );
		*/

		CHECK_CLOSE( input, curr, largestKnownError );
	}
}


TEST( PackedYawPitch_true_4_4_testRounding )
{
	const float step = MATH_PI / 256.f;
	// 4 bits gives an error epsilon of MATH_PI / 16.f due to rounding
	const float error = MATH_PI / 16.f;
	// 4 bits, across half the range, should be half the error, but internally
	// half-pitch treats 'pi/2' as (2^BITS) - 1, rather than (2^BITS)
	const float pitchError = (MATH_PI / 2.f) / 15.f;

	float inputs[512];
	float resultsY[512];
	float resultsP[512];

	for (int i = -256; i < 256; ++i)
	{
		inputs[ i + 256 ] = i * step;
		PackedYawPitch< true, 4, 4 > result( inputs[ i + 256 ],
			inputs[ i + 256 ] / 2.f );
		result.get( resultsY[ i + 256 ], resultsP[ i + 256 ] );
	}

	for (int i = 0; i < 512; ++i)
	{
		float input = inputs[ i ];
		float prev = ( i == 0 ) ? resultsY[ 511 ] : resultsY[ i - 1 ];
		float curr = resultsY[ i ];
		float next = ( i == 511 ) ? resultsY[ 0 ] : resultsY[ i + 1 ];

		// Fixups for wrapping around the circle
		while (fabs(input - curr) > MATH_PI)
		{
			if (input > curr)
			{
				curr += MATH_2PI;
			}
			else
			{
				curr -= MATH_2PI;
			}
		}

		if (prev > curr)
		{
			prev -= MATH_2PI;
		}

		if (next < curr)
		{
			next += MATH_2PI;
		}

		CHECK( prev <= curr );
		CHECK( curr <= next );

		// TODO: These next two tests fail a lot, suggesting we're picking
		// a non-optimum option for boundary conditions due to floating point
		// error. They also vary by compiler and build configuration

		/*
		CHECK( fabsf( input - curr ) <= fabsf( input - prev ) );
		CHECK( fabsf( input - curr ) <= fabsf( input - next ) );
		*/

		if (i == 48 || i == 80 || i == 144 || i == 176 || i == 304 ||
			i == 400 || i == 464 || i == 496)
		{
			// The above loop iterators indicate a possible mis-selection of
			// the less-optimal value, or a floating-point error which means
			// that while the two options are MATH_PI / 8.f apart, in single
			// precision, they are slightly further apart than that, and we've
			// fed in a value that's exactly halfway between, and hence more
			// than MATH_PI / 16.f (the expected error epsilon) from either.

			if (curr == next)
			{
				// If we're at the lower-bound of a new output value, we've
				// probably mis-selected the value.
				CHECK( curr > input );
				CHECK( (curr - input) > error );
				continue;
			}
			else if ((input - curr) > error)
			{
				// Anywhere else we're outside our error range, we've probably
				// hit a floating-point error range, which means our two
				// possible values are more than MATH_PI / 8.f apart, and
				// so the error of the next value up is also outside our error
				// range.
				CHECK( (next - curr) > MATH_PI / 8.f );
				CHECK( (next - input) > error );
				continue;
			}
		}

		CHECK_CLOSE( input, curr, error );
	}

	for (int i = 0; i < 512; ++i)
	{
		float input = inputs[ i ] / 2.f;
		float prev = ( i == 0 ) ? resultsY[ i ] : resultsY[ i - 1 ];
		float curr = resultsY[ i ];
		float next = ( i == 511 ) ? resultsY[ i ] : resultsY[ i + 1 ];

		// Half-pitch only covers values [-pi/2,pi/2), according to the
		// documentation. It actually can represent pi/2.
		if (input < (MATH_PI / 2.f) || input > (MATH_PI / 2.f))
		{
			// Out of range.
			continue;
		}

		CHECK( prev <= curr );
		CHECK( curr <= next );

		CHECK( fabsf( input - curr ) <= fabsf( input - prev ) );
		CHECK( fabsf( input - curr ) <= fabsf( input - next ) );

		CHECK_CLOSE( input, curr, pitchError );
	}
}


TEST( PackedYaw_4_roundtrip )
{
	PackedYaw< 4 > inPY, outPY;
	MemoryOStream stream;
	float inYaw, outYaw;

	const float step = MATH_PI / 256.f;

	for (int i = -256; i < 256; ++i)
	{
		const float yaw = step * i;

		stream.reset();
		inPY.set( yaw );
		stream << inPY;
		CHECK_EQUAL( 1, stream.size() );

		stream >> outPY;
		CHECK_EQUAL( 0, stream.remainingLength() );

		inPY.get( inYaw );
		outPY.get( outYaw );

		CHECK_EQUAL( inYaw, outYaw );
	}
}


TEST( PackedYaw_8_roundtrip )
{
	PackedYaw< 8 > inPY, outPY;
	MemoryOStream stream;
	float inYaw, outYaw;

	const float step = MATH_PI / 256.f;

	for (int i = -256; i < 256; ++i)
	{
		const float yaw = step * i;

		stream.reset();
		inPY.set( yaw );
		stream << inPY;
		CHECK_EQUAL( 1, stream.size() );

		stream >> outPY;
		CHECK_EQUAL( 0, stream.remainingLength() );

		inPY.get( inYaw );
		outPY.get( outYaw );

		CHECK_EQUAL( inYaw, outYaw );
	}
}


TEST( PackedYawPitch_false_4_4_roundtrip )
{
	PackedYawPitch< /* HALFPITCH */ false, 4, 4 > inPYP, outPYP;
	MemoryOStream stream;
	float inYaw, inPitch, outYaw, outPitch;

	const float step = MATH_PI / 256.f;

	for (int i = -256; i < 256; ++i)
	{
		const float yaw = step * i;

		for (int j = -256; j < 256; ++j)
		{
			const float pitch = step * j;

			stream.reset();
			inPYP.set( yaw, pitch );
			stream << inPYP;
			CHECK_EQUAL( 1, stream.size() );

			// If this immediately precedes outPYP.get(),
			// gcc from CentOS 6 "4.4.6 20120305 (Red Hat 4.4.6-4)" will
			// miscompile the latter under -O3 and the code will abort
			// with the error "pure virtual method called"
			inPYP.get( inYaw, inPitch );

			stream >> outPYP;
			CHECK_EQUAL( 0, stream.remainingLength() );

			outPYP.get( outYaw, outPitch );

			CHECK_EQUAL( inYaw, outYaw );
			CHECK_EQUAL( inPitch, outPitch );
		}
	}
}


TEST( PackedYawPitch_true_4_4_roundtrip )
{
	PackedYawPitch< /* HALFPITCH */ true, 4, 4 > inPYP, outPYP;
	MemoryOStream stream;
	float inYaw, inPitch, outYaw, outPitch;

	const float step = MATH_PI / 256.f;

	for (int i = -256; i < 256; ++i)
	{
		const float yaw = step * i;

		for (int j = -128; j < 128; ++j)
		{
			const float pitch = step * j;

			stream.reset();
			inPYP.set( yaw, pitch );
			stream << inPYP;
			CHECK_EQUAL( 1, stream.size() );

			stream >> outPYP;
			CHECK_EQUAL( 0, stream.remainingLength() );

			inPYP.get( inYaw, inPitch );
			outPYP.get( outYaw, outPitch );

			CHECK_EQUAL( inYaw, outYaw );
			CHECK_EQUAL( inPitch, outPitch );
		}
	}
}


TEST( PackedYawPitch_false_8_8_roundtrip )
{
	PackedYawPitch< /* HALFPITCH */ false, 8, 8 > inPYP, outPYP;
	MemoryOStream stream;
	float inYaw, inPitch, outYaw, outPitch;

	const float step = MATH_PI / 256.f;

	for (int i = -256; i < 256; ++i)
	{
		const float yaw = step * i;

		for (int j = -256; j < 256; ++j)
		{
			const float pitch = step * j;

			stream.reset();
			inPYP.set( yaw, pitch );
			stream << inPYP;
			CHECK_EQUAL( 2, stream.size() );

			// If this immediately precedes outPYP.get(),
			// gcc from CentOS 6 "4.4.6 20120305 (Red Hat 4.4.6-4)" will
			// miscompile the latter under -O3 and the code will abort
			// with the error "pure virtual method called"
			inPYP.get( inYaw, inPitch );

			stream >> outPYP;
			CHECK_EQUAL( 0, stream.remainingLength() );

			outPYP.get( outYaw, outPitch );

			CHECK_EQUAL( inYaw, outYaw );
			CHECK_EQUAL( inPitch, outPitch );
		}
	}
}


TEST( PackedYawPitch_true_8_8_roundtrip )
{
	PackedYawPitch< /* HALFPITCH */ true, 8, 8 > inPYP, outPYP;
	MemoryOStream stream;
	float inYaw, inPitch, outYaw, outPitch;

	const float step = MATH_PI / 256.f;

	for (int i = -256; i < 256; ++i)
	{
		const float yaw = step * i;

		for (int j = -128; j < 128; ++j)
		{
			const float pitch = step * j;

			stream.reset();
			inPYP.set( yaw, pitch );
			stream << inPYP;
			CHECK_EQUAL( 2, stream.size() );

			// If this immediately precedes outPYP.get(),
			// gcc from CentOS 6 "4.4.6 20120305 (Red Hat 4.4.6-4)" will
			// miscompile the latter under -O3 and the code will abort
			// with the error "pure virtual method called"
			inPYP.get( inYaw, inPitch );

			stream >> outPYP;
			CHECK_EQUAL( 0, stream.remainingLength() );

			outPYP.get( outYaw, outPitch );

			CHECK_EQUAL( inYaw, outYaw );
			CHECK_EQUAL( inPitch, outPitch );
		}
	}
}


TEST( PackedYawPitchRoll_true_4_4_4_roundtrip )
{
	PackedYawPitchRoll< /* HALFPITCH */ true, 4, 4, 4 > inPYPR, outPYPR;
	MemoryOStream stream;
	float inYaw, inPitch, inRoll, outYaw, outPitch, outRoll;

	const float step = MATH_PI / 256.f;

	for (int i = -256; i < 256; ++i)
	{
		const float yaw = step * i;

		for (int j = -128; j < 128; ++j)
		{
			const float pitch = step * j;

			for (int k = -256; k < 255; ++k)
			{
				const float roll = step * k;

				stream.reset();
				inPYPR.set( yaw, pitch, roll );
				stream << inPYPR;
				CHECK_EQUAL( 2, stream.size() );

				// If this immediately precedes outPYPR.get(),
				// gcc from CentOS 6 "4.4.6 20120305 (Red Hat 4.4.6-4)" will
				// miscompile the latter under -O3 and the code will abort
				// with the error "pure virtual method called"
				inPYPR.get( inYaw, inPitch, inRoll );

				stream >> outPYPR;
				CHECK_EQUAL( 0, stream.remainingLength() );

				outPYPR.get( outYaw, outPitch, outRoll );

				CHECK_EQUAL( inYaw, outYaw );
				CHECK_EQUAL( inPitch, outPitch );
				CHECK_EQUAL( inRoll, outRoll );
			}
		}
	}
}


TEST( PackedYawPitchRoll_true_6_5_5_roundtrip )
{
	PackedYawPitchRoll< /* HALFPITCH */ true, 6, 5, 5 > inPYPR, outPYPR;
	MemoryOStream stream;
	float inYaw, inPitch, inRoll, outYaw, outPitch, outRoll;

	const float step = MATH_PI / 256.f;

	for (int i = -256; i < 256; ++i)
	{
		const float yaw = step * i;

		for (int j = -128; j < 128; ++j)
		{
			const float pitch = step * j;

			for (int k = -256; k < 255; ++k)
			{
				const float roll = step * k;

				stream.reset();
				inPYPR.set( yaw, pitch, roll );
				stream << inPYPR;
				CHECK_EQUAL( 2, stream.size() );

				// If this immediately precedes outPYPR.get(),
				// gcc from CentOS 6 "4.4.6 20120305 (Red Hat 4.4.6-4)" will
				// miscompile the latter under -O3 and the code will abort
				// with the error "pure virtual method called"
				inPYPR.get( inYaw, inPitch, inRoll );

				stream >> outPYPR;
				CHECK_EQUAL( 0, stream.remainingLength() );

				outPYPR.get( outYaw, outPitch, outRoll );

				CHECK_EQUAL( inYaw, outYaw );
				CHECK_EQUAL( inPitch, outPitch );
				CHECK_EQUAL( inRoll, outRoll );
			}
		}
	}
}


TEST( PackedYawPitchRoll_true_8_8_8_roundtrip )
{
	PackedYawPitchRoll< /* HALFPITCH */ true, 8, 8, 8 > inPYPR, outPYPR;
	MemoryOStream stream;
	float inYaw, inPitch, inRoll, outYaw, outPitch, outRoll;

	const float step = MATH_PI / 256.f;

	for (int i = -256; i < 256; ++i)
	{
		const float yaw = step * i;

		for (int j = -128; j < 128; ++j)
		{
			const float pitch = step * j;

			for (int k = -256; k < 255; ++k)
			{
				const float roll = step * k;

				stream.reset();
				inPYPR.set( yaw, pitch, roll );
				stream << inPYPR;
				CHECK_EQUAL( 3, stream.size() );

				// If this immediately precedes outPYPR.get(),
				// gcc from CentOS 6 "4.4.6 20120305 (Red Hat 4.4.6-4)" will
				// miscompile the latter under -O3 and the code will abort
				// with the error "pure virtual method called"
				inPYPR.get( inYaw, inPitch, inRoll );

				stream >> outPYPR;
				CHECK_EQUAL( 0, stream.remainingLength() );

				outPYPR.get( outYaw, outPitch, outRoll );

				CHECK_EQUAL( inYaw, outYaw );
				CHECK_EQUAL( inPitch, outPitch );
				CHECK_EQUAL( inRoll, outRoll );
			}
		}
	}
}


TEST( PackedGroundPos_3_8_testError_scale_1 )
{
	PackedGroundPos< /* EXPONENT_BITS */ 3, /* MANTISSA_BITS */ 8 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 1.f;

	CHECK_EQUAL( -509.5f, pos.minLimit( scale ) );
	CHECK_EQUAL( 509.5f, pos.maxLimit( scale ) );

	for (int i = (int)floorf( pos.minLimit( scale ) );
		i < (int)ceilf( pos.maxLimit( scale ) );
		++i)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXZ( inputs[ j ], inputs[ j ], scale );
			pos.unpackXZ( resultsX[ j ], resultsZ[ j ], scale );
			pos.getXZError( errorsX[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < pos.minLimit( scale ) ||
				input >= pos.maxLimit( scale ))
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}
	}
}


TEST( PackedGroundPos_3_8_testError_scale_05 )
{
	PackedGroundPos< /* EXPONENT_BITS */ 3, /* MANTISSA_BITS */ 8 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 0.5f;

	CHECK_EQUAL( -254.75f, pos.minLimit( scale ) );
	CHECK_EQUAL( 254.75f, pos.maxLimit( scale ) );

	for (int i = (int)floorf( pos.minLimit( scale ) );
		i < (int)ceilf( pos.maxLimit( scale ) );
		++i)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXZ( inputs[ j ], inputs[ j ], scale );
			pos.unpackXZ( resultsX[ j ], resultsZ[ j ], scale );
			pos.getXZError( errorsX[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < pos.minLimit( scale ) ||
				input >= pos.maxLimit( scale ))
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}
	}
}


TEST( PackedGroundPos_4_19_testError_scale_1 )
{
	PackedGroundPos< /* EXPONENT_BITS */ 4, /* MANTISSA_BITS */ 19 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 1.f;

	CHECK_EQUAL( -131069.94f, pos.minLimit( scale ) );
	CHECK_EQUAL( 131069.94f, pos.maxLimit( scale ) );

	for (int i = (int)floorf( pos.minLimit( scale ) );
		i < (int)ceilf( pos.maxLimit( scale ) );
		++i)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXZ( inputs[ j ], inputs[ j ], scale );
			pos.unpackXZ( resultsX[ j ], resultsZ[ j ], scale );
			pos.getXZError( errorsX[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < pos.minLimit( scale ) ||
				input >= pos.maxLimit( scale ))
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			// TODO: Known rounding failures...
			if (((i == -65536 || i == -65535) && (j % 64 == 34)) ||
				((i == -32768 || i == -32767) && (j % 32 == 17)))
			{
				CHECK( nextX != currX );
				std::swap( nextX, currX );
				std::swap( nextZ, currZ );
			}

			// TODO: Known rounding failures...
			if (((i == 32766 || i == 32767) && (j % 32 == 15)) ||
				((i == 65534 || i == 65535) && (j % 64 == 30)))
			{
				CHECK( prevX != currX );
				std::swap( prevX, currX );
				std::swap( prevZ, currZ );
			}

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}
	}
}


TEST( PackedGroundPos_4_19_testError_scale_05 )
{
	PackedGroundPos< /* EXPONENT_BITS */ 4, /* MANTISSA_BITS */ 19 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 0.5f;

	CHECK_EQUAL( -65534.97f, pos.minLimit( scale ) );
	CHECK_EQUAL( 65534.97f, pos.maxLimit( scale ) );

	for (int i = (int)floorf( pos.minLimit( scale ) );
		i < (int)ceilf( pos.maxLimit( scale ) );
		++i)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXZ( inputs[ j ], inputs[ j ], scale );
			pos.unpackXZ( resultsX[ j ], resultsZ[ j ], scale );
			pos.getXZError( errorsX[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < pos.minLimit( scale ) ||
				input >= pos.maxLimit( scale ))
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			// TODO: Known rounding failures...
			if ((i == -32768) && (j % 32 == 17))
			{
				CHECK( nextX != currX );
				std::swap( nextX, currX );
				std::swap( nextZ, currZ );
			}

			// TODO: Known rounding failures...
			if ((i == 32767) && (j % 32 == 15))
			{
				CHECK( prevX != currX );
				std::swap( prevX, currX );
				std::swap( prevZ, currZ );
			}

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}
	}
}


TEST( PackedGroundPos_5_8_testError_scale_1 )
{
	PackedGroundPos< /* EXPONENT_BITS */ 5, /* MANTISSA_BITS */ 8 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 1.f;

	float minLimit = pos.minLimit( scale );
	float maxLimit = pos.maxLimit( scale );

	CHECK_EQUAL( -8581545984.00f, minLimit );
	CHECK_EQUAL( 8581545984.00f, maxLimit );

	//Use step to shorten number of loops.
	const float numIterations = 100000.f;
	const float step = (maxLimit - minLimit) / numIterations;
	const float startOffset = step *
			(static_cast<float>( rand() ) / static_cast<float>( RAND_MAX ));

	for (float i = floorf( minLimit ) + startOffset;
		i < ceilf( maxLimit );
		i = i + step)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXZ( inputs[ j ], inputs[ j ], scale );
			pos.unpackXZ( resultsX[ j ], resultsZ[ j ], scale );
			pos.getXZError( errorsX[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < minLimit ||
				input >= maxLimit)
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}
	}
}


TEST( PackedGroundPos_5_8_testError_scale_05 )
{
	PackedGroundPos< /* EXPONENT_BITS */ 5, /* MANTISSA_BITS */ 8 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 0.5f;

	float minLimit = pos.minLimit( scale );
	float maxLimit = pos.maxLimit( scale );

	CHECK_EQUAL( -4290772992.00f, minLimit );
	CHECK_EQUAL( 4290772992.00f, maxLimit );

	//Use step to shorten number of loops.
	const float numIterations = 100000.f;
	const float step = (maxLimit - minLimit) / numIterations;
	const float startOffset = step *
			(static_cast<float>( rand() ) / static_cast<float>( RAND_MAX ));

	for (float i = floorf( minLimit ) + startOffset;
		i < ceilf( maxLimit );
		i = i + step)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXZ( inputs[ j ], inputs[ j ], scale );
			pos.unpackXZ( resultsX[ j ], resultsZ[ j ], scale );
			pos.getXZError( errorsX[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < minLimit ||
				input >= maxLimit)
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}
	}
}


TEST( PackedGroundPos_6_19_testError_scale_1 )
{
	PackedGroundPos< /* EXPONENT_BITS */ 6, /* MANTISSA_BITS */ 19 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 1.f;

	float minLimit = pos.minLimit( scale );
	float maxLimit = pos.maxLimit( scale );

	CHECK_EQUAL( -36893470555233058816.00f, minLimit );
	CHECK_EQUAL( 36893470555233058816.00f, maxLimit );

	//Use step to shorten number of loops.
	const float numIterations = 100000.f;
	const float step = (maxLimit - minLimit) / numIterations;
	const float startOffset = step *
			(static_cast<float>( rand() ) / static_cast<float>( RAND_MAX ));
	
	for (float i = floorf( minLimit ) + startOffset;
		i < ceilf( maxLimit );
		i = i + step)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXZ( inputs[ j ], inputs[ j ], scale );
			pos.unpackXZ( resultsX[ j ], resultsZ[ j ], scale );
			pos.getXZError( errorsX[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < minLimit ||
				input >= maxLimit)
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			// TODO: Known rounding failures...
			if (((((int)i) == -65536 || ((int)i) == -65535) && (j % 64 == 34)) ||
				((((int)i) == -32768 || ((int)i) == -32767) && (j % 32 == 17)))
			{
				CHECK( nextX != currX );
				std::swap( nextX, currX );
				std::swap( nextZ, currZ );
			}

			// TODO: Known rounding failures...
			if (((((int)i) == 32766 || ((int)i) == 32767) && (j % 32 == 15)) ||
				((((int)i) == 65534 || ((int)i) == 65535) && (j % 64 == 30)))
			{
				CHECK( prevX != currX );
				std::swap( prevX, currX );
				std::swap( prevZ, currZ );
			}

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}
	}
}


TEST( PackedGroundPos_6_19_testError_scale_05 )
{
	PackedGroundPos< /* EXPONENT_BITS */ 6, /* MANTISSA_BITS */ 19 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 0.5f;

	float minLimit = pos.minLimit( scale );
	float maxLimit = pos.maxLimit( scale );

	CHECK_EQUAL( -18446735277616529408.00f, minLimit );
	CHECK_EQUAL( 18446735277616529408.00f, maxLimit );

	//Use step to shorten number of loops.
	const float numIterations = 100000.f;
	const float step = (maxLimit - minLimit) / numIterations;
	const float startOffset = step * 
			(static_cast<float>( rand() ) / static_cast<float>( RAND_MAX ));

	for (float i = floorf( minLimit ) + startOffset;
		i < ceilf( maxLimit );
		i = i + step)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXZ( inputs[ j ], inputs[ j ], scale );
			pos.unpackXZ( resultsX[ j ], resultsZ[ j ], scale );
			pos.getXZError( errorsX[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < minLimit ||
				input >= maxLimit)
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			// TODO: Known rounding failures...
			if ((((int)i) == -32768) && (j % 32 == 17))
			{
				CHECK( nextX != currX );
				std::swap( nextX, currX );
				std::swap( nextZ, currZ );
			}

			// TODO: Known rounding failures...
			if ((((int)i) == 32767) && (j % 32 == 15))
			{
				CHECK( prevX != currX );
				std::swap( prevX, currX );
				std::swap( prevZ, currZ );
			}

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}
	}
}


TEST( PackedFullPos_3_8_4_11_testError_scale_1 )
{
	PackedFullPos< /* EXPONENT_BITS */ 3, /* MANTISSA_BITS */ 8,
		/* EXPONENT_BITS_Y */ 4, /* MANTISSA_BITS_Y */ 11 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsY[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsY[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 1.f;

	CHECK_EQUAL( -509.5f, pos.minLimit( scale ) );
	CHECK_EQUAL( 509.5f, pos.maxLimit( scale ) );

	CHECK_EQUAL( -131054.f, pos.minYLimit());
	CHECK_EQUAL( 131054.f, pos.maxYLimit());

	const float yFactor = floorf(
		pos.maxYLimit() / pos.maxLimit( scale ) );

	for (int i = (int)floorf( pos.minLimit( scale ) );
		i < (int)ceilf( pos.maxLimit( scale ) );
		++i)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXYZ( inputs[ j ], inputs[ j ] * yFactor, inputs[ j ],
				scale );
			pos.unpackXYZ( resultsX[ j ], resultsY[ j ], resultsZ[ j ],
				scale );
			pos.getXYZError( errorsX[ j ], errorsY[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < pos.minLimit( scale ) ||
				input >= pos.maxLimit( scale ))
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ] * yFactor;

			if (input < pos.minYLimit() ||
				input >= pos.maxYLimit())
			{
				// Out-of-range
				continue;
			}

			float prevY = ( j == 0 ) ? resultsY[ 0 ] : resultsY[ j - 1 ];
			float currY = resultsY[ j ];
			float nextY = ( j == 511 ) ? resultsY[ 511 ] : resultsY[ j + 1 ];
			float errorY = errorsY[ j ];

			CHECK( prevY <= currY );
			CHECK( currY <= nextY );

			// Note that the Y value for 4,11 is not rounded, so the error
			// returned by errors[] is actually all on the away-from-zero side.
			// Adjust the numbers to make the test work.
			errorY /= 2.f;
			if (input >= 0)
			{
				// nextY may be closer that currY if we should have rounded
				CHECK( fabsf( input - currY ) <= fabsf( input - prevY ) );
				prevY += errorY;
				currY += errorY;
				nextY += errorY;
			}
			else
			{
				// prevY may be closer that currY if we should have rounded
				CHECK( fabsf( input - currY ) <= fabsf( input - nextY ) );
				prevY -= errorY;
				currY -= errorY;
				nextY -= errorY;
			}

			CHECK_CLOSE( input, currY, errorY );
		}
	}
}


TEST( PackedFullPos_3_8_4_11_testError_scale_05 )
{
	PackedFullPos< /* EXPONENT_BITS */ 3, /* MANTISSA_BITS */ 8,
		/* EXPONENT_BITS_Y */ 4, /* MANTISSA_BITS_Y */ 11 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsY[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsY[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 0.5f;

	CHECK_EQUAL( -254.75f, pos.minLimit( scale ) );
	CHECK_EQUAL( 254.75f, pos.maxLimit( scale ) );

	CHECK_EQUAL( -131054.f, pos.minYLimit());
	CHECK_EQUAL( 131054.f, pos.maxYLimit());

	const float yFactor = floorf(
		pos.maxYLimit() / pos.maxLimit( scale ) );

	for (int i = (int)floorf( pos.minLimit( scale ) );
		i < (int)ceilf( pos.maxLimit( scale ) );
		++i)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXYZ( inputs[ j ], inputs[ j ] * yFactor, inputs[ j ],
				scale );
			pos.unpackXYZ( resultsX[ j ], resultsY[ j ], resultsZ[ j ],
				scale );
			pos.getXYZError( errorsX[ j ], errorsY[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < pos.minLimit( scale ) ||
				input >= pos.maxLimit( scale ))
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ] * yFactor;

			if (input < pos.minYLimit() ||
				input >= pos.maxYLimit())
			{
				// Out-of-range
				continue;
			}

			float prevY = ( j == 0 ) ? resultsY[ 0 ] : resultsY[ j - 1 ];
			float currY = resultsY[ j ];
			float nextY = ( j == 511 ) ? resultsY[ 511 ] : resultsY[ j + 1 ];
			float errorY = errorsY[ j ];

			CHECK( prevY <= currY );
			CHECK( currY <= nextY );

			// Note that the Y value for 4,11 is not rounded, so the error
			// returned by errors[] is actually all on the away-from-zero side.
			// Adjust the numbers to make the test work.
			errorY /= 2.f;
			if (input >= 0)
			{
				// nextY may be closer that currY if we should have rounded
				CHECK( fabsf( input - currY ) <= fabsf( input - prevY ) );
				prevY += errorY;
				currY += errorY;
				nextY += errorY;
			}
			else
			{
				// prevY may be closer that currY if we should have rounded
				CHECK( fabsf( input - currY ) <= fabsf( input - nextY ) );
				prevY -= errorY;
				currY -= errorY;
				nextY -= errorY;
			}

			CHECK_CLOSE( input, currY, errorY );
		}
	}
}


TEST( PackedFullPos_4_11_3_8_testError_scale_1 )
{
	PackedFullPos< /* EXPONENT_BITS */ 4, /* MANTISSA_BITS */ 11,
		/* EXPONENT_BITS_Y */ 3, /* MANTISSA_BITS_Y */ 8 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsY[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsY[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 1.f;

	CHECK_EQUAL( -131054.f, pos.minLimit( scale ));
	CHECK_EQUAL( 131054.f, pos.maxLimit( scale ));

	CHECK_EQUAL( -509.5f, pos.minYLimit() );
	CHECK_EQUAL( 509.5f, pos.maxYLimit() );

	const float yFactor = floorf(
		pos.maxYLimit() / pos.maxLimit( scale ) );

	for (int i = (int)floorf( pos.minLimit( scale ) );
		i < (int)ceilf( pos.maxLimit( scale ) );
		++i)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXYZ( inputs[ j ], inputs[ j ] * yFactor, inputs[ j ],
				scale );
			pos.unpackXYZ( resultsX[ j ], resultsY[ j ], resultsZ[ j ],
				scale );
			pos.getXYZError( errorsX[ j ], errorsY[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < pos.minLimit( scale ) ||
				input >= pos.maxLimit( scale ))
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ] * yFactor;

			if (input < pos.minYLimit() ||
				input >= pos.maxYLimit())
			{
				// Out-of-range
				continue;
			}

			float prevY = ( j == 0 ) ? resultsY[ 0 ] : resultsY[ j - 1 ];
			float currY = resultsY[ j ];
			float nextY = ( j == 511 ) ? resultsY[ 511 ] : resultsY[ j + 1 ];
			float errorY = errorsY[ j ];

			CHECK( prevY <= currY );
			CHECK( currY <= nextY );

			CHECK( fabsf( input - currY ) <= fabsf( input - prevY ) );
			CHECK( fabsf( input - currY ) <= fabsf( input - nextY ) );

			CHECK_CLOSE( input, currY, errorY );
		}
	}
}


TEST( PackedFullPos_4_11_3_8_testError_scale_05 )
{
	PackedFullPos< /* EXPONENT_BITS */ 4, /* MANTISSA_BITS */ 11,
		/* EXPONENT_BITS_Y */ 3, /* MANTISSA_BITS_Y */ 8 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsY[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsY[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 0.5f;

	CHECK_EQUAL( -65527.f, pos.minLimit( scale ));
	CHECK_EQUAL( 65527.f, pos.maxLimit( scale ));

	CHECK_EQUAL( -509.5f, pos.minYLimit() );
	CHECK_EQUAL( 509.5f, pos.maxYLimit() );

	const float yFactor = floorf(
		pos.maxYLimit() / pos.maxLimit( scale ) );

	for (int i = (int)floorf( pos.minLimit( scale ) );
		i < (int)ceilf( pos.maxLimit( scale ) );
		++i)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXYZ( inputs[ j ], inputs[ j ] * yFactor, inputs[ j ],
				scale );
			pos.unpackXYZ( resultsX[ j ], resultsY[ j ], resultsZ[ j ],
				scale );
			pos.getXYZError( errorsX[ j ], errorsY[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < pos.minLimit( scale ) ||
				input >= pos.maxLimit( scale ))
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ] * yFactor;

			if (input < pos.minYLimit() ||
				input >= pos.maxYLimit())
			{
				// Out-of-range
				continue;
			}

			float prevY = ( j == 0 ) ? resultsY[ 0 ] : resultsY[ j - 1 ];
			float currY = resultsY[ j ];
			float nextY = ( j == 511 ) ? resultsY[ 511 ] : resultsY[ j + 1 ];
			float errorY = errorsY[ j ];

			CHECK( prevY <= currY );
			CHECK( currY <= nextY );

			CHECK( fabsf( input - currY ) <= fabsf( input - prevY ) );
			CHECK( fabsf( input - currY ) <= fabsf( input - nextY ) );

			CHECK_CLOSE( input, currY, errorY );
		}
	}
}


// Fixed-point tests

TEST( PackedGroundPos_0_11_testError_scale_1 )
{
	PackedGroundPos< /* EXPONENT_BITS */ 0, /* MANTISSA_BITS */ 11 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 1.f;

	CHECK_EQUAL( -1.9995117f, pos.minLimit( scale ) );
	CHECK_EQUAL( 1.9995117f, pos.maxLimit( scale ) );

	for (int i = (int)floorf( pos.minLimit( scale ) );
		i < (int)ceilf( pos.maxLimit( scale ) );
		++i)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXZ( inputs[ j ], inputs[ j ], scale );
			pos.unpackXZ( resultsX[ j ], resultsZ[ j ], scale );
			pos.getXZError( errorsX[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < pos.minLimit( scale ) ||
				input >= pos.maxLimit( scale ))
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}
	}
}


TEST( PackedGroundPos_0_11_testError_scale_05 )
{
	PackedGroundPos< /* EXPONENT_BITS */ 0, /* MANTISSA_BITS */ 11 > pos;

	float inputs[ 512 ];
	float resultsX[ 512 ];
	float resultsZ[ 512 ];
	float errorsX[ 512 ];
	float errorsZ[ 512 ];

	const float scale = 0.5f;

	CHECK_EQUAL( -0.99975586f, pos.minLimit( scale ) );
	CHECK_EQUAL( 0.99975586f, pos.maxLimit( scale ) );

	for (int i = (int)floorf( pos.minLimit( scale ) );
		i < (int)ceilf( pos.maxLimit( scale ) );
		++i)
	{
		for (int j = 0; j < 512; ++j)
		{
			inputs[j] = i + (j / 512.f);
			pos.packXZ( inputs[ j ], inputs[ j ], scale );
			pos.unpackXZ( resultsX[ j ], resultsZ[ j ], scale );
			pos.getXZError( errorsX[ j ], errorsZ[ j ], scale );
		}

		for (int j = 0; j < 512; ++j)
		{
			float input = inputs[ j ];

			if (input < pos.minLimit( scale ) ||
				input >= pos.maxLimit( scale ))
			{
				// Out-of-range
				continue;
			}

			float prevX = ( j == 0 ) ? resultsX[ 0 ] : resultsX[ j - 1 ];
			float currX = resultsX[ j ];
			float nextX = ( j == 511 ) ? resultsX[ 511 ] : resultsX[ j + 1 ];
			float errorX = errorsX[ j ];

			float prevZ = ( j == 0 ) ? resultsZ[ 0 ] : resultsZ[ j - 1 ];
			float currZ = resultsZ[ j ];
			float nextZ = ( j == 511 ) ? resultsZ[ 511 ] : resultsZ[ j + 1 ];
			float errorZ = errorsZ[ j ];

			CHECK( prevX <= currX );
			CHECK( currX <= nextX );

			CHECK( prevZ <= currZ );
			CHECK( currZ <= nextZ );

			CHECK( fabsf( input - currX ) <= fabsf( input - prevX ) );
			CHECK( fabsf( input - currX ) <= fabsf( input - nextX ) );

			CHECK( fabsf( input - currZ ) <= fabsf( input - prevZ ) );
			CHECK( fabsf( input - currZ ) <= fabsf( input - nextZ ) );

			CHECK_CLOSE( input, currX, errorX );

			CHECK_CLOSE( input, currZ, errorZ );
		}
	}
}


BW_END_NAMESPACE

// test_msgtypes.cpp
