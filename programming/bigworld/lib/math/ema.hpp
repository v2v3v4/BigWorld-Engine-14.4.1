#ifndef __EMA_HPP__
#define __EMA_HPP__

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class represents an exponentially weighted moving average.
 *
 *	The nth sample (given via the sample() method) updates the average
 *	according to:
 *		avg(n) = (1 - bias) * sample( n ) + bias * avg( n - 1 )
 *
 *	This class is entirely inlined.
 */
class EMA
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param bias 	Determines the exponential bias.
	 *	@param initial 	The initial value of the average.
	 */
	explicit EMA( float bias, float initial=0.f ):
		bias_( bias ),
		average_( initial )
	{}

	/**
	 *	Sample a value into the average.
	 *
	 *	@param value 	The value to be sampled.
	 */
	void sample( float value )
	{
		average_ = (1.f - bias_) * value + bias_ * average_;
	}

	/**
	 *	Return the current value of the average.
	 */
	float average() const 	{ return average_; }

	/**
	 *	Resets the average to the given value.
	 */
	void resetAverage( float newValue = 0.f ) { average_ = newValue; };


	/**
	 *  Samples a value into the average, overriding the average
	 *  if value is larger than the current average.
	 */
	void highWaterSample( float value )
	{
		if (value > average_)
		{
			this->resetAverage( value );
		}
		else
		{
			sample( value );
		}
	}

	/**
	 *  Returns the bias being used for weighting.
	 */
	float bias() const 
	{
		return bias_;
	}

	static float calculateBiasFromNumSamples( float numSamples,
			float weighting = 0.95f );

private:
	/// The bias.
	float bias_;

	/// The current value of the average.
	float average_;

};

/**
 *	This templated class is a helper class for accumulating values, and
 *	periodically sampling that value into an EMA.
 */
template< typename TYPE >
class AccumulatingEMA
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param bias 			Determines the exponential bias.
	 *	@param initialAverage	The initial value of the average.
	 *	@param initialValue 	The initial value of the accumulated value.
	 */
	AccumulatingEMA( float bias, float initialAverage=0.f,
			TYPE initialValue=TYPE() ):
		average_( bias, initialAverage ),
		accum_( initialValue )
	{
	}


	/**
	 *	Accumulated value accessor.
	 */
	TYPE & value() 				{ return accum_; }


	/**
	 *	Accumulated value const accessor.
	 */
	const TYPE & value() const 	{ return accum_; }


	/**
	 *	Average accessor.
	 */
	float average()	const		{ return average_.average(); }


	/**
	 *	Sample method. This should be called periodically to sample the
	 *	accumulated value into the weighted average. The accumulated value
	 *	is converted to a float before sampling.
	 *
	 *	@param shouldReset 		If true, the accumulated value is reset to the
	 *							TYPE default (e.g. 0 for numeric TYPEs) after
	 *							sampling. If false, it is unchanged.
	 */
	void sample( bool shouldReset=true )
	{
		average_.sample( float( accum_ ) );
		if (shouldReset)
		{
			accum_ = TYPE();
		}
	}

private:
	/// The exponentially weighted average.
	EMA 	average_;
	/// The accumulated value.
	TYPE 	accum_;
};

BW_END_NAMESPACE

#endif // __EMA_HPP__

