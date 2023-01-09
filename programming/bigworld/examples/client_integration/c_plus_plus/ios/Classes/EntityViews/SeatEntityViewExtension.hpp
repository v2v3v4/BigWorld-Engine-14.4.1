#ifndef SEAT_ENTITY_VIEW_EXTENSION_HPP
#define SEAT_ENTITY_VIEW_EXTENSION_HPP

#include "EntityExtensions/SeatExtension.hpp"
#include "GenericEntityViewExtension.hpp"

class SeatEntityViewExtension :
		public GenericEntityViewExtension< SeatExtension >
{
public:
	SeatEntityViewExtension( const Seat * pEntity ) :
			GenericEntityViewExtension( pEntity )
	{}
	

	// Override from EntityViewDataProvider
	virtual BW::string spriteName() const;
};


#endif // SEAT_ENTITY_VIEW_EXTENSION_HPP