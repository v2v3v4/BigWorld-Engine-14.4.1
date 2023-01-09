#include "pch.hpp"

#include "physics2/worldtri.hpp"


BW_BEGIN_NAMESPACE

struct Fixture
{
	Fixture()
	{
		triangle_ = new WorldTriangle(	Vector3( 0.0f, 0.0f, 0.0f ),
										Vector3( 0.0f, 0.0f, 1.0f ),
										Vector3( 0.0f, 1.0f, 0.0f ) );
	}

	~Fixture()
	{
		bw_safe_delete(triangle_);
	}

	WorldTriangle* triangle_;
};

TEST_F( Fixture, WorldTri_Construction )
{
	WorldTriangle wt;

	CHECK( wt.v0()		== Vector3::zero() );
	CHECK( wt.v1()		== Vector3::zero() );
	CHECK( wt.v2()		== Vector3::zero() );
	CHECK( wt.flags()	== 0			   );

	WorldTriangle wt2(	Vector3::zero(),
						Vector3( 1.0f, 2.0f, 3.0f ),
						Vector3( 3.0f, 2.0f, 1.0f ),
						65535 );

	CHECK( wt2.v0()		== Vector3::zero()				);
	CHECK( wt2.v1()		== Vector3( 1.0f, 2.0f, 3.0f )	);
	CHECK( wt2.v2()		== Vector3( 3.0f, 2.0f, 1.0f )	);
	CHECK( wt2.flags()	== 65535						);
}

TEST_F( Fixture, WorldTri_VMethods )
{
	CHECK( triangle_->v0() == triangle_->v(0) );
	CHECK( triangle_->v1() == triangle_->v(1) );
	CHECK( triangle_->v2() == triangle_->v(2) );
}

TEST_F( Fixture, WorldTri_Normal )
{
	Vector3 n = triangle_->normal();

	CHECK( isEqual( n.x, -1.0f ) );
	CHECK( isZero( n.y ) );
	CHECK( isZero( n.z ) );
}

TEST_F( Fixture, WorldTri_Flags )
{
	CHECK_EQUAL( false, triangle_->isBlended() );
	CHECK_EQUAL( false, triangle_->isTransparent() );

	triangle_->flags( TRIANGLE_BLENDED | TRIANGLE_TRANSPARENT );

	CHECK_EQUAL( true, triangle_->isBlended() );
	CHECK_EQUAL( true, triangle_->isTransparent() );
}

TEST_F( Fixture, WorldTri_Equality )
{
	WorldTriangle wt;

	CHECK( !(wt == *triangle_) );

	WorldTriangle wt2(	Vector3::zero(),
						Vector3( 0.0f, 0.0f, 1.0f ),
						Vector3( 0.0f, 1.0f, 0.0f )  );

	CHECK( wt2 == *triangle_ );
}

TEST_F( Fixture, WorldTri_Intersects )
{
	// Null triangle should not intersect
	WorldTriangle null;

	CHECK_EQUAL( false, triangle_->intersects( null ) );
	CHECK_EQUAL( false, null.intersects( *triangle_ ) );

	// Good triangle should intersect (on same plane)
	WorldTriangle good(	Vector3( 0.0f, 0.0f, 0.0f ),
						Vector3( 0.0f, 0.0f, 1.0f ),
						Vector3( 0.0f, 1.0f, 1.0f ) );

	CHECK_EQUAL( false, triangle_->intersects( good ) );
	CHECK_EQUAL( false, good.intersects( *triangle_ ) );

	WorldTriangle triangleA( Vector3( 20.046995, 8.2170086, -3.5280411),
		Vector3( 14.806141, -2.0349665, -3.4898233),
		Vector3( 14.795850, 8.2146082, -3.4820471) );

	WorldTriangle triangleB( Vector3( 15.789289, 6.2242312, -3.8863811),
		Vector3( 15.686880, 6.2242312, -3.6270800),
		Vector3( 16.124022, 6.2242312, -3.4544334) );

	CHECK_EQUAL( true, triangleA.intersects( triangleB, Vector3(0,-0.91617918, 0) ) );


	triangleA = WorldTriangle( Vector3( 0.090930186, -0.68017411, -1.7901510 ),
		Vector3( -0.43109962, -0.65896052, -0.30945054 ),
		Vector3( -0.38739645, 0.0087520061, -1.1047082 ) );

	triangleB = WorldTriangle( Vector3( -0.71026230, 0.77062988, -1.2783508 ),
		Vector3( -0.66361618, 0.77062988, -1.0453491 ),
		Vector3( -0.27078629, 0.77062988, -1.1239777 ) );

	CHECK_EQUAL( true, triangleA.intersects( triangleB, Vector3( 0.00000000, -0.78094482, 0.00000000 ) ) );

	// Test case for when the triangle prism intersects exactly with one of the
	// points in the collision triangle
	triangleA = WorldTriangle( Vector3( 2, 0, 3 ),
		Vector3( 1, 0, 1 ),
		Vector3( 3, 0, 1 ) );

	triangleB = WorldTriangle( Vector3( 0, 7, 4 ),
		Vector3( 0, 7, 0 ),
		Vector3( 4, -7, 2 ) );

	CHECK_EQUAL( true, triangleA.intersects( triangleB, Vector3( 0.00000000, -0.78094482, 0.00000000 ) ) );

	
	// Test case for when the start of the volume intersects with the plane of the
	// test triangle but the test triangle does not intersect the test volume
	// The 2d projection that is used for the collision does intersect
	triangleA = WorldTriangle( Vector3( 0, 0, 0 ),
		Vector3( 1, 0, 1 ),
		Vector3( 1, 0, 0 ) );

	triangleB = WorldTriangle( Vector3( 0.f, -0.1f, .5f ),
		Vector3( 0.75f, 0.4f, 0.25f ),
		Vector3( .5f, -0.1f, 1.f ) );

	CHECK_EQUAL( false, triangleA.intersects( triangleB, Vector3( 0.00000000, 0.6f, 0.00000000 ) ) );

	
	// Test case for when the end of the volume intersects with the plane of the
	// test triangle but the test triangle does not intersect the test volume
	// The 2d projection that is used for the collision does intersect
	triangleA = WorldTriangle( Vector3( 0, 0, 0 ),
		Vector3( 1, 0, 1 ),
		Vector3( 1, 0, 0 ) );

	triangleB = WorldTriangle( Vector3( 0.f, 0.5f, .5f ),
		Vector3( 0.75f, 1.f, 0.25f ),
		Vector3( .5f, .5f, 1.f ) );

	CHECK_EQUAL( false, triangleA.intersects( triangleB, Vector3( 0.00000000, -0.6f, 0.00000000 ) ) );

	// TODO add more interesting test cases here...
}

BW_END_NAMESPACE

// test_particle.cpp
