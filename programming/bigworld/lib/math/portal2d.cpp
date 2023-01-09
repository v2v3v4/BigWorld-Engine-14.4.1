#include "pch.hpp"

#include "cstdmf/debug.hpp"
#include "mathdef.hpp"
#include "portal2d.hpp"
#include <string.h>

DECLARE_DEBUG_COMPONENT2( "Math", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "portal2d.ipp"
#endif

/**
 *	Constructor
 */
Portal2D::Portal2D() :
	refs_( 0 )
{
}

/**
 *	Copy constructor
 */
Portal2D::Portal2D( const Portal2D & other ) :
	points_( other.points_ ),
	refs_( 0 )
{
}

/**
 *	Assignment operator
 */
Portal2D & Portal2D::operator =( const Portal2D & other )
{
	points_ = other.points_;
	return *this;
}

/**
 *	Destructor
 */
Portal2D::~Portal2D()
{
}

/**
 *	Make a default (clip-space) portal
 */
void Portal2D::setDefaults()
{
	this->erasePoints();

	Vector2 v( -1, 1 );
	points_.push_back( v );
	v[0] = 1;
	points_.push_back( v );
	v[1] = -1;
	points_.push_back( v );
	v[0] = -1;
	points_.push_back( v );
}


typedef VectorNoDestructor<char>	InsideVec;
static InsideVec s_insideVec;

const float PORTAL_EPSILON = 1.e-10F;

/**
 *	This method combines the two input portals (which must have >=3 pts each)
 *	into a single portal, by finding their intersection.
 */
bool Portal2D::combine( Portal2D *pt1, Portal2D *pt2 )
{
	MF_ASSERT( pt1 );
	MF_ASSERT( pt2 );

	InsideVec & inside = s_insideVec;

	//points_ = pt2->points_;
	points_.resize( pt2->points_.size() );
	memcpy( &points_.front(), &pt2->points_.front(),
		sizeof(Vector2) * pt2->points_.size() );

	pt1->points_.push_back( pt1->points_.front() );

	for( uint32 i = 0; i < ( pt1->points_.size() - 1 ); i++ )
	{
		inside.clear();
		Vector2 n2 = pt1->points_[ i + 1 ] - pt1->points_[ i ];

		float ls = n2.lengthSquared();

		// Don't try to clip against really small edges
		if (ls > -PORTAL_EPSILON && ls < PORTAL_EPSILON)
		{
			continue;
		}

		Vector2 n( n2[1], -n2[0] );
		float d = n.dotProduct( pt1->points_[ i ] );

		uint32 j ;
		for( j = 0; j < points_.size(); j++ )
		{
        	char isInside = ( n.dotProduct( points_[j] ) >= d ) ? 1 : 0;
			inside.push_back( isInside );
		}
		points_.push_back( points_.front() );

        int firstInside = inside[ 0 ];
		inside.push_back( firstInside );

        // create new point
		for( j = 0; j < ( points_.size() - 1 ); j++ )
		{
        	int inside1 = inside[j];
            int inside2 = inside[ j + 1 ];

			if( inside1 != inside2 )
			{
				Vector2 v = points_[ j ];
				Vector2 dv = points_[ j + 1 ] - v;

				float t = ( d - n.dotProduct( v ) ) / n.dotProduct( dv );
				t = Math::clamp( 0.f, t, 1.f );
				dv *= t;
				v += dv;

				j++;
				//points_.insert( points_.begin() + j, v );
				points_.push_back( points_.back() );
				for (size_t k = points_.size()-1; k > j; k--)
					points_[k] = points_[k-1];
				points_[j] = v;
				//inside.insert( inside.begin() + j, 1 );
				inside.push_back( inside.back() );
				for (size_t k = inside.size()-1; k > j; k--)
					inside[k] = inside[k-1];
				inside[j] = 1;
			}
		}

		points_.pop_back();
		inside.pop_back();

		j = 0;
		for (InsideVec::iterator bit = inside.begin(); bit != inside.end(); bit++)
		{
			if( *bit == 0 )
				points_.erase( points_.begin() + j );
			else
				j++;
		}
	}

	pt1->points_.pop_back();

    return points_.size() >= 3;
}

#if 0
/**
 *	Under construction
 */
bool Portal2D::combine( const Portal2D & inpor1, const Portal2D & inpor2 )
{
	points_.clear();

	// first figure out if we have to go around por2 instead of por1
	bool por2Compelling = inpor1.contains( inpor2.points_[0] );
	const Portal2D & porA = por2Compelling ? inpor2 : inpor1;
	const Portal2D & porB = por2Compelling ? inpor1 : inpor2;

	// now we go around porA
	Vector2 ptA = porA.points_.back(), ptB;
	bool aInside = porB.contains( ptA ), bInside;
	if (aInside) points_.push_back( ptA );
	for (uint i = 0; i < porA.points_.size(); i++)
	{
		ptB = porA.points_[i];
		bInside = porB.contains( ptB );

		if (!(aInside | bInside))
		{
			// one of the points isn't inside
			// so see where the line segment intersects porB
			// if aInside ^ bInside, then there will be only one intersection,
			// otherwise there will be two or zero.
			Vector2 meets[4];
			uint meetsLen = 0;

			Vector2 seg = ptB - ptA;
			Vector2 nor( seg.y, -seg.x );
			float dis = nor.dotProduct( ptA );

			Vector2 ptC = porB.points_.back(), ptD;
			for (uint j = 0; j < porB.points_.size(); j++, ptC = ptD)
			{
				ptD = porB.points_[j];

				// make sure A => B crosses the line through C & D
				Vector2 seg2 = ptD - ptC;
				Vector2 nor2( seg2.y, -seg2.x );
				float dis2 = nor2.dotProduct( ptC );
				if ((nor2.dotProduct( ptA ) > dis2) ==
					(nor2.dotProduct( ptB ) > dis2)) continue;

				// find param of segment from ptC to ptD
				float f = nor.dotProduct( seg2 );
				if (f == 0.f) continue;				// make sure not parallel

				float t = (dis - nor.dotProduct( ptC )) / f;
				if (t >= 0.f && t < 1.f) continue;	// make sure on segment

				// add intersection point then
				Vector2 meet = ptC + seg2 * t;
				meets[meetsLen++] = meet;
			}

			if (meetsLen > 2)
			{
				dprintf( "Line segment (%f,%f) to (%f,%f) has %d meets\n",
					ptA.x, ptA.y, ptB.x, ptB.y, meetsLen );
				dprintf( "aInside %d bInside %d\n", aInside?1:0, bInside?1:0 );
				for (uint j = 0; j < porB.points_.size(); j++)
				{
					dprintf( "point %d: (%f,%f)\n", j,
						porB.points_[j].x, porB.points_[j].y );
				}
				for (uint j = 0; j < meetsLen; j++)
				{
					dprintf( "meets[%d] = (%f,%f)\n", j,
						meets[j].x, meets[j].y );
				}
			}
			MF_ASSERT( meetsLen <= 2);
			if (meetsLen == 1)
			{
				points_.push_back( meets[0] );
			}
			else if (meetsLen == 2)
			{
				// if there are two add them in the right order
				if (seg.dotProduct( meets[0] ) < seg.dotProduct( meets[1] ))
				{
					points_.push_back( meets[0] );
					points_.push_back( meets[1] );
				}
				else
				{
					points_.push_back( meets[1] );
					points_.push_back( meets[0] );
				}
			}
		}

		if (bInside) points_.push_back( ptB );
		ptA = ptB;
		aInside = bInside;
	}

    return points_.size() >= 3;
}
#endif // 0


/**
 *	This method returns whether or not the given pt is inside the portal
 */
bool Portal2D::contains( const Vector2 & v ) const
{
	Vector2 ptA = points_.back(), ptB;
	for (uint i = 0; i < points_.size(); i++)
	{
		ptB = points_[i];
		Vector2 seg = ptB - ptA;
		Vector2 nor( seg.y, -seg.x );
		if (nor.dotProduct( v ) < nor.dotProduct( ptA )) return false;

		ptA = ptB;
	}

	return true;
}

BW_END_NAMESPACE

// portal2d.cpp
