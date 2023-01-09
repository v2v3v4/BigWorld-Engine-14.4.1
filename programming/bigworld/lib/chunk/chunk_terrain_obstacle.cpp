#include "pch.hpp"
#include "chunk_terrain_obstacle.hpp"

#include "physics2/worldtri.hpp"

#include "terrain/base_terrain_block.hpp"

#include "terrain/terrain_collision_callback.hpp"
#include "terrain/terrain_hole_map.hpp"
#include "terrain/dominant_texture_map.hpp"
#include "terrain/terrain_ray_collision_callback.hpp"
#include "terrain/terrain_prism_collision_callback.hpp"

#include "scene/scene_object.hpp"

#include "cstdmf/guard.hpp"


BW_BEGIN_NAMESPACE

ChunkTerrainObstacle::ChunkTerrainObstacle( const Terrain::BaseTerrainBlock & tb,
		const Matrix & transform, const BoundingBox* bb,
		ChunkItemPtr pItem ) :
	ChunkObstacle( transform, bb, pItem ),
	tb_( tb )
{
}


/**
 *	This method collides the input ray with the terrain block.
 *	start and end should be inside (or on the edges of)
 *	our bounding box.
 *
 *	@param start	The start of the ray.
 *	@param end		The end of the ray.
 *	@param cs		Collision state holder.
 */

bool ChunkTerrainObstacle::collide( const Vector3 & start,
	const Vector3 & end, CollisionState & cs ) const
{
	BW_GUARD;
	Terrain::TerrainRayCollisionCallback toc( cs, transform_,
		transformInverse_, tb_,
		this->pItem()->sceneObject(),
		start, end );

#ifdef BW_WORLDTRIANGLE_DEBUG
	WorldTriangle::BeginDraw( transform_, 100, 500 );
#endif

	tb_.collide( start, end, &toc ); 

#ifdef BW_WORLDTRIANGLE_DEBUG	
	WorldTriangle::EndDraw();
#endif

	return toc.finishedColliding();
}


/**
 *	This method collides the input prism with the terrain block.
 * 
 *	@param start	The start triangle of prism.
 *	@param end		The position of the first vertex on end triangle.
 *	@param cs		Collision state holder.
 *
 *	@see collide
 */
bool ChunkTerrainObstacle::collide( const WorldTriangle & start,
	const Vector3 & end, CollisionState & cs ) const
{
    BW_GUARD;
	Terrain::TerrainPrismCollisionCallback toc( cs, transform_,
		transformInverse_, tb_,
		this->pItem()->sceneObject(),
		start, end );

    return tb_.collide( start, end, &toc );
}

BW_END_NAMESPACE

// chunk_terrain_obstacle.cpp
