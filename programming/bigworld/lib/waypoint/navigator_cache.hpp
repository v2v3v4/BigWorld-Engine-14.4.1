#ifndef NAVIGATOR_CACHE_HPP
#define NAVIGATOR_CACHE_HPP

#include "astar.hpp"
#include "chunk_waypoint_set_state.hpp"
#include "chunk_waypoint_set_state_path.hpp"
#include "chunk_waypoint_state.hpp"
#include "chunk_waypoint_state_path.hpp"

#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This class caches data from recent searches. It is purposefully not
 *  defined in the header file so that our users need not know its contents.
 */
class NavigatorCache : public ReferenceCount
{
public:
	NavigatorCache();

	virtual ~NavigatorCache() 
	{}

	void saveWayPath( 
		AStar< ChunkWaypointState, ChunkWaypointState::SearchData > & astar,
		const ChunkWaypointState & src,
		const ChunkWaypointState & dst );

	/**
	 * Return this cache's waypoint path.
	 */
	const ChunkWaypointStatePath & wayPath() const 
		{ return wayPath_; }

	bool findWayPath( const ChunkWaypointState & src,
			const ChunkWaypointState & dest );

	/**
	 *	Get the location of the next waypoint in the stored waypoint path.
	 */
	const ChunkWaypointState * pNextWaypointState() const 
		{ return wayPath_.pNext(); }

	/**
	 *	Clear the waypoint path.
	 */
	void clearWayPath()
		{ wayPath_.clear(); }

	/**
	 * Clean up cache until a given state
	 */
	void moveStateAlongPath( const ChunkWaypointState & state )
		{ wayPath_.moveStateAlongPath( state ); }

	void saveWaySetPath( AStar< ChunkWPSetState > & astar );

	bool findWaySetPath(
		const ChunkWPSetState & src, const ChunkWPSetState & dst );

	/**
	 *	Return the next waypoint set state in the path.
	 */
	const ChunkWPSetState * pNextWaypointSetState() const
	{ 
		return waySetPath_.pNext();
	}

	/**
	 *	Clears the waypoint set path.
	 *	
	 *	This may be necessary because different results will be obtained
	 *	depending on whether ChunkWPSetState::blockNonPermissive is true or
	 *	false.
	 */
	void clearWaySetPath()
		{ waySetPath_.clear(); }

	size_t getWaySetPathSize() const;

	/**
	 *	Return true if the waypoint set path traverses a shell boundary.
	 */
	bool waySetPathPassedShellBoundary() const
		{ return waySetPath_.passedShellBoundary(); }

private:
	ChunkWaypointStatePath 		wayPath_;
	ChunkWaypointSetStatePath	waySetPath_;

};

BW_END_NAMESPACE

#endif // NAVIGATOR_CACHE_HPP

