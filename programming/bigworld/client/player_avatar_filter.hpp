#ifndef PLAYER_AVATAR_FILTER_HPP
#define PLAYER_AVATAR_FILTER_HPP

#include "filter.hpp"

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class PyPlayerAvatarFilter;

/**
 *	This is a simple filter intended for client controlled player entities.
 *
 *	The PlayerAvatarFilter behaves almost exactly like the DumbFilter except
 *	it does not try and resolve 'on ground' input positions as it assumes it
 *	will never be given one.
 */
class PlayerAvatarFilter : public Filter
{
public:
	PlayerAvatarFilter( PyPlayerAvatarFilter * pOwner );
	~PlayerAvatarFilter();

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

#endif // PLAYER_AVATAR_FILTER_HPP
