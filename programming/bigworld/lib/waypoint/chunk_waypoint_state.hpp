#ifndef CHUNK_WAYPOINT_STATE_HPP
#define CHUNK_WAYPOINT_STATE_HPP

#include "chunk_waypoint_set.hpp"
#include "navloc.hpp"

#include "math/vector3.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

// Forward declarations
class ChunkWaypointStateSearchData;

/**
 *	This class is a state in an A-Star search of some waypoint graph.
 */
class ChunkWaypointState
{
public:
	typedef EdgeIndex adjacency_iterator;
	typedef ChunkWaypointStateSearchData SearchData;

	ChunkWaypointState();

	ChunkWaypointState( ChunkWaypointSetPtr dstSet, 
		const Vector3 & dstPoint );

	explicit ChunkWaypointState( const NavLoc & navLoc );

	intptr	compare( const ChunkWaypointState & other ) const
	{ 
		return (navLoc_.pSet() == other.navLoc_.pSet()) ?
			navLoc_.waypoint() - other.navLoc_.waypoint() :
			intptr( navLoc_.pSet().get() ) - 
				intptr( other.navLoc_.pSet().get() ); 
	}

	BW::string desc() const;

	uintptr hash() const
	{
		return uintptr( navLoc_.pSet().get() ) + navLoc_.waypoint();
	}

	bool isGoal( const ChunkWaypointState & goal ) const
	{ 
		if ( goal.navLoc_.waypoint() == -1)
		{
			return goal.navLoc_.pSet() == navLoc_.pSet();
		}

		return navLoc_.waypointsEqual( goal.navLoc_ );
	}

	adjacency_iterator adjacenciesBegin() const
		{ return 0; }

	adjacency_iterator adjacenciesEnd() const
	{
		if( navLoc_.waypoint() >= 0 )
		{
			const ChunkWaypoint& wayPoint = 
				navLoc_.pSet()->waypoint( navLoc_.waypoint() );

			return static_cast<adjacency_iterator>( wayPoint.edges_.size() );
		}

		return 0;
	}

	bool getAdjacency( adjacency_iterator iter, ChunkWaypointState & neigh,
		const ChunkWaypointState & goal,
		SearchData & searchData ) const;

	float distanceFromParent() const
		{ return distanceFromParent_; }

	float distanceToGoal( const ChunkWaypointState & goal ) const
		{ return (navLoc_.point() - goal.navLoc_.point()).length(); }

	const NavLoc & navLoc() const
		{ return navLoc_; }

private:
	NavLoc	navLoc_;
	float	distanceFromParent_;
	bool	crossedPortal_;
};


/**
 *	This class contains additional data to be gathered from the AStar search.
 */
class ChunkWaypointStateSearchData
{

public:
	std::pair< EdgeIndex, EdgeIndex > getConnection(
			const ChunkWaypointSetPtr fromSet, WaypointIndex from,
			const ChunkWaypointSetPtr toSet, WaypointIndex to ) const;
	void setConnection(
			const ChunkWaypointSetPtr fromSet, WaypointIndex from,
			const ChunkWaypointSetPtr toSet, WaypointIndex to,
			EdgeIndex vp1, EdgeIndex vp2 );

private:

	// Maps two waypoints and their waypoint sets to the indices of the
	// vertices of the edge that connects them.
	typedef std::pair< ChunkWaypointSet *, WaypointIndex > WaypointKey;
	typedef BW::map<
				std::pair< WaypointKey, WaypointKey >,
				std::pair< EdgeIndex, EdgeIndex > >
			Connections;

	Connections connections_;
};

BW_END_NAMESPACE

#endif // CHUNK_WAYPOINT_STATE_HPP

