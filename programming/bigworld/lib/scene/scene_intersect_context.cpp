#include "pch.hpp"

#include "scene_intersect_context.hpp"

namespace BW
{

SceneIntersectContext::SceneIntersectContext( const Scene & scene ) :
	scene_( scene ),
	flags_( INCLUDE_DYNAMIC_OBJECTS | INCLUDE_STATIC_OBJECTS )
{

}

bool SceneIntersectContext::onlyShadowCasters() const
{
	return (flags_ & ONLY_SHADOW_CASTERS) != 0;
}

void SceneIntersectContext::onlyShadowCasters( bool include )
{
	flags_ &= ~ONLY_SHADOW_CASTERS;
	flags_ |= (include) ? ONLY_SHADOW_CASTERS : 0;
}

bool SceneIntersectContext::includeDynamicObjects() const
{
	return (flags_ & INCLUDE_DYNAMIC_OBJECTS) != 0;
}

void SceneIntersectContext::includeDynamicObjects( bool include )
{
	flags_ &= ~INCLUDE_DYNAMIC_OBJECTS;
	flags_ |= (include) ? INCLUDE_DYNAMIC_OBJECTS : 0;
}

bool SceneIntersectContext::includeStaticObjects() const
{
	return (flags_ & INCLUDE_STATIC_OBJECTS) != 0;
}

void SceneIntersectContext::includeStaticObjects( bool include )
{
	flags_ &= ~INCLUDE_STATIC_OBJECTS;
	flags_ |= (include) ? INCLUDE_STATIC_OBJECTS : 0;
}

} // namespace BW
