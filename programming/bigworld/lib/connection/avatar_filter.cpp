#include "pch.hpp"

#include "avatar_filter.hpp"

#include "movement_filter_target.hpp"

BW_BEGIN_NAMESPACE

int AvatarFilter_Token = 0;

/**
 *	Constructor.
 */
AvatarFilter::AvatarFilter( FilterEnvironment & filterEnvironment ) :
	MovementFilter( filterEnvironment ),
	helper_( filterEnvironment )
{}


/**
 *	Destructor.
 */
AvatarFilter::~AvatarFilter()
{

}


/**
 *	Override from MovementFilter.
 */
void AvatarFilter::reset( double time )
{
	helper_.reset( time );
}


/**
 *	Override from MovementFilter.
 */
void AvatarFilter::input( double time, SpaceID spaceID, EntityID vehicleID, 
		const Position3D & pos, const Vector3 & posError,
		const Direction3D & dir )
{
	helper_.input( time, spaceID, vehicleID, pos, posError, dir );
}


/**
 *	Override from MovementFilter.
 */
void AvatarFilter::output( double time, MovementFilterTarget & target )
{
	SpaceID spaceID;
	EntityID vehicleID;
	Position3D position;
	Vector3 velocity;
	Direction3D direction;

	helper_.output( time, spaceID, vehicleID, position, velocity, 
		direction );

	target.setSpaceVehiclePositionAndDirection( spaceID, vehicleID, position,
		direction );
}


/**
 *	Override from MovementFilter.
 */
bool AvatarFilter::getLastInput( double & time, SpaceID & spaceID, 
		EntityID & vehicleID, Position3D & pos, Vector3 & posError, 
		Direction3D & dir ) const
{
	return helper_.getLastInput( time, spaceID, vehicleID, pos, posError, 
		dir );
}


/**
 *	This method will grab the history from another AvatarFilter if one is
 *	so provided
 */
bool AvatarFilter::tryCopyState( const MovementFilter & rOtherFilter )
{
	const AvatarFilter * pOtherAvatarFilter =
		dynamic_cast< const AvatarFilter * >( &rOtherFilter );

	if (pOtherAvatarFilter == NULL)
	{
		return false;
	}

	helper_ = pOtherAvatarFilter->helper_;

	return true;
}

BW_END_NAMESPACE

// avatar_filter.cpp
