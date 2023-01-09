#include "pch.hpp"

#include "chunk_waypoint.hpp"
#include "chunk_waypoint_vertex_provider.hpp"
#include "waypoint_stats.hpp"

#include "chunk/chunk.hpp"
#include "math/vector3.hpp"

#include <cfloat>


BW_BEGIN_NAMESPACE

namespace
{

/**
 *	Returns a point on the line segment that is closest to the given point
 */
void closestPointOnLineSegment( const Vector2 & start, const Vector2 & end,
	Vector2 & point )
{
	Vector2 dir = end - start;
	Vector2 project = point - start;
	float length = dir.length();

	dir.normalise();

	float dot = project.dotProduct( dir );

	if (0.f <= dot && dot <= length)
	{
		point = start + dir * dot;
	}
	else
	{
		point = (dot < 0.f) ? start : end;
	}
}

} // end (anonymous) namespace

const EdgeIndex ChunkWaypoint::Edge::adjacentRangeStart = 32768;
const EdgeIndex ChunkWaypoint::Edge::adjacentRangeEnd = 65535;
const WaypointIndex ChunkWaypoint::Edge::otherChunk = -1;

BW::vector<ChunkWaypoint*> ChunkWaypoint::s_visitedWaypoints_;
const float ChunkWaypoint::HEIGHT_RANGE_TOLERANCE = 0.1f;

/**
 *	Constructor.
 */
ChunkWaypoint::ChunkWaypoint():
		minHeight_( 0.f ),
		maxHeight_( 0.f ),
		centre_( 0.f, 0.f ),
		edges_(),
		edgeCount_( 0 ),
		visited_( 0 )
{
	WaypointStats::addWaypoint();
}


/**
 *	Copy constructor.
 */
ChunkWaypoint::ChunkWaypoint( const ChunkWaypoint & other ):
		minHeight_( other.minHeight_ ),
		maxHeight_( other.maxHeight_ ),
		centre_( other.centre_ ),
		edges_( other.edges_ ),
		edgeCount_( other.edgeCount_ ),
		visited_( other.visited_ )
{
	WaypointStats::addWaypoint();
}


/**
 *	Destructor.
 */
ChunkWaypoint::~ChunkWaypoint()
{
	WaypointStats::removeWaypoint();
}


/**
 *	This method indicates whether or not the waypoint contains the input
 *	point.
 *
 *	@param provider		The vertex provider to pass through for projection
 *						testing.
 *	@param point		The point to test.
 *
 *	@return				True if the point is contained inside the waypoint,
 *						false otherwise.
 */
bool ChunkWaypoint::contains( const ChunkWaypointVertexProvider & provider,
		const WaypointSpaceVector3 & point ) const
{
	if (point.y < minHeight_ - 0.1f)
	{
		return false;
	}

	if (point.y > maxHeight_ + 0.1f)
	{
		return false;
	}

	return this->containsProjection( provider, point );
}


/**
 *	This method indicates whether the input point lies within the waypoint
 *	polygon in x-z plane.
 *
 *	@param provider		The vertex provider to pass through for projection
 *						testing.
 *	@param point		The point to test.
 *
 *	@return				True if the point lies inside the waypoint polygon in
 *						the x-z plane, false otherwise.
 */
bool ChunkWaypoint::containsProjection( 
		const ChunkWaypointVertexProvider & provider, 
		const WaypointSpaceVector3 & point ) const
{
	float u, v, xd, zd, c;
	bool inside = true;
	Edges::const_iterator iEdge = edges_.begin();
	Edges::const_iterator end = edges_.end();

	const Vector2 * pLastPoint = &(provider.vertexByIndex( 
		edges_.back().vertexIndex_ ));

	while (iEdge != end)
	{
		const Vector2 & thisPoint = provider.vertexByIndex( 
			iEdge->vertexIndex_ );

		u = thisPoint.x - pLastPoint->x;
		v = thisPoint.y - pLastPoint->y;

		xd = point.x - pLastPoint->x;
		zd = point.z - pLastPoint->y;

		c = xd * v - zd * u;

		// since lpoint now clips to the edge add a
		// fudge factor to accommodate floating point error.
		inside &= (c > -0.01f);

		if (!inside)
		{
			return false;
		}

		pLastPoint = &thisPoint;
		++iEdge;
	}

	return inside;
}


/**
 *	This method returns the squared distance from waypoint to input point.
 *
 *	@param provider		The vertex provider for the waypoint.
 *	@param pChunk		The chunk to search in.
 *	@param point		The point to test.
 *
 *	@return				The squared distance from lpoint to the waypoint
 *						polygon.
 */
float ChunkWaypoint::distanceSquared( 
		const ChunkWaypointVertexProvider & provider, const Chunk * pChunk,
		const WorldSpaceVector3 & point ) const
{
	// TODO: This is a fairly inefficient.  We may want to look at
	// optimising it.
	WorldSpaceVector3 clipPoint( point );
	this->clip( provider, pChunk, clipPoint );

	// Clamp the y within the min and max height, instead of just max height
	clipPoint.y = Math::clamp(minHeight_, point.y, maxHeight_);

	return (point - clipPoint).lengthSquared();
}


/**
 *	This method clips the lpoint to the edge of the waypoint.
 *
 *	@param provider		The vertex provider for the waypoint.
 *	@param pChunk		The chunk the waypoint resides.
 *	@param wpoint		Initially this is the point to clip.
 *						The point is modified as follows:
 *						For each edge that the point is outside of,
 *						project the point onto the edge.  There are 3 cases:
 *						1) If the point is before the start of the edge,
 *							then the clip point is the start since it wasn't
 *							a projection point on the previous edge.  First
 *							edge is special cased.
 *						2) If the point is between the start and end of the
 *							edge then this is the clip point.
 *						3) If the projection point is after the end of the
 *							edge, then the clip point may be the end point
 *							or on the next edge.  Leave it for the next edge
 *							to decide.
 */
void ChunkWaypoint::clip( const ChunkWaypointVertexProvider & provider,
		const Chunk * pChunk, WorldSpaceVector3 & wpoint ) const
{
	WaypointSpaceVector3 point = MappedVector3( wpoint, pChunk );

	Edges::const_iterator eit = edges_.begin();
	Edges::const_iterator end = edges_.end();

	const Vector2 * pPrevPoint = &(provider.vertexByIndex( 
		edges_.back().vertexIndex_ ));

	while (eit != end)
	{
		const Vector2 & thisPoint = provider.vertexByIndex(
			eit->vertexIndex_ );

		Vector2 edgeVec( thisPoint - *pPrevPoint );
		Vector2 pointVec( point.x - pPrevPoint->x,
			point.z - pPrevPoint->y );

		bool isOutsidePoly = edgeVec.crossProduct( pointVec ) > 0.f;

		if (isOutsidePoly)
			break;

		pPrevPoint = &thisPoint;
		++eit;
	}

	if (eit != end)
	{
		Vector2 p2d( point.x, point.z );
		Vector2 p2dBest( p2d );
		float bestDistSquared = FLT_MAX;
		Vector2 prev = provider.vertexByIndex(
			( edges_.end() - 1 )->vertexIndex_ );

		for (eit = edges_.begin(); eit != end; ++eit)
		{
			const Vector2& now = provider.vertexByIndex( eit->vertexIndex_ );
			Vector2 temp( p2d );

			closestPointOnLineSegment( prev, now, temp );

			if (( temp - p2d ).lengthSquared() < bestDistSquared)
			{
				bestDistSquared = ( temp - p2d ).lengthSquared();
				p2dBest = temp;
			}

			prev = now;
		}

		point.x = p2dBest.x;
		point.z = p2dBest.y;
	}

	const BoundingBox & bb = pChunk->boundingBox();
	float avgHeight = ( minHeight_ + maxHeight_ ) / 2.f;
	wpoint = MappedVector3( point, pChunk );

	wpoint.y = bb.centre().y;

	if (!bb.intersects( wpoint ))
	{
		for (eit = edges_.begin(); eit != end; ++eit)
		{
			const Edge & edge = *eit;

			if (!edge.adjacentToChunk())
			{
				const Edge & next = ( eit + 1 == end ) ?
					edges_.front() : *( eit + 1 );

				const Vector2 & thisStart = provider.vertexByIndex(
					edge.vertexIndex_ );
				const Vector2 & nextStart = provider.vertexByIndex(
					next.vertexIndex_ );

				WaypointSpaceVector3 start( thisStart.x, avgHeight, 
					thisStart.y );
				WaypointSpaceVector3 end( nextStart.x, avgHeight, 
					nextStart.y );

				WorldSpaceVector3 wvStart = MappedVector3( start, pChunk );
				WorldSpaceVector3 wvEnd = MappedVector3( end, pChunk );

				bb.clip( wvStart, wvEnd );

				WorldSpaceVector3 middle( ( wvStart + wvEnd ) / 2 );

				bb.clip( middle, wpoint );
				// move in a bit
				wpoint = WorldSpaceVector3( middle + 
					( wpoint - middle ) * 0.99f );

				break;
			}
		}
	}

	this->makeMaxHeight( pChunk, wpoint );
}


/**
 *	This method makes the world space point the same height as maxHeight_
 *	of this waypoint in waypoint space
 *
 *	@param chunk		The chunk the waypoint resides. 
 *	@param wpoint		This point will be transformed into waypoint space
 *						to do assignment and then transformed back to world
 *						space.
 */
void ChunkWaypoint::makeMaxHeight( const Chunk* chunk, 
		WorldSpaceVector3 & wpoint ) const
{
	WaypointSpaceVector3 point = MappedVector3( wpoint, chunk );
	point.y = maxHeight_;
	wpoint = MappedVector3( point, chunk );
}


/**
 *	This method prints some debugging information about this waypoint.
 *
 *	@param provider The provider to resolve the waypoint vertices.
 */
void ChunkWaypoint::print( const ChunkWaypointVertexProvider & provider ) const
{
	DEBUG_MSG( "MinHeight: %g\tMaxHeight: %g\tEdgeNum:%" PRIzu "\n",
		minHeight_, maxHeight_, edges_.size() );

	for (Edges::size_type i = 0; i < edges_.size(); ++i)
	{
		const Vector2 & start = provider.vertexByIndex(
			edges_[i].vertexIndex_ );
		DEBUG_MSG( "\t%" PRIzu " (%g, %g) %d - %s\n", i,
			start.x, start.y,
			edges_[ i ].neighbouringWaypoint(),
			edges_[ i ].adjacentToChunk() ? "chunk" : "no chunk" );
	}
}


/**
 *	This function calculates the centre of the navmesh
 */
void ChunkWaypoint::calcCentre( const ChunkWaypointVertexProvider & provider )
{
	float totalLength = 0.f;

	centre_ = Vector2( 0, 0 );

	for (Edges::size_type i = 0; i < edgeCount_; ++i)
	{
		const Vector2 & start = provider.vertexByIndex( 
			edges_[ i ].vertexIndex_ );
		const Vector2 & end = provider.vertexByIndex(
			edges_[ ( i + 1 ) % edgeCount_ ].vertexIndex_ );

		float length = ( end - start ).length();

		centre_ += ( end + start ) / 2 * length;
		totalLength += length;
	}

	centre_ /= totalLength;
}

BW_END_NAMESPACE

// chunk_waypoint.cpp

