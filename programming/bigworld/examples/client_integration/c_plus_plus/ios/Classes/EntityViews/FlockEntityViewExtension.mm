#include "FlockEntityViewExtension.hpp"

#include "Entities/Flock.hpp"


/*
 *	Override from EntityViewDataProvider.
 */
const BW::Vector3 FlockEntityViewExtension::spriteTint() const
{
	// NOTE: the colour format is R,G,B
	return BW::Vector3( 160, 255, 50 );
}


// FlockEntityViewExtension.cpp
