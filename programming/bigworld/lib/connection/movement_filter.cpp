#include "pch.hpp"

#include "movement_filter.hpp"

#include "filter_environment.hpp"

#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param environment 	The filter environment.
 */
MovementFilter::MovementFilter( FilterEnvironment & environment ) :
		environment_( environment )
{}


/**
 *	This method is called to copy the state from an existing MovementFilter,
 *	usually because this MovementFilter is replacing the other MovementFilter
 *	as the filter for a MovementFilterTarget
 *
 *	@param rOtherFilter		The MovementFilter to copy state from
 */
void MovementFilter::copyState( const MovementFilter & rOtherFilter )
{
	BW_GUARD;

	// Firstly, let our subclass override this.
	if (this->tryCopyState( rOtherFilter ))
	{
		return;
	}

	// Fallback case: Copy last input of rOtherFilter as our first input
	double time;
	SpaceID spaceID;
	EntityID vehicleID;
	Position3D position;
	Vector3 positionError;
	Direction3D direction;
	if (rOtherFilter.getLastInput( time, spaceID, vehicleID, position,
		positionError, direction ) )
	{
		this->input( time, spaceID, vehicleID, position, positionError,
			direction );
	}
}


/**
 *	This method is called to drop a position on to the terrain.
 *
 *	@param spaceID			The ID of the space the movement filter is in.
 *	@param fall				The position to drop from.
 *	@param land 			This is filled with the landing position.
 *	@param groundNormal 	If not NULL, this is filled with the ground normal
 *							at the landing position.
 */
bool MovementFilter::filterDropPoint( SpaceID spaceID, const Position3D & fall,
	Position3D & land, Vector3 * groundNormal )
{
	return environment_.filterDropPoint( spaceID, fall, land, groundNormal );
}


/**
 *	This method is called to determine whether the given position can be
 *	regarded as being "on-ground".
 *
 *	@param position 	The position to test.
 *	@param isOnGround 	Whether the position is on-ground.
 * 
 *	@return 			true on success, false on failure (there is no terrain
 *						under the given position).
 */
bool MovementFilter::resolveOnGroundPosition( Position3D & position,
		bool & isOnGround )
{
	return environment_.resolveOnGroundPosition( position, isOnGround );
}


/**
 *	This method transforms the given position and direction from the specified
 *	vehicle's local space, into world coordinates.
 *
 *	@param spaceID 		The ID of the space containing the vehicle.
 *	@param vehicleID 	The ID of the vehicle entity for which the position and
 *						direction are in its local space.
 *	@param position 	The position to transform into world-coordinates.
 *	@param direction 	The direction to transform into world-coordinates.
 */
void MovementFilter::transformIntoCommon( SpaceID spaceID, EntityID vehicleID,
		Position3D & position, Direction3D & direction )
{
	environment_.transformIntoCommon( spaceID, vehicleID, position,
		direction );
}


/**
 *	This method transforms the given position and direction from world
 *	coordinates to the specified vehicle's local space.
 *
 * 	@param spaceID 		The ID of the space containing the vehicle.
 *	@param vehicleID 	The ID of the vehicle entity for which the position and
 *						direction are to be transformed into its local space.
 * 	@param position 	The position to transform into the vehicle local-space.
 *	@param direction 	The direction to transform into the vehicle local-space.
 */
void MovementFilter::transformFromCommon( SpaceID spaceID, EntityID vehicleID,
		Position3D & position, Direction3D & direction )
{
	environment_.transformFromCommon( spaceID, vehicleID, position,
		direction );
}

BW_END_NAMESPACE

// movement_filter.cpp
