#include "pch.hpp"
#include "chunk_obstacle.hpp"
#include "chunk_manager.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "physics2/bsp.hpp"
#include "physics2/sweep_helpers.hpp"

#include "scene/scene_object.hpp"

#if defined( MF_SERVER )
#include "server_super_model.hpp"
#else
#include "model/model.hpp"
#endif // MF_SERVER

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "chunk_obstacle.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: ChunkObstacle
// -----------------------------------------------------------------------------


/// static Obstacle data initialiser
uint32 ChunkObstacle::s_nextMark_ = 0;

/**
 *	Obstacle constructor
 *  NOTE: The caller must ensure that the life time of bb covers
 *  the life time of the obstacle object, since bb_ is a pointer.
 *	@param	transform	world transform matrix
 *	@param	bb	BoundingBox of the obstacle
 *	@param	pItem	pointer to ChunkItem link to this obstacle
 */
ChunkObstacle::ChunkObstacle( const Matrix & transform,
		const BoundingBox* bb, ChunkItemPtr pItem ) :
	mark_( s_nextMark_ - 16 ),
	bb_( *bb ),
	pItem_( pItem ),
	transform_( transform )
{
	transformInverse_.invert( transform );

	//MF_ASSERT_DEV( this->pChunk() != NULL );
	// can't check this for dynamic obstacles
}

/**
 *	Obstacle destructor
 */
ChunkObstacle::~ChunkObstacle()
{
}

/**
 *	This method sets the mark to the current 'next mark' value.
 *	@return true if the mark was already set
 */
bool ChunkObstacle::mark() const
{
	if (mark_ == s_nextMark_) return true;
	mark_ = s_nextMark_;
	return false;
}


// -----------------------------------------------------------------------------
// Section: ChunkBSPObstacle
// -----------------------------------------------------------------------------


/**
 *	Constructor
 *	@param	bspModel	the bsp model of the obstacle
 *	@param	transform	the world transform of the obstacle
 *	@param	bb	pointer to a bounding box of this obstacle
 *	@param	pItem	pointer to the ChunkItem associates with the obstacle
 */
ChunkBSPObstacle::ChunkBSPObstacle( Model* bspModel,
		const Matrix & transform, const BoundingBox * bb, ChunkItemPtr pItem ) :
		ChunkObstacle( transform, bb, pItem ),
#if ENABLE_RELOAD_MODEL
		bspTreeModel_( bspModel ),
		bspTree_( NULL )
#else
		bspTree_( (BSPTree *)bspModel->decompose() )
#endif // ENABLE_RELOAD_MODEL
{
}

/**
 *	Constructor
 *	@param	bspTree	the bsp tree of the obstacle
 *	@param	transform	the world transform of the obstacle
 *	@param	bb	pointer to a bounding box of this obstacle
 *	@param	pItem	pointer to the ChunkItem associates with the obstacle
 */
ChunkBSPObstacle::ChunkBSPObstacle( const BSPTree* bspTree,
		const Matrix & transform, const BoundingBox * bb, ChunkItemPtr pItem ) :
	ChunkObstacle( transform, bb, pItem ),
	bspTree_( (BSPTree*)bspTree )
#if ENABLE_RELOAD_MODEL
	,bspTreeModel_(NULL)
#endif
{
}

/**
 *	This is the collide function for BSP obstacles, called once intersection
 *	with the bounding box has been determined. It returns true to stop
 *	the chunk space looking for more collisions (nearer or further),
 *	or it can set some values in the state object to indicate what
 *	other collisions it is interested in.
 *	@param	source	the start point of the ray
 *	@param	extent	the extent of the ray
 *	@param	state	the collision state object used to return
 *					the collision result
 *	@result	true to stop, false to continue searching
 */
bool ChunkBSPObstacle::collide( const Vector3 & source,
	const Vector3 & extent, CollisionState & state ) const
{
	BW_GUARD;

	bool isMoving = 
		pItem()->userFlags() & DYNAMIC_OBSTACLE_USER_FLAG ? true : false;

	SceneObject sceneObject;

	if (pItem())
	{
		sceneObject = pItem()->sceneObject();
	}

	CollisionObstacle ob( &transform_, &transformInverse_,
		sceneObject,
		isMoving ? CollisionObstacle::FLAG_MOVING_OBSTACLE : 0 );

	SweepHelpers::BSPSweepVisitor cscv( ob, state );

	float rd = 1.0f;

	BSPTree * pBSPTree = bspTree_;
#if ENABLE_RELOAD_MODEL
	if (bspTreeModel_)
	{
		pBSPTree = (BSPTree *)bspTreeModel_->decompose();
	}
#endif

	if (pBSPTree)
	{
#ifdef COLLISION_DEBUG
	DEBUG_MSG( "ChunkBSPObstacle::collide(pt): %s\n",
			pBSPTree->name().c_str() );
#endif
		pBSPTree->intersects( source, extent, rd, NULL, &cscv );
	}

	return cscv.shouldStop();
}

/**
 *	This is the collide function for prisms formed from a world triangle.
 *
 *	@see collide
 *	@param	source	The start triangle to collide with
 *	@param	extent	The extent of the triangle
 *	@param	state	The collision state object used to return
 *					the collision result
 *	@result	true to stop, false to continue searching
 */
bool ChunkBSPObstacle::collide( const WorldTriangle & source,
	const Vector3 & extent, CollisionState & state ) const
{
	bool isMoving = 
		pItem()->userFlags() & DYNAMIC_OBSTACLE_USER_FLAG ? true : false;

	SceneObject sceneObject;

	if (pItem())
	{
		sceneObject = pItem()->sceneObject();
	}

	CollisionObstacle ob( &transform_, &transformInverse_,
		sceneObject,
		isMoving ? CollisionObstacle::FLAG_MOVING_OBSTACLE : 0 );

	SweepHelpers::BSPSweepVisitor cscv( ob, state );

	BSPTree * pBSPTree = bspTree_;
#if ENABLE_RELOAD_MODEL
	if (bspTreeModel_)
	{
		pBSPTree = (BSPTree *)bspTreeModel_->decompose();
	}
#endif

	if (pBSPTree)
	{
#ifdef COLLISION_DEBUG
	DEBUG_MSG( "ChunkBSPObstacle::collide(tri): %s\n",
			pBSPTree->name().c_str() );
#endif
		pBSPTree->intersects( source, extent - source.v0(), &cscv );
	}

	return cscv.shouldStop();
}


// -----------------------------------------------------------------------------
// Section: ChunkObstacleOccluder
// -----------------------------------------------------------------------------

#ifndef MF_SERVER

BW_END_NAMESPACE

#include "chunk/chunk_space.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is a chunk obstacle version of the bsp photon visibility tester
 */
class PhotonVisibility2 : public CollisionCallback
{
public:
	/**
	 *	Constructor
	 */
	PhotonVisibility2() : gotone_( false ), blended_( false ) { }

	/**
	 *	The overridden operator()
	 */
	int operator()( const CollisionObstacle & co,
		const WorldTriangle & hitTriangle, float dist )
	{
		// record if this one's blended
		if (hitTriangle.isBlended()) blended_ = true;

		// if it's not transparent, we can stop now
		if (!hitTriangle.isTransparent()) { gotone_ = true; return 0; }

		// otherwise we have to keep on going
		return COLLIDE_BEFORE | COLLIDE_AFTER;
	}

	bool gotone()			{ return gotone_; }
	bool hitBlended()		{ return blended_; }

private:
	bool gotone_;
	bool blended_;
};


/**
 *	This method performs the main function of this class: a collision test
 */
float ChunkObstacleOccluder::collides(
	const Vector3 & photonSourcePosition,
	const Vector3 & cameraPosition,
	const LensEffect& le )
{
	BW_GUARD;
	float result = 1.f;

	Vector3 trSourcePos;
	//csTransformInverse_.applyPoint( trSourcePos, photonSourcePosition );
	trSourcePos = photonSourcePosition;

	Vector3 trCamPos;
	//csTransformInverse_.applyPoint( trCamPos, cameraPosition );
	trCamPos = cameraPosition;

	//Fudge the source position by 50cm.
	//This is a test fix for lens flares that are placed
	//inside light bulbs, or geometric candle flame.
	Vector3 ray(trCamPos - trSourcePos);
	ray.normalise();
	Vector3 fudgedSource = ray * 0.5f + trSourcePos;

	PhotonVisibility2	obVisitor;

	ChunkSpace * pCS = &*ChunkManager::instance().cameraSpace();
	if (pCS != NULL)
		pCS->collide( trCamPos, fudgedSource, obVisitor );

	if (obVisitor.gotone())
	{
		//sun ended up on a solid poly
		result = 0.f;
	}
	else
	{
		if (obVisitor.hitBlended())
		{
			//sun is essentially visible, but maybe through a translucent poly
			result = 0.5f;
		}
		else
		{
			//sun is completely unobstructed
			result = 1.f;
		}
	}

	return result;
}



// -----------------------------------------------------------------------------
// Section: ChunkRompCollider
// -----------------------------------------------------------------------------


/**
 *	This method returns the height of the ground under pos, doing only
 *  one-sided collisions tests.
 *
 *	@param pos				The position from which to drop.
 *	@param dropDistance		The distance over which the drop is checked.
 *  @param oneSided         If true then only consider collisions where 
 *                          the collision triangle is pointing up.
 *
 *	@return The height of the ground found. If no ground was found within the
 *			dropDistance from the position supplied, 
 *          RompCollider::NO_GROUND_COLLISION is returned.
 */
float ChunkRompCollider::ground( const Vector3 &pos, float dropDistance, bool oneSided )
{
	BW_GUARD;
	Vector3 lowestPoint( pos.x, pos.y - dropDistance, pos.z );

	float dist = -1.f;
	ChunkSpace * pCS = &*ChunkManager::instance().cameraSpace();
	if (pCS != NULL)
	{
		ClosestObstacleEx coe
		(
			oneSided, 
			Vector3(0.0f, -1.0f, 0.0f)
		);

		dist = pCS->collide
		( 
			pos, 
			lowestPoint, 
			coe
		);
	}
	if (dist < 0.0f) return RompCollider::NO_GROUND_COLLISION;

	return pos.y - dist;
}


/**
 *	This method returns the height of the ground under pos.
 */
float ChunkRompCollider::ground( const Vector3 & pos )
{
	BW_GUARD;
	Vector3 dropSrc( pos.x, pos.y + 15.f, pos.z );
	Vector3 dropMax( pos.x, pos.y - 15.f, pos.z );

	float dist = -1.f;
	ChunkSpace * pCS = &*ChunkManager::instance().cameraSpace();
	if (pCS != NULL)
		dist = pCS->collide( dropSrc, dropMax, ClosestObstacle::s_default );
	if (dist < 0.f) return RompCollider::NO_GROUND_COLLISION;

	return pos.y + 15.f - dist;
}


/**
 *	This method returns the height of the terrain under pos.
 */
#define GROUND_TEST_INTERVAL 500.f
float ChunkRompTerrainCollider::ground( const Vector3 & pos )
{
	BW_GUARD;
	//RA: increased the collision test interval
	Vector3 dropSrc( pos.x, pos.y + GROUND_TEST_INTERVAL, pos.z );
	Vector3 dropMax( pos.x, pos.y - GROUND_TEST_INTERVAL, pos.z );

	float dist = -1.f;
	ChunkSpace * pCS = &*ChunkManager::instance().cameraSpace();
	ClosestTerrainObstacle terrainCallback;
	if (pCS != NULL)
		dist = pCS->collide( dropSrc, dropMax, terrainCallback );
	if (dist < 0.f) return RompCollider::NO_GROUND_COLLISION;

	return pos.y + GROUND_TEST_INTERVAL - dist;
}


// -----------------------------------------------------------------------------
// Section: CRCClosestTriangle
// -----------------------------------------------------------------------------

/**
 *	This method returns the height of the ground under pos.
 */
BW_END_NAMESPACE
#include "physics2/worldtri.hpp"
BW_BEGIN_NAMESPACE
#ifdef EDITOR_ENABLED
extern char * g_chunkParticlesLabel;
#endif // EDITOR_ENABLED

class CRCClosestTriangle : public CollisionCallback
{
public:
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist )
	{
		BW_GUARD;	
#ifdef EDITOR_ENABLED
		// don't collide against the particle system selectable representation
		// Editor can assume everything is a ChunkItem.
		const SceneObject & sceneObject = obstacle.sceneObject();
		if (sceneObject.isType<ChunkItem>() &&
			sceneObject.getAs<ChunkItem>()->label() == g_chunkParticlesLabel)
		{
			return COLLIDE_ALL;
		}
#endif // EDITOR_ENABLED

		// transform into world space
		s_collider = WorldTriangle(
			obstacle.transform().applyPoint( triangle.v0() ),
			obstacle.transform().applyPoint( triangle.v1() ),
			obstacle.transform().applyPoint( triangle.v2() ) );

		return COLLIDE_BEFORE;
	}

	static CRCClosestTriangle s_default;
	WorldTriangle s_collider;
};
CRCClosestTriangle CRCClosestTriangle::s_default;

float ChunkRompCollider::collide( const Vector3 &start, const Vector3& end, WorldTriangle& result )
{
	BW_GUARD;
	float dist = -1.f;
	ChunkSpace * pCS = &*ChunkManager::instance().cameraSpace();
	if (pCS != NULL)
		dist = pCS->collide( start, end, CRCClosestTriangle::s_default );
	if (dist < 0.f) return RompCollider::NO_GROUND_COLLISION;

	//set the world triangle.
	result = CRCClosestTriangle::s_default.s_collider;

	return dist;
}


RompColliderPtr ChunkRompCollider::create( FilterType filter )
{
	if (filter == TERRAIN_ONLY)
	{
		return new ChunkRompTerrainCollider();
	}
	else
	{
		return new ChunkRompCollider();
	}
}


// -----------------------------------------------------------------------------
// Section: ClosestTerrainObstacle
// -----------------------------------------------------------------------------

ClosestTerrainObstacle::ClosestTerrainObstacle() :
	dist_( -1 ),
	collided_( false )
{
}


int ClosestTerrainObstacle::operator() ( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist )
{
    if (triangle.flags() & TRIANGLE_TERRAIN)
    {
    	// Set the distance if it's the first collision, or if the distance is
    	// less than the previous collision.
		if (!collided_ || dist < dist_)
		{
	        dist_ = dist;
			collided_ = true;
		}

        return COLLIDE_BEFORE;
    }
    else
	{
		return COLLIDE_ALL;
	}
}


void ClosestTerrainObstacle::reset()
{
	dist_ = -1;
	collided_ = false;
}


float ClosestTerrainObstacle::dist() const
{
	return dist_;
}


bool ClosestTerrainObstacle::collided() const
{
	return collided_;
}


#endif // MF_SERVER


// -----------------------------------------------------------------------------
// Section: Static initialisers
// -----------------------------------------------------------------------------


/// static initialiser for the default 'any-obstacle' callback object
CollisionCallback CollisionCallback::s_default;
/// static initialiser for the 'closest-obstacle' callback object
ClosestObstacle ClosestObstacle::s_default;

/// static initialiser for the reference to the default callback object
CollisionCallback & CollisionCallback_s_default = CollisionCallback::s_default;
// (it's a reference so that chunkspace.hpp can use it without including us)

BW_END_NAMESPACE

// chunk_obstacle.cpp
