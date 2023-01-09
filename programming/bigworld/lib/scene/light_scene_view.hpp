#ifndef _LIGHT_SCENE_VIEW_HPP
#define _LIGHT_SCENE_VIEW_HPP

#include "scene_provider.hpp"

BW_BEGIN_NAMESPACE
class AABB;
BW_END_NAMESPACE

namespace BW {

class ConvexHull;

namespace Moo {
class LightContainer;
} // namespace Moo

class ILightSceneViewProvider
{
public:
	virtual size_t intersect( const ConvexHull & hull,
		Moo::LightContainer & lightContainer ) const = 0;

	virtual size_t intersect( const AABB & bbox,
		Moo::LightContainer & lightContainer ) const = 0;

	virtual void debugDrawLights() const = 0;
};

class LightSceneView : public SceneView<ILightSceneViewProvider>
{
public:
	size_t intersect( const ConvexHull & hull,
		Moo::LightContainer & lightContainer ) const;

	size_t intersect( const AABB & bbox,
		Moo::LightContainer & lightContainer ) const;

	void debugDrawLights() const;
};

} // namespace BW

#endif // _LIGHT_SCENE_VIEW_HPP
