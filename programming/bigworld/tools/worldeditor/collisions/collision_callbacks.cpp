#include "pch.hpp"
#include "worldeditor/collisions/collision_callbacks.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "physics2/collision_obstacle.hpp"
#include "gizmo/matrix_mover.hpp"

DECLARE_DEBUG_COMPONENT2( "editor", 0 )

BW_BEGIN_NAMESPACE

ObstacleLockCollisionCallback ObstacleLockCollisionCallback::s_default;


/**
 *  This operator is called by the collision code to test the collided
 *  triangles.
 *
 *  @param obstacle		ChunkObstacle that owns the triangle.
 *  @param triangle		Triangle that collided, in world coordinates.
 *	@param dist			Collision distance to the triangle.
 *	@return				Collision flags to stop/continue testing triangles.
 *
 *  @see ChunkObstacle
 */
int ObstacleLockCollisionCallback::operator() ( 
	const CollisionObstacle & obstacle,
	const WorldTriangle & triangle, float dist )
{
	BW_GUARD;

	ChunkItem * pItem = obstacle.sceneObject().isType<ChunkItem>() ?
			obstacle.sceneObject().getAs<ChunkItem>() : NULL;
	MF_ASSERT( pItem );
	if (!pItem)
	{
		return COLLIDE_ALL;
	}

	if( MatrixMover::moving() && !pItem->isShellModel() && WorldManager::instance().isItemSelected(pItem) )
		return COLLIDE_ALL;
	// ignore selected
	if ( !pItem->edIsSnappable() )
		return COLLIDE_ALL;

	// ignore back facing
	Vector3 trin = obstacle.transform().applyVector( triangle.normal() );
	trin.normalise();
	Vector3 lookv = obstacle.transform().applyPoint( triangle.v0() ) -
		Moo::rc().invView()[3];
	lookv.normalise();
	const float dotty = trin.dotProduct( lookv );
	if ( dotty > 0.f )
		return COLLIDE_ALL;

	// choose if closer
	if ( dist < dist_ )
	{
		dist_ = dist;
		normal_ = trin;
		pItem_ = pItem;
		triangleNormal_ = triangle.normal();
	}

	//return COLLIDE_BEFORE;
	return COLLIDE_ALL;
}


/**
 *  This method releases the references/resources gathered during the collision
 *  test. This method should be called after performing a collision, when the
 *  callback result is no longer needed, or when finalising the app.
 */
void ObstacleLockCollisionCallback::clear()
{
	BW_GUARD;

	pItem_ = NULL;
}

BW_END_NAMESPACE

