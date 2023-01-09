#ifndef FLOCK_ENTITY_VIEW_EXTENSION_HPP
#define FLOCK_ENTITY_VIEW_EXTENSION_HPP

#include "EntityExtensions/FlockExtension.hpp"
#include "GenericEntityViewExtension.hpp"

class FlockEntityViewExtension :
		public GenericEntityViewExtension< FlockExtension >
{
public:
	FlockEntityViewExtension( const Flock * pEntity ) :
			GenericEntityViewExtension( pEntity )
	{}
	
	virtual ~FlockEntityViewExtension() {}
	
	
	// Override from EntityViewDataProvider
	virtual const BW::Vector3 spriteTint() const;
};


#endif // FLOCK_ENTITY_VIEW_EXTENSION_HPP