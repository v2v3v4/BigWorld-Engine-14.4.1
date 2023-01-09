#ifdef CODE_INLINE
#define INLINE    inline
#else
/// INLINE macro.
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: PropertyEventStamps
// -----------------------------------------------------------------------------

/**
 *	This method is used to initialise PropertyEventStamps. This basically means
 *	that the number of stamps that this object can store is set to the number of
 *	properties in the associated entity that are stamped.
 */
INLINE void PropertyEventStamps::init(
		const EntityDescription & entityDescription )
{
	// Resize the stamps to the required size and set all values to 1.
	eventStamps_.resize( entityDescription.numEventStampedProperties(), 1 );
}


/**
 *	This method is also used to initialise PropertyEventStamps but sets all
 *	values to the input value.
 */
INLINE void PropertyEventStamps::init(
		const EntityDescription & entityDescription, EventNumber number )
{
	// TODO: This is probably only temporary. PropertyEventStamps shouldn't be
	// initialised with one stamps. The event stamps should probably be stored
	// in the database for each property.

	this->init( entityDescription );

	Stamps::iterator iter = eventStamps_.begin();

	while (iter != eventStamps_.end())
	{
		(*iter) = number;

		iter++;
	}
}


/**
 *	This method is used to set an event number corresponding to a data
 *	description.
 */
INLINE void PropertyEventStamps::set(
		const DataDescription & dataDescription, EventNumber eventNumber )
{
	// Each DataDescription has an index for which element it stores its stamp
	// in.
	const int index = dataDescription.eventStampIndex();
	IF_NOT_MF_ASSERT_DEV( 0 <= index && index < (int)eventStamps_.size() )
	{
		MF_EXIT( "invalid event stamp index" );
	}


	eventStamps_[ index ] = eventNumber;
}


/**
 *	This method is used to get an event number corresponding to a data
 *	description.
 */
INLINE EventNumber PropertyEventStamps::get(
		const DataDescription & dataDescription ) const
{
	const int index = dataDescription.eventStampIndex();
	IF_NOT_MF_ASSERT_DEV( 0 <= index && index < (int)eventStamps_.size() )
	{
		MF_EXIT( "invalid event stamp index" );
	}

	return eventStamps_[ index ];
}

// property_event_stamps.ipp
