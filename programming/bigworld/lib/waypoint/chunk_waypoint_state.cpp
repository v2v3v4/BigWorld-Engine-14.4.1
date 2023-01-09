#include "pch.hpp"

#include "chunk_waypoint_state.hpp"

#include <sstream>


BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
ChunkWaypointState::ChunkWaypointState() :
	distanceFromParent_( 0.f ),
	crossedPortal_( false )
{
}


/**
 *	Constructor.
 *
 *	@param pDstSet A pointer to the destination ChunkWaypointSet.
 *	@param dstPoint The destination point in the coordinates of the destination
 *		ChunkWaypointSet.
 */
ChunkWaypointState::ChunkWaypointState( ChunkWaypointSetPtr pDstSet,
		const Vector3 & dstPoint ):
	distanceFromParent_( 0.f ),
	crossedPortal_( false )
{
	navLoc_.pSet_ = pDstSet;
	navLoc_.waypoint_ = -1;

	navLoc_.point_ = dstPoint;

	navLoc_.clip();
}


/**
 *	Constructor
 */
ChunkWaypointState::ChunkWaypointState( const NavLoc & navLoc ) :
	navLoc_( navLoc ),
	distanceFromParent_( 0.f ),
	crossedPortal_( false )
{
}


/**
 *	Return a string description of this search state.
 */
BW::string ChunkWaypointState::desc() const
{
	if (!navLoc_.pSet())
	{
		return "no set";
	}

	if (!navLoc_.pSet()->chunk())
	{
		return "no chunk";
	}

	const Vector3 & v = navLoc_.point();
	BW::stringstream ss;
	ss << navLoc_.waypoint() << 
		" (" << v.x << ' ' << v.y << ' ' << v.z << ')' <<
		" in " << navLoc_.pSet()->chunk()->identifier() <<
		( crossedPortal_ ? " (portal)" : "" );
	return ss.str();
}


/**
 *  This method gets the given adjacency, if it can be traversed
 */
bool ChunkWaypointState::getAdjacency( adjacency_iterator index,
	ChunkWaypointState & neigh,
	const ChunkWaypointState & goal,
	ChunkWaypointState::SearchData & searchData ) const
{
	const ChunkWaypoint & cw = navLoc_.pSet()->waypoint( navLoc_.waypoint() );
	const ChunkWaypoint::Edge & cwe = cw.edges_[ index ];

	WaypointIndex waypoint = cwe.neighbouringWaypoint();
	bool waypointAdjToChunk = cwe.adjacentToChunk();

	if (crossedPortal_)
	{
		return false;
	}

	if (waypoint >= 0)
	{
		neigh.navLoc_.pSet_ = navLoc_.pSet();
		neigh.navLoc_.waypoint_ = waypoint;
		neigh.crossedPortal_ = false;
	}
	else if (waypointAdjToChunk)
	{
		const EdgeLabel & edgeLabel = navLoc_.pSet()->edgeLabel ( cwe );
		neigh.navLoc_.pSet_ = edgeLabel.pSet_;
		neigh.navLoc_.waypoint_ = edgeLabel.waypoint_;
		neigh.crossedPortal_ = true;

		if (!neigh.navLoc_.pSet() || !neigh.navLoc_.pSet()->chunk())
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	WaypointSpaceVector3 wsrc = MappedVector3( navLoc_.point(),
		navLoc_.pSet()->chunk(), MappedVector3::WORLD_SPACE );
	WaypointSpaceVector3 wdst = MappedVector3( goal.navLoc_.point(),
		goal.navLoc_.pSet()->chunk(), MappedVector3::WORLD_SPACE );
	Vector2 src( wsrc.x, wsrc.z );
	Vector2 dst( wdst.x, wdst.z );
	Vector2 way;
	Vector2 del = dst - src;
	EdgeIndex vp1 = cwe.vertexIndex_;
	EdgeIndex vp2 = cw.edges_[ (index+1) % cw.edges_.size() ].vertexIndex_;
	const Vector2 & p1 = navLoc_.pSet()->vertexByIndex( vp1 );
	const Vector2 & p2 = navLoc_.pSet()->vertexByIndex( vp2 );

	float cp1 = del.crossProduct( p1 - src );
	float cp2 = del.crossProduct( p2 - src );
	// see if our path goes through this edge
	if (cp1 > 0.f && cp2 < 0.f)
	{

		// calculate the intersection of the line (src->dst) and (p1->p2).
		// Brief description of how this works. cp1 and cp2 are the areas of
		// the parallelograms formed by the intervals of the cross product.
		// We want the ratio that the intersection point divides p1->p2.
		// This is the same as the ratio of the perpendicular heights of p1
		// and p2 to del. This is also the same as the ratio between the
		// area of the parallelograms (divide by len(del)).
		// Trust me, this works.
		way = p1 + (cp1/(cp1-cp2)) * (p2-p1);

		/*
		// yep, use the intersection
		LineEq moveLine( src, dst, true );
		LineEq edgeLine( p1, p2, true );
		way = moveLine.param( moveLine.intersect( edgeLine ) );
		*/
	}
	else
	{
		if ((p1 - dst).length() + (p1 - src).length() <
			(p2 - dst).length() + (p2 - src).length())
		{
			way = p1;
		}
		else
		{
			way = p2;
		}
	}

	if (neigh.navLoc_.waypoint_ == -1)
	{
		del.normalise();
		way += del * 0.2f;
	}

	WaypointSpaceVector3 wway( way.x, cw.maxHeight_, way.y );
	WorldSpaceVector3 wv = MappedVector3( wway, navLoc_.pSet()->chunk() );

	neigh.navLoc_.point_ = wv;

	searchData.setConnection(
			navLoc_.pSet(), navLoc_.waypoint(),
			neigh.navLoc_.pSet(), neigh.navLoc_.waypoint(),
			vp1, vp2 );

	if (waypointAdjToChunk)
	{
		BoundingBox bb = neigh.navLoc_.pSet()->chunk()->boundingBox();
		static const float inABit = 0.01f;
		neigh.navLoc_.point_.x = Math::clamp( bb.minBounds().x + inABit,
			neigh.navLoc_.point_.x, bb.maxBounds().x - inABit );
		neigh.navLoc_.point_.z = Math::clamp( bb.minBounds().z + inABit,
			neigh.navLoc_.point_.z, bb.maxBounds().z - inABit );
	}

	neigh.navLoc_.clip();
	neigh.distanceFromParent_ =
		(neigh.navLoc_.point() - navLoc_.point()).length();

	//DEBUG_MSG( "AStar: Considering adjacency from %d to %d "
	//	"dest (%f,%f,%f) dist %f\n",
	//	navLoc_.waypoint(), neigh.navLoc_.waypoint(),
	//	way.x, cw.maxHeight_, way.y, neigh.distanceFromParent_ );

	return true;
}


/**
 *	This method records the details of the edge that connects two adjacent
 *	waypoints.
 */
void ChunkWaypointStateSearchData::setConnection(
		const ChunkWaypointSetPtr fromSet, WaypointIndex from,
		const ChunkWaypointSetPtr toSet, WaypointIndex to,
		EdgeIndex v1, EdgeIndex v2)
{
	WaypointKey keyFrom = std::make_pair( fromSet.get(), from );
	WaypointKey keyTo = std::make_pair( toSet.get(), to );

	connections_[ std::make_pair( keyFrom, keyTo ) ] = std::make_pair( v1, v2 );
}


/**
 *	This method finds the connecting edge between a pair of waypoints, and
 *	returns the indices of the edge's two vertices.
 */
std::pair< EdgeIndex, EdgeIndex > ChunkWaypointStateSearchData::getConnection(
		const ChunkWaypointSetPtr fromSet, WaypointIndex from,
		const ChunkWaypointSetPtr toSet, WaypointIndex to ) const
{
	WaypointKey keyFrom = std::make_pair( fromSet.get(), from );
	WaypointKey keyTo = std::make_pair( toSet.get(), to );

	Connections::const_iterator connection =
			connections_.find( std::make_pair( keyFrom, keyTo ) );

	if (connection == connections_.end())
	{
		return std::make_pair(
			ChunkWaypoint::Edge::adjacentRangeEnd, ChunkWaypoint::Edge::adjacentRangeEnd );
	}

	return connection->second;
}

BW_END_NAMESPACE


// chunk_waypoint_state.cpp

