#include "pch.hpp"

#include "property_event_stamps.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "property_event_stamps.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: PropertyEventStamps
// -----------------------------------------------------------------------------

/**
 *	This method adds this object to the input stream.
 */
void PropertyEventStamps::addToStream(
		BinaryOStream & stream ) const
{
	Stamps::const_iterator iter = eventStamps_.begin();

	while (iter != eventStamps_.end())
	{
		stream << (*iter);

		iter++;
	}
}


/**
 *	This method removes this object to the input stream.
 */
void PropertyEventStamps::removeFromStream(
		BinaryIStream & stream )
{
	Stamps::iterator iter = eventStamps_.begin();

	while (iter != eventStamps_.end())
	{
		stream >> (*iter);

		iter++;
	}
}

BW_END_NAMESPACE

// property_event_stamps.cpp
