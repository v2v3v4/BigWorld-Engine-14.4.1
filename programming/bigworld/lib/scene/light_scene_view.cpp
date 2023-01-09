#include "pch.hpp"
#include "light_scene_view.hpp"

#include "scene_provider.hpp"
#include "cstdmf/profiler.hpp"

namespace BW {

typedef SceneView<ILightSceneViewProvider>::ProviderInterface ProviderInterface;
typedef SceneView<ILightSceneViewProvider>::ProviderCollection ProviderCollection;

namespace {
template <typename IntersectionObjectType>
size_t intersectAllProviders( const IntersectionObjectType & obj,
	const ProviderCollection & providers,
	Moo::LightContainer & lightContainer )
{
	ProviderCollection::const_iterator iter = providers.begin(),
	                                   end = providers.end();
	size_t numLights = 0;
	for (; iter != end; ++iter)
	{
		ProviderInterface * pProvider = *iter;
		numLights += pProvider->intersect( obj, lightContainer );
	}
	return numLights;
}

} // unnamed namespace

size_t LightSceneView::intersect( const ConvexHull & hull,
	Moo::LightContainer & lightContainer ) const
{
	PROFILER_SCOPED( LightSceneView_intersect_hull );
	return intersectAllProviders( hull, providers_, lightContainer );
}

size_t LightSceneView::intersect( const AABB & bbox,
	Moo::LightContainer & lightContainer ) const
{
	PROFILER_SCOPED( LightSceneView_intersect_bb );
	return intersectAllProviders( bbox, providers_, lightContainer );
}

void LightSceneView::debugDrawLights() const
{
	ProviderCollection::const_iterator iter = providers_.begin(),
	                                   end = providers_.end();
	for (; iter != end; ++iter)
	{
		ProviderInterface * pProvider = *iter;
		pProvider->debugDrawLights();
	}
}

} // namespace BW
