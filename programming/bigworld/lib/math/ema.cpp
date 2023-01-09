#include "pch.hpp"

#include "ema.hpp"
#include "cstdmf/stdmf.hpp"

#include <math.h>

BW_BEGIN_NAMESPACE

/**
 *	This method calculates a bias required to give the most recent samples the
 *	desired weighting.
 *
 *	For example, if numSamples is 60 and weighting is 0.95, using the resulting
 *	bias in an EMA means that the latest 60 samples account for 95% of the
 *	average and all other samples account for 5%.
 *
 *	@param numSamples	The number of samples having the desired weighting.
 *	@param weighting	The proportion of the average that these samples
 *						contribute.
 *
 *	@returns The bias to achieve the desired weighting on the specified samples.
 */
float EMA::calculateBiasFromNumSamples( float numSamples,
		float weighting )
{
	return (!isZero( numSamples )) ?
		exp( log( 1.f - weighting )/numSamples ) :
		0.f;
}

BW_END_NAMESPACE

// ema.cpp
