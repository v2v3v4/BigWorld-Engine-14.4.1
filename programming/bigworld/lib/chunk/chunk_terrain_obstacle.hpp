#ifndef CHUNK_TERRAIN_OBSTACLE_HPP
#define CHUNK_TERRAIN_OBSTACLE_HPP

#include "chunk_obstacle.hpp"


BW_BEGIN_NAMESPACE

class BoundingBox;
// class WorldTriangle;

namespace Moo
{
	class BaseTerrainBlock;
}

namespace Terrain
{
	class BaseTerrainBlock;
}

/**
 *	This class treats a terrain block as an obstacle
 */
class ChunkTerrainObstacle : public ChunkObstacle
{
public:
	ChunkTerrainObstacle( const Terrain::BaseTerrainBlock & tb,
			const Matrix & transform, const BoundingBox* bb,
			ChunkItemPtr pItem );

	virtual bool collide( const Vector3 & start, const Vector3 & end,
		CollisionState & state ) const;
	virtual bool collide( const WorldTriangle & start, const Vector3 & end,
		CollisionState & state ) const;


	const Terrain::BaseTerrainBlock& block() const { return tb_; }

private:

	const Terrain::BaseTerrainBlock& tb_;
};

BW_END_NAMESPACE

#endif // CHUNK_TERRAIN_OBSTACLE_HPP
// chunk_terrain_obstacle.hpp

