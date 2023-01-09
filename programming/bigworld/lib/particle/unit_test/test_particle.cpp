#include "pch.hpp"

#include "particle/particle.hpp"


BW_BEGIN_NAMESPACE

TEST( Particle_Construction )
{
	// There is a larger difference in the getter and setter methods 
	// with the float values when dealing with conversions to and from
	// 8-bit and 16-bit uints.
	const float epsilon8 = 0.04f;
	const float epsilon16 = 0.004f;

	Vector3 position(1.0f, 2.0f, 3.0f);
	Vector3 velocity(4.0f, 5.0f, 6.0f);
	Vector4 colour(7.0f, 8.0f, 9.0f, 10.f);

	Particle p1( position, velocity, colour,
		1.0f, 0.3f, 0.7f, 1.0f );

	CHECK_EQUAL( 1.0f, p1.position().x );
	CHECK_EQUAL( 2.0f, p1.position().y );
	CHECK_EQUAL( 3.0f, p1.position().z );

	Vector3 returnVelocity;
	p1.getVelocity( returnVelocity );
	CHECK_EQUAL( 4.0f, returnVelocity.x );
	CHECK_EQUAL( 5.0f, returnVelocity.y );
	CHECK_EQUAL( 6.0f, returnVelocity.z );

	CHECK_EQUAL( Colour::getUint32FromNormalised(colour), p1.colour() );

	CHECK_CLOSE( 1.0f, p1.size(), epsilon16 );
	CHECK_CLOSE( 0.3f, p1.pitch(), epsilon8 );
	CHECK_CLOSE( 0.7f, p1.yaw(), epsilon8 );
	CHECK_CLOSE( 1.0f, p1.age(), epsilon16 );

	Vector3 spinAxis( 7.0f, 8.0f, 9.0f );
	spinAxis.normalise();
	Particle p2( position, velocity, 0.4f, 0.5f,
		spinAxis, 0.6f, 13.0f );

	CHECK_EQUAL( 1.0f, p2.position().x );
	CHECK_EQUAL( 2.0f, p2.position().y );
	CHECK_EQUAL( 3.0f, p2.position().z );

	p2.getVelocity( returnVelocity );
	CHECK_EQUAL( 4.0f, returnVelocity.x );
	CHECK_EQUAL( 5.0f, returnVelocity.y );
	CHECK_EQUAL( 6.0f, returnVelocity.z );

	CHECK_CLOSE( spinAxis.x, p2.meshSpinAxis().x, epsilon16 );
	CHECK_CLOSE( spinAxis.y, p2.meshSpinAxis().y, epsilon16 );
	CHECK_CLOSE( spinAxis.z, p2.meshSpinAxis().z, epsilon16 );

	CHECK_CLOSE( 0.4f, p2.pitch(), epsilon8 );
	CHECK_CLOSE( 0.5f, p2.yaw(), epsilon8 );
	CHECK_EQUAL( 0.6f, p2.meshSpinSpeed() );
	CHECK_CLOSE( 13.0f, p2.age(), epsilon16 );
}

BW_END_NAMESPACE

// test_particle.cpp
