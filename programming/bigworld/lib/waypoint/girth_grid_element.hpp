#ifndef GIRTH_GRID_ELEMENT_HPP
#define GIRTH_GRID_ELEMENT_HPP

#include "cstdmf/bw_namespace.hpp"


BW_BEGIN_NAMESPACE

class ChunkWaypointSet;

class GirthGridElement
{
public:
	GirthGridElement( ChunkWaypointSet * pSet, int waypoint ) :
		pSet_( pSet ), 
		waypoint_( waypoint )
	{}

	ChunkWaypointSet * pSet() const
		{ return pSet_; }

	int waypoint() const 
		{ return waypoint_; }


private:
	ChunkWaypointSet	* pSet_;	// dumb pointer
	int					waypoint_;

};

BW_END_NAMESPACE

#endif // GIRTH_GRID_ELEMENT_HPP

