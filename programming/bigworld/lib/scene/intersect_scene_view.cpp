#include "pch.hpp"
#include "intersect_scene_view.hpp"

#include "cstdmf/profiler.hpp"

#include "math/vector4.hpp"
#include "math/matrix.hpp"
#include "math/boundbox.hpp"
#include "math/convex_hull.hpp"
#include "math/frustum_hull.hpp"
#include "math/planeeq.hpp"

namespace BW
{

size_t IntersectSceneView::intersect( const SceneIntersectContext& context,
	const ConvexHull& hull, 
	IntersectionSet & intersection) const
{
	PROFILER_SCOPED( IntersectSceneView_intersect );

	size_t totalVisible = 0;
	for ( ProviderCollection::const_iterator iter = providers_.begin();
		iter != providers_.end(); ++iter )
	{
		const ProviderInterface * pProvider = *iter;

		totalVisible += pProvider->intersect( context, hull, intersection );
	}

	return totalVisible;
}

size_t IntersectSceneView::cull( const SceneIntersectContext& context,
	const Matrix & worldViewProjectionMat, 
	IntersectionSet & visibleSet ) const
{
	FrustumHull frustum( worldViewProjectionMat );
	return this->intersect( context, frustum.hull(), visibleSet );
}


} // namespace BW
