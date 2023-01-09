#include "pch.hpp"

#include "space_romp_collider.hpp"
#include "space_manager.hpp"
#include "client_space.hpp"
#include "scene/scene_object.hpp"
#include "deprecated_space_helpers.hpp"

// TODO: Remove this dependency
#include <chunk/chunk_obstacle.hpp>

namespace BW
{

class RompCollider_ClosestTriangle : public CollisionCallback
{
public:
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist )
	{
		BW_GUARD;

		// transform into world space
		s_collider = WorldTriangle(
			obstacle.transform().applyPoint( triangle.v0() ),
			obstacle.transform().applyPoint( triangle.v1() ),
			obstacle.transform().applyPoint( triangle.v2() ) );

		return COLLIDE_BEFORE;
	}

	static RompCollider_ClosestTriangle s_default;
	WorldTriangle s_collider;
};
RompCollider_ClosestTriangle RompCollider_ClosestTriangle::s_default;


RompColliderPtr SpaceRompCollider::create( FilterType filter )
{
	if (filter == TERRAIN_ONLY)
	{
		return new SpaceRompTerrainCollider();
	}
	else
	{
		return new SpaceRompCollider();
	}
}

float SpaceRompCollider::ground( const Vector3 &pos, float dropDistance, 
	bool onesided )
{
	BW_GUARD;
	Vector3 lowestPoint( pos.x, pos.y - dropDistance, pos.z );

	float dist = -1.f;
	ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
	if (pSpace != NULL)
	{
		ClosestObstacleEx coe( onesided, Vector3(0.0f, -1.0f, 0.0f) );

		dist = pSpace->collide( pos, lowestPoint, coe );
	}

	if (dist < 0.0f) 
	{
		return RompCollider::NO_GROUND_COLLISION;
	}

	return pos.y - dist;
}

float SpaceRompCollider::ground( const Vector3 & pos )
{
	BW_GUARD;
	Vector3 dropSrc( pos.x, pos.y + 15.f, pos.z );
	Vector3 dropMax( pos.x, pos.y - 15.f, pos.z );

	float dist = -1.f;
	ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
	if (pSpace != NULL)
	{
		dist = pSpace->collide( dropSrc, dropMax, ClosestObstacle::s_default );
	}

	if (dist < 0.f)
	{
		return RompCollider::NO_GROUND_COLLISION;
	}

	return pos.y + 15.f - dist;
}

float SpaceRompCollider::collide( const Vector3 &start, const Vector3& end, 
	WorldTriangle& result )
{
	BW_GUARD;
	float dist = -1.f;
	ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
	if (pSpace != NULL)
	{
		dist = pSpace->collide( start, end, 
			RompCollider_ClosestTriangle::s_default );
	}

	if (dist < 0.f) 
	{
		return RompCollider::NO_GROUND_COLLISION;
	}

	//set the world triangle.
	result = RompCollider_ClosestTriangle::s_default.s_collider;

	return dist;
}

/**
 *	This method returns the height of the terrain under pos.
 */
#define GROUND_TEST_INTERVAL 500.f
float SpaceRompTerrainCollider::ground( const Vector3 & pos )
{
	BW_GUARD;
	//RA: increased the collision test interval
	Vector3 dropSrc( pos.x, pos.y + GROUND_TEST_INTERVAL, pos.z );
	Vector3 dropMax( pos.x, pos.y - GROUND_TEST_INTERVAL, pos.z );

	float dist = -1.f;
	ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
	ClosestTerrainObstacle terrainCallback;
	if (pSpace != NULL)
	{
		dist = pSpace->collide( dropSrc, dropMax, terrainCallback );
	}

	if (dist < 0.f) 
	{
		return RompCollider::NO_GROUND_COLLISION;
	}

	return pos.y + GROUND_TEST_INTERVAL - dist;
}

} // namespace BW