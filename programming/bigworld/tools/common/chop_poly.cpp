#include "pch.hpp"
#include "chop_poly.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------
/**
 *	@internal
 *	This is a simple helper function used by splitPoly. It returns whether or
 *	not two points are too close.
 */
inline static bool tooClose( const Vector3 & p1, const Vector3 & p2 )
{
	const float MIN_DIST = 0.001f; // A millimetre
	const float MIN_DIST2 = MIN_DIST * MIN_DIST;

	return (p1 - p2).lengthSquared() <= MIN_DIST2;
}


/**
 *	@internal
 *	This is a simple helper function used by splitPoly. It adds a point to the
 *	polygon if it is not too close to the previous point.
 */
inline static void addToPoly( ChopPolygon & poly, const Vector3 & point )
{
	if (poly.empty() || !tooClose( poly.back(), point ))
	{
		poly.push_back( point );
	}
}


/**
 *	@internal
 *	This is a simple helper function used by splitPoly. It removes some
 *	unnecessary points.
 */
inline static void compressPoly( ChopPolygon & poly )
{
	if (poly.size() < 3)
	{
		poly.clear();
		return;
	}

	if (tooClose( poly.front(), poly.back() ))
	{
		poly.pop_back();
	}

	if (poly.size() < 3)
	{
		poly.clear();
	}
}

inline void ChopPolygon::split( const PlaneEq & planeEq, ChopPolygon & frontPoly,
							   float* dists, BOOL* sides ) const
{
	if (this->size() < 3)
	{
		return;
	}

	const Vector3 * pPrevPoint = &this->back();
	float prevDist = dists[ size() - 1 ];
	BOOL wasInFront = sides[ size() - 1 ];

	for (int idx = 0; idx < this->size(); ++idx )
	{
		float currDist = dists[ idx ];
		BOOL isInFront = sides[ idx ];

		if (isInFront != wasInFront)
		{
			float ratio = prevDist / ( prevDist - currDist );

			Vector3 cutPoint =
				*pPrevPoint + ratio * ( v_[ idx ] - *pPrevPoint );

			addToPoly( frontPoly, cutPoint );
		}

		if (isInFront)
		{
			if (wasInFront)
				frontPoly.push_back( v_[ idx ] );
			else
				addToPoly( frontPoly, v_[ idx ] );
		}

		wasInFront = isInFront;
		pPrevPoint = v_ + idx;
		prevDist = currDist;
	}

	compressPoly( frontPoly );
}


/**
 *	This method chops this polygon by the input plane. Only the part of the
 *	polygon in the half-space in front of the plane equation is kept.
 */
void ChopPolygon::chop( const PlaneEq & planeEq )
{
	float dists[ MAX_WORLD_POLYGON_VECTOR_SIZE ];
	BOOL sides[ MAX_WORLD_POLYGON_VECTOR_SIZE ];
	BOOL allFront = TRUE;

	for (int idx = 0; idx < size(); ++idx)
	{
		dists[ idx ] = planeEq.distanceTo( v_[ idx ] );
		sides[ idx ] = dists[ idx ] > 0;

		allFront = allFront && sides[idx];
	}

	if (allFront)
	{
		return;
	}

	ChopPolygon newPoly;

	this->split( planeEq, newPoly, dists, sides );

	*this = newPoly;
}

BW_END_NAMESPACE

// chop_poly.cpp
