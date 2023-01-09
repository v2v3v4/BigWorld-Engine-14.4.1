/**
 *	This file contains the implementation of the AStar class.
 */

/**
 *	This is the constructor.
 */
template<class State, class SearchData>
AStar<State, SearchData>::AStar( StateScopedFreeList * freeList ) :
	freeList_( freeList ),
	iter_( NULL ),
	start_( NULL ),
	close_( freeList ),
	searchData_()
{}

/**
 *	This is the destructor.
 */
template<class State, class SearchData>
AStar<State, SearchData>::~AStar()
{
	close_.reset();
	freeList_->free( start_ );
}


/**
 *	This method performs an A* search.
 *
 *	@param start	This is the initial state of the search.
 *	@param goal		This is the goal state
 *	@param maxDistance		This is max distance to search
 *							(-1. means no limit)
 *
 *	@return	True if successful.
 */
template<class State, class SearchData>
bool AStar<State, SearchData>::search(const State& start, const State& goal,
	float maxDistance )
{
	IntState* pCurrent;
	IntState* pAdjacency;
	IntState key;
	float g, f;
	bool checkMaxDist = maxDistance > 0.f;

	// Ensure that recalling this doesn't recycle old data
	close_.reset();
	freeList_->free( start_ );

	// Create an initial start node, and push it onto the open queue.

	pCurrent = freeList_->alloc();
	pCurrent->ext = start;
	pCurrent->g = 0;
	pCurrent->h = start.distanceToGoal(goal);
	pCurrent->f = pCurrent->g + pCurrent->h;
	pCurrent->pParent = NULL;
	pCurrent->pChild = NULL;
	start_ = pCurrent;

	open_.insert(pCurrent);

	while(!open_.empty())
	{
		// Grab the top element from the queue, the one with the smallest
		// total cost.
		typename AStarQueue::iterator openIter = open_.begin();
		typename AStarQueue::iterator currIter = openIter;

		pCurrent = *openIter;
		++openIter;

		while (openIter != open_.end())
		{
			if (cmp_f()( *openIter, pCurrent ))
			{
				pCurrent = *openIter;
				currIter = openIter;
			}

			++openIter;
		}

		open_.erase( currIter );

		//DEBUG_MSG( "pop pCurrent->f %f open size %d\n", pCurrent->f, open_.size() );

		// If it satisfies the goal requirements, we are done. Walk
		// backwards and construct the path.

		if(pCurrent->ext.isGoal(goal))
		{
			while(pCurrent->pParent)
			{
				pCurrent->pParent->pChild = pCurrent;
				pCurrent = pCurrent->pParent;
			}

			return true;
		}

		// Now check all the adjacencies.

		typename State::adjacency_iterator adjIter =
			pCurrent->ext.adjacenciesBegin();

		for(; adjIter != pCurrent->ext.adjacenciesEnd(); ++adjIter)
		{
			// To make things easier for the State implementation,
			// not every adacency need be pursued. If getAdjacencyState
			// returns false, this is not a real adjacency, so ignore it.

			if(!pCurrent->ext.getAdjacency( adjIter, key.ext, goal,
					searchData_ ))
				continue;

			// Don't return to the start state. This would cause
			// a loop in the path.

			if(key.ext.compare(start_->ext) == 0)
				continue;

			if (checkMaxDist)
			{
				if (key.ext.distanceToGoal( start ) > maxDistance)
				{
					continue;
				}
			}

			g = pCurrent->g + key.ext.distanceFromParent();
			f = g + key.ext.distanceToGoal( goal );

			IntState* setIter = close_.find( &key );

			if( setIter != NULL )
			{
				// If we have already visited this state, it will be in
				// our set. If we previously visited it with a lower cost,
				// then forget about it this time.
				if(setIter->f <= f)
				{
					//DEBUG_MSG( "Found dup old g %g, new g %g, old f %g, new f %g\n",
					//	setIter->g, g, setIter->f, f );
					continue;
				}

				// Otherwise remove it from the set, we'll re-add it below
				// with a smaller cost.

				pAdjacency = setIter;
				close_.erase(&key);

				if (open_.find( setIter ) != open_.end())
				{
					open_.erase( open_.find( setIter ) );
				}
			}
			else
			{
				pAdjacency = freeList_->alloc();
			}

			// Sometimes they enter a loop in this case so we should break it
			IntState* p = pCurrent;

			while (p && p != pAdjacency)
			{
				p = p->pParent;
			}

			if (p)
			{
				continue;
			}

			pAdjacency->ext = key.ext;
			pAdjacency->g = g;

			pAdjacency->h = pAdjacency->ext.distanceToGoal(goal);

			pAdjacency->f = f; //pAdjacency->g + pAdjacency->h;
			pAdjacency->pParent = pCurrent;
			pAdjacency->pChild = NULL;

			// The set contains all states that we have encountered. The
			// priority queue contains all states that we have not yet
			// fully expanded. Add this new state to both.

			// DEBUG_MSG( "Adding new adjacency to open list: %f\n", pAdjacency->f );

			close_.insert(pAdjacency);
			open_.insert(pAdjacency);
		}
	}

	return false;
}

/**
 *	If the search was successful, this method is used to find the first
 *	state in the search result. It will always be the same as the start
 *	state that was passed in.
 *
 *	@return First state in the search.
 */
template<class State, class SearchData>
const State* AStar<State, SearchData>::first()
{
	iter_ = start_;
	return iter_ ? &iter_->ext : NULL;
}

/**
 *	This method should be called repeatedly to find subsequent states in
 *	the search result. It will return NULL if there are no more states.
 *
 *	@return Next state in the search, or NULL if complete.
 */
template<class State, class SearchData>
const State* AStar<State, SearchData>::next()
{
	if(iter_)
		iter_ = iter_->pChild;
	return iter_ ? &iter_->ext : NULL;
}

