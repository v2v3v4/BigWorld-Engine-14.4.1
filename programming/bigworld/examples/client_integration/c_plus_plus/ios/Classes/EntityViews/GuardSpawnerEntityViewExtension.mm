#include "GuardSpawnerEntityViewExtension.hpp"

#include "Entities/GuardSpawner.hpp"


/*
 *	Override from EntityViewDataProvider.
 */
const BW::Vector3 GuardSpawnerEntityViewExtension::spriteTint() const
{
	return BW::Vector3( 0, 255, 255 );
}


// GuardSpawnerEntityViewExtension.cpp
