#ifndef GUARD_SPAWNER_ENTITY_VIEW_EXTENSION_HPP
#define GUARD_SPAWNER_ENTITY_VIEW_EXTENSION_HPP

#include "Classes/EntityView.hpp"
#include "EntityExtensions/GuardSpawnerExtension.hpp"
#include "GenericEntityViewExtension.hpp"

class GuardSpawnerEntityViewExtension :
		public GenericEntityViewExtension< GuardSpawnerExtension >
{
public:
	GuardSpawnerEntityViewExtension( const GuardSpawner * pEntity ) :
			GenericEntityViewExtension( pEntity )
	{}
	
	virtual ~GuardSpawnerEntityViewExtension() {}
	
	// Override from EntityViewDataProvider
	virtual const BW::Vector3 spriteTint() const;
};


#endif // GUARD_SPAWNER_ENTITY_VIEW_EXTENSION_HPP