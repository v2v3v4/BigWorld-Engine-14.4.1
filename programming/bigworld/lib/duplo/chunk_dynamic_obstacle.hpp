#ifndef CHUNK_DYNAMIC_OBSTACLE_HPP
#define CHUNK_DYNAMIC_OBSTACLE_HPP

#include "chunk_embodiment.hpp"

#include "chunk/chunk_obstacle.hpp"

BW_BEGIN_NAMESPACE

class PyModelObstacle;


/*~ class BigWorld.PyModelObstacle
 *
 *	The PyModelObstacle object is a Model that is integrated into the collision scene.  They have no motors, 
 *	attachments or fashions, like a PyModel and so are driven by a single MatrixProvider, which is often linked to 
 *	the Entity that will use the PyModelObstacle.  the PyModelObstacle can be dynamic and move around the world, 
 *	still engaged in the collision scene, or static, remaining in a single location.  In the latter case, it makes 
 *	sense for the owner Entity to be stationary in the world, otherwise the PyModelObstacle and Entity position 
 *	would be dispersed, which is undesirable.  The PyModelObstacle should be assigned to the self.model attribute 
 *	of an Entity to become part of the world and once this occurs, the dynamic or static state of the 
 *	PyModelObstacle is fixed and cannot be changed.  PyModelObstacles are more expensive that standard PyModels, 
 *	hence should only be used when completely necessary.
 *
 *	PyModelObstacles are constructed using BigWorld.PyModelObstacle function.
 */
/**
 *	This class is a dynamic obstacle in a chunk. It moves around every
 *	frame and you can collide with it.
 */
class ChunkDynamicObstacle : public ChunkDynamicEmbodiment
{
public:
	ChunkDynamicObstacle( PyModelObstacle * pObstacle );
	virtual ~ChunkDynamicObstacle();

	virtual void tick( float dTime );
	virtual void updateAnimations();
	virtual void draw( Moo::DrawContext& drawContext );
	virtual void toss( Chunk * pChunk );

	virtual void enterSpace( ChunkSpacePtr pSpace, bool transient );
	virtual void leaveSpace( bool transient );
	virtual void move( float dTime );

	virtual const Matrix & worldTransform() const;
	virtual void localBoundingBox( BoundingBox & bb, bool skinny = false ) const;
	virtual void worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny = false ) const;

	PyModelObstacle * pObstacle() const;

	typedef BW::vector<ChunkObstaclePtr> Obstacles;
	Obstacles & obstacles()	{ return obstacles_; }

	static int convert( PyObject * pObj, ChunkEmbodimentPtr & pNew,
		const char * varName );

private:
	ChunkDynamicObstacle( const ChunkDynamicObstacle& );
	ChunkDynamicObstacle& operator=( const ChunkDynamicObstacle& );


	Obstacles			obstacles_;
};

typedef SmartPointer<ChunkDynamicObstacle> ChunkDynamicObstaclePtr;


/**
 *	This class is a static obstacle in a chunk. It's still a little bit
 *	dynamic because scripts create and delete these, but it doesn't move
 *	around every frame like ChunkDynamicObstacle does.
 */
class ChunkStaticObstacle : public ChunkStaticEmbodiment
{
public:
	ChunkStaticObstacle( PyModelObstacle * pObst );
	virtual ~ChunkStaticObstacle();

	virtual void tick( float dTime );
	virtual void updateAnimations();
	virtual void draw( Moo::DrawContext& drawContext );
	virtual void toss( Chunk * pChunk );
	virtual void lend( Chunk * pLender );

	virtual void enterSpace( ChunkSpacePtr pSpace, bool transient );
	virtual void leaveSpace( bool transient );

	virtual const Matrix & worldTransform() const;
	virtual void localBoundingBox( BoundingBox & bb, bool skinny = false ) const;
	virtual void worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny = false ) const;

	PyModelObstacle * pObstacle() const;

	typedef BW::vector<ChunkObstaclePtr> Obstacles;
	Obstacles & obstacles()	{ return obstacles_; }

	static int convert( PyObject * pObj, ChunkEmbodimentPtr & pNew,
		const char * varName );

private:
	ChunkStaticObstacle( const ChunkStaticObstacle& );
	ChunkStaticObstacle& operator=( const ChunkStaticObstacle& );

	Matrix worldTransform_;
	Obstacles obstacles_;
};


class ChunkObstacle;
typedef SmartPointer<ChunkObstacle> ChunkObstaclePtr;


BW_END_NAMESPACE

#endif // CHUNK_DYNAMIC_OBSTACLE_HPP
