
#include "pch.hpp"

#include "collision_advance.hpp"
#include "physics2/collision_obstacle.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: CollisionAdvance
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
CollisionAdvance::CollisionAdvance( const Vector3 & origin,
			const Vector3 & axis1, const Vector3 & axis2,
			const Vector3 & direction,
			float maxAdvance ) :
		origin_( origin ),
		planeEq0_( origin, axis2 ),
		planeEq1_( origin, axis1 ),
		planeEq2_( origin + axis2, Vector3(-axis2) ),
		planeEq3_( origin + axis1, Vector3(-axis1) ),
		planeEq4_( origin, direction ),
		dir_( direction ),
		advance_( maxAdvance ),
		ignoreFlags_( TRIANGLE_NOCOLLIDE ),
		shouldFindCentre_( false )
{
}


/**
 *	Destructor.
 */
CollisionAdvance::~CollisionAdvance()
{
}


/**
 *	This is the function that is called by the collision function for each
 *	triangle that we hit.
 */
int CollisionAdvance::operator()( const CollisionObstacle & obstacle,
	const WorldTriangle & triangle, float dist )
{
	BW_GUARD;

	if (triangle.flags() & ignoreFlags_)
	{
		return COLLIDE_ALL;
	}

	ChopPolygon poly(3);

	poly[0] = obstacle.transform().applyPoint( triangle.v0() );
	poly[1] = obstacle.transform().applyPoint( triangle.v1() );
	poly[2] = obstacle.transform().applyPoint( triangle.v2() );

	WorldTriangle wt( poly[0], poly[1], poly[2],
		triangle.flags() );

	poly.chop( planeEq0_ );
	poly.chop( planeEq1_ );
	poly.chop( planeEq2_ );
	poly.chop( planeEq3_ );
	poly.chop( planeEq4_ );

	const PlaneEq pe( dir_, dir_.dotProduct( origin_ ) );

	int idx = 0;

	// Usually, we want to find the first point where we hit a polygon. If
	// shouldFindCentre_ is true, we want to find the mid-point of the polygon
	// we hit.

	if ( shouldFindCentre_ && !poly.empty() )
	{
		float currMin = pe.distanceTo( poly[ idx ] );
		float currMax = currMin;
		++idx;

		while (idx < poly.size())
		{
			const float dist = pe.distanceTo( poly[ idx ] );
			currMin = min( dist, currMin );
			currMax = max( dist, currMax );

			++idx;
		}

		const float average = (currMin + currMax) / 2.f;
		if ( average < advance_ )
		{
			advance_ = average;
			hitTriangle_ = wt;
		}
	}
	else
	{
		while (idx < poly.size())
		{
			const float dist = pe.distanceTo( poly[ idx ] );

			if ( dist < advance_ )
			{
				advance_ = dist;

				hitTriangle_ = wt;
			}

			++idx;
		}
	}

	return COLLIDE_ALL;
}

BW_END_NAMESPACE
