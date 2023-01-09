#include "pch.hpp"

#include "avatar_filter.hpp"

#include "avatar_filter_callback.hpp"
#include "entity.hpp"
#include "py_avatar_filter.hpp"

#include "connection/avatar_filter_helper.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
///  Member Documentation for AvatarFilter
///////////////////////////////////////////////////////////////////////////////

/**
 *	@property	ListNode AvatarFilter::callbackListHead_
 *
 *	This member holds a queue of callbacks to be triggered once the filter's
 *	output time (game time - latency_) reaches that specified in the callback.
 *	The queue is sorted by the time stamps of the callbacks referenced.
 *
 *	@see	AvatarFilter::addCallback()
 *	@see	AvatarFilter::output()
 */
/**
 *	@property	AvatarFilterHelper AvatarFilter::helper_
 *
 *	This member holds the AvatarFilterHelper which manages all the filter's
 *	inputs and generates waypoints along the historical movement path.
 *
 *	@see	AvatarFilter::reset()
 *	@see	AvatarFilter::input()
 *	@see	AvatarFilter::output()
 *	@see	AvatarFilter::getLastInput()
 *	@see	AvatarFilter::latency()
 *	@see	AvatarFilter::getStoredInput()
 */


///////////////////////////////////////////////////////////////////////////////
/// End Member Documentation for AvatarFilter
///////////////////////////////////////////////////////////////////////////////


/**
 *	Constructor
 *
 *	@param	pType	The python object defining the type of the filter. 
 *					The default is 'AvatarFilter' but it may be overridden by
 *					derived types like AvatarDropFilter.
 */
AvatarFilter::AvatarFilter( PyAvatarFilter * pOwner ) :
	Filter( pOwner ),
	helper_( this->environment() ),
	callbackListHead_()
{
	BW_GUARD;

	callbackListHead_.setAsRoot();
}


/**
 *	Destructor
 */
AvatarFilter::~AvatarFilter()
{
	BW_GUARD;

	while (callbackListHead_.getNext() != &callbackListHead_)
	{
		ListNode * pFirstNode = callbackListHead_.getNext();

		AvatarFilterCallback * pFirstCallback =
			AvatarFilterCallback::getFromListNode( pFirstNode );

		pFirstCallback->removeFromList();

		bw_safe_delete( pFirstCallback );
	}
}


/**
 *	This method invalidates all previously collected inputs. They will
 *	then be discarded by the next input that is received.
 *
 *	@param	time	This value is ignored.
 */
void AvatarFilter::reset( double time )
{
	helper_.reset( time );
}


/**
 *	This method gives the filter a new set of inputs will most likely have
 *	come from the server. If reset() has been called since the last input,
 *	the input history will be wiped and filled with this new value.
 *
 *	@param	time			The estimated client time when the input was sent
 *							from the server.
 *	@param	spaceID			The server space that the position resides in.
 *	@param	vehicleID		The ID of the vehicle in who's coordinate system
 *							the position is defined. A null vehicle ID means
 *							that the position is in world coordinates.
 *	@param	position		The new position in either local vehicle or
 *							world space/common coordinates. The player relative
 *							compression will have already been decoded at this
 *							point by the network layer.
 *	@param	positionError	The amount of uncertainty in the position.
 *	@param	dir				The direction of the entity, in the same
 *							coordinates as pos.
 */
void AvatarFilter::input(	double time,
							SpaceID spaceID,
							EntityID vehicleID,
							const Position3D & position,
							const Vector3 & positionError,
							const Direction3D & direction )
{
	BW_GUARD;
	helper_.input( time, spaceID, vehicleID, position, positionError,
		direction );
}



/**
 *	This method updates an entity's position, velocity, yaw, pitch and
 *	roll to match the estimated values at the time specified.
 *	This function also moves the filter's latency towards its estimated ideal.
 *
 *	@param	time	The client game time in seconds that the entity's volatile
 *					members should be updated for.
 *	@param	target	The entity to be updated
 *
 *	@see	AvatarFilter::extract()
 */
void AvatarFilter::output( double time, MovementFilterTarget & target )
{
	BW_GUARD;
	static DogWatch dwAvatarFilterOutput("AvatarFilter");
	dwAvatarFilterOutput.start();

	Entity & entity = static_cast< Entity & >( target );

	SpaceID		resultSpaceID;
	EntityID	resultVehicleID;
	Position3D	resultPosition;
	Vector3		resultVelocity;
	Direction3D	resultDirection;

	double outputTime = helper_.output( time, resultSpaceID, resultVehicleID,
		resultPosition, resultVelocity, resultDirection );

	target.setSpaceVehiclePositionAndDirection( resultSpaceID,
		resultVehicleID, resultPosition, resultDirection );
	entity.localVelocity( resultVelocity );

	if (entity.waitingForVehicle())
	{
		dwAvatarFilterOutput.stop();
		return;
	}

	// This may override the values set above.
	this->onEntityPositionUpdated( entity );

	dwAvatarFilterOutput.stop();

	// also call any timers that have gone off
	while (callbackListHead_.getNext() != &callbackListHead_)
	{
		ListNode * pFirstNode = callbackListHead_.getNext();

		AvatarFilterCallback * pFirstCallback =
			AvatarFilterCallback::getFromListNode( pFirstNode );

		if (pFirstCallback->targetTime() > outputTime)
		{
			break;
		}

		if ( pFirstCallback->triggerCallback( outputTime ) )
		{
			// TODO: Should we also change outputTime to be targetTime?
			// Otherwise, we'll keep calling future callbacks even though
			// we've popped backwards.
			// Also, perhaps we should adjust filter latency so that it _would_
			// have output this time, since helper_.output( time ) is
			// helper_.extract( time - latency_ ) (and that's what it returns)
			// Also, this pop will only last until next output() call anyway.
			helper_.extract( pFirstCallback->targetTime(), resultSpaceID,
				resultVehicleID, resultPosition, resultVelocity,
				resultDirection );

			target.setSpaceVehiclePositionAndDirection( resultSpaceID,
				resultVehicleID, resultPosition, resultDirection );

			entity.localVelocity( resultVelocity );

			this->onEntityPositionUpdated( entity );
		}

		pFirstCallback->removeFromList();

		bw_safe_delete( pFirstCallback );
	}
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
 *	@return				Returns true if an input existed and the values in the
 *						parameters were set.
 */
bool AvatarFilter::getLastInput(	double & time,
									SpaceID & spaceID,
									EntityID & vehicleID,
									Position3D & pos,
									Vector3 & posError,
									Direction3D & dir ) const
{
	BW_GUARD;

	return helper_.getLastInput( time, spaceID, vehicleID, pos, posError, dir );
}


/**
 *	Add the given AvatarFilterCallback to our callback list. After this, the
 *	callback pointer belongs to this AvatarFilter, and will be deleted when it
 *	is no longer needed.
 */
void AvatarFilter::addCallback( AvatarFilterCallback * pCallback )
{
	// This takes care of inserting in time-order
	pCallback->insertIntoList( &callbackListHead_ );
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

	// Clear any existing callbacks
	while (callbackListHead_.getNext() != &callbackListHead_)
	{
		ListNode * pFirstNode = callbackListHead_.getNext();

		AvatarFilterCallback * pFirstCallback =
			AvatarFilterCallback::getFromListNode( pFirstNode );

		pFirstCallback->removeFromList();

		bw_safe_delete( pFirstCallback );
	}

	// Steal the callbacks from pOtherAvatarFilter
	// We assume the other filter is going away and we want to take over its
	// callbacks. This holds as the only current caller of this code is
	// BWEntity::pFilter() which immediately deletes pOtherAvatarFilter
	ListNode * pNode = pOtherAvatarFilter->callbackListHead_.getNext();

	while (pNode != &pOtherAvatarFilter->callbackListHead_)
	{
		AvatarFilterCallback * pCallback =
			AvatarFilterCallback::getFromListNode( pNode );

		pNode = pNode->getNext();

		pCallback->removeFromList();

		MF_ASSERT( pNode->getPrev() == &pOtherAvatarFilter->callbackListHead_ );

		this->addCallback( pCallback );
	}

	return true;
}


BW_END_NAMESPACE

// avatar_filter.cpp
