#include "pch.hpp"

#include "waypoint_stats.hpp"

#include "cstdmf/watcher.hpp"


BW_BEGIN_NAMESPACE

static uint s_numWaypoints = 0;
static uint s_numEdges = 0;
static uint s_numVertices = 0;
static SimpleMutex s_mutex;


/**
 *	Count a single waypoint.
 */
void WaypointStats::addWaypoint()
{
	SimpleMutexHolder smh( s_mutex );
	++s_numWaypoints;
}


/**
 *	Remove a single waypoint from the count.
 */
void WaypointStats::removeWaypoint()
{
	SimpleMutexHolder smh( s_mutex );
	--s_numWaypoints;
}


/**
 *	Add the given number of edges to the count.	
 *
 *	@param numEdges	The number of edges to add.
 */
void WaypointStats::addEdges( uint numEdges )
{
	SimpleMutexHolder smh( s_mutex );
	s_numEdges += numEdges;
}


/**
 *	Add the given number of distinct vertices to the count.
 *
 *	@param numVertices	The number of distinct vertices to add.
 */
void WaypointStats::addVertices( uint numVertices )
{
	SimpleMutexHolder smh( s_mutex );
	s_numVertices += numVertices;
}


/**
 *	Remove the given number of edges and distinct vertices from the count.
 *
 *	@param numEdges			The number of edges to remove from the count.
 *	@param numVertices		The number of vertices to remove from the count.
 */
void WaypointStats::removeEdgesAndVertices( uint numEdges, uint numVertices )
{
	SimpleMutexHolder smh( s_mutex );
	s_numEdges -= numEdges;
	s_numVertices -= numVertices;
}


#if ENABLE_WATCHERS
/**
 *	Add watchers for the counts.
 */
void WaypointStats::addWatchers()
{
	MF_WATCH( "stats/waypoint/numWaypoints", s_numWaypoints );
	MF_WATCH( "stats/waypoint/numEdges", s_numEdges );
	MF_WATCH( "stats/waypoint/numVertices", s_numVertices );
}

#endif 

BW_END_NAMESPACE

// waypoint_stats.cpp

