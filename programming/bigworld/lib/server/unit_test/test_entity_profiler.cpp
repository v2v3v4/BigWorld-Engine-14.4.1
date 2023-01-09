#include "CppUnitLite2/src/CppUnitLite2.h"

#if !defined( _DEBUG )
// We override timestamp() function
// in the hope the entity_profiler.hpp will pick our version in 
// the methods implementations.
// However in DEBUG, the methods don't get inlined, they compiled in .cpp
// instead which prevents us from overriding of timestamp() and consequently
// from testing entity profilers in DEBUG.

#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/timestamp.hpp"


// ***************************************************************************

/*
 * Hacky overrides. These are needed in order to get deterministic time.
 * We're testing entity profiling, not the timer accuracy and delays
 * caused by parallel threads.
 */

static BW::uint64 s_artificialTickCount;

BW_BEGIN_NAMESPACE

int usleep( BW::uint64 ticks )
{
	s_artificialTickCount += (BW::uint64)ticks * 1000;
	return 0;
}

BW_END_NAMESPACE


BW::uint64 timestamp_override()
{
	return (BW::uint64)s_artificialTickCount;
}


#define timestamp() timestamp_override()

// * End of hacky overrides
// ***************************************************************************

#include "server/entity_profiler.hpp"
#include "server/entity_type_profiler.hpp"


BW_BEGIN_NAMESPACE

class TestEntity;
typedef TestEntity BaseOrEntity;

class TestEntity : public ReferenceCount
{
public:
	TestEntity( uint perfHit ) :
		perfHit_ ( perfHit )
	{
	}

	const EntityProfiler & profiler() const { return profiler_; }
	EntityProfiler & profiler() { return profiler_; }

	void testMethodScoped()
	{
		AUTO_SCOPED_THIS_ENTITY_PROFILE;

		BW::usleep( 1000 * perfHit_ );
	}

private:
	EntityProfiler profiler_;

	uint perfHit_;
};

typedef SmartPointer< TestEntity > TestEntityPtr;


TEST( EntityProfiler )
{
	// simulating game loop
	TestEntityPtr testEntity0 = new TestEntity( 0 );
	TestEntityPtr testEntity1 = new TestEntity( 7 );
	TestEntityPtr testEntity2 = new TestEntity( 14 );
	TestEntityPtr testEntity3 = new TestEntity( 0 );
	TestEntityPtr testEntity4 = new TestEntity( 7 );

	EntityTypeProfiler typeProfiler;

	const uint64 fixedTickDt = 100L * 1000L; // 100ms
	const float smoothingFactor = 0.369;

	uint64 prevTickTime = timestamp();

	BW::usleep( 1000 );

	// 10 ticks 100ms each
	for (uint i = 0; i < 10; i++)
	{
		uint64 currTickTime = timestamp();
		uint64 tickDt = currTickTime - prevTickTime;
		prevTickTime = currTickTime;

		testEntity0->profiler().tick( tickDt, smoothingFactor, typeProfiler );
		testEntity1->profiler().tick( tickDt, smoothingFactor, typeProfiler );
		testEntity2->profiler().tick( tickDt, smoothingFactor, typeProfiler );
		testEntity3->profiler().tick( tickDt, smoothingFactor, typeProfiler );
		testEntity4->profiler().tick( tickDt, smoothingFactor, typeProfiler );

		testEntity4->testMethodScoped();
		testEntity3->testMethodScoped();
		testEntity1->testMethodScoped();
		testEntity2->testMethodScoped();
		testEntity3->testMethodScoped();
		testEntity4->testMethodScoped();

		uint64 dt = (timestamp() - currTickTime) / 1000;
		uint64 delay = fixedTickDt - dt;
		BW::usleep( delay );
	}

	CHECK( isZero( testEntity0->profiler().load() ) );
	CHECK( testEntity1->profiler().load() > 0.0f );
	CHECK( testEntity2->profiler().load() > 0.0f );
	CHECK( testEntity3->profiler().load() >= 0.0f );
	CHECK( testEntity4->profiler().load() > 0.0f );
	CHECK( testEntity1->profiler().load() < 1.0f );
	CHECK( testEntity2->profiler().load() < 1.0f );
	CHECK( testEntity3->profiler().load() < 1.0f );
	CHECK( testEntity4->profiler().load() < 1.0f );
	CHECK_CLOSE( testEntity1->profiler().load(),
					testEntity1->profiler().rawLoad(), 0.01 );
	CHECK_CLOSE( testEntity2->profiler().load(),
					testEntity2->profiler().rawLoad(), 0.01 );
	CHECK_CLOSE( testEntity3->profiler().load(),
					testEntity3->profiler().rawLoad(), 0.01 );
	CHECK_CLOSE( testEntity4->profiler().load(),
					testEntity4->profiler().rawLoad(), 0.01 );
	CHECK_CLOSE( testEntity2->profiler().load(),
				testEntity4->profiler().load(),
				0.02 );

	printf( "testEntity0 smoothed load = %f, raw = %f\n",
			testEntity0->profiler().load(),
			testEntity0->profiler().rawLoad() );
	printf( "testEntity1 smoothed load = %f, raw = %f\n",
			testEntity1->profiler().load(),
			testEntity1->profiler().rawLoad() );
	printf( "testEntity2 smoothed load = %f, raw = %f\n",
			testEntity2->profiler().load(),
			testEntity2->profiler().rawLoad() );
	printf( "testEntity3 smoothed load = %f, raw = %f\n",
			testEntity3->profiler().load(),
			testEntity3->profiler().rawLoad() );
	printf( "testEntity4 smoothed load = %f, raw = %f\n",
			testEntity4->profiler().load(),
			testEntity4->profiler().rawLoad() );
}

BW_END_NAMESPACE

#endif // if !defined( _DEBUG )

// test_updatables.cpp
