#include "pch.hpp"

#include "chunk_waypoint_set_state_path.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Initialise the chunk waypoint set path from a A* search result.
 */
void ChunkWaypointSetStatePath::init( AStar< ChunkWPSetState, void* > & astar )
{
	this->clear();

	typedef BW::vector< const ChunkWPSetState * > StatePtrs;
	StatePtrs forwardPath;

	passedShellBoundary_ = false;

	// get out all the pointers
	const ChunkWPSetState * pSearchState = astar.first();
	while (pSearchState != NULL)
	{
		// See if we crossed between an inside chunk and an outside chunk
		// anywhere along this path.
		passedShellBoundary_ = passedShellBoundary_ || 
			pSearchState->passedShellBoundary();

		forwardPath.push_back( pSearchState );
		pSearchState = astar.next();
	}

	// and store the path in reverse order (for pop_back)

	StatePtrs::reverse_iterator iState = forwardPath.rbegin();
	while (iState != forwardPath.rend())
	{
		reversePath_.push_back( **iState );
		++iState;
	}
}

BW_END_NAMESPACE

// chunk_waypoint_set_state_path.cpp

