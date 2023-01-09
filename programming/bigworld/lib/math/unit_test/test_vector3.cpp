#include "pch.hpp"

#include <limits>
#include <algorithm>
#include "math/vector3.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

TEST( Vector3_testConstruction )
{	
	BW_GUARD;

	// test constructors
	Vector3 v1( 1.0f, 2.0f, 3.0f );
	CHECK( isEqual( 1.0f, v1.x ) );
	CHECK( isEqual( 2.0f, v1.y ) );
	CHECK( isEqual( 3.0f, v1.z ) );

	Vector3 v2( v1 );
	CHECK( isEqual( 1.0f, v2.x ) );
	CHECK( isEqual( 2.0f, v2.y ) );
	CHECK( isEqual( 3.0f, v2.z ) );
}

TEST( Vector3_testSet )
{
	BW_GUARD;

	Vector3 v1;

	// test setting
	v1.setZero();
	CHECK( isZero( v1.x ) );
	CHECK( isZero( v1.y ) );
	CHECK( isZero( v1.z ) );
	CHECK( v1 == Vector3::ZERO );

	v1.set( 1.0f, 2.0f, 3.0f );
	CHECK( isEqual( 1.0f, v1.x ) );
	CHECK( isEqual( 2.0f, v1.y ) );
	CHECK( isEqual( 3.0f, v1.z ) );
}

TEST( Vector3_testAlmostEqual )
{
	BW_GUARD;

	Vector3 v1( 1.0f, 0.0f, 0.0f );

	// AlmostEqual epsilon is hard-coded to 0.0004f. This will detect if that
	// changes.
	Vector3 yes( 1.0004f, 0.0f, 0.0f );
	Vector3 no( 1.0005f, 0.0f, 0.0f );

	CHECK( almostEqual( v1, yes) == true );
	CHECK( almostEqual( v1, no) == false );
}

TEST( Vector3_testDotProduct )
{
	BW_GUARD;

	Vector3 v1;

	// zero dot product
	v1.setZero();
	CHECK( almostZero( v1.dotProduct( v1 ) ) );

	// unit vector product
	v1.set( 1.0f, 1.0f, 1.0f );
	CHECK( isEqual( 3.0f, v1.dotProduct( v1 ) ) );

}

TEST( Vector3_testNormaliseAndLength )
{
	BW_GUARD;

	Vector3 v1;

	// zero
	v1.setZero();
	CHECK( almostEqual( 0.0f, v1.length() ) );

	// unit length
	v1.set( 1.0f, 0.0f, 0.0f );
	CHECK( almostEqual( 1.0f, v1.length() ) );

	// unit length, normalised
	v1.normalise();
	CHECK( almostEqual( 1.0f, v1.length() ) );

	// other simple normalised
	v1.set( 1.0f, 2.0f, 3.0f );
	v1.normalise();
	CHECK( almostEqual(1.0f, v1.length()) );
}

TEST( Vector3_testLerp )
{
	BW_GUARD;

	Vector3 v1( Vector3::ZERO );
	Vector3 v2( 1.0f, 2.0f, 3.0f );

	Vector3 result;

	result.lerp( v1, v2, 0.25f );
	CHECK( almostEqual( 0.25f, result.x ) );
	CHECK( almostEqual( 0.5f, result.y ) );
	CHECK( almostEqual( 0.75f, result.z ) );

	result.lerp( v1, v2, 0.5f );
	CHECK( almostEqual( 0.5f, result.x ) );
	CHECK( almostEqual( 1.0f, result.y ) );
	CHECK( almostEqual( 1.5f, result.z ) );

	result.lerp( v1, v2, 0.75f );
	CHECK( almostEqual( 0.75f, result.x ) );
	CHECK( almostEqual( 1.5f, result.y ) );
	CHECK( almostEqual( 2.25f, result.z ) );

	//Make sure interpolation works when the output is one of the arguments.
	v1.lerp( v1, v2, 0.75f );
	CHECK_EQUAL( result, v1 );
	v1 = Vector3::ZERO;
	v2.lerp( v1, v2, 0.75f );
	CHECK_EQUAL( result, v2 );
}

TEST( Vector3_testIndexing )
{
	BW_GUARD;

	Vector3 v( 1.0f, 2.0f, 3.0f );

	CHECK( isEqual( v.x, v[0] ) );
	CHECK( isEqual( v.y, v[1] ) );
	CHECK( isEqual( v.z, v[2] ) );
}


namespace
{
	/**
	 *	This functor is used to check angle calculations between the given
	 *	vectors.
	 */
	class CheckAngles
	{
	public:
		/**
		 *	This will compare @a a with @a b, expecting the angle to always.
		 *	be @a angle.
		 *	@pre @a and @a b should be unit vectors.
		 */
		CheckAngles( const Vector3 & a, const Vector3 & b, float angle )
			:
			a_( a )
			, b_( b )
			, angle_( angle )
		{
			BW_GUARD;

			MF_ASSERT( almostEqual( a_.lengthSquared(), 1.0f ) );
			MF_ASSERT( almostEqual( b_.lengthSquared(), 1.0f ) );
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			//Using low precision acos method for angles, need a pretty
			//large epsilon to pass.
			const float EPS = 0.001f;//std::numeric_limits<float>::epsilon();

			Vector3 testA( a_ );
			Vector3 testB( b_ );
			for( int pass = 0; pass < 3; ++pass)
			{
				CHECK_CLOSE(
					angle_, testA.getUnitVectorAngle( testB ), EPS );
				CHECK_CLOSE(
					angle_, testA.getAngle( testB ), EPS );
				//Apply some scales, shouldn't affect the angle.
				CHECK_CLOSE(
					angle_, (testA * 0.35f).getAngle( testB * 2.1f ), EPS );

				if (pass == 0)
				{
					//Swap vectors for the second pass to test that the results
					//are the same.
					using std::swap;
					swap( testA, testB );
				}
				else if (pass == 1)
				{
					//Negate vectors, expect the angle to be unchanged.
					testA = -testA;
					testB = -testB;
				}
			}
		}

	private:
		const Vector3 a_;
		const Vector3 b_;
		const float angle_;
	};
}

TEST( Vector3_angle )
{
	BW_GUARD;

	TEST_SUB_FUNCTOR(
		CheckAngles( Vector3::I, Vector3::I, 0.0f ) );
	TEST_SUB_FUNCTOR(
		CheckAngles( Vector3::I, Vector3::J, MATH_PI * 0.5f ) );
	TEST_SUB_FUNCTOR(
		CheckAngles( Vector3::J, Vector3::K, MATH_PI * 0.5f ) );
	TEST_SUB_FUNCTOR(
		CheckAngles( Vector3::J, -Vector3::J, MATH_PI ) );

	TEST_SUB_FUNCTOR( CheckAngles(
		Vector3(1.0f, 1.0f, 0.0f).unitVector(), Vector3::I, MATH_PI * 0.25f ) );
	TEST_SUB_FUNCTOR( CheckAngles(
		Vector3(1.0f, 1.0f, 0.0f).unitVector(), Vector3::J, MATH_PI * 0.25f ) );
	TEST_SUB_FUNCTOR( CheckAngles(
		Vector3(1.0f, 0.0f, 1.0f).unitVector(), Vector3::K, MATH_PI * 0.25f ) );
}


namespace
{
	/**
	 *	This functor is used to check distance calculations between the given
	 *	points.
	 */
	class CheckDistance
	{
	public:
		/**
		 *	This will compare @a a with @a b, expecting the distance to always.
		 *	be @a distance.
		 *	@pre @a distance is positive.
		 */
		CheckDistance( const Vector3 & a, const Vector3 & b, float distance )
			:
			a_( a )
			, b_( b )
			, distance_( distance )
		{
			BW_GUARD;

			MF_ASSERT( distance >= 0.0f );
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			//Need larger epsilon for the simple float comparison that's used.
			const float EPS = 0.001f;//std::numeric_limits<float>::epsilon();
			
			CHECK_CLOSE( distance_, a_.distance( b_ ), EPS );
			CHECK_CLOSE( distance_, (-a_).distance( -b_ ), EPS );

			const float distSq = distance_ * distance_;

			CHECK_CLOSE( distSq, a_.distanceSquared( b_ ), EPS );
			CHECK_CLOSE( distSq, (-a_).distanceSquared( -b_ ), EPS );
		}

	private:
		const Vector3 a_;
		const Vector3 b_;
		const float distance_;
	};
}

TEST( Vector3_distance )
{
	BW_GUARD;
	
	TEST_SUB_FUNCTOR(
		CheckDistance( Vector3::I, Vector3::J, sqrt(2.0f) ) );
	TEST_SUB_FUNCTOR(
		CheckDistance( Vector3::I, Vector3::K, sqrt(2.0f) ) );
	TEST_SUB_FUNCTOR(
		CheckDistance( Vector3::J, Vector3::K, sqrt(2.0f) ) );
	TEST_SUB_FUNCTOR(
		CheckDistance( Vector3::J, Vector3::ZERO, 1.0f ) );
	
	TEST_SUB_FUNCTOR(
		CheckDistance( Vector3::ZERO, Vector3::ZERO, 0.0f ) );
	TEST_SUB_FUNCTOR(
		CheckDistance( Vector3::I, Vector3::I, 0.0f ) );
	TEST_SUB_FUNCTOR(
		CheckDistance( Vector3::J, Vector3::J, 0.0f ) );
	TEST_SUB_FUNCTOR(
		CheckDistance( Vector3::J, -Vector3::J, 2.0f ) );
	TEST_SUB_FUNCTOR(
		CheckDistance( Vector3::K, Vector3::K, 0.0f ) );
	TEST_SUB_FUNCTOR(
		CheckDistance( -Vector3::K, Vector3::K, 2.0f ) );

	const Vector3 testDistVect( 10.0f, 5.0f, 100.0f );
	TEST_SUB_FUNCTOR( CheckDistance( Vector3::ZERO, testDistVect,
		sqrt(
			Math::squared( testDistVect[0] ) +
			Math::squared( testDistVect[1] ) +
			Math::squared( testDistVect[2] ) ) ) );
}

TEST( Vector3_print )
{
	BW_GUARD;

	const Vector3 testVect(
		-std::numeric_limits<float>::max(),
		-std::numeric_limits<float>::max(),
		-std::numeric_limits<float>::max() );

	//We are mainly aiming to test that the output isn't truncated and
	//is NULL-terminated correctly, even if the largest value is printed.
	
	//Stream insertion
	{
		BW::ostringstream str;
		str << testVect;

		char compareStr[10240] = {};
		RETURN_ON_FAIL_CHECK( bw_snprintf(
			compareStr, sizeof(compareStr) - 1,
			"(%1.1f, %1.1f, %1.1f)", testVect.x, testVect.y, testVect.z ) > 0 );

		CHECK_EQUAL( compareStr, str.str() );
	}
	//desc
	{
		char compareStr[10240] = {};
		RETURN_ON_FAIL_CHECK( bw_snprintf(
			compareStr, sizeof(compareStr) - 1,
			"(%g, %g, %g)", testVect.x, testVect.y, testVect.z ) > 0 );

		CHECK_EQUAL( compareStr, testVect.desc() );
	}
}

BW_END_NAMESPACE

// test_vector3.cpp
