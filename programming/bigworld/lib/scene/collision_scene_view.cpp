#include "pch.hpp"
#include "collision_scene_view.hpp"

#include "physics2/sweep_helpers.hpp"
#include "cstdmf/profiler.hpp"

namespace BW
{

float CollisionSceneView::collide( const Vector3 & start, 
	const Vector3 & end, CollisionCallback & cc ) const
{
	PROFILER_SCOPED( CollisionSceneView_collide_ray );

	SweepShape<Vector3> sweepShape( start );
	SweepParams sp( sweepShape, end );
	CollisionState cs( cc );

	for ( ProviderCollection::const_iterator iter = providers_.begin();
		iter != providers_.end(); ++iter )
	{
		ICollisionSceneViewProvider* pProvider = *iter;
		if (pProvider->collide( start, end, sp, cs ))
		{
			return cs.dist_;
		}
	}
	
	return cs.dist_;
}


float CollisionSceneView::collide( const WorldTriangle & start, 
	const Vector3 & end, CollisionCallback & cc ) const
{
	PROFILER_SCOPED( CollisionSceneView_collide_prism );

	SweepShape<WorldTriangle> sweepShape( start );
	SweepParams sp( sweepShape, end );
	CollisionState cs( cc );

	for ( ProviderCollection::const_iterator iter = providers_.begin();
		iter != providers_.end(); ++iter )
	{
		ICollisionSceneViewProvider* pProvider = *iter;
		if (pProvider->collide( start, end, sp, cs ))
		{
			return cs.dist_;
		}
	}

	return cs.dist_;
}

} // namespace BW
