#include "pch.hpp"

#include "math/ema.hpp"
#include "math/stat_with_rates_of_change.hpp"


BW_BEGIN_NAMESPACE

/*
 *	This tests calculateBiasFromNumSamples. If an EMA starts at 0 and then
 *	numSamples of 1 are added, the average should become the weighting of those
 *	first numSamples samples.
 */
TEST( EMA_calculateBiasFromNumSamples )
{
	struct TestData
	{
		int numSamples;
		float weighting;
	};

	TestData data[] =
	{
		{ 60, 0.95f },
		{ 2, 0.5f },
		{ 100, 0.01f },
	};

	for (unsigned int i = 0; i < sizeof( data )/sizeof( data[0] ); ++i)
	{
		const int numSamples = data[i].numSamples;
		const float weighting = data[i].weighting;
		const float bias =
			EMA::calculateBiasFromNumSamples( float( numSamples ), weighting );
		EMA ema( bias );

		for (int j = 0; j < numSamples; ++j)
		{
			ema.sample( 1.f );
		}

		CHECK_CLOSE( weighting, ema.average(), weighting/100.f );
	}
}

TEST( StatWithRatesOfChange )
{
	StatWithRatesOfChange< float > stat;
	stat.monitorRateOfChange( 10 );
	stat.monitorRateOfChange( 60 );
	stat.monitorRateOfChange( 180 );

	const float CHANGE_SIZE = 15.f;
	const float TICK_SIZE = 0.1f;

	int i = 0;

	while (i < 10)
	{
		stat += CHANGE_SIZE;
		stat.tick( TICK_SIZE );
		++i;
	}

	CHECK_CLOSE( stat.getRateOfChange( 0 ), 0.95f * CHANGE_SIZE / TICK_SIZE,
			0.001f );

	while (i < 60)
	{
		stat += CHANGE_SIZE;
		stat.tick( TICK_SIZE );
		++i;
	}

	CHECK_CLOSE( stat.getRateOfChange( 1 ), 0.95f * CHANGE_SIZE / TICK_SIZE,
			0.001f );

	while (i < 180)
	{
		stat += CHANGE_SIZE;
		stat.tick( TICK_SIZE );

		++i;
	}

	CHECK_CLOSE( stat.getRateOfChange( 2 ), 0.95f * CHANGE_SIZE / TICK_SIZE,
			0.001f );
}

BW_END_NAMESPACE

// test_ema.cpp
