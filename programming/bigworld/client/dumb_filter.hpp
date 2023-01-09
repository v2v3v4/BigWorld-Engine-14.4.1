#ifndef DUMB_FILTER_HPP
#define DUMB_FILTER_HPP

#include "filter.hpp"

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class PyDumbFilter;

/**
 *	This is a dumb filter class.
 *	It simply sets the position to the last input position.
 *	It does however ensure ordering on time, and doesn't call
 *	the Entity's 'pos' function unless necessary. This could
 *	almost be the Teleport filter.
 */
class DumbFilter : public Filter
{
public:
	DumbFilter( PyDumbFilter * pOwner );
	~DumbFilter() {}

	// Overrides from MovementFilter
	void reset( double time );

	void input(	double time,
				SpaceID spaceID,
				EntityID vehicleID,
				const Position3D & pos,
				const Vector3 & posError,
				const Direction3D & dir );

	void output( double time, MovementFilterTarget & target );

	bool getLastInput(	double & time,
						SpaceID & spaceID,
						EntityID & vehicleID,
						Position3D & pos,
						Vector3 & posError,
						Direction3D & dir ) const;

private:
	// Doxygen comments for all members can be found in the .cpp
	double		time_;
	SpaceID		spaceID_;
	EntityID	vehicleID_;
	Position3D	pos_;
	Vector3		posError_;
	Direction3D	dir_;
};

BW_END_NAMESPACE

#endif // DUMB_FILTER_HPP
