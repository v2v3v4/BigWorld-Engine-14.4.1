#include "pch.hpp"
#include "terrain_prism_collision_callback.hpp"

#include "base_terrain_block.hpp"
#include "terrain_hole_map.hpp"
#include "dominant_texture_map.hpp"

#include "physics2/collision_obstacle.hpp"
#include "physics2/collision_callback.hpp"

BW_BEGIN_NAMESPACE


namespace Terrain
{

TerrainPrismCollisionCallback::TerrainPrismCollisionCallback( CollisionState& cs,
	const Matrix& transform,
	const Matrix& inverseTransform,
	const Terrain::BaseTerrainBlock& block,
	const SceneObject & sceneObject,
	const WorldTriangle& start, const Vector3& end ) :
		TerrainCollisionCallback( cs.cc_ ),
		cs_( cs ),
		block_( block ),
		sceneObject_( sceneObject ),
		transform_( transform ),
		inverseTransform_( inverseTransform ),
		start_( start ),
		end_( end )
{
	dir_ = end - start.v0();
	dist_ = dir_.length();

	dir_ *= 1.f / dist_;
}

TerrainPrismCollisionCallback::~TerrainPrismCollisionCallback()
{

}


bool TerrainPrismCollisionCallback::collide( const WorldTriangle& triangle, float dist )
{
	BW_GUARD;
	// see how far we really traveled (handles scaling, etc.)
	float ndist = cs_.sTravel_ + (cs_.eTravel_ - cs_.sTravel_) * (dist / dist_);

	if (cs_.onlyLess_ && ndist > cs_.dist_) return false;
	if (cs_.onlyMore_ && ndist < cs_.dist_) return false;

	// Calculate the impact point we use the centre of the triangle for this
	Vector3 impact = 
		(triangle.v0() + triangle.v1() + triangle.v2()) * (1.f /3.f);

	// check the hole map
	if (block_.holeMap().holeAt( impact.x, impact.z ))
	{
		return false;
	}

	// get the material kind and insert it in our triangle
	uint32 materialKind = 0;
	if ( block_.dominantTextureMap() != NULL )
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
	if (!say) return true;	// nope, we're outta here!

	// some are wanted ... see if it's only one side
	cs_.onlyLess_ = !(say & 2);
	cs_.onlyMore_ = !(say & 1);

	// We want more collisions
	return false;
}

}

BW_END_NAMESPACE


