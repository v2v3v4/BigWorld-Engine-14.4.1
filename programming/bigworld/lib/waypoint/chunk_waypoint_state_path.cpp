#include "pch.hpp"

#include "chunk_waypoint_state_path.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Initialise from a A* search through waypoints in one waypoint set.
 */
void ChunkWaypointStatePath::init( 
		AStar< ChunkWaypointState, ChunkWaypointState::SearchData > & astar )
{
	reversePath_.clear();

	typedef BW::vector< const ChunkWaypointState * > StatePtrs;
	StatePtrs forwardPath;

	// get out all the pointers
	const ChunkWaypointState * pSearchState = astar.first();
	const ChunkWaypointState * pLast = pSearchState;

	while (pSearchState != NULL)
	{
		forwardPath.push_back( pSearchState );

		pSearchState = astar.next();

		if (pSearchState)
		{
			pLast = pSearchState;
		}
	}

	if (forwardPath.size() < 2)
	{
		// make sure that forwardPath has at least 2 nodes
		forwardPath.push_back( pLast );
	}

	// and store the path in reverse order (for pop_back)
	StatePtrs::reverse_iterator iState = forwardPath.rbegin();

	while (iState != forwardPath.rend())
	{
		reversePath_.push_back( **iState );
		++iState;
	}

	fullPath_.assign(reversePath_.begin(), reversePath_.end());
	stateIterOnFullPath_ = fullPath_.rbegin();

	canBeSmoothed_ = true;
}


/**
 *	This method traverses the path, removing any unnecessary points. These
 *	include duplicates of other points, as well as the middle of any three
 *	consecutive, colinear points.
 */
void ChunkWaypointStatePath::removeDuplicates()
{
	MF_ASSERT( reversePath_.size() >= 2 );
	uint32 pathIndex = 1;

	while (pathIndex != reversePath_.size())
	{
		const Vector3 & current = reversePath_[ pathIndex ].navLoc().point();
		const Vector3 & prev = reversePath_[ pathIndex - 1 ].navLoc().point();

		bool isPointUnnecessary = false;

		// Duplicate point
		if (almostEqual( current, prev ))
		{
			isPointUnnecessary = true;
		}

		// The centre of three colinear points
		if (pathIndex != reversePath_.size() - 1)
		{
			const Vector3 & next =
					reversePath_[ pathIndex + 1 ].navLoc().point();

			Vector3 fromPrev = current - prev;
			Vector3 toNext = next - current;

			fromPrev.normalise();
			toNext.normalise();

			if (almostEqual( fromPrev, toNext ))
			{
				isPointUnnecessary = true;
			}
		}

		if (isPointUnnecessary && reversePath_.size() > 2)
		{
			reversePath_.erase( reversePath_.begin() + pathIndex );
			canBeSmoothed_ = false;
		}
		else
		{
			++pathIndex;
		}
	}
}



/**
 *	This method modifies the points in the waypoint path, by moving them along
 *	the edge on which they currently reside, such that the resulting path is
 *	optimised.
 *
 *	The resulting path will remain on the waypoints in the search result,
 *	but will undergo string-pulling (funneling).
 */
bool ChunkWaypointStatePath::smoothPath(
		AStar< ChunkWaypointState, ChunkWaypointState::SearchData > & astar,
		const ChunkWaypointState & src, const ChunkWaypointState & dst )
{
	if (!canBeSmoothed_)
	{
		ERROR_MSG( "ChunkWaypointStatePath::smoothPath: path cannot be "
				"smoothed. It may not have been initialised, or it may have "
				"already been smoothed.\n" );
		return false;
	}

	Corridor corridorVertices;

	const ChunkWaypointState * pSearchState = astar.first();
	const ChunkWaypointState * pNextState = astar.next();

	// The source and destination points are on the same waypoint, and so
	// smoothing is unnecessary.
	if ((src.navLoc().waypoint() == dst.navLoc().waypoint() &&
			src.navLoc().pSet() == dst.navLoc().pSet() ) ||
			reversePath_.size() == 2)
	{
		return true;
	}

	while (pNextState != NULL)
	{
		// Get the positions of the edge vertices and add them to the corridor
		const ChunkWaypointSetPtr pSet = pSearchState->navLoc().pSet();

		std::pair< EdgeIndex, EdgeIndex > edgeVertices =
			astar.searchData().getConnection(
			pSearchState->navLoc().pSet(), pSearchState->navLoc().waypoint(),
			pNextState->navLoc().pSet(), pNextState->navLoc().waypoint() );

		IF_NOT_MF_ASSERT_DEV(
				edgeVertices.first != ChunkWaypoint::Edge::adjacentRangeEnd &&
				edgeVertices.second != ChunkWaypoint::Edge::adjacentRangeEnd )
		{
			ERROR_MSG( "ChunkWaypointStatePath::smoothPath: could not find "
					"connecting edge between waypoints %d->%d.\n",
					pSearchState->navLoc().waypoint(),
					pNextState->navLoc().waypoint()	);
			return false;
		}

		const Vector2 & p1 = pSet->vertexByIndex( edgeVertices.first );
		const Vector2 & p2 = pSet->vertexByIndex( edgeVertices.second );
		corridorVertices.push_back( std::make_pair( p1, p2 ) );

		pSearchState = pNextState;
		pNextState = astar.next();
	}

	IF_NOT_MF_ASSERT_DEV( !corridorVertices.empty() )
	{
		ERROR_MSG( "ChunkWaypointStatePath::smoothPath: Path not found.\n" );
		return false;
	}

	BW::vector< std::pair< uint32, Vector2 > > pathPoints;
	Vector2 srcPoint( src.navLoc().point().x, src.navLoc().point().z );

	// If the search is not over a chunk boundary, we can use the given
	// destination point
	bool crossesWaypointSet =
			(pSearchState->navLoc().pSet() != astar.first()->navLoc().pSet());
	const NavLoc & dstNavLoc =
			crossesWaypointSet ? dst.navLoc() : pSearchState->navLoc();

	Vector2 dstPoint( dstNavLoc.point().x, dstNavLoc.point().z );

	if (!this->funnel( corridorVertices, srcPoint, dstPoint, pathPoints ))
	{
		return false;
	}

	Path newPath( pathPoints.size() + 2 );

	// Add the first element of the original path, the source location of the
	// search.
	newPath.back() = reversePath_.back();
	float maxY = reversePath_.back().navLoc().point().y;


	// Generate the new path from pathPoints
	uint32 newPathIndex = static_cast<uint32>(newPath.size() - 2);
	uint32 prevReverseIndex = static_cast<uint32>(reversePath_.size() - 2);

	for (uint32 pathIndex = 0; pathIndex != pathPoints.size(); ++pathIndex)
	{
		// We subtract 2 from these indices: 1 as we are subtracting from the
		// size, and 1 to account for the additional node (the last node of the
		// path), which is at the beginning of reversePath_ and newPath.
		uint32 reversePathIndex =
			static_cast<uint32>(reversePath_.size()) -
				pathPoints[ pathIndex ].first - 2;

		// Find the maxY of the segment
		while (prevReverseIndex > reversePathIndex)
		{
			maxY = std::max(
					reversePath_[ prevReverseIndex ].navLoc().point().y, maxY
					);
			--prevReverseIndex;
		}

		// Use the navLoc from the old state, but with the new point.
		const NavLoc & navLoc = reversePath_[ reversePathIndex ].navLoc();
		const Vector2 & pathPoint( pathPoints[ pathIndex ].second );
		Vector3 newPoint( pathPoint.x, std::max( maxY, navLoc.point().y ),
						pathPoint.y );
		ChunkWaypointState newState( NavLoc( navLoc, newPoint ) );
		newPath[ newPathIndex ] = newState;

		maxY = navLoc.point().y;
		--newPathIndex;
	}

	// Find maxY of last segment
	while (prevReverseIndex)
	{
		maxY = std::max(
				reversePath_[ prevReverseIndex ].navLoc().point().y, maxY );
		--prevReverseIndex;
	}

	// Add the last element of the original path, so that we can assume the
	// end of the path is in the same waypoint as the destination point.
	const NavLoc & navLocLast = reversePath_.front().navLoc();
	Vector3 pathPointLast( navLocLast.point().x,
			std::max( maxY, navLocLast.point().y ),
			navLocLast.point().z
			);

	// Use the navLoc from the old state, but with the new point.
	ChunkWaypointState newStateLast( NavLoc( navLocLast, pathPointLast ) );
	newPath.front() = newStateLast;

	newPath.swap( reversePath_ );
	canBeSmoothed_ = false;

	return true;
}


/**
 *	This method takes the line that passes through points v1 and v2, and
 *	determines on which side of the line the point p lies. If you were standing
 *	on point v1 and looking towards v2, this method will return LEFT if the
 *	point is to your left, RIGHT if it is to your right, and ON_LINE if it
 *	is on the line.
 */
ChunkWaypointStatePath::RelativePosition
ChunkWaypointStatePath::locationOfPointFromLine( const Vector2 & v1,
		const Vector2 & v2,	const Vector2 & p )
{
	float crossProduct = (v2 - v1).crossProduct( p - v1 );

	if (almostZero( crossProduct ))
	{
		return ON_LINE;
	}

	if (crossProduct > 0.f)
	{
		return LEFT;
	}
	else
	{
		return RIGHT;
	}
}


/**
 *	This method will determine where a point lies in relation to the wedge
 *	formed by the root point and points along the arms, left and right.
 *	The method will return LEFT, RIGHT, or INSIDE.
 *
 *	This method can also return an error code:
 *	 - BAD_FUNNEL, if the arms form a reflex angle. This is caused by the
 *	   root being on the wrong side of the edge formed by left and right.
 */
ChunkWaypointStatePath::RelativePosition
ChunkWaypointStatePath::isPointInsideFunnel( const Vector2 & root,
		const Vector2 & left, const Vector2 & right,
		const Vector2 & query )
{
	if (ChunkWaypointStatePath::locationOfPointFromLine( left, right, root ) ==
			LEFT)
	{
		return BAD_FUNNEL;
	}

	ChunkWaypointStatePath::RelativePosition leftRelative =
		ChunkWaypointStatePath::locationOfPointFromLine( root, left, query );

	if (leftRelative == LEFT)
	{
		return LEFT;
	}

	ChunkWaypointStatePath::RelativePosition rightRelative =
		ChunkWaypointStatePath::locationOfPointFromLine( root, right, query );

	if (rightRelative == RIGHT)
	{
		return RIGHT;
	}

	return INSIDE;
}


/**
 *	This method tests a point against the 'funnel' created by a root point, and
 *	two points that form left and right arms from the root. It then progresses
 *	none or some of the four points along the corridor. The first parameter,
 *	'corridorPoints', is the set of points along the corridor, and the other
 *	four parameters are indices into corridorPoints.
 */
bool ChunkWaypointStatePath::processCorridorPoint(
		const BW::vector< Vector2 > & corridorPoints, uint32 & root,
		uint32 & funnelLeft, uint32 & funnelRight, uint32 & current )
{
	// If one of the funnel arm points is at the same location as the root, we
	// can progress the root to that point. This will allow us to start
	// searching again from further along the corridor.
	if (almostEqual( corridorPoints[ root ],
				corridorPoints[ funnelLeft ]))
	{
		root = funnelLeft;
		return true;
	}

	if (almostEqual( corridorPoints[ root ],
				corridorPoints[ funnelRight ]))
	{
		root = funnelRight;
		return true;
	}

	// If the point is the same as the previous point on the same side
	// of the corridor, skip it.
	if (almostEqual( corridorPoints[ current ],
				corridorPoints[ current - 2 ]) &&
			current != corridorPoints.size()-1 )
	{
		return true;
	}

	RelativePosition relativeToFunnel =
			ChunkWaypointStatePath::isPointInsideFunnel(
				corridorPoints[ root ], corridorPoints[ funnelLeft ],
				corridorPoints[ funnelRight ], corridorPoints[ current ] );

	const RelativePosition side = (current % 2 == 1 ? LEFT : RIGHT );

	switch (relativeToFunnel)
	{
	// The root is on the wrong side of the left-right edge. This can happen
	// when the src position lies on the overlap with another waypoint.
	case BAD_FUNNEL:
		if (root != 0)
		{
			INFO_MSG( "ChunkWaypointStatePath::processCorridorPoint: "
					"Failed to smooth unusual path.\n" );
			return false;
		}
		funnelLeft += 2;
		funnelRight += 2;
		++current;
		break;

	// The funnel has a zero angle, so use the closer of the two ends as the new
	// root.
	case ON_LINE:
		{
			Vector2 leftVector = corridorPoints[ funnelLeft ] -
				corridorPoints[ root ];
			Vector2 rightVector = corridorPoints[ funnelRight ] -
				corridorPoints[ root ];

			// left node is closer
			if (leftVector.lengthSquared() < rightVector.lengthSquared())
			{
				root = funnelLeft;
			}

			// right node is closer
			else
			{
				root = funnelRight;
			}
		}
		break;

	// The point is inside the funnel, so narrow the funnel
	case INSIDE:
		if (side == LEFT)
		{
			funnelLeft = current;
		}
		else
		{
			funnelRight = current;
		}
		break;

	default:
		MF_ASSERT( relativeToFunnel == LEFT || relativeToFunnel == RIGHT );

		// The point is behind the current node's wall of the corridor, so
		// ignore this point, unless it is the destination point.
		if (relativeToFunnel == side)
		{
			if (current == corridorPoints.size() - 1)
			{
				root = (side == LEFT) ? funnelLeft : funnelRight;
			}
		}

		// The point crosses the funnel, thus closing it off. The stored
		// node for the side getting crossed is the new root.
		else
		{
			root = (side == LEFT) ? funnelRight : funnelLeft;
		}
		break;
	}

	return true;
}


/**
 *	This method smooths a path using string-pulling, or the Funnel algorithm.
 *	It performs the smoothing on a set of edges forming a corridor, along with
 *	a source point and destination point. A list of points that define the
 *	smooth, optimal path are stored in pathPoints.
 *
 *	The list of edges must be ordered from source to destination, with the
 *	edge pair containing the vertices in the order <left, right>.
 *
 *	The source and destination points, along with the vertices of each edge
 *	are initially placed in a list (corridorPoints), in the following order:
 *	0:			source point (srcPos)
 *	1,2:		First edge's vertices: the left and right vertices respectively,
 *				of the first edge of the corridor
 *	3,4:		Second edge's vertices
 *	2n-1,2n:	nth edge's vertices
 *	2n+1:		destination point (dstPos)
 *
 *	The search will always be performed on this list. A funnel is made starting
 *	from a root point (initially the srcPos). The left and right arms will
 *	initially be the two vertices of the edge immediately following the edge
 *	containing the root (or the first edge, when root==0). This means that when
 *	root is odd (on the left side of the corridor), the funnel arms will be
 *	at root+2 and root+3, and when root is even (on the right side of the
 *	corridor, or at the srcPos), the arms will be at root+1 and root+2. The
 *	first point that is queried will be the left side of the edge immediately
 *	following the edge of the funnel arms, which will always be funnelRight+2.
 *
 *	As a new root is found, it will be added to the list of path points, and
 *	the search will continue from this point.
 */
bool ChunkWaypointStatePath::funnel( const Corridor & corridor,
		const Vector2 & srcPos, const Vector2 & dstPos,
		BW::vector< std::pair< uint32, Vector2 > > & pathPoints )
{
	BW::vector< Vector2 > corridorPoints( corridor.size() * 2 + 2 );

	corridorPoints.front() = srcPos;
	corridorPoints.back() = dstPos;

	for (uint32 corridorIndex = 0; corridorIndex != corridor.size();
			++corridorIndex)
	{
		corridorPoints[ corridorIndex * 2 + 1 ] =
				corridor[ corridorIndex ].first;
		corridorPoints[ corridorIndex * 2 + 2] =
				corridor[ corridorIndex ].second;
	}

	uint32 root = 0;
	uint32 prevRoot = UINT_MAX;

	while (prevRoot != root)
	{
		prevRoot = root;

		// Add the root to the list of path points
		if (root != 0)
		{
			// corridorIndex is the edge index of 'corridor' in which the root
			// point lies.
			uint32 corridorIndex = (root - 1) / 2;

			MF_ASSERT(corridorIndex < corridorPoints.size());

			bool shouldAddRoot = true;

			if (!pathPoints.empty())
			{
				const std::pair< uint32, Vector2 > & prevPathPoint =
						*(pathPoints.end()-1);

				// Points should all be from different edges
				MF_ASSERT( corridorIndex != prevPathPoint.first );

				// Disregard duplicate points
				if (almostEqual( corridorPoints[ root ],
							prevPathPoint.second ))
				{
					shouldAddRoot = false;
				}
			}

			// Do not add a point on the last edge, as this will always be
			// explicitly added to the end of the path.
			if (corridorIndex == corridor.size() - 1)
			{
				shouldAddRoot = false;
			}

			if (shouldAddRoot)
			{
				pathPoints.push_back( std::make_pair(
							corridorIndex, corridorPoints[ root ] ) );
			}
		}

		uint32 funnelLeft;
		uint32 funnelRight;

		// root is on the left of the corridor
		if (root % 2 == 1)
		{
			funnelLeft = root + 2;
			funnelRight = root + 3;
		}

		// root is on the right of the corridor, or root is the src position
		else
		{
			funnelLeft = root + 1;
			funnelRight = root + 2;
		}


		// Search until we reach the end or we have a new root.
		for (uint32 current = funnelRight + 1;
				current < corridorPoints.size() && prevRoot == root;
				++current)
		{
			if (!processCorridorPoint( corridorPoints, root, funnelLeft,
					funnelRight, current ))
			{
				return false;
			}
		}
	}

	return true;
}


/**
 *	Validate the NavLoc location, attempts to validate the position and create
 *	a valid NavLoc located on the current cached path, and output as outNavLoc
 */
bool ChunkWaypointStatePath::validateNavLoc( const Vector3 & position,
	NavLoc * outNavLoc ) const
{
	if ( !this->isValid() )
	{
		return false;
	}

	const ChunkWaypointState *nextState = this->pNext();

	if ( nextState && nextState->navLoc().containsProjection(position) )
	{
		if ( outNavLoc )
		{
			*outNavLoc = NavLoc(nextState->navLoc(), position);
		}
		return true;
	}

	for( BW::vector< ChunkWaypointState >::const_reverse_iterator
		iter = stateIterOnFullPath_;
		iter != fullPath_.rend();
		++iter)
	{
		if (iter->navLoc().containsProjection(position))
		{
			if ( outNavLoc )
			{
				*outNavLoc = NavLoc(iter->navLoc(), position);
			}
			return true;
		}
	}
	return false;
}


/**
 *	Move the current state iterator along the path until the current state
 */
void ChunkWaypointStatePath::moveStateAlongPath( 
	const ChunkWaypointState & state )
{
	const ChunkWaypointState *nextState = this->pNext();

	if ( !nextState )
	{
		// No state, path is probably empty
		return;
	}

	// Skip over states till one matches the current state, or
	// we reach the next state.
	while ( stateIterOnFullPath_ != fullPath_.rend() &&
		!this->statesAreEquivalent( state, *stateIterOnFullPath_) &&
		!this->statesAreEquivalent( *stateIterOnFullPath_, *nextState ) )
	{
		++stateIterOnFullPath_;
	}

	BW::vector<ChunkWaypointState>::const_reverse_iterator
		nextOnFullPath = (stateIterOnFullPath_ + 1);

	// We have reached the next state, this is the state we are meant to reach
	if ( this->statesAreEquivalent( *stateIterOnFullPath_, *nextState ) || 
		nextOnFullPath >= fullPath_.rend() )
	{
		return;
	}

	// To prevent being stuck on a state which overlaps an existing
	// state we check if the next state could be matched by waypoint
	// (statesAreEquivalent) or point.
	if (this->statesAreEquivalent( state, *nextOnFullPath ) ||
		almostEqual(nextOnFullPath->navLoc().point(), state.navLoc().point()) )
	{
		stateIterOnFullPath_ = nextOnFullPath;
	}
}


/**
 *	Check if a ChunkWaypointState is located on the current cached path
 */
bool ChunkWaypointStatePath::isPartOfPath(
		const ChunkWaypointState & current ) const
{
	for( BW::vector< ChunkWaypointState >::const_reverse_iterator
		iter = stateIterOnFullPath_;
		iter != fullPath_.rend();
		++iter )
	{
		if(this->statesAreEquivalent( current, *iter ))
		{
			return true;
		}
	}
	return false;
}

BW_END_NAMESPACE

// chunk_waypoint_state_path.cpp

