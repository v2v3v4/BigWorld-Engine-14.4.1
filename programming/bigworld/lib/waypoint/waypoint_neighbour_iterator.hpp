#ifndef WAYPOINT_NEIGHBOUR_ITERATOR
#define WAYPOINT_NEIGHBOUR_ITERATOR

#include "chunk_waypoint_set.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to iterate through the immediate neighbourhood of a
 *	certain waypoint.  
 *	It will also return the neighbour waypoints in other chunks.
 */
class WaypointNeighbourIterator
{
public:
	WaypointNeighbourIterator( ChunkWaypointSetPtr pSet, WaypointIndex waypoint );

	bool ended() const
	{
		const ChunkWaypoint & wp = pSet_->waypoint( waypointIndex_ );
		return currentEdgeIndex_ >= wp.edges_.size();
	}

	ChunkWaypointSetPtr pNeighbourSet() const
		{ return pNeighbourSet_; }

	WaypointIndex neighbourWaypointIndex() const
		{ return neighbourWaypoint_; }


	ChunkWaypoint & neighbourWaypoint() const
		{ return pNeighbourSet_->waypoint( neighbourWaypoint_ ); }

	void advance();

private:
	ChunkWaypointSetPtr pSet_;
	WaypointIndex waypointIndex_;
	EdgeIndex currentEdgeIndex_;

	ChunkWaypointSetPtr pNeighbourSet_;
	WaypointIndex neighbourWaypoint_;

};

BW_END_NAMESPACE

#endif // WAYPOINT_NEIGHBOUR_ITERATOR

