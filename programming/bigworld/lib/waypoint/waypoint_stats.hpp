#ifndef WAYPOINT_STATS_HPP
#define WAYPOINT_STATS_HPP

#include "cstdmf/concurrency.hpp"
#include "cstdmf/singleton.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Class to count how many waypoint objects are currently stored in memory.
 */
class WaypointStats
{
public:
	static void addWaypoint();
	static void removeWaypoint();
	static void addEdges( uint numEdges );
	static void addVertices( uint numVertices );
	static void removeEdgesAndVertices( uint numEdges, uint vertices );
#if ENABLE_WATCHERS
	static void addWatchers();
#endif

private:
	WaypointStats(); // Never created
};

BW_END_NAMESPACE

#endif // WAYPOINT_STATS_HPP
