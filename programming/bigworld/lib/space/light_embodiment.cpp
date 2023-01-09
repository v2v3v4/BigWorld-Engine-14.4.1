#include "pch.hpp"

#include "light_embodiment.hpp"

#include <space/client_space.hpp>
#include <space/space_manager.hpp>

namespace BW
{

BW_SINGLETON_STORAGE( LightEmbodimentManager )

namespace
{
	LightEmbodimentManager s_instance;
}

void LightEmbodimentManager::addLightEmbodiment( ILightEmbodiment* pEmb )
{
	// WARNING: Must only occur on the main thread
	// Concept was brought across from ChunkOmniLightEmbodiment etc
	// which must have had the same limitation but was never enforced.
	// Since those previous versions worked, it should work here too.
	// future versions will have an assertion here to enforce this.
	currentLights_.push_back( pEmb );
}

void LightEmbodimentManager::removeLightEmbodiment( ILightEmbodiment* pEmb )
{
	// WARNING: Must only occur on the main thread
	// See above comment in "addLightEmbodiment"

	BW_GUARD_PROFILER( LightEmbodimentManager_removeLightEmbodiment );

	LightEmbodiments::iterator found = std::find(
		currentLights_.begin(), currentLights_.end(), pEmb );
	MF_ASSERT_DEV( found != currentLights_.end() );

	if (found != currentLights_.end())
	{
		currentLights_.erase( found );
	}
}

void LightEmbodimentManager::tick()
{
	// Dynamic lights are not assigned to any particular space,
	// they can be transient and move between spaces like an entity could.
	// For this reason, this manager needs to exist in some context larger
	// than the spaces themselves. Updating the location of a light will
	// push it into the current camera space if it has been removed from the
	// previous one.

	BW_GUARD;
	for (LightEmbodiments::iterator it = currentLights_.begin();
		it != currentLights_.end();
		it++)
	{
		ILightEmbodiment* pEmb = (*it);
		pEmb->updateLocation();
	}
}

ILightEmbodiment::ILightEmbodiment()
{
	LightEmbodimentManager::instance().addLightEmbodiment( this );
}


ILightEmbodiment::~ILightEmbodiment()
{
	LightEmbodimentManager::instance().removeLightEmbodiment( this );
}

void ILightEmbodiment::drawBBox() const
{
	// do nothing as default implementation
}

bool ILightEmbodiment::intersectAndAddToLightContainer( const ConvexHull & hull,
	Moo::LightContainer & lightContainer ) const
{
	return doIntersectAndAddToLightContainer( hull, lightContainer );
}

bool ILightEmbodiment::intersectAndAddToLightContainer( const AABB & bbox,
	Moo::LightContainer & lightContainer ) const
{
	return doIntersectAndAddToLightContainer( bbox, lightContainer );
}

void ILightEmbodiment::tick( float dTime )
{
	doTick( dTime );
}

void ILightEmbodiment::leaveSpace()
{
	doLeaveSpace();
}

void ILightEmbodiment::updateLocation()
{
	doUpdateLocation();
}

} // namespace BW
