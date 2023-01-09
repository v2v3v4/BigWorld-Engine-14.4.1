#include "pch.hpp"

#include "girth_grid_list.hpp"

#include "chunk_waypoint.hpp"
#include "chunk_waypoint_set.hpp"
#include "navigator_find_result.hpp"

#include "math/vector3.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Add an element to the list.
 */
void GirthGridList::add( const GirthGridElement & element )
{
	container_.push_back( element );
}


/**
 *	Remove an element of the list using its index.
 */
void GirthGridList::eraseIndex( size_t index )
{
	container_.erase( container_.begin() + index );	
}

/**
 *	This method finds a waypoint containing the point in the list of
 *	waypoints overlapping a given girth grid square.
 *
 *	@param point		The point to search with.
 *	@param res			The result of the search.
 *	@param ignoreHeight	If true then the search is done only in the x-z
 *						directions.
 *	@return				True if the search was successful, false otherwise.
 */
bool GirthGridList::find( const WaypointSpaceVector3 & point,
		NavigatorFindResult & res, bool ignoreHeight ) const
{
	if (ignoreHeight)
	{
		const float INVALID_HEIGHT = 1000000.f;
		float bestHeight = INVALID_HEIGHT;
		NavigatorFindResult r;

		for (Container::const_iterator it = container_.begin(); 
				it != container_.end(); 
				++it)
		{
			const ChunkWaypoint & wp = it->pSet()->waypoint( it->waypoint() );

			if (wp.containsProjection( *(it->pSet()), point ))
			{
				// Try to find a reasonable navmesh if there are more than one
				// navmeshes overlapped in the same x,z range.
				// We would like to pick a relatively close navmesh with height
				// range a bit lower than y coordinate of the given point.
				if (fabsf( wp.maxHeight_ - point.y ) <
						fabsf( bestHeight - point.y ))
				{
					bestHeight = wp.maxHeight_;
					r.pSet( it->pSet() );
					r.waypoint( it->waypoint() );
				}
			}
		}

		if (!isEqual( bestHeight, INVALID_HEIGHT ))
		{
			r.exactMatch( true );
			res = r;

			return true;
		}
	}
	else
	{
		for (Container::const_iterator it = container_.begin(); 
				it != container_.end(); ++it)
		{
			const ChunkWaypoint & wp = it->pSet()->waypoint( it->waypoint() );

			if (wp.contains( *(it->pSet()), point ))
			{
				res.pSet( it->pSet() );
				res.waypoint( it->waypoint() );
				res.exactMatch( true );
				return true;
			}
		}
	}

	return false;
}


/**
 *	This method finds a waypoint containing the point with height matching
 *	exactly or lower in the list of waypoints overlapping a given girth
 *	grid square.
 *
 *	@param point		The point to search with.
 *	@param res			The result of the search.
 *	@return				True if the search was successful, false otherwise.
 */
bool GirthGridList::findMatchOrLower( const WaypointSpaceVector3 & point,
		NavigatorFindResult & res ) const
{
	const float INVALID_HEIGHT = 1000000.f;
	float bestHeight = INVALID_HEIGHT;
	NavigatorFindResult r;

	for (Container::const_iterator it = container_.begin(); 
		it != container_.end(); 
		++it)
	{
		const ChunkWaypoint & wp = it->pSet()->waypoint( it->waypoint() );

		if (wp.minHeight_ > point.y + 0.1f)
		{
			continue;
		}

		if (wp.containsProjection( *(it->pSet()), point ))
		{
			if (wp.maxHeight_ + 0.1f >= point.y )
			{
				res.pSet( it->pSet() );
				res.waypoint( it->waypoint() );
				res.exactMatch( true );

				return true;
			}

			// Try to find a reasonable navmesh if there are more than one
			// navmeshes overlapped in the same x,z range.
			// We would like to pick a relatively close navmesh with height
			// range a bit lower than y coordinate of the given point.
			if (fabsf( wp.maxHeight_ - point.y ) <
				fabsf( bestHeight - point.y ))
			{
				bestHeight = wp.maxHeight_;
				r.pSet( it->pSet() );
				r.waypoint( it->waypoint() );
			}
		}
	}

	if (!isEqual( bestHeight, INVALID_HEIGHT ))
	{
		r.exactMatch( true );
		res = r;

		return true;
	}

	return false;
}


/**
 *	This method finds a waypoint closest to the point in the list of
 *	waypoints overlapping a given girth grid square.
 *
 *	@param pChunk		The chunk to search in.
 *	@param point		The point to search with.
 *	@param bestDistanceSquared
 *						Initially all waypoints further away than this are
 *						ignored.  This is set to the distance of the closest
 *						waypoint set that is not ignored.
 *	@param res			The results of the search.
 */
void GirthGridList::find( const Chunk * pChunk, 
		const WaypointSpaceVector3 & point, float & bestDistanceSquared, 
		NavigatorFindResult & res ) const
{
	for (Container::const_iterator it = container_.begin(); 
			it != container_.end(); ++it)
	{
		const ChunkWaypoint & wp = it->pSet()->waypoint( it->waypoint() );
		float distanceSquared = wp.distanceSquared( *(it->pSet()), pChunk,
			MappedVector3( point, pChunk ) );

		if (bestDistanceSquared > distanceSquared)
		{
			bestDistanceSquared = distanceSquared;
			res.pSet( it->pSet() );
			res.waypoint( it->waypoint() );
		}
	}

	if (res.pSet() != NULL)
	{
		const ChunkWaypoint & wp = res.pSet()->waypoint( res.waypoint() );
		res.exactMatch( wp.contains( (*res.pSet()), point ) );
	}
}


/**
 *	This method finds a waypoint closest to the point in the list of
 *	waypoints overlapping a given girth grid square. This function will check
 *	the mark to see if the waypoint has been visited.
 *
 *	@param pChunk		The chunk to search in.
 *	@param point		The point to search with.
 *	@param bestDistanceSquared
 *						Initially all waypoints further away than this are
 *						ignored.  This is set to the distance of the closest
 *						waypoint set that is not ignored.
 *	@param res			The results of the search.
 */
void GirthGridList::findNonVisited( const Chunk * pChunk, 
		const WaypointSpaceVector3 & point,
		float & bestDistanceSquared, NavigatorFindResult & res ) const
{
	MappedVector3 mv( point, pChunk );

	for (Container::const_iterator it = container_.begin(); 
			it != container_.end(); ++it)
	{
		ChunkWaypoint & wp = it->pSet()->waypoint( it->waypoint() );

		if (wp.visited_)
		{
			continue;
		}

		wp.visited_ = 1;
		ChunkWaypoint::s_visitedWaypoints_.push_back( &wp );

		float distanceSquared = wp.distanceSquared( *(it->pSet()), pChunk, mv );

		if (bestDistanceSquared > distanceSquared)
		{
			bestDistanceSquared = distanceSquared;
			res.pSet( it->pSet() );
			res.waypoint( it->waypoint() );
		}
	}

	if (res.pSet() != NULL)
	{
		const ChunkWaypoint & wp = res.pSet()->waypoint( res.waypoint() );
		res.exactMatch( wp.contains( *(res.pSet()), point ) );
	}
}

BW_END_NAMESPACE

// girth_grid_list.cpp

