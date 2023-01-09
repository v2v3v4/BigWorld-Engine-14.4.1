#include "pch.hpp"

#include "dynamic_light_scene_provider.hpp"

#include "light_embodiment.hpp"

#include <scene/scene.hpp>
#include <scene/scene_object.hpp>
#include <scene/tick_scene_view.hpp>
#include <scene/light_scene_view.hpp>

#include <cstdmf/bw_vector.hpp>
#include <math/matrix.hpp>
#include <math/vector3.hpp>
#include <math/boundbox.hpp>
#include <math/convex_hull.hpp>

namespace BW
{

void * DynamicLightSceneProvider::getView( 
	const SceneTypeSystem::RuntimeTypeID & viewTypeID)
{
	void * result = NULL;

	exposeView<ITickSceneProvider>( this, viewTypeID, result );
	exposeView<ILightSceneViewProvider>( this, viewTypeID, result );

	return result;
}


void DynamicLightSceneProvider::addLightEmbodiment(
	const ILightEmbodimentPtr & pLightEmb )
{
	MF_ASSERT( std::find( lights_.begin(), lights_.end(), pLightEmb ) == 
		lights_.end() );
	lights_.push_back( pLightEmb );
}


void DynamicLightSceneProvider::removeLightEmbodiment(
	const ILightEmbodimentPtr & pLightEmb )
{
	LightEmbodiments::iterator iter = std::find(lights_.begin(), lights_.end(),
		pLightEmb);
	if (iter != lights_.end())
	{
		std::swap( *iter, lights_.back() );
		lights_.pop_back();
	}
}


void DynamicLightSceneProvider::clear()
{
	// Request each light leave the space.
	// Each light will then remove itself from this list
	while (!lights_.empty())
	{
		// Copy the pointer because the leave space function may
		// cause the last reference count to drop
		ILightEmbodimentPtr pLightEmb = lights_.front();
		pLightEmb->leaveSpace();
	}
}


void DynamicLightSceneProvider::tick( float dTime )
{
	if (lights_.empty())
	{
		return;
	}
	
	LightEmbodiments::iterator iter = lights_.begin(),
	                           end = lights_.end();
	for (; iter != end; ++iter)
	{
		ILightEmbodimentPtr & pLightEmb = *iter;
		pLightEmb->tick( dTime );
	}
}


void DynamicLightSceneProvider::updateAnimations( float dTime )
{
}


size_t DynamicLightSceneProvider::intersect( const ConvexHull & hull, 
	Moo::LightContainer & lightContainer ) const
{
	size_t lightCount = 0;
	LightEmbodiments::const_iterator iter = lights_.begin(),
	                                 end = lights_.end();
	for (; iter != end; ++iter)
	{
		const ILightEmbodimentPtr & pLightEmb = *iter;
		if ( pLightEmb->intersectAndAddToLightContainer( hull, lightContainer ))
		{
			++lightCount;
		}
	}
	return lightCount;
}


size_t DynamicLightSceneProvider::intersect( const AABB & bbox, 
	Moo::LightContainer & lightContainer ) const
{
	size_t lightCount = 0;
	LightEmbodiments::const_iterator iter = lights_.begin(),
	                                 end = lights_.end();
	for (; iter != end; ++iter)
	{
		const ILightEmbodimentPtr & pLightEmb = *iter;
		if ( pLightEmb->intersectAndAddToLightContainer( bbox, lightContainer ))
		{
			++lightCount;
		}
	}
	return lightCount;
}


void DynamicLightSceneProvider::debugDrawLights() const
{
	LightEmbodiments::const_iterator iter = lights_.begin(),
	                                 end = lights_.end();
	for (; iter != end; ++iter)
	{
		const ILightEmbodimentPtr & pLightEmb = *iter;
		pLightEmb->drawBBox();
	}
}

} // namespace BW
