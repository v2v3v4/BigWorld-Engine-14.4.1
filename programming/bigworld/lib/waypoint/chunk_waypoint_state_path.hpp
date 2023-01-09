#ifndef CHUNK_WAYPOINT_STATE_PATH_HPP
#define CHUNK_WAYPOINT_STATE_PATH_HPP

#include "astar.hpp"
#include "chunk_waypoint_state.hpp"
#include "search_path_base.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is responsible for holding the path from an A* search through a
 *	single waypoint set.
 */
class ChunkWaypointStatePath :
	public SearchPathBase< ChunkWaypointState, ChunkWaypointState::SearchData >
{
public:

	/**
	 *	Constructor.
	 */
	ChunkWaypointStatePath():
			SearchPathBase< ChunkWaypointState,
					ChunkWaypointState::SearchData >(),
			canBeSmoothed_( false )
	{}


	/**
	 *	Destructor.
	 */
	virtual ~ChunkWaypointStatePath()
	{}


	virtual void init(
			AStar< ChunkWaypointState,
					ChunkWaypointState::SearchData  > & astar );

	/**
	 *	Return whether two ChunkWaypointStates are equivalent. For our
	 *	purposes, we see if they refer to the same waypoint in the same
	 *	waypoint set.
	 */
	virtual bool statesAreEquivalent( const ChunkWaypointState & s1, 
			const ChunkWaypointState & s2 ) const
	{
		// If either is a portal, then accept it being equal if same set
		if ( s1.navLoc().waypoint() == -1 || s2.navLoc().waypoint() == -1)
		{
			return s1.navLoc().pSet() == s2.navLoc().pSet();
		}
		return s1.navLoc().waypointsEqual( s2.navLoc() );
	}

	void removeDuplicates();

	bool smoothPath(
			AStar< ChunkWaypointState, ChunkWaypointState::SearchData > & astar,
			const ChunkWaypointState & srcState,
			const ChunkWaypointState & dstState);

	bool validateNavLoc( const Vector3 & position, NavLoc * outNavLoc ) const;

	void moveStateAlongPath( const ChunkWaypointState & state );

	virtual bool isPartOfPath( const ChunkWaypointState & current ) const;

	const BW::vector< ChunkWaypointState > & fullPath( ) const
		{ return fullPath_; }

private:
	typedef BW::vector< std::pair< Vector2, Vector2 > > Corridor;

	enum RelativePosition{ LEFT, INSIDE, RIGHT, ON_LINE, BAD_FUNNEL };

	static RelativePosition locationOfPointFromLine( const Vector2 & v1,
			const Vector2 & v2, const Vector2 & p );

	static RelativePosition isPointInsideFunnel( const Vector2 & root,
		const Vector2 & left, const Vector2 & right,
		const Vector2 & query );

	static bool processCorridorPoint(
			const BW::vector< Vector2 > & corridorPoints, uint32 & root,
			uint32 & funnelLeft, uint32 & funnelRight,
			uint32 & current );

	static bool funnel( const Corridor & corridor, const Vector2 & srcPos,
			const Vector2 & dstPos,
			BW::vector< std::pair< uint32, Vector2 > > & pathPoints );

	bool canBeSmoothed_;

	typedef BW::vector< ChunkWaypointState > WaypointList;
	WaypointList fullPath_;
	WaypointList::const_reverse_iterator stateIterOnFullPath_;
};

BW_END_NAMESPACE

#endif // CHUNK_WAYPOINT_STATE_PATH_HPP
