#ifndef CHUNK_VLO_OBSTACLE_HPP
#define CHUNK_VLO_OBSTACLE_HPP

#include "chunk_model_obstacle.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the collection of obstacles formed by chunk VLO models
 *	in a chunk.
 */
class ChunkVLOObstacle : public ChunkModelObstacle
{
public:
	ChunkVLOObstacle( Chunk & chunk );
	~ChunkVLOObstacle();

	static Instance<ChunkVLOObstacle> instance;
protected:
	virtual int addToSpace( ChunkObstacle & cso );
	virtual void delFromSpace( ChunkObstacle & cso );
};

BW_END_NAMESPACE

#endif // CHUNK_VLO_OBSTACLE_HPP

