#ifndef NAVLOC_HPP
#define NAVLOC_HPP

#include "chunk_waypoint_set.hpp"

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stdmf.hpp"

#include "math/vector3.hpp"


BW_BEGIN_NAMESPACE

class ChunkSpace;
class Chunk;
class ChunkWaypointSet;
typedef SmartPointer<ChunkWaypointSet> ChunkWaypointSetPtr;

/**
 *	This class is a location in the navigation mesh
 */
class NavLoc
{
public:
	NavLoc();

	NavLoc( ChunkSpace * pSpace, const Vector3 & point, float girth );

	NavLoc( Chunk * pChunk, const Vector3 & point, float girth );

	NavLoc( const NavLoc & guess, const Vector3 & point );

	~NavLoc();

	bool valid() const	
		{ return pSet_ && pSet_->chunk(); }

	ChunkWaypointSetPtr pSet() const			
		{ return pSet_; }

	WaypointIndex	waypoint() const	
		{ return waypoint_; }

	Vector3	point() const	
		{ return point_; }

	float gridSize() const
		{ return gridSize_; }

	bool isWithinWP() const;

	void clip();

	void makeMaxHeight( Vector3 & point ) const;

	void clip( Vector3 & point ) const;

	BW::string desc() const;

	bool waypointsEqual( const NavLoc & other ) const
		{ return pSet_ == other.pSet_ && waypoint_ == other.waypoint_; }

	bool containsProjection( const Vector3 & point ) const;

private:
	ChunkWaypointSetPtr	pSet_;
	WaypointIndex		waypoint_;
	Vector3				point_;
	float				gridSize_;

	friend class ChunkWaypointState;
	friend class Navigator;
};

BW_END_NAMESPACE

#endif // NAVLOC_HPP
