#ifndef CHUNK_OBSTACLE_HPP
#define CHUNK_OBSTACLE_HPP

#include "cstdmf/smartpointer.hpp"
#include "math/matrix.hpp"

#if !defined( MF_SERVER )
#include "romp/photon_occluder.hpp"
#include "romp/romp_collider.hpp"
#endif // !defined( MF_SERVER )

#if ENABLE_RELOAD_MODEL
#include "model/model.hpp"
#endif // ENABLE_RELOAD_MODEL

#include "chunk_item.hpp"

#include "physics2/worldtri.hpp"
#include "physics2/collision_callback.hpp"
#include "physics2/collision_obstacle.hpp"
#include "physics2/collision_interface.hpp"

#include "moo/reload.hpp"


BW_BEGIN_NAMESPACE

class ChunkObstacle;
class CollisionState;
class Chunk;
class ChunkSpace;
class BSPTree;
class BoundingBox;
class Model;

/**
 *	This class defines an obstacle that can live in a column in a chunk space.
 */
class ChunkObstacle : 
	public ReferenceCount,
	public CollisionInterface
{
public:
	static const uint32 DYNAMIC_OBSTACLE_USER_FLAG = 4;

public:
	ChunkObstacle( const Matrix & transform, const BoundingBox* bb,
		ChunkItemPtr pItem );
	virtual ~ChunkObstacle();

	mutable uint32 mark_;

	const BoundingBox & bb_;

private:
	ChunkItemPtr pItem_;

public:
	Matrix	transform_;
	Matrix	transformInverse_;

	bool mark() const;
	static void nextMark() { s_nextMark_++; }
	static uint32 s_nextMark_;

public:

	ChunkItem* pItem() const;
	Chunk * pChunk() const;
};

typedef SmartPointer<ChunkObstacle> ChunkObstaclePtr;


/**
 *	This class is an obstacle represented by a BSP tree
 */
class ChunkBSPObstacle : 
	public ChunkObstacle
{
public:
	ChunkBSPObstacle( Model* bspModel, const Matrix & transform,
		const BoundingBox * bb, ChunkItemPtr pItem );

	ChunkBSPObstacle( const BSPTree* bsp, const Matrix & transform,
		const BoundingBox * bb, ChunkItemPtr pItem );

	virtual bool collide( const Vector3 & source, const Vector3 & extent,
		CollisionState & state ) const;
	virtual bool collide( const WorldTriangle & source, const Vector3 & extent,
		CollisionState & state ) const;

private:
	// We need use bspTree() instead of bspTree_ 
	// in case of model reload.
	BSPTree* bspTree_;

	BSPTree * bspTree();

#if ENABLE_RELOAD_MODEL
	Model* bspTreeModel_;
#endif // ENABLE_RELOAD_MODEL

};


#ifndef MF_SERVER
/**
 *	This class checks line-of-sight through the chunk space for the
 *	LensEffectManager.
 */
class ChunkObstacleOccluder : public PhotonOccluder
{
public:
	/**
	 *	Constructor
	 */
	ChunkObstacleOccluder() { }

	virtual float collides(
		const Vector3 & photonSourcePosition,
		const Vector3 & cameraPosition,
		const LensEffect& le );
};


/**
 *	This class implements the standard ground specifier for the
 *	grounded source action of a particle system.
 *
 *  This class is a chunk specific version of "SpaceRompCollider"
 *  Any changes here should really match the space romp collider implementation
 *  too.
 */
class ChunkRompCollider : public RompCollider
{
public:
	/**
	 *	Constructor
	 */
	ChunkRompCollider() { }

    virtual float ground( const Vector3 &pos, float dropDistance, bool onesided );
	virtual float ground( const Vector3 & pos );
	virtual float collide( const Vector3 &start, const Vector3& end, WorldTriangle& result );

	static RompColliderPtr create( FilterType filter );
};


/**
 *	This class implements the standard ground specifier for the
 *	grounded source action of a particle system.
 *	It uses terrain as ground
 */
class ChunkRompTerrainCollider : public ChunkRompCollider
{
public:
	/**
	 *	Constructor
	 */
	ChunkRompTerrainCollider() { }

	virtual float ground( const Vector3 & pos );
};


/**
*	This utility class finds the first collision with a terrain block
*/
class ClosestTerrainObstacle : public CollisionCallback
{
public:
	ClosestTerrainObstacle();

	virtual int operator() (const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist );

	void reset();

	float dist() const;

	bool collided() const;

private:
	float dist_;
	bool collided_;
};
#endif // MF_SERVER


#ifdef CODE_INLINE
#include "chunk_obstacle.ipp"
#endif

BW_END_NAMESPACE

#endif // CHUNK_OBSTACLE_HPP

