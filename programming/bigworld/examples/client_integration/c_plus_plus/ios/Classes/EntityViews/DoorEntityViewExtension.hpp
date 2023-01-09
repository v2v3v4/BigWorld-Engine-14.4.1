#ifndef DOOR_ENTITY_VIEW_EXTENSION_HPP
#define DOOR_ENTITY_VIEW_EXTENSION_HPP

#include "EntityExtensions/DoorExtension.hpp"
#include "GenericEntityViewExtension.hpp"


class DoorEntityViewExtension :
		public GenericEntityViewExtension< DoorExtension >
{
public:
	DoorEntityViewExtension( const Door * pEntity ) :
			GenericEntityViewExtension( pEntity )
	{}
	
	virtual ~DoorEntityViewExtension() {}
	
	// Override from EntityViewDataProvider
	virtual const BW::Vector3 spriteTint() const;
};


#endif // DOOR_ENTITY_VIEW_EXTENSION_HPP