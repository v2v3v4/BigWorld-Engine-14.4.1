#ifndef NAVIGATOR_FIND_RESULT_HPP
#define NAVIGATOR_FIND_RESULT_HPP

#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

class ChunkWaypointSet;
typedef SmartPointer< ChunkWaypointSet > ChunkWaypointSetPtr;

/**
 *	This class is used to return the results of a ChunkNavigator::find operation.
 */
class NavigatorFindResult
{
public:
	NavigatorFindResult() :
		pSet_( NULL ),
		waypoint_( -1 ),
		exactMatch_( false )
	{ }

	ChunkWaypointSetPtr pSet()
		{ return pSet_; }

	void pSet( ChunkWaypointSetPtr pSet )
		{ pSet_ = pSet; }

	WaypointIndex waypoint() const
		{ return waypoint_; }

	void waypoint( WaypointIndex waypoint )
		{ waypoint_ = waypoint; }

	bool exactMatch() const
		{ return exactMatch_; }

	void exactMatch( bool exactMatch )
		{ exactMatch_ = exactMatch; }

private:
	ChunkWaypointSetPtr	pSet_;
	WaypointIndex		waypoint_;
	bool				exactMatch_;
};

BW_END_NAMESPACE

#endif // NAVIGATOR_FIND_RESULT_HPP

