#include "pch.hpp"

#include "dumb_filter.hpp"

#include "entity.hpp"
#include "py_dumb_filter.hpp"

#include "terrain/base_terrain_block.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
///  Member Documentation for DumbFilter
///////////////////////////////////////////////////////////////////////////////

/**
 *	@property	double DumbFilter::time_
 *
 *	This member holds the time stamp of the most recent input to be received.
 */


/**
 *	@property	Position3D DumbFilter::pos_
 *
 *	This member holds the position provided by the most recent input to be
 *	received in vehicle coordinates.
 */


/**
 *	@property	float DumbFilter::dir_
 *
 *	This member holds the local space yaw, pitch and roll of the most recent
 *	input to be received.
 */


/**
 *	@property	SpaceID DumbFilter::spaceID_
 *
 *	This member holds the space ID of the most recent input to be
 *	received.
 */


/**
 *	@property	EntityID DumbFilter::vehicleID_
 *
 *	This member holds the vehicle ID of the most recent input to be
 *	received. It may be null.
 */

///////////////////////////////////////////////////////////////////////////////
///  End Member Documentation for DumbFilter
///////////////////////////////////////////////////////////////////////////////


/**
 *	This constructor sets up a DumbFilter for the input entity.
 */
DumbFilter::DumbFilter( PyDumbFilter * pOwner ) :
	Filter( pOwner ),
	time_( -1000.0 ),
	spaceID_( 0 ),
	vehicleID_( 0 ),
	pos_( 0.f, 0.f, 0.f ),
	posError_( 0.f, 0.f, 0.f ),
	dir_( Vector3::zero() )
{
	BW_GUARD;	
}


/**
 *	This method invalidates all previously collected inputs. They will then
 *	be discarded when the next input is received.
 *
 *	@param	time	Ignored
 */
void DumbFilter::reset( double time )
{
	time_ = -1000.0;
	posError_ = Vector3::zero();
}


/**
 *	This method updates the stored position and direction information with that
 *	provided. But only it the new data's time stamp if at least as recent.
 *
 *	@param	time		The estimated client time when the input was sent from
 *						the server.
 *	@param	spaceID		The server space that the position resides in.
 *	@param	vehicleID	The ID of the vehicle in who's coordinate system the
 *						position is defined. A null vehicle ID means that the
 *						position is in world coordinates. 
 *	@param	pos			The new position in either local vehicle or 
 *						world space/common coordinates. The player relative
 *						compression will have already been decoded at this
 *						point by the network layer.
 *	@param	posError	The amount of uncertainty in the position.
 *	@param	dir			The direction of the entity, in the same coordinates
 *						as pos.
 */
void DumbFilter::input(	double time,
						SpaceID spaceID,
						EntityID vehicleID,
						const Position3D & pos,
						const Vector3 & posError,
						const Direction3D & dir )
{
	BW_GUARD;

	// TODO: not sure if this check is still needed.
	if (time < time_)
	{
		return;
	}

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
 *	This method updates an entity's position, velocity and direction to match
 *	that stored from the last input.
 *
 *	@param	time	The client game time in seconds that the entity's volatile
 *					members should be updated for.
 *	@param	target	The entity to be updated
 */
void DumbFilter::output( double time, MovementFilterTarget & target )
{
	BW_GUARD;

	static DogWatch dwDumbFilterOutput("DumbFilter");
	dwDumbFilterOutput.start();

	Entity & entity = static_cast< Entity & >( target );

	// make sure it's above the ground if it's a bot
	if (pos_.y < -12000.f)
	{
		float oldY = entity.localPosition().y;
		pos_.y = (oldY < -12000.f) ?  90.f : oldY + 1.f;

		Position3D rpos;
		if (this->filterDropPoint( spaceID_, pos_, rpos ))
		{
			pos_.y = rpos.y;
		}
		else
		{
			float terrainHeight =
				Terrain::BaseTerrainBlock::getHeight( pos_.x, pos_.z );

			if (terrainHeight != Terrain::BaseTerrainBlock::NO_TERRAIN)
			{
				pos_.y = terrainHeight;
			}
			else
			{
				WARNING_MSG( "DumbFilter::output: Could not get terrain\n" );
			}
		}
	}

	target.setSpaceVehiclePositionAndDirection( spaceID_, vehicleID_, pos_,
		dir_ );

	dwDumbFilterOutput.stop();
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
bool DumbFilter::getLastInput(	double & time,
								SpaceID & spaceID,
								EntityID & vehicleID,
								Position3D & pos,
								Vector3 & posError,
								Direction3D & dir ) const
{
	BW_GUARD;
	if (time_ == -1000.0)
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

// dumb_filter.cpp
