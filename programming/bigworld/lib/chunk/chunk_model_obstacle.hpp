#ifndef CHUNK_MODEL_OBSTACLE_HPP
#define CHUNK_MODEL_OBSTACLE_HPP

#include "chunk_cache.hpp"
#include "chunk_item.hpp"
#include "chunk.hpp"


BW_BEGIN_NAMESPACE

class ChunkObstacle;
class Model;

/**
 *	This class is the collection of obstacles formed by a chunk models
 *	(or other Model-based items) in a chunk.
 */
class ChunkModelObstacle : public ChunkCache
{
public:
	ChunkModelObstacle( Chunk & chunk );
	~ChunkModelObstacle();

	virtual int focus();

	void addObstacle( ChunkObstacle * pObstacle );
	void delObstacles( ChunkItemPtr pItem );

	static Instance<ChunkModelObstacle> instance;

	void addModel( SmartPointer<Model> model, 
		const Matrix & world, ChunkItemPtr pItem,
		bool editorProxy = false );

private:
	typedef BW::list< SmartPointer< ChunkObstacle > > Obstacles;
	Obstacles obstacles_;
#ifdef EDITOR_ENABLED
	// In the tools, we add a mutex because we don't want the tools to crash
	// and any potential performance spikes are not as important as crashes.
	SimpleMutex obstaclesMutex_;
#endif // EDITOR_ENABLED

protected:
	virtual int addToSpace( ChunkObstacle & cso );
	virtual void delFromSpace( ChunkObstacle & cso );

	Chunk * pChunk_;
};

BW_END_NAMESPACE

#endif // CHUNK_MODEL_OBSTACLE_HPP

