#include "pch.hpp"
#include "worldeditor/collisions/collision_terrain_only.hpp"

BW_BEGIN_NAMESPACE

CollisionTerrainOnly CollisionTerrainOnly::s_default;


/*virtual*/ int CollisionTerrainOnly::operator()(
	const CollisionObstacle& /*obstacle*/, const WorldTriangle& triangle,
	float /*dist*/ )
{
	BW_GUARD;

	if (triangle.flags() & TRIANGLE_TERRAIN)
		return COLLIDE_BEFORE;
	else
		return COLLIDE_ALL;
}   

BW_END_NAMESPACE

