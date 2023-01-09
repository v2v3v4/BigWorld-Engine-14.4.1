#ifndef CREATURE_ENTITY_VIEW_EXTENSION_HPP
#define CREATURE_ENTITY_VIEW_EXTENSION_HPP

#include "EntityExtensions/CreatureExtension.hpp"
#include "GenericEntityViewExtension.hpp"

class CreatureEntityViewExtension :
		public GenericEntityViewExtension< CreatureExtension >
{
public:
	CreatureEntityViewExtension( const Creature * pEntity ) :
			GenericEntityViewExtension( pEntity )
	{}
	
	virtual ~CreatureEntityViewExtension() {}
	
	// Overrides from EntityViewDataProvider
	virtual BW::string spriteName() const;
	virtual BW::string popoverString() const;
	
};


#endif // CREATURE_ENTITY_VIEW_EXTENSION_HPP