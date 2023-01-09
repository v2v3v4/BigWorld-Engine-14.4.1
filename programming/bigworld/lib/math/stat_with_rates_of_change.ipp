

// -----------------------------------------------------------------------------
// Section: StatWithRatesOfChange
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
template <class TYPE>
inline StatWithRatesOfChange< TYPE >::StatWithRatesOfChange() :
	total_( 0 ),
	prevTotal_( 0 ),
	averages_()
{
}


/**
 *	This method is used to start the monitoring of the rate of change of this
 *	stat. It does this using an Exponential Moving Average. This is set up so
 *	that the most recent "numSamples" samples makes up "weighting" of the
 *	result.
 */
template <class TYPE>
inline void StatWithRatesOfChange< TYPE >::monitorRateOfChange(
	float numSamples )
{
	float bias = EMA::calculateBiasFromNumSamples( numSamples );
	averages_.push_back( EMA( bias ) );
}


/**
 *	This method updates the rate of change values using a sample over the
 *	input period.
 */
template <class TYPE>
inline void StatWithRatesOfChange< TYPE >::tick( double deltaTime )
{
	IF_NOT_MF_ASSERT_DEV( deltaTime > 0.f )
	{
		return;
	}

	TYPE delta = total_ - prevTotal_;
	double avg = delta / deltaTime;
	prevTotal_ = total_;

	Averages::iterator iter = averages_.begin();

	while (iter != averages_.end())
	{
		iter->sample( float( avg ) );

		++iter;
	}
}


/**
 *	This method changes this stat by the specified amount.
 */
template <class TYPE>
inline void StatWithRatesOfChange< TYPE >::add( TYPE value )
{
	total_ += value;
}


/**
 *	This method reduces this stat by the specified amount.
 */
template <class TYPE>
inline void StatWithRatesOfChange< TYPE >::subtract( TYPE value )
{
	total_ -= value;
}


/**
 *	This method returns the value of this stat.
 */
template <class TYPE>
inline TYPE StatWithRatesOfChange< TYPE >::total() const
{
	return total_;
}


/**
 *	This method returns the value of this stat at the last tick call.
 */
template <class TYPE>
inline void StatWithRatesOfChange< TYPE >::setTotal( TYPE total )
{
	total_ = total;
}


/**
 *	This method returns a rate of change of this statistic. The rates of change
 *	can vary based on their weightings.
 */
template <class TYPE>
inline double StatWithRatesOfChange< TYPE >::getRateOfChange( int index ) const
{
	return averages_[ index ].average();
}


// -----------------------------------------------------------------------------
// Section: IntrusiveStatWithRatesOfChange
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
template <class TYPE>
inline IntrusiveStatWithRatesOfChange< TYPE >::IntrusiveStatWithRatesOfChange(
		Container *& pContainer ) :
	StatWithRatesOfChange< TYPE >(),
	IntrusiveObject< IntrusiveStatWithRatesOfChange< TYPE > >( pContainer )
{
}

// super_stat.ipp
