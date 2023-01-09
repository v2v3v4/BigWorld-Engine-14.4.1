
#include "pch.hpp"
#include "draw_helpers.hpp"

#include <scene/scene.hpp>
#include <scene/intersect_scene_view.hpp>
#include <scene/intersection_set.hpp>
#include <scene/light_scene_view.hpp>
#include <scene/draw_operation.hpp>

#include <math/boundbox.hpp>
#include <math/convex_hull.hpp>
#include <moo/render_context.hpp>

namespace BW
{
namespace DrawHelpers
{

void cull( const SceneIntersectContext& context,
		  const Scene & scene, const Matrix & projectionMatrix,
		  IntersectionSet & visibleObjects )
{
	const IntersectSceneView* cullView = scene.getView<IntersectSceneView>();
	MF_ASSERT( cullView );
	cullView->cull( context, projectionMatrix, visibleObjects );
}


void drawSceneVisibilitySet( const SceneDrawContext & context,
	Scene & scene, const IntersectionSet & visibleObjects )
{
	DrawOperation* pDrawOp =
		scene.getObjectOperation<DrawOperation>();
	MF_ASSERT( pDrawOp != NULL );

	pDrawOp->drawAll( context, visibleObjects );
}


void addIntersectingSceneLightsToContainer( const Scene & scene,
	const BoundingBox & bbox, Moo::LightContainer & lightContainer )
{
	const LightSceneView * lightView = scene.getView<LightSceneView>();
	MF_ASSERT( lightView );
	lightView->intersect( bbox, lightContainer );
}

void addIntersectingSceneLightsToContainer( const Scene & scene,
	const ConvexHull & hull, Moo::LightContainer & lightContainer )
{
	const LightSceneView * lightView = scene.getView<LightSceneView>();
	MF_ASSERT( lightView );
	lightView->intersect( hull, lightContainer );
}

} // namespace DrawHelpers
} // namespace BW
