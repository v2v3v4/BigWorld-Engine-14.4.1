#include "pch.hpp"

#include "math/xp_math.hpp"
#include "math/boundbox.hpp"
#include "math/vector3.hpp"


BW_BEGIN_NAMESPACE

TEST( BoundingBox_testConstruction )
{	
	// test default constructor - should be inside out with outcodes initialised to
	// zero.
	BoundingBox b1;

	CHECK_EQUAL( true,			b1.insideOut() );
	CHECK_EQUAL( Outcode( 0 ),	b1.outcode() );
	CHECK_EQUAL( Outcode( 0 ),	b1.combinedOutcode() );

	// create with bounds
	BoundingBox b2( Vector3(0.0f,0.0f,0.0f), Vector3(0.0f,0.0f,0.0f) );

	CHECK( b2.maxBounds() == Vector3( 0.0f, 0.0f, 0.0f ) );
	CHECK( b2.minBounds() == Vector3( 0.0f, 0.0f, 0.0f ) );

	BoundingBox b3( Vector3(0.0f,0.0f,0.0f), Vector3(1.0f,1.0f,1.0f) );

	CHECK( b3.maxBounds() == Vector3( 1.0f, 1.0f, 1.0f ) );
	CHECK( b3.minBounds() == Vector3( 0.0f, 0.0f, 0.0f ) );
}

TEST( TestBoundingBox_testSetBounds )
{
	// set bounds
	BoundingBox b;
	b.setBounds( Vector3(0.0f,0.0f,0.0f), Vector3(1.0f,1.0f,1.0f) );
	CHECK( b.maxBounds() == Vector3( 1.0f, 1.0f, 1.0f ) );
	CHECK( b.minBounds() == Vector3( 0.0f, 0.0f, 0.0f ) );
}

TEST( TestBoundingBox_testDimensions )
{
	// set bounds
	BoundingBox b;
	b.setBounds( Vector3(0.0f,0.0f,0.0f), Vector3(1.0f,2.0f,3.0f) );
	CHECK( isEqual( 1.0f, b.width() ) );
	CHECK( isEqual( 2.0f, b.height() ) );
	CHECK( isEqual( 3.0f, b.depth() ) );
}

TEST( TestBoundingBox_testAddBounds )
{
	// test adding points
	BoundingBox b;

	b.addBounds( Vector3(0.0f,0.0f,0.0f) );
	CHECK( b.maxBounds() == Vector3( 0.0f, 0.0f, 0.0f ) );
	CHECK( b.minBounds() == Vector3( 0.0f, 0.0f, 0.0f ) );

	b.addBounds( Vector3(1.0f,1.0f,1.0f) );
	CHECK( b.maxBounds() == Vector3( 1.0f, 1.0f, 1.0f ) );
	CHECK( b.minBounds() == Vector3( 0.0f, 0.0f, 0.0f ) );

	b.addBounds( Vector3(2.0f,2.0f,2.0f) );
	CHECK( b.maxBounds() == Vector3( 2.0f, 2.0f, 2.0f ) );
	CHECK( b.minBounds() == Vector3( 0.0f, 0.0f, 0.0f ) );

	b.addBounds( Vector3(-1.0f, 0.0f, 0.0f) );
	CHECK( b.maxBounds() == Vector3( 2.0f, 2.0f, 2.0f ) );
	CHECK( b.minBounds() == Vector3( -1.0f, 0.0f, 0.0f ) );
}

TEST( TestBoundingBox_testIntersects )
{
	BoundingBox b, a;

	// two point boxes will intersect
	b.addBounds( Vector3(0.0f,0.0f,0.0f) );
	a.addBounds( Vector3(0.0f,0.0f,0.0f) );
	CHECK_EQUAL( true, a.intersects( b ) );

	// a zero vector will not intersect
	CHECK_EQUAL( false, a.intersects( Vector3::zero() ) );

	// Note the above tests were based on existing code - the actual logic
	// seems wrong - ie both should have same result

	// a simple intersection
	b.addBounds(Vector3(1.0f, 1.0f, 1.0f ));
	a.addBounds(Vector3(2.0f, 2.0f, 2.0f ));
	CHECK_EQUAL( true, a.intersects( b ) );
	// simple vector
	CHECK_EQUAL( true, a.intersects( Vector3(0.5f,0.5f,0.5f) ) );

	// TODO boundary tests (eg when boxes share a side).
}

BW_END_NAMESPACE

// test_boundingbox.cpp
