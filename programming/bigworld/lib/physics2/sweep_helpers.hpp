#ifndef SWEEP_HELPERS_HPP
#define SWEEP_HELPERS_HPP

#include "sweep_shape.hpp"
#include "collision_obstacle.hpp"
#include "collision_interface.hpp"
#include "bsp.hpp"

#include "math/vector3.hpp"
#include "math/matrix.hpp"
#include "math/boundbox.hpp"

BW_BEGIN_NAMESPACE

class CollisionState;

namespace SweepHelpers
{

inline int biggestAxisOfDifference( const Vector3 & a, const Vector3 & b )
{
	int bax = 0;
	float diff1 = fabsf( a[0] - b[0] );
	float diff2 = fabsf( a[1] - b[1] );
	if (diff2 > diff1) 
	{ 
		diff1 = diff2; 
		bax = 1; 
	}

	diff2 = fabsf( a[2] - b[2] );
	if (diff2 > diff1) 
	{ 
		bax = 2; 
	}

	return bax;
}

/**
 *	This helper function is used by broad-phase sweep code to
 *	decide whether to enter into the given object (represented
 *	by a bounding box), or to skip over the object entirely.
 */
template<typename ShapeType>
bool collideIntoObject(
	const SweepShape<ShapeType>& start,
	const SweepParams& sp,
	const CollisionInterface& collisionFuncs,
	const AABB& obstacleLocalBB,
	const Matrix& obstacleInvTransform,
	CollisionState& cs )
{
	Vector3 sTr, eTr;
	obstacleInvTransform.applyPoint( sTr, sp.csource() );
	obstacleInvTransform.applyPoint( eTr, sp.cextent() );

	int bax = biggestAxisOfDifference( eTr, sTr );

	float sTrba = sTr[bax];
	float dTrba = eTr[bax] - sTr[bax];

	// Clip the line to the bounding box ('tho it should always
	//  be inside since we found it through the hull tree)
	float bloat = start.transformRangeToRadius(
		obstacleInvTransform, sp.shapeRange() ) + 0.01f;

	Vector3 sTr_clipped( sTr );
	Vector3 eTr_clipped( eTr );
	if (!obstacleLocalBB.clip( sTr_clipped, eTr_clipped, bloat ))
	{
		return false;
	}
		
	// set traveled and traveled to be the start and end distances
	//  along the line (not their original use, but it fits)
	cs.sTravel_ = (sTr_clipped[bax] - sTrba) / dTrba * sp.fullDistance();
	cs.eTravel_ = (eTr_clipped[bax] - sTrba) / dTrba * sp.fullDistance();

	// see if we can reject this bb outright
	if (cs.onlyLess_ && cs.sTravel_ > cs.dist_)
	{
		return false;
	}

	if (cs.onlyMore_ && cs.eTravel_ < cs.dist_)
	{
		return false;
	}

	// ok, let's search in it then
	ShapeType shape;
	Vector3 shapeEnd;
	start.transform( shape, shapeEnd, obstacleInvTransform,
		cs.sTravel_, cs.eTravel_, sp.direction(),
		sTr_clipped, eTr_clipped );

	return collisionFuncs.collide( shape, shapeEnd, cs );
}


/**
 *	This helper class is the sweep collision visitor,
 *	for interacting with the BSP class, until it is moved over
 *	to the CollisionCallback or traversal object inspection style.
 *
 *	@note Currently, the BSP will not get all the triangles that
 *	 are hit - it will skip multiple collisions on the same plane
 *	 (i.e. with different triangles). This is not good.
 *	TODO: Fix the problem described in the note above
 */
class BSPSweepVisitor : public CollisionVisitor
{
public:
	/**
	 *	constructor
	 *	@param	ob	the obstacle to collide with
	 *	@param	cs	the CollisionState object that returns the result of the collision
	 */
	BSPSweepVisitor( const CollisionObstacle & ob, CollisionState & cs ) :
		ob_( ob ), cs_( cs ), stop_( false ) { }

	bool shouldStop() const { return stop_; }

private:
	virtual bool visit( const WorldTriangle & hitTriangle, float rd );

	const CollisionObstacle & ob_;
	CollisionState & cs_;
	bool	stop_;
};

} // namepsace SweepHelpers


BW_END_NAMESPACE

#endif // SWEEP_HELPERS
