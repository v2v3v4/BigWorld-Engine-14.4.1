#ifndef MOVING_PLATFORM_ENTITY_VIEW_EXTENSION_HPP
#define MOVING_PLATFORM_ENTITY_VIEW_EXTENSION_HPP

#include "EntityExtensions/MovingPlatformExtension.hpp"
#include "GenericEntityViewExtension.hpp"


class MovingPlatformEntityViewExtension :
		public GenericEntityViewExtension< MovingPlatformExtension >
{
public:
	MovingPlatformEntityViewExtension( const MovingPlatform * pEntity ) :
			GenericEntityViewExtension( pEntity )
	{}
	
	virtual ~MovingPlatformEntityViewExtension() {}

	// Override from EntityViewDataProvider.
	virtual BW::string spriteName() const;
};


#endif // MOVING_PLATFORM_ENTITY_VIEW_EXTENSION_HPP