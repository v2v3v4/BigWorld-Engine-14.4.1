#include "pch.hpp"

#include "cstdmf/binaryfile.hpp"
#include "cstdmf/bwrandom.hpp"
#include "cstdmf/guard.hpp"
#include "animation_test_helper.hpp"
#include "math/math_extra.hpp"
#include "moo/interpolated_animation_channel.hpp"
#include "moo/moo_math.hpp"

BW_USE_NAMESPACE


TEST_F( AnimTestsFixture, Interpolated_Animation_Channel_IO )
{
	BW_GUARD;

	//No tests with compression for the moment. These tests would require
	//interpolation to check transforms across the restored animation with
	//some appropriate epsilon.
	const bool USE_COMPRESSION = false;
	
	Moo::InterpolatedAnimationChannel::inhibitCompression(
		!USE_COMPRESSION );

	const int NUM_TEST_SETS = 100;
	const float MAX_ANIM_DURATION = 10.0f;
	const int MAX_ANIM_KEY_FRAMES = 10;
	//The minimum number of key frames is 3 because a valid channel
	//must have one of each type (position, rotation, scale).
	MF_ASSERT( MAX_ANIM_KEY_FRAMES >= 3 );

	for (unsigned int seed = 0; seed < NUM_TEST_SETS; ++seed)
	{
		//Create test data.
		BWRandom random( seed );
		const int numKeys =
			3 + random( 0, MAX_ANIM_KEY_FRAMES - 3 );
		const float animDuration =
			random( 0.0f, MAX_ANIM_DURATION );
		InterpolatedAnimationChannelPtr channel =
			createRandomInterpolatedAnimationChannel(
				seed, numKeys, animDuration, !USE_COMPRESSION );
		channel->identifier( "testChannel" );

		CHECK_EQUAL( numKeys,
			(channel->nScaleKeys() + channel->nPositionKeys() +
				channel->nRotationKeys()) );
		CHECK( channel->valid() );

		//Check I/O round-trips.
		MF_ASSERT(
			Moo::InterpolatedAnimationChannel::s_saveConverter_ == NULL );
		BinaryFile testFile( NULL );
		testFile.reserve( 1 );//Make memory-backed.

		RETURN_ON_FAIL_CHECK( channel->save( testFile ) );
		RETURN_ON_FAIL_CHECK( !testFile.error() );
		
		Moo::InterpolatedAnimationChannel restoredChannel(
			false, !USE_COMPRESSION );
		testFile.rewind();
		RETURN_ON_FAIL_CHECK( restoredChannel.load( testFile ) );
		RETURN_ON_FAIL_CHECK( !testFile.error() );

		CHECK( channel->compareChannel( restoredChannel ) );
	}
};
