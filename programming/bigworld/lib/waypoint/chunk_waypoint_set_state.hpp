#ifndef CHUNK_WAYPOINT_SET_STATE_HPP
#define CHUNK_WAYPOINT_SET_STATE_HPP

#include "chunk_waypoint_set.hpp"

#include "cstdmf/stdmf.hpp"

#include "math/vector3.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class NavLoc;

/**
 *	This class is a state in an A-Star search of the chunk waypoint set graph.
 */
class ChunkWPSetState
{
public:
	ChunkWPSetState();
	explicit ChunkWPSetState( ChunkWaypointSetPtr pSet );
	explicit ChunkWPSetState( const NavLoc & loc );

	typedef ChunkWaypointConns::const_iterator adjacency_iterator;

	intptr compare( const ChunkWPSetState & other ) const
		{ return intptr(pSet_.get()) - intptr(other.pSet_.get()); }

	BW::string desc() const;

	uintptr hash() const
		{ return (uintptr)(pSet_.get()); }

	bool isGoal( const ChunkWPSetState & goal ) const
		{ return pSet_ == goal.pSet_; }

	adjacency_iterator adjacenciesBegin() const
		{ return pSet_->connectionsBegin(); }

	adjacency_iterator adjacenciesEnd() const
		{ return pSet_->connectionsEnd(); }

	bool getAdjacency( adjacency_iterator iter, ChunkWPSetState & neigh,
		const ChunkWPSetState & goal, void* & searchData ) const;

	float distanceFromParent() const
		{ return distanceFromParent_; }

	float distanceToGoal( const ChunkWPSetState & goal ) const
		{ return ( position_ - goal.position_ ).length(); }

	ChunkWaypointSetPtr pSet() const
		{ return pSet_; }

	void passedActivatedPortal( bool a )
		{ passedActivatedPortal_ = a; }

	bool passedActivatedPortal() const
		{ return passedActivatedPortal_; }

	void passedShellBoundary( bool a )
		{ passedShellBoundary_ = a; }

	bool passedShellBoundary() const
		{ return passedShellBoundary_; }

	const Vector3 & position() const
		{ return position_; }

	void blockNonPermissive( bool value )
		{ blockNonPermissive_ = value; }

private:
	ChunkWaypointSetPtr	pSet_;
	bool				blockNonPermissive_;
	float				distanceFromParent_;
	bool				passedActivatedPortal_;
	bool				passedShellBoundary_;
	Vector3				position_;
};

BW_END_NAMESPACE

#endif // CHUNK_WAYPOINT_SET_STATE_HPP

