#include "pch.hpp"

#include "player_avatar_filter.hpp"

#include "entity.hpp"
#include "py_player_avatar_filter.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
///  Member Documentation for PlayerAvatarFilter
///////////////////////////////////////////////////////////////////////////////

/**
 *	@property	double PlayerAvatarFilter::time_
 *
 *	This member holds the time stamp of the most recent input to be received.
 */


/**
 *	@property	SpaceID PlayerAvatarFilter::spaceID_
 *
 *	This member holds the space ID of the most recent input to be
 *	received.
 */


/**
 *	@property	EntityID PlayerAvatarFilter::vehicleID_
 *
 *	This member holds the vehicle ID of the most recent input to be
 *	received. It may be null.
 */


/**
 *	@property	Position3D PlayerAvatarFilter::pos_
 *
 *	This member holds the position provided by the most recent input to be
 *	received in vehicle coordinates.
 */


/**
 *	@property	Direction3D PlayerAvatarFilter::dir_
 *
 *	This member holds the yaw, pitch and roll of the avatar in vehicle
 *	coordinates. Yaw and pitch are usually applied to the head of a biped as
 *	they most likely reflect the facing of the camera.
 */


///////////////////////////////////////////////////////////////////////////////
///  End Member Documentation for PlayerAvatarFilter
///////////////////////////////////////////////////////////////////////////////




/**
 *	Constructor
 *
 *	@param	pType	The python object defining the type of the filter. 
 */
PlayerAvatarFilter::PlayerAvatarFilter( PyPlayerAvatarFilter * pOwner ) :
	Filter( pOwner ),
	time_( -1000.0 ),
	spaceID_( 0 ),
	vehicleID_( 0 ),
	pos_( 0, 0, 0 ),
	posError_( 0, 0, 0 ),
	dir_( Vector3::zero() )
{
}


/**
 *	Destructor
 */
PlayerAvatarFilter::~PlayerAvatarFilter()
{
}


/**
 *	This method invalidates all previously collected inputs. They will then
 *	be discarded when the next input is received.
 *
 *	@param	time	Ignored
 */
void PlayerAvatarFilter::reset( double time )
{
	posError_ = Vector3::zero();
}


/**
 *	This method updates the player filter with a new set of inputs.
 *
 *	@param	time		The client time when the input was received.
 *	@param	spaceID		The server space that the position resides in.
 *	@param	vehicleID	The ID of the vehicle in who's coordinate system the
 *						position is defined. A null vehicle ID means that the
 *						position is in world coordinates. 
 *	@param	pos			The new position in either local vehicle or 
 *						world space/common coordinates.
 *	@param	posError	The amount of uncertainty in the position.
 *	@param	dir			The direction of the entity, in the same coordinates
 *						as pos.
 */
void PlayerAvatarFilter::input(	double time,
								SpaceID spaceID,
								EntityID vehicleID,
								const Position3D & pos,
								const Vector3 & posError,
								const Direction3D & dir )
{
	BW_GUARD;

	time_ = time;

	BoundingBox thisErrorBox( pos - posError,
		pos + posError );

	if (spaceID_ != spaceID || vehicleID_ != vehicleID ||
		almostEqual( posError_, posError ))
	{
		// Same co-ordinate system and accuracy, or changed co-ordinate system

		// Just trust the input
		spaceID_ = spaceID;
		vehicleID_ = vehicleID;
		pos_ = pos;
		posError_ = posError;
	}
	else if (thisErrorBox.intersects( pos_ ))
	{
		// Change in error box, but our old position still lies within it
		// Most likely the reference position is moving away from us, and just
		// crossed an error boundary, reducing our accuracy. Or it moved closer
		// and we were already on the correct side of the new higher accuracy.

		// Don't move, but keep the more accurate of old and new error boxes.
		if (posError.lengthSquared() < posError_.lengthSquared())
		{
			posError_ = posError;
		}
	}
	else
	{
		// Change in error box, and our old position is no longer within the
		// current error box.
		// The reference position moved closer, our input got more accurate,
		// and turns out to have been on the error side of the new higher
		// accuracy.

		// No choice, just trust the input. We may visually pop a little.
		pos_ = pos;
		posError_ = posError;
	}

	dir_ = dir;
}


/**
 *	This method updates an entity's position and direction with that
 *	stored in the filter; but only if time has progressed since the last
 *	time the filter was queried.
 *
 *	@param	time	The client game time in seconds to which the entity's volatile
 *					members should be updated.
 *	@param	target	The entity to be updated
 */
void PlayerAvatarFilter::output( double time, MovementFilterTarget & target )
{
	BW_GUARD;

	target.setSpaceVehiclePositionAndDirection( spaceID_, vehicleID_, pos_,
		dir_ );
}


/**
 *	This function gets the last input received by the filter.
 *	Its arguments will be untouched if no input has yet been received.
 *
 *	@param	time		Will be set to the time stamp of the last input.
 *	@param	spaceID		Will be set to the space ID of the last input.
 *	@param	vehicleID	Will be set to the vehicle ID provided in the last
 *						input.
 *	@param	pos			The position of received in the last input, with its
 *						height fully resolved.
 *	@param	posError	The amount of uncertainty in the position.
 *	@param	dir			The direction received in the last input.
 *
 *	@return				Returns true if an input existed and the values in the parameters
 *						were set.
 */
bool PlayerAvatarFilter::getLastInput(	double & time,
										SpaceID & spaceID,
										EntityID & vehicleID,
										Position3D & pos,
										Vector3 & posError,
										Direction3D & dir ) const
{
	BW_GUARD;
	if ( time_ == -1000.0 )
	{
		return false;
	}

	time = time_;
	spaceID = spaceID_;
	vehicleID = vehicleID_;
	pos = pos_;
	posError = posError_;
	dir = dir_;
	return true;
}


BW_END_NAMESPACE

// player_avatar_filter.cpp
