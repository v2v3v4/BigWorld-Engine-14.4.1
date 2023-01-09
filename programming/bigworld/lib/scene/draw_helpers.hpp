#ifndef _DRAW_HELPERS_HPP
#define _DRAW_HELPERS_HPP

#include "forward_declarations.hpp"
#include "scene/draw_operation.hpp"

BW_BEGIN_NAMESPACE
namespace Moo {
	class LightContainer;
	typedef SmartPointer< LightContainer > LightContainerPtr;
}

class BoundingBox;
BW_END_NAMESPACE

namespace BW
{
class ConvexHull;
class IntersectionSet;
class SceneDrawContext;
class SceneIntersectContext;

namespace DrawHelpers
{

void cull( const SceneIntersectContext& context, const Scene & scene, 
		  const Matrix & projectionMatrix,
		  IntersectionSet & visibleObjects);

void drawSceneVisibilitySet( const SceneDrawContext& context, 
	Scene& scene, const IntersectionSet& visibleObjects );

void addIntersectingSceneLightsToContainer( const Scene & scene,
	const BoundingBox & bbox, Moo::LightContainer & lightContainer );

void addIntersectingSceneLightsToContainer( const Scene & scene,
	const ConvexHull & hull, Moo::LightContainer & lightContainer );

} // namespace DrawHelpers
} // namespace BW

#endif // _DRAW_HELPERS_HPP
