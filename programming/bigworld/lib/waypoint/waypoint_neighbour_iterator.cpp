#include "pch.hpp"

#include "waypoint_neighbour_iterator.hpp"

#include "navloc.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
WaypointNeighbourIterator::WaypointNeighbourIterator( 
			ChunkWaypointSetPtr pSet, WaypointIndex waypoint ) :
		pSet_( pSet ), 
		waypointIndex_( waypoint ), 
		currentEdgeIndex_( ChunkWaypoint::Edge::adjacentRangeEnd ),
		neighbourWaypoint_( ChunkWaypoint::Edge::otherChunk )
{
	this->advance();
}


/**
 *	Advance to the next neighbouring waypoint.
 */
void WaypointNeighbourIterator::advance()
{
	pNeighbourSet_ = NULL;
	neighbourWaypoint_ = -1;

	const ChunkWaypoint & wp = pSet_->waypoint( waypointIndex_ );

	while (!pNeighbourSet_ &&
			currentEdgeIndex_ < static_cast< EdgeIndex >( wp.edges_.size() ) )
	{
		++currentEdgeIndex_;

		if (currentEdgeIndex_ >= static_cast< EdgeIndex >( wp.edges_.size() ) )
		{
			break;
		}

		const ChunkWaypoint::Edge & wpe = wp.edges_[ currentEdgeIndex_ ];
		WaypointIndex neighbour = wpe.neighbouringWaypoint();
		bool adjToChunk = wpe.adjacentToChunk();

		if (neighbour != -1)
		{
			pNeighbourSet_ = pSet_;
			neighbourWaypoint_ = neighbour;
		}
		else if (adjToChunk)
		{
			const EdgeLabel & edgeLabel = pSet_->edgeLabel( wpe );

			// There is a connection here
			if ( edgeLabel.waypoint_ != -1 )
			{
				const ChunkBoundary::Portal * pPortal =
					pSet_->connectionPortal( edgeLabel.pSet_ );

				// Check if portal can be crossed
				if ( pPortal != NULL && pPortal->permissive )
				{
					pNeighbourSet_ = edgeLabel.pSet_;
					neighbourWaypoint_ = edgeLabel.waypoint_;
				}
			}
		}
	}
}

BW_END_NAMESPACE

// waypoint_neighbour_iterator.cpp

