#ifndef PROPERTY_EVENT_STAMPS_HPP
#define PROPERTY_EVENT_STAMPS_HPP

#include "network/basictypes.hpp"

#include "entity_description.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class DataDescription;
class EntityDescription;

/**
 *	This class is used to store the event number when a property last
 *	changed for each property in an entity that is 'otherClient'.
 */
class PropertyEventStamps
{
public:
	void init( const EntityDescription & entityDescription );
	void init( const EntityDescription & entityDescription,
		   EventNumber lastEventNumber );

	void set( const DataDescription & dataDescription,
			EventNumber eventNumber );

	EventNumber get( const DataDescription & dataDescription ) const;

	void addToStream( BinaryOStream & stream ) const;
	void removeFromStream( BinaryIStream & stream );

private:
	typedef BW::vector< EventNumber > Stamps;
	Stamps eventStamps_;
};


#ifdef CODE_INLINE
#include "property_event_stamps.ipp"
#endif

BW_END_NAMESPACE

#endif // PROPERTY_EVENT_STAMPS_HPP
