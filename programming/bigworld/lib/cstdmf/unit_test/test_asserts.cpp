#include "pch.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/debug_filter.hpp"


BW_BEGIN_NAMESPACE

namespace TestAsserts
{
	class Fixture
	{
	public:
		Fixture()
			:	exp1_( false ), 
				exp2_( false )
		{
		}

		bool doExp1()
		{
			exp1_ = true;
			return true;
		}
		bool wasExp1Done() { return exp1_; }

		bool doExp2()
		{
			exp2_ = true;
			return false; // this expression "fails", so assert is generated
		}
		bool wasExp2Done() { return exp2_; }

		// Members
	private:
		bool exp1_;
		bool exp2_;
	};

TEST_F( Fixture, MfAssertDev)
{
#if defined( MF_SERVER ) || defined( EDITOR_ENABLED ) || !defined(_RELEASE)

	// Server,tools, or non release client build
	// Assert expression 1, and check that it was executed
	MF_ASSERT_DEV( doExp1() );
	CHECK_EQUAL( true, wasExp1Done() );

#else

	// Client release
	// Assert expression 1, and check that it was not executed
	MF_ASSERT_DEV( doExp1() );
	CHECK_EQUAL( false, wasExp1Done() );

#endif
}

TEST_F( Fixture, IfNotMfAssertDev )
{
	INFO_MSG( "Ignore following ASSERT_DEV message.\n" );

	// Runtime disable fatal assertions
	DebugFilter::instance().hasDevelopmentAssertions( false );

	// Both expressions should always be evaluated in every configuration.
	IF_NOT_MF_ASSERT_DEV( doExp2() )
	{
		doExp1();
	}

	// Expressions 1 & 2 should both have been executed.
	CHECK_EQUAL( true, wasExp1Done() );
	CHECK_EQUAL( true, wasExp2Done() );
}

TEST_F( Fixture, IfNotMfAssertDev2 )
{
	// Runtime disable fatal assertions
	DebugFilter::instance().hasDevelopmentAssertions( false );

	// First expression succeeds, so second should not be executed.
	IF_NOT_MF_ASSERT_DEV( doExp1() )
	{
		doExp2();
	}

	// Expressions 1 & 2 should both have been executed.
	CHECK_EQUAL( true, wasExp1Done() );
	CHECK_EQUAL( false, wasExp2Done() );
}

} // namespace TestAsserts

BW_END_NAMESPACE

// test_asserts.cpp
