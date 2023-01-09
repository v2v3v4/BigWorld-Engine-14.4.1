#include "pch.hpp"
#include "terrain_ray_collision_callback.hpp"

#include "base_terrain_block.hpp"
#include "terrain_hole_map.hpp"
#include "dominant_texture_map.hpp"

#include "physics2/collision_obstacle.hpp"
#include "physics2/collision_callback.hpp"

BW_BEGIN_NAMESPACE

// TODO: this is being accessed by declaring it as 'extern' in WE, and should be
// modified in another way, such as through a static member variable.
// This flag is set externally to allow colliding with holes
#ifdef EDITOR_ENABLED
bool s_collideTerrainHoles = false;
#endif

namespace Terrain
{

TerrainRayCollisionCallback::TerrainRayCollisionCallback( CollisionState& cs,
	const Matrix& transform,
	const Matrix& inverseTransform,
	const Terrain::BaseTerrainBlock& block,
	const SceneObject & sceneObject,
	const Vector3& start, const Vector3& end ) :
		TerrainCollisionCallback( cs.cc_ ),
		cs_( cs ),
		block_( block ),
		sceneObject_( sceneObject ),
		transform_( transform ),
		inverseTransform_( inverseTransform ),
		start_( start ),
		end_( end ),
		finishedColliding_( false )
{
	dir_ = end - start;
	dist_ = dir_.length();

	dir_ *= 1.f / dist_;
}

TerrainRayCollisionCallback::~TerrainRayCollisionCallback()
{

}


bool TerrainRayCollisionCallback::finishedColliding() const
{
	return finishedColliding_;
}


bool TerrainRayCollisionCallback::collide( const WorldTriangle& triangle, float dist )
{
	BW_GUARD;
// see how far we really traveled (handles scaling, etc.)
	float ndist = cs_.sTravel_ + (cs_.eTravel_ - cs_.sTravel_) * (dist / dist_);

	if (cs_.onlyLess_ && ndist > cs_.dist_) return false;
	if (cs_.onlyMore_ && ndist < cs_.dist_) return false;

	// Calculate the impact point
	Vector3 impact = start_ + dir_ * dist;

	// check the hole map
#ifdef EDITOR_ENABLED
	if ( !s_collideTerrainHoles )
	{
#endif // EDITOR_ENABLED
		if (block_.holeMap().holeAt( impact.x, impact.z ))
		{
			return false;
		}
#ifdef EDITOR_ENABLED
	}
#endif // EDITOR_ENABLED

	// get the material kind and insert it in our triangle
	uint32 materialKind = 0;
	if (block_.dominantTextureMap() != NULL)
	{
		materialKind = block_.dominantTextureMap()->materialKind( impact.x, impact.z );
	}

	WorldTriangle wt = triangle;

	wt.flags( WorldTriangle::packFlags( wt.collisionFlags(), materialKind ) );

	cs_.dist_ = ndist;

	// call the callback function
	CollisionObstacle ob( &transform_, &inverseTransform_, sceneObject_ );
	int say = cs_.cc_.visit( ob, wt, cs_.dist_ );

	// see if any other collisions are wanted
	if (!say) 
	{
		// if no further collisions are needed
		// set the finished colliding flag
		finishedColliding_ = true;
		return true;
	}

	// some are wanted ... see if it's only one side
	cs_.onlyLess_ = !(say & 2);
	cs_.onlyMore_ = !(say & 1);

	// Return true if only nearer collisions are wanted,
	// this allows us to do an early out, as the collision
	// order for terrain blocks are nearest to furthest. 
	// As the finishedColliding_ flag is not set we can 
	// still get collisions from other objects, just not 
	// this terrain block.
	return cs_.onlyLess_;
}

} // namespace Terrain

BW_END_NAMESPACE


