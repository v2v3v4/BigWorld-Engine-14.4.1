#include "pch.hpp"

#include "navigator.hpp"

#include "astar.hpp"
#include "chunk_navigator.hpp"
#include "chunk_waypoint_set.hpp"
#include "chunk_waypoint_set_state.hpp"
#include "chunk_waypoint_state_path.hpp"
#include "navigator_cache.hpp"
#include "navloc.hpp"
#include "waypoint_neighbour_iterator.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "WayPoint", 0 )


BW_BEGIN_NAMESPACE

namespace // (anonymous)
{

AStar< ChunkWaypointState, ChunkWaypointState::SearchData >::StateScopedFreeList s_chunkWPStateFreeList;
AStar< ChunkWPSetState >::StateScopedFreeList s_chunkWPSetStateFreeList;

/**
 *	Convert the ChunkWaypointStatePath to a path of corresponding Vector3
 *	positions.
 *
 *	@param path 			The A* search results for ChunkWaypointStates.
 *	@param waypointPath 	The output path of Vector3 positions. 
 */
void addWaypointPathPoints( const ChunkWaypointStatePath & path,
		Vector3Path & waypointPath )
{
	ChunkWaypointStatePath pathCopy( path );

	// No need to add the start point.

	while (pathCopy.isValid())
	{
		// Add the intermediate points and the destination
		waypointPath.push_back( pathCopy.pNext()->navLoc().point() );
		pathCopy.pop();
	}
}

} // end namespace (anonymous)


// -----------------------------------------------------------------------------
// Section: Navigator
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Navigator::Navigator() :
	pCache_( new NavigatorCache() )
{
}


/**
 *	Destructor.
 */
Navigator::~Navigator()
{
	bw_safe_delete(pCache_);
}


/**
 *	Gets a NavLoc which is part of the cached path, or a new NavLoc based on
 *	the position which has been passed in
 *	@param pSpace		The space within which to create the NavLoc
 *	@param pos			The position at which to create the NavLoc
 *	@param girth		The girth with which to create the NavLoc
 *
 *	@return A NavLoc on the cached path or a new NavLoc
 */
NavLoc Navigator::createPathNavLoc( ChunkSpace * pSpace, const Vector3 & pos,
	float girth ) const
{
	NavLoc navLoc;
	if ( pCache_->wayPath().validateNavLoc(pos, &navLoc) )
	{
		return navLoc;
	}

	return NavLoc( pSpace, pos, girth );
}


/**
 *	Find the path between the given NavLocs. They must be valid and distinct.
 *
 *	@note The NavLoc returned in 'nextWaypoint' should be handled with care, as
 *		  it may only be semi-valid. Do not pass it into any methods without
 *		  first verifying it using the 'guess' NavInfo constructor.  For
 *		  example, do not pass it back into findPath without doing this.  
 *
 *	@param src 					The source position.
 *	@param dst 					The destination position.
 *	@param maxSearchDistance 	The maximum search distance.
 *	@param blockNonPermissive 	Whether to take into account non-permissive
 *								portals.
 *	@param nextWaypoint			The next waypoint in the found path.
 *	@param passedActivatedPortal	A boolean that is filled with a value
 *								indicated whether an activated portal was
 *								passed when traversing between waypoint sets.
 *	@param cacheState			The state which to access to the cache in either
 *								READ_CACHE or READ_WRITE_CACHE
 *
 *	@return 					true if a way could be found, false otherwise.
 */
bool Navigator::findPath( const NavLoc & src, const NavLoc & dst, 
		float maxSearchDistance, bool blockNonPermissive,
		NavLoc & nextWaypoint, bool & passedActivatedPortal,
		CacheState cacheState ) const
{
	// When the cache is set for Read only, we need to verify the path
	// matches the existing path destination and source, if so we can continue
	// using the stored cache in pCache_.
	if ( cacheState == READ_CACHE &&
		!pCache_->wayPath().empty() &&
		(!pCache_->wayPath().isCurrentState( ChunkWaypointState( src ) ) ||
		!pCache_->wayPath().isDestinationState( ChunkWaypointState( dst ) )))
	{
		NavigatorCache cache;
		if (this->searchNextWaypoint( cache, src, dst, maxSearchDistance, 
				blockNonPermissive, &passedActivatedPortal ))
		{
			nextWaypoint = cache.pNextWaypointState()->navLoc();
			return true;
		}
		return false;
	}

	// Using cache, or cache matches use cache (won't be overwritten).
	if (this->searchNextWaypoint( *pCache_, src, dst, maxSearchDistance, 
			blockNonPermissive, &passedActivatedPortal ))
	{
		const NavLoc & next = pCache_->pNextWaypointState()->navLoc();
		if ( next.waypointsEqual(dst) )
		{
			// Use dst as point might not match
			nextWaypoint = dst;
		}
		else
		{
			nextWaypoint = next;
		}
		return true;
	}

	return false;
}


/**
 *	This method finds the complete path between the source position and the
 *	destination position, and fills the waypoint path. It does not clobber the
 *	cache for this Navigator. 
 *
 *	@param pSpace				The chunk space to navigate in.
 *	@param srcPos				The source position.
 *	@param dstPos				The destination position.
 *	@param maxSearchDistance 	The maximum search distance.
 *	@param blockNonPermissive 	Whether to take into account non-permissive
 *								portals.
 *	@param girth				The girth of the path to search for.
 *	@param waypointPath 		The Vector3Path which will be filled with the
 *								positions for the full path.
 */
bool Navigator::findFullPath( ChunkSpace * pSpace,
		const Vector3 & srcPos, const Vector3 & dstPos,
		float maxSearchDistance, bool blockNonPermissive, float girth,
		Vector3Path & waypointPath ) const
{
	bool ok = false;
	NavLoc src( pSpace, srcPos, girth );
	NavLoc dst( pSpace, dstPos, girth );

	if (src.valid() && dst.valid())
	{
		src.clip();
		dst.clip();
		waypointPath.clear();

		// We don't clobber the entity's current navigator cache, so we use our
		// internal one.
		NavigatorCache cache; 

		ChunkWPSetState dstSetState( dst );

		NavLoc current( src );
		bool done = false;

		while (!done)
		{
			if (this->searchNextWaypoint( cache, current, dst,
					maxSearchDistance, blockNonPermissive ))
			{
				// We found a way through the current chunk, add the points in
				// the cache.
				const ChunkWaypointStatePath & path = cache.wayPath();
				addWaypointPathPoints( path, waypointPath );

				// Need to resolve the waypoint path destination's waypoint
				// index.
				NavLoc pathDestLoc = path.pDest()->navLoc();
				current = NavLoc( pathDestLoc, pathDestLoc.point() );

				if (current.waypointsEqual( dst ))
				{
					waypointPath.push_back( dst.point() );
					done = true;
					ok = true;
				}
			}
			else
			{
				waypointPath.clear();
				done = true;
			}
		}
	}

	return ok;
}


/**
 *	Find a point nearby random point in a connected navmesh
 */
bool Navigator::findRandomNeighbourPointWithRange( ChunkSpace * pSpace,
		Vector3 position, float minRadius, float maxRadius, float girth,
		Vector3 & result )
{
	// Start from a bit above so we drop to the correct navpoly
	position.y += 0.1f;

	NavLoc navLoc( pSpace, position, girth );

	if (!navLoc.valid())
	{
		return false;
	}

	float radius = float( rand() % 100 ) * ( maxRadius - minRadius ) /
		100.0f + minRadius;
	float angle = float( rand() % 360 ) / 180 * MATH_PI;
	Vector3 navPosition( navLoc.point() );

	navPosition.x += radius * cos( angle );
	navPosition.z += radius * sin( angle );

	ChunkWaypointSetPtr pSet = navLoc.pSet();
	WaypointIndex waypoint = navLoc.waypoint();

	ChunkWaypointSetPtr pLastSet;
	WaypointIndex lastWaypoint = -1;

	WorldSpaceVector3 clipPos( navPosition );
	navLoc.clip( clipPos );
	float minDistSquared = 
		( clipPos.x - navPosition.x ) * ( clipPos.x - navPosition.x ) +
		( clipPos.z - navPosition.z ) * ( clipPos.z - navPosition.z );

	bool foundNew = true;

	while (foundNew)
	{
		foundNew = false;

		for (WaypointNeighbourIterator wni( pSet, waypoint );
				!wni.ended(); 
				wni.advance())
		{
			if (wni.pNeighbourSet() == pLastSet &&
					wni.neighbourWaypointIndex() == lastWaypoint)
			{
				continue;
			}

			ChunkWaypoint& wp = wni.neighbourWaypoint();

			Chunk* pChunk = wni.pNeighbourSet()->chunk();

			clipPos = WorldSpaceVector3( navPosition );
			wp.clip( *(wni.pNeighbourSet()), pChunk, 
				clipPos );

			// Outside chunk, but point located in inside chunk (shell)
			if (pChunk->isOutsideChunk() && !pChunk->owns( clipPos ))
			{
				// Using point on waypoint edge which isn't in inside chunk
				// TODO: Clip to edge of inside chunk instead of waypoint edge
				for (ChunkWaypoint::Edges::const_iterator edgeIter 
							= wp.edges_.begin();
							edgeIter != wp.edges_.end(); ++edgeIter)
				{
					const ChunkWaypoint::Edge & edge = *edgeIter;

					const Vector2 & thisPoint =
						wni.pNeighbourSet()->vertexByIndex( edge.vertexIndex_ );

					WaypointSpaceVector3 point( thisPoint.x,  
						( wp.minHeight_ + wp.maxHeight_ ) / 2.f, 
							thisPoint.y );

					if (pChunk->owns( point ))
					{
						clipPos = MappedVector3( point, pChunk );
						break;
					}
				}
			}

			float distSquared = 
				( clipPos.x - navPosition.x ) * ( clipPos.x - navPosition.x ) +
				( clipPos.z - navPosition.z ) * ( clipPos.z - navPosition.z );

			const float DISTANCE_ADJUST_FACTOR = 0.01f;
			if (distSquared + DISTANCE_ADJUST_FACTOR < minDistSquared)
			{
				minDistSquared = distSquared;
				pSet = wni.pNeighbourSet();
				waypoint = wni.neighbourWaypointIndex();
				foundNew = true;
			}
		}

		lastWaypoint = waypoint;
		pLastSet = pSet;
	}

	ChunkWaypoint& wp = pSet->waypoint( waypoint );
	clipPos = WorldSpaceVector3( navPosition );
	wp.clip( *pSet, pSet->chunk(), clipPos );
	wp.makeMaxHeight( pSet->chunk(), clipPos );

	result = NavLoc( pSet->chunk()->space(), clipPos, girth ).point();

	return navLoc.valid();
}


/**
 *	This returns the cached waypoint path. If the destination point was in a
 *	different chunk, this will be only to the edge of the source point's
 *	waypoint set. If there was no cached path, vec3Path will be empty.
 */
void Navigator::getCachedWaypointPath( Vector3Path & vec3Path )
{
	vec3Path.clear();

	// Add the intermediate points and the destination.
	addWaypointPathPoints( pCache_->wayPath(), vec3Path );
}

/**
 *  This method returns the number of elements in the waypoint set path
 *  currently cached.
 */
size_t Navigator::getWaySetPathSize() const
{
	return pCache_->getWaySetPathSize();
}


/**
 *	Clears the cached waypoint path.
 */
void Navigator::clearCachedWayPath()
{
	pCache_->clearWayPath();
}


/**
 *	Clears the cached waypoint set path.
 */
void Navigator::clearCachedWaySetPath()
{
	pCache_->clearWaySetPath();
}


/**
 *	This method indicates whether there is a waypoint path between the two
 *	given search states, that may be in different waypoint sets.
 *
 *	If there is no corresponding path in the cache, it performs a search and,
 *	if successful, stores it in the cache.
 *
 *	@param cache				The navigation cache to use.
 *	@param src					The position to navigate from.
 *	@param dst					The position to navigate to.
 *	@param maxSearchDistance 	The maximum search distance.
 *	@param blockNonPermissive 	If true, then non-permissive portals are taken
 *								into account, otherwise permissively.
 *	@param pPassedActivatedPortal
 *								An optional pointer to a boolean that is filled
 *								with a value indicated whether an activated
 *								portal was passed when traversing between
 *								waypoint sets.
 */
bool Navigator::searchNextWaypoint( NavigatorCache & cache,
		const NavLoc & src, const NavLoc & dst,
		float maxSearchDistance,
		bool blockNonPermissive,
		bool * pPassedActivatedPortal /* = NULL */ ) const
{
	MF_ASSERT_DEBUG( src.valid() && dst.valid() );
	PROFILER_SCOPED( searchNextWaypoint );

	if (pPassedActivatedPortal)
	{
		*pPassedActivatedPortal = false;
	}

	ChunkWaypointState srcState( src );
	ChunkWaypointState dstState( dst );

	const ChunkWPSetState * pNextSetState = NULL;

	if (src.pSet() != dst.pSet())
	{
		PROFILER_SCOPED( searchNextWaypoint_set );
		// The source and destination are in different waypoint sets.

		// We need to find a path amongst the waypoint sets, it may be cached
		// from an earlier navigation query.
		ChunkWPSetState srcSetState( src );
		ChunkWPSetState dstSetState( dst );

		srcSetState.blockNonPermissive( blockNonPermissive );

		if (!this->searchNextWaypointSet( cache, srcSetState, dstSetState, 
					maxSearchDistance ))
		{
			return false;
		}

		pNextSetState = cache.pNextWaypointSetState();
		MF_ASSERT( pNextSetState != NULL );

		// Search to the edge of the current waypoint set.
		dstState = ChunkWaypointState( pNextSetState->pSet(), dst.point() );
	}

	// It makes no sense to check for maxSearchDistance inside navpoly set
	// if it is greater than gridSize.

	const float maxSearchDistanceInSet = 
		(maxSearchDistance > src.gridSize()) ?
			-1 : maxSearchDistance;

	{
		PROFILER_SCOPED( searchNextWaypoint_local );
		if (!this->searchNextLocalWaypoint( cache, srcState, dstState, 
				maxSearchDistanceInSet ))
		{
			return false;
		}

		// If we traversed a set, see if we went through an activated portal.
		if (pPassedActivatedPortal != NULL && 
				pNextSetState != NULL &&
				cache.pNextWaypointState()->navLoc().pSet() != src.pSet() && 
				pNextSetState->passedActivatedPortal())
		{
			*pPassedActivatedPortal = true;
		}
	}

	return true;
}


/**
 *	This method indicates whether there is a waypoint path between the
 *	two given search states, assumed to be in the same waypoint set. If there
 *	is no corresponding path in the cache, it performs a search and, if
 *	successful, stores it in the cache.
 *
 *	@param cache 				The navigation cache to use.
 *	@param srcState 			The source search state.
 *	@param dstState 			The destination search state.
 *	@param maxSearchDistance	The maximum search distance.
 *
 *	@return 	true if a path exists, either from the cache or it was
 *				searched, otherwise false if no path exists. If a search
 *				occurred, the cache is updated with the result. 
 */
bool Navigator::searchNextLocalWaypoint( NavigatorCache & cache,
		const ChunkWaypointState & srcState, 
		const ChunkWaypointState & dstState, 
		float maxSearchDistance ) const
{
	PROFILER_SCOPED( searchNextLocalWaypoint );
	if (cache.findWayPath( srcState, dstState ))
	{
		// Remove potential overlapping source states
		pCache_->moveStateAlongPath(srcState);
	}
	else
	{
		// Search, and reset the cache.
		AStar< ChunkWaypointState, ChunkWaypointState::SearchData > astar( &s_chunkWPStateFreeList );
		PROFILER_SCOPED( searchNextLocal_astar );
		if (astar.search( srcState, dstState, maxSearchDistance ))
		{
			cache.saveWayPath( astar, srcState, dstState );
		}
		else
		{
			cache.clearWayPath(); // for sanity

			return false;
		}
	}
	return true;
}


/**
 *	This method indicates whether there is a waypoint set path between the two
 *	given search states. If there is no corresponding path in the cache, it
 *	performs a search and, if successful, stores it in the cache.
 *
 *	@param cache 		The navigation cache to use.
 *	@param srcState 	
 */
bool Navigator::searchNextWaypointSet( NavigatorCache & cache,
		const ChunkWPSetState & srcState,
		const ChunkWPSetState & dstState,
		float maxSearchDistance ) const
{
	bool bypassCache = false;

	// Don't use the cache if the cache contains a waypoint set path that
	// passed through a portal belonging to an indoor chunk.
	bypassCache = bypassCache || cache.waySetPathPassedShellBoundary();

	if (bypassCache ||
			!cache.findWaySetPath( srcState, dstState ))
	{
		// Recalculate the waypoint set path via A* search.
		cache.clearWayPath();
		cache.clearWaySetPath();

		AStar< ChunkWPSetState > astarSet( &s_chunkWPSetStateFreeList );

		if (astarSet.search( srcState, dstState, maxSearchDistance ))
		{
			cache.saveWaySetPath( astarSet );
		}
		else
		{
			cache.clearWaySetPath(); // for sanity

			return false;
		}
	}

	return true;
}


void Navigator::dumpCache()
{
	typedef BW::vector< ChunkWaypointState > WPStateVec;

	BW::stringstream ss;
	ss << "Cache:" << std::endl;

	const WPStateVec & rp = pCache_->wayPath().reversePath();
	WPStateVec::const_iterator wpIter = rp.begin();
	size_t i = 0;

	ss << "Waypoint Reverse Path:" << std::endl;
	for ( ; wpIter != rp.end(); ++wpIter )
	{
		ss << "\t[" << i++ << "] " << wpIter->desc() << std::endl;
	}

	const WPStateVec & fp = pCache_->wayPath().fullPath();
	wpIter = fp.begin();
	i = 0;

	ss << "Waypoint Full Path:" << std::endl;
	for ( ; wpIter != fp.end(); ++wpIter )
	{
		ss << "\t[" << i++ << "] " << wpIter->desc() << std::endl;
	}
	DEBUG_MSG( "%s\n", ss.str().c_str() );
}

BW_END_NAMESPACE

// navigator.cpp
