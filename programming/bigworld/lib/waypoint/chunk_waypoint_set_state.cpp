#include "pch.hpp"

#include "chunk_waypoint_set_state.hpp"
#include "navigator.hpp"
#include "navloc.hpp"

#include <cfloat>
#include <sstream>


BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
ChunkWPSetState::ChunkWPSetState() :
	pSet_(),
	blockNonPermissive_( false ),
	distanceFromParent_( 0.f ),
	passedActivatedPortal_( false ),
	passedShellBoundary_( false ),
	position_()
{
}

/**
 *	Constructor
 */
ChunkWPSetState::ChunkWPSetState( ChunkWaypointSetPtr pSet ) :
	pSet_( pSet ),
	blockNonPermissive_( false ),
	distanceFromParent_( 0.f ),
	passedActivatedPortal_( false ),
	passedShellBoundary_( false ),
	position_( pSet->chunk()->centre() )
{
}

/**
 *	Constructor
 */
ChunkWPSetState::ChunkWPSetState( const NavLoc & loc ) :
	pSet_( loc.pSet() ),
	blockNonPermissive_( false ),
	distanceFromParent_( 0.f ),
	passedActivatedPortal_( false ),
	passedShellBoundary_( false ),
	position_( loc.point() )
{
}


BW::string ChunkWPSetState::desc() const
{
	BW::stringstream ss;
	ss << '(' << position_.x << ", " <<
		position_.y << ", " <<
		position_.z << ") at " <<
		pSet_->chunk()->identifier();
	return ss.str();
}




/**
 *	This method gets the given adjacency, if it can be traversed.
 */
bool ChunkWPSetState::getAdjacency( ChunkWaypointConns::const_iterator iter,
		ChunkWPSetState & neigh, const ChunkWPSetState & goal,
		void* & /*searchData*/ ) const
{
	ChunkWaypointSetPtr pDestWaypointSet = iter->first;
	ChunkBoundary::Portal * pPortal = iter->second;

	Chunk * pFromChunk = pSet_->chunk();
	Chunk * pToChunk = pPortal->pChunk;

	/*
	DEBUG_MSG( "ChunkWPSetState::getAdjacency: "
		"considering connection from chunk %s to chunk %s\n",
		pFromChunk->identifier().c_str(),
		pToChunk->identifier().c_str() );
	*/


	// if blocking non permissive, and this non-permissive, then don't
	// consider.
	if (pPortal != NULL && !pPortal->permissive)
	{
		if (blockNonPermissive_)
		{
			return false;
		}
	}

	// Adjacent state inherits our value for blockNonPermissive_.
	neigh.blockNonPermissive_ = blockNonPermissive_;

	// find portal corresponding portal in cwc the other way.
	ChunkBoundary::Portal * pBackPortal = NULL;
	Chunk::piterator iPortal = pToChunk->pbegin();
	for (; iPortal != pToChunk->pend(); iPortal++)
	{
		if (iPortal->pChunk == pFromChunk)
		{
			ChunkBoundary::Portal & portal = *iPortal;
			pBackPortal = &portal;
			break;
		}
	}

	if (pBackPortal == NULL)
	{
		// TODO: Fix and change to error.
		WARNING_MSG( "ChunkWPSetState::getAdjacency: "
			"Encountered one way portal connection, "
			"assuming non passable.\n" );
		return false;
	}

	// Check that there is a waypoint to get back to set
	bool foundLinkback = false;

	for (adjacency_iterator iDestConnection 
			= pDestWaypointSet->connectionsBegin();
		iDestConnection != pDestWaypointSet->connectionsEnd();
		++iDestConnection)
	{
		if (iDestConnection->first == pSet_)
		{
			foundLinkback = true;
			break;
		}
	}

	if (!foundLinkback)
	{
		return false;
	}

	// now check if portal activated or not.
	neigh.passedShellBoundary(
		pFromChunk != pToChunk &&
		( !pFromChunk->isOutsideChunk() ||
			!pToChunk->isOutsideChunk() ) );

	if (neigh.passedShellBoundary())
	{
		neigh.passedActivatedPortal(
			pPortal->hasChunkItem() || pBackPortal->hasChunkItem() );
	}
	else
	{
		neigh.passedActivatedPortal( false );
	}

	neigh.pSet_ = pDestWaypointSet;

	if (!neigh.pSet_->chunk())
	{
		// TODO: Fix this properly. Nav system needs to be able to better deal
		// with chunks going away.
		WARNING_MSG( "ChunkWPSetState::getAdjacency: "
			"Chunk associated with neighbouring waypoint set "
			"no longer exists.\n" );
		return false;
	}

	// If we are in the same set as the goal then use it as the position
	if ( goal.pSet() == neigh.pSet_ )
	{
		neigh.position_ = goal.position_;
	}
	// If we are not in the goal set, then loop through all the edges labels
	// (one per waypoint connection), and find the closest edge label to the
	// entry position and use this as the position of the state.
	else
	{
		float minDist = FLT_MAX;
		const Vector3 *closestEdge = NULL;
		for (ChunkWaypointEdgeLabels::const_iterator destEdgeLabel 
				= pDestWaypointSet->edgeLabelsBegin();
			destEdgeLabel != pDestWaypointSet->edgeLabelsEnd();
			++destEdgeLabel)
		{
			float dist = ( destEdgeLabel->second.entryPoint_ - position_ )
									.length();

			if ( dist < minDist )
			{
				minDist = dist;
				closestEdge = &destEdgeLabel->second.entryPoint_;
			}
		}

		if (closestEdge)
		{
			neigh.position_ = *closestEdge;
		}
		else
		{
			return false;
		}	
	}

	neigh.distanceFromParent_ = this->distanceToGoal( neigh );

	return true;
}

BW_END_NAMESPACE

// chunk_waypoint_set_state.cpp

