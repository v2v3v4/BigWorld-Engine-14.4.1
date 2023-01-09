#include "pch.hpp"

#include "avatar_filter_helper.hpp"
#include "filter_environment.hpp"

#include "cstdmf/guard.hpp"
#include "cstdmf/watcher.hpp"

#include "math/angle.hpp"
#include "math/boundbox.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: AvatarFilterSettings
// -----------------------------------------------------------------------------

/**
 *	The speed in seconds per second that the avatar filters output method
 *	should adjust its latency offset.\n
 *	@note	This setting is exposed through the watcher
 *			"Comms/Latency Velocity"
 *
 *	@note	This value is scaled by \f$(latency-desiredLatency)^2\f$ before its
 *			application.
 */
float AvatarFilterSettings::s_latencyVelocity_ = 1.00f;


/**
 *	The minimum ideal latency in seconds\n
 *	@note	This setting is exposed through the watcher
 *			"Comms/Minimum Latency"
 *
 */
float AvatarFilterSettings::s_latencyMinimum_ = 0.10f;


/**
 *	The ideal latency measured in intervals of input packets.\n
 *	@note	This setting is exposed through the watcher
 *			"Comms/Ideal Latency Frames"
 */
float AvatarFilterSettings::s_latencyFrames_ = 2.0f;


/**
 *	The power used to scale the latency velocity
 *	\f$\left|latency - idealLatency\right|^power\f$.\n
 *
 *	@note	This setting is exposed through the watcher
 *			"Comms/Latency Speed Curve Power"
 */
float AvatarFilterSettings::s_latencyCurvePower_ = 2.0f;

/**
 *	This static member stores the active state of the AvatarFilter class.
 *	When the AvatarFilter is inactive (false) all AvatarFilters behave
 *	like the DumbFilter, returning their most recently received input.
 */
bool AvatarFilterSettings::s_isActive_ = true;

namespace
{

/**
 *	This type creates a number of watchers during its construction. A single
 *	static instance (s_avf_initer) is then used to then initiate their creation
 *	during program start-up.
 */
class AvatarFilterWatcherCreator
{
public:
	AvatarFilterWatcherCreator()
	{
		BW_GUARD;

		MF_WATCH( "Client Settings/Filters/Latency Velocity",
			AvatarFilterSettings::s_latencyVelocity_,
			Watcher::WT_READ_WRITE,
			"The speed at which the latency changes in seconds per second" );

		MF_WATCH( "Client Settings/Filters/Minimum Latency",
			AvatarFilterSettings::s_latencyMinimum_,
			Watcher::WT_READ_WRITE, "The minimum ideal latency in seconds" );

		MF_WATCH( "Client Settings/Filters/Ideal Latency Frames",
			AvatarFilterSettings::s_latencyFrames_,
			Watcher::WT_READ_WRITE,
			"The ideal latency in update frequency (0.0-4.0)" );

		MF_WATCH( "Client Settings/Filters/Latency Speed Curve Power",
			AvatarFilterSettings::s_latencyCurvePower_,
			Watcher::WT_READ_WRITE,
			"The power used to scale the latency velocity "
				"|latency - idealLatency|^power" );

		MF_WATCH( "Client Settings/Filters/Enabled",
			AvatarFilterSettings::s_isActive_,
			Watcher::WT_READ_WRITE,
			"Enable or disable entity position historical filtering" );
	}
};

AvatarFilterWatcherCreator s_avf_initer;
}



///////////////////////////////////////////////////////////////////////////////
///  Member Documentation for AvatarFilter
///////////////////////////////////////////////////////////////////////////////


/**
 *	@property	float AvatarFilterHelper::latency_
 *
 *	This is the offset applied to client time when updating the entity.\n
 *	This latency is applied prevent the filter hitting the front of the input
 *	history. Over time the latency_ is moved closer to the idealLatency_
 *	derived from the frequency of received inputs.
 *
 *	@see	AvatarFilter::output()
 *	@see	AvatarFilter::idealLatency_
 *	@see	AvatarFilter::s_latencyVelocity_;
 *	@see	AvatarFilter::s_latencyCurvePower_;
 */


/**
 *	@property	float AvatarFilterHelper::idealLatency_
 *
 *	This member holds the estimated ideal latency based on the frequency of
 *	inputs.\n
 *	It is recalculated after the arrival of new input using the period of time
 *	represented by the last four inputs.
 *
 *	@see	AvatarFilter::output()
 *	@see	AvatarFilter::s_latencyMinimum_
 *	@see	AvatarFilter::s_latencyFrames_
 */


/**
 *	@property	bool AvatarFilterHelper::gotNewInput_
 *
 *	This member is used to notify the output method that new input was
 *	received.
 */


/**
 *	@property	bool AvatarFilterHelper::reset_
 *
 *	This member is used to flag to the input method that the filter has been
 *	reset and that it should reinitialise the history upon arrival of next
 *	input.
 *
 *	@see	AvatarFilter::reset()
 *	@see	AvatarFilter::input()
 */


///////////////////////////////////////////////////////////////////////////////
/// End Member Documentation for AvatarFilter
///////////////////////////////////////////////////////////////////////////////


/**
 *	Constructor
 *
 *	@param	environment	The environment filter implementation to use with this
 *						filter.
 *	@param	pOtherHelper Another AvatarFilterHelper from which to take some
 *						 initial data from to seed this filters behaviour.
 */
AvatarFilterHelper::AvatarFilterHelper( FilterEnvironment & environment,
			const AvatarFilterHelper * pOtherHelper ) :
		FilterHelper( environment ),
		currentInputIndex_( 0 ),
		inputCount_( 0 ),
		latency_( 0 ),
		idealLatency_( 0 ),
		timeOfLastOutput_( 0 ),
		gotNewInput_( false ),
		reset_( true )
{
	BW_GUARD;

	this->init( pOtherHelper );
}


/**
 *	Copy constructor.
 */
AvatarFilterHelper::AvatarFilterHelper( const AvatarFilterHelper & other ):
		FilterHelper( *(other.pEnvironment_) ),
		currentInputIndex_( other.currentInputIndex_ ),
		inputCount_( other.inputCount_ ),
		latency_( other.latency_ ),
		idealLatency_( other.idealLatency_ ),
		timeOfLastOutput_( other.timeOfLastOutput_ ),
		gotNewInput_( other.gotNewInput_ ),
		reset_( other.reset_ )
{
	this->init( &other );
}


/**
 *	Assignment operator
 */
AvatarFilterHelper & AvatarFilterHelper::operator= (
	const AvatarFilterHelper & other )
{
	// Ignore self-assignment
	if (&other == this)
	{
		return *this;
	}

	this->FilterHelper::operator=( other );

	currentInputIndex_ = other.currentInputIndex_;
	inputCount_ = other.inputCount_;
	latency_ = other.latency_;
	idealLatency_ = other.idealLatency_;
	timeOfLastOutput_ = other.timeOfLastOutput_;
	gotNewInput_ = other.gotNewInput_;
	reset_ = other.reset_;

	this->init( &other );

	return *this;
}


/**
 *	This method initialises the helper with another helper.
 */
void AvatarFilterHelper::init( const AvatarFilterHelper * pHelper )
{
	if (pHelper)
	{
		for (uint i = 0; i < NUM_STORED_INPUTS; ++i)
		{
			storedInputs_[i] = pHelper->storedInputs_[i];
		}

		currentInputIndex_ = pHelper->currentInputIndex_;

		nextWaypoint_ = pHelper->nextWaypoint_;
		previousWaypoint_ = pHelper->previousWaypoint_;
	}
	else
	{
		this->resetStoredInputs( -2000, 0, 0, Position3D::zero(),
			Vector3::zero(), Direction3D( Vector3::zero() ) );
		this->reset( 0 );
	}
}


/**
 *	This method invalidates all previously collected inputs. They will
 *	then be discarded by the next input that is received.
 *
 *	@param	time	This value is ignored.
 */
void AvatarFilterHelper::reset( double time )
{
	reset_ = true;
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
 *	@param	direction		The direction of the entity, in the same
 *							coordinates as the position.
 */
void AvatarFilterHelper::input(	double time, SpaceID spaceID,
		EntityID vehicleID, const Position3D & position,
		const Vector3 & positionError, const Direction3D & direction )
{
	BW_GUARD;

	if (reset_)
	{
		this->resetStoredInputs( time, spaceID, vehicleID, position,
			positionError, direction );
		reset_ = false;
		return;
	}

	if (time <= this->getStoredInput( 0 ).time_)
	{
		return;
	}

	currentInputIndex_ = (currentInputIndex_ + NUM_STORED_INPUTS - 1) %
		NUM_STORED_INPUTS;

	StoredInput & storedInput = this->getStoredInput( 0 );

	storedInput.time_ = time;
	storedInput.spaceID_ = spaceID;
	storedInput.vehicleID_ = vehicleID;
	storedInput.position_ = position;
	storedInput.positionError_ = positionError;
	storedInput.direction_ = direction;

	this->resolveOnGroundPosition( storedInput.position_,
		storedInput.onGround_ );

	gotNewInput_ = true;
	++inputCount_;
}



/**
 *	This method updates the slave entity's position, velocity, yaw, pitch and
 *	roll to match the estimated values at the time specified.
 *	This function also moves the filter's latency towards its estimated ideal.
 *
 *	@param time			The client game time in seconds that the entity's
 *						volatile members should be updated for.
 *	@param rSpaceID		The filtered space ID.
 *	@param rVehicleID	The filtered vehicle ID.
 *	@param rPosition	The filtered position.
 *	@param rVelocity	The filtered velocity.
 *	@param rDirection	The filtered direction, in yaw-pitch-roll order.
 *
 *	@see AvatarFilter::extract()
 */

double AvatarFilterHelper::output( double time, SpaceID & rSpaceID,
		EntityID & rVehicleID, Position3D & rPosition, Vector3 & rVelocity,
		Direction3D & rDirection )
{
	// Adjust ideal latency if we got something new.
	if (gotNewInput_)
	{
		gotNewInput_ = false;

		const int maxLatencyFrame = std::min( inputCount_, (uint)NUM_STORED_INPUTS ) - 1;

		if (maxLatencyFrame > 0)
		{
			const double newestTime = this->getStoredInput( 0 ).time_;
			const double olderTime = this->getStoredInput( maxLatencyFrame ).time_;

			float latencyFrames = AvatarFilterSettings::s_latencyFrames_;
			latencyFrames =
				Math::clamp( 0.0f, latencyFrames, float( maxLatencyFrame ) );

			double ratio = (maxLatencyFrame - latencyFrames) / (double)maxLatencyFrame;

			idealLatency_ = float( time - Math::lerp( ratio, olderTime, newestTime ) );
		}

		// idealLatency cannot be less than s_latencyMinimum_ (usually set to 1 frame)
		idealLatency_ = std::max( idealLatency_, AvatarFilterSettings::s_latencyMinimum_ );
	}

	// Move latency towards the ideal.
	float dTime = float(time - timeOfLastOutput_);
	float dLatency = (AvatarFilterSettings::s_latencyVelocity_ * dTime) *
		std::min( 1.0f,
			powf( fabsf( idealLatency_ - latency_ ),
				AvatarFilterSettings::s_latencyCurvePower_ ) );

	if (idealLatency_ > latency_)
	{
		latency_ += dLatency;
		latency_ = std::min( latency_, idealLatency_ );
	}
	else
	{
		latency_ -= dLatency;
		latency_ = std::max( latency_, idealLatency_ );
	}


	// Record this so we can move latency at a velocity independent of the
	// number of times we're called.
	timeOfLastOutput_ = time;

	// find the position at 'time - latency'
	double outputTime = time - latency_;

	this->extract( outputTime, rSpaceID, rVehicleID, rPosition, rVelocity,
		rDirection );

	return outputTime;
}


/**
 *	This method changes the coordinate system of the waypoint by first
 *	transforming into common coordinates and then into the new coordinates.
 *	spaceID_ and vehicleID_ are also set to that of the new coordinate system.
 */
void AvatarFilterHelper::Waypoint::changeCoordinateSystem(
		FilterEnvironment & environment, SpaceID spaceID,
		EntityID vehicleID )
{
	BW_GUARD;

	if (spaceID_ == spaceID && vehicleID_ == vehicleID)
	{
		return;
	}

	environment.transformIntoCommon( spaceID_, vehicleID_, position_,
		direction_ );

	spaceID_ = spaceID;
	vehicleID_ = vehicleID;

	environment.transformFromCommon( spaceID_, vehicleID_, position_,
		direction_ );
}


void AvatarFilterHelper::resetStoredInputs(	double time,
		SpaceID spaceID, EntityID vehicleID, const Position3D & position,
		const Vector3 & positionError, const Direction3D & direction )
{
	BW_GUARD;
	const float NONZERO_TIME_DIFFERENCE = 0.01f;

	currentInputIndex_ = 0;
	inputCount_ = 0;
	gotNewInput_ = true;
	timeOfLastOutput_ = time;

	bool onGround = false;
	Position3D correctedPosition = position;

	this->resolveOnGroundPosition( correctedPosition, onGround );

	for (uint i = 0; i < NUM_STORED_INPUTS; ++i)
	{
		StoredInput & storedInput = this->getStoredInput( i );

		// set times of older inputs as to avoid zero time differences
		storedInput.time_ = time - (i * NONZERO_TIME_DIFFERENCE);

		storedInput.spaceID_ = spaceID;
		storedInput.vehicleID_ = vehicleID;
		storedInput.position_ = correctedPosition;
		storedInput.positionError_ = positionError;
		storedInput.direction_ = direction;
		storedInput.onGround_ = onGround;
	}

	latency_ = AvatarFilterSettings::s_latencyFrames_ * NONZERO_TIME_DIFFERENCE;

	nextWaypoint_.time_ = time - NONZERO_TIME_DIFFERENCE;
	nextWaypoint_.spaceID_ = spaceID;
	nextWaypoint_.vehicleID_ = vehicleID;
	nextWaypoint_.position_ = position;
	nextWaypoint_.direction_ = direction;

	previousWaypoint_ = nextWaypoint_;
	previousWaypoint_.time_ -= NONZERO_TIME_DIFFERENCE;
}



/**
 *	This method 'extracts' a set of filtered values from the input history,
 *	clamping at the extremes of the time period stored. In the case that
 *	requested time falls between two inputs a weighed blend is performed
 *	taking into account vehicle transitions.
 *	A small amount of speculative movement supported when the most recent
 *	value in the history is older than the time requested.
 *
 *	@param	time			The client time stamp of the values required.
 *	@param	outputSpaceID	The resultant space ID
 *	@param	outputVehicleID The resultant vehicle ID
 *	@param	outputPosition	The estimated position.
 *	@param	outputVelocity	The estimated velocity of the entity at the time
 *							specified.
 *	@param	outputDirection	The estimated yaw and pitch of the entity, as
 *							a Direction3D. The roll is always given as 0.
 */
void AvatarFilterHelper::extract( double time, SpaceID & outputSpaceID,
		EntityID & outputVehicleID, Position3D & outputPosition,
		Vector3 & outputVelocity, Direction3D & outputDirection )
{
	BW_GUARD;

	if (!AvatarFilterHelper::isActive())
	{
		const StoredInput & mostRecentInput = this->getStoredInput( 0 );

		outputSpaceID = mostRecentInput.spaceID_;
		outputVehicleID = mostRecentInput.vehicleID_;
		outputPosition = mostRecentInput.position_;
		outputDirection = mostRecentInput.direction_;
		outputVelocity.setZero();

		return;
	}

	if (time > nextWaypoint_.time_)
	{
		this->chooseNextWaypoint( time );
	}

	float proportionateDifferenceInTime =
		float((time - previousWaypoint_.time_ ) /
			(nextWaypoint_.time_ - previousWaypoint_.time_));

	outputSpaceID = nextWaypoint_.spaceID_;
	outputVehicleID = nextWaypoint_.vehicleID_;
	outputPosition.lerp( previousWaypoint_.position_, nextWaypoint_.position_,
		proportionateDifferenceInTime );

	outputVelocity = (nextWaypoint_.position_ - previousWaypoint_.position_) /
		float( nextWaypoint_.time_- previousWaypoint_.time_ );

	Angle yaw;
	yaw.lerp( previousWaypoint_.direction_.yaw,
		nextWaypoint_.direction_.yaw,
		proportionateDifferenceInTime );

	Angle pitch;
	pitch.lerp( previousWaypoint_.direction_.pitch,
		nextWaypoint_.direction_.pitch,
		proportionateDifferenceInTime );

	outputDirection.yaw = yaw;
	outputDirection.pitch = pitch;
	outputDirection.roll = 0.f;
}


/**
 *	This internal method choses a new set of waypoints to traverse based on the
 *	history of stored input and the requested time. A few approaches are used
 *	depending on the number of received inputs ahead of the requested time.
 *
 *	Two inputs ahead of time
 *	A vector is made from head of the previous waypoints to the centre of the
 *	input two ahead. The point on this vector that exists one input ahead in
 *	time is then found and its position clamped to the box of error tolerance
 *	of that same input. This point forms the new head waypoint and the previous
 *	becomes the new tail.
 *
 *	Only one input ahead of time
 *	The current pair of waypoints are projected into the future to the time of
 *	the next input ahead. The resultant position is then clamped to the box of
 *	error tolerance of that input.
 *
 *	No inputs ahead of time
 *	In the event no inputs exist ahead of the time both waypoints are set to
 *	the same position. The entity will stand still until an input is received
 *	that is ahead of game time minus latency.
 *
 *	Note: Both waypoints are always in the same coordinate system; that of the
 *	next input ahead.
 *
 *	@param time	The time which the new waypoints should enclose
 */
void AvatarFilterHelper::chooseNextWaypoint( double time )
{
	BW_GUARD;

	Waypoint & previousWaypoint = previousWaypoint_;
	Waypoint & currentWaypoint = nextWaypoint_;
	Waypoint newWaypoint;

	if (this->getStoredInput( 0 ).time_ < time)
	{
		// In the event there is no more input data, stand still for one frame.
		previousWaypoint_ = nextWaypoint_;
		nextWaypoint_.time_ = time;
		return;
	}

	int nextInputIndex = (NUM_STORED_INPUTS - 1);
	while (nextInputIndex > 0)
	{
		if (this->getStoredInput( nextInputIndex ).time_ > time)
		{
			break;
		}
		--nextInputIndex;
	}

	const StoredInput & lookAheadInput = this->getStoredInput( 0 );
	const StoredInput & nextInput = this->getStoredInput( nextInputIndex );

	newWaypoint.time_ = nextInput.time_;
	newWaypoint.spaceID_ = nextInput.spaceID_;
	newWaypoint.vehicleID_ = nextInput.vehicleID_;
	newWaypoint.direction_ = nextInput.direction_;

	newWaypoint.storedInput_ = nextInput;

	previousWaypoint.changeCoordinateSystem( this->environment(),
		newWaypoint.spaceID_,
		newWaypoint.vehicleID_ );

	currentWaypoint.changeCoordinateSystem(	this->environment(),
		newWaypoint.spaceID_,
		newWaypoint.vehicleID_ );

	float lookAheadRelativeDifferenceInTime =
		float((lookAheadInput.time_ - previousWaypoint.time_) /
			(currentWaypoint.time_ - previousWaypoint.time_) );

	Position3D lookAheadPosition;
	lookAheadPosition.lerp(	previousWaypoint.position_,
							currentWaypoint.position_,
							lookAheadRelativeDifferenceInTime );

	Direction3D unusedDirection( Vector3::zero() );

	this->transformIntoCommon( newWaypoint.spaceID_,
		newWaypoint.vehicleID_,	lookAheadPosition,
		unusedDirection );

	this->transformFromCommon( lookAheadInput.spaceID_,
		lookAheadInput.vehicleID_, lookAheadPosition,
		unusedDirection );

	lookAheadPosition.clamp(
		lookAheadInput.position_ - lookAheadInput.positionError_,
		lookAheadInput.position_ + lookAheadInput.positionError_ );

	this->transformIntoCommon( lookAheadInput.spaceID_,
		lookAheadInput.vehicleID_, lookAheadPosition,
		unusedDirection );

	this->transformFromCommon( newWaypoint.spaceID_,
		newWaypoint.vehicleID_, lookAheadPosition,
		unusedDirection );

	// Handle overlapping error rectangles
	BoundingBox newWaypointBB(
		newWaypoint.storedInput_.position_ -
			newWaypoint.storedInput_.positionError_,
		newWaypoint.storedInput_.position_ +
			newWaypoint.storedInput_.positionError_);

	BoundingBox currentWaypointBB(
		currentWaypoint.storedInput_.position_ -
			currentWaypoint.storedInput_.positionError_,
		currentWaypoint.storedInput_.position_ +
			currentWaypoint.storedInput_.positionError_ );

	if ((newWaypoint.spaceID_ ==
				currentWaypoint.storedInput_.spaceID_) &&
			(newWaypoint.vehicleID_ ==
				currentWaypoint.storedInput_.vehicleID_) &&
			!almostEqual( newWaypoint.storedInput_.positionError_,
				currentWaypoint.storedInput_.positionError_) &&
			newWaypointBB.intersects( currentWaypointBB ))
	{
		// Remain still if the previous move was only to adjust for
		// changes in position error (ie overlapping error
		// regions).
		newWaypoint.position_ = currentWaypoint.position_;
	}
	else
	{
		float proportionateDifferenceInTime =
			float( (nextInput.time_ - currentWaypoint.time_) /
				(lookAheadInput.time_ - currentWaypoint.time_) );

		newWaypoint.position_.lerp(	currentWaypoint.position_,
			lookAheadPosition,
			proportionateDifferenceInTime );
	}

	// Constrain waypoint position to its input error rectangle
	BoundingBox nextInputBB(
		nextInput.position_ - nextInput.positionError_,
		nextInput.position_ + nextInput.positionError_ );

	if (!nextInputBB.intersects(newWaypoint.position_))
	{
		Position3D clampedPosition( newWaypoint.position_ );

		clampedPosition.clamp(
			nextInput.position_ - nextInput.positionError_,
			nextInput.position_ + nextInput.positionError_ );

		Vector3 lookAheadVector = newWaypoint.position_ -
			currentWaypoint.position_;

		Vector3 clampedVector = clampedPosition - currentWaypoint.position_;

		if (lookAheadVector.lengthSquared() > 0.0f)
		{
			newWaypoint.position_ = currentWaypoint.position_ +
				clampedVector.projectOnto( lookAheadVector );
		}
		else
		{
			newWaypoint.position_ = currentWaypoint.position_;
		}

		newWaypoint.position_.clamp(
			nextInput.position_ - nextInput.positionError_,
			nextInput.position_ + nextInput.positionError_ );
	}

	previousWaypoint_ = currentWaypoint;
	nextWaypoint_ = newWaypoint;
}


/**
 *	This function returns a reference to one of the stored input structures
 *
 *	@param index	The index of the required input. Zero being the newest and
 *					NUM_STORED_INPUTS - 1 being the oldest
 */
AvatarFilterHelper::StoredInput & AvatarFilterHelper::getStoredInput(
		uint index )
{
	BW_GUARD;
	MF_ASSERT( index < NUM_STORED_INPUTS );

	return storedInputs_[(currentInputIndex_ + index) % NUM_STORED_INPUTS];
}


/**
 *	This function returns a reference to one of the stored input structures
 *
 *	@param index	The index of the required input. Zero being the newest and
 *					NUM_STORED_INPUTS - 1 being the oldest
 */
const AvatarFilterHelper::StoredInput & AvatarFilterHelper::getStoredInput(
		uint index ) const
{
	BW_GUARD;
	return const_cast<AvatarFilterHelper*>(this)->getStoredInput( index );
}


/**
 *	This function gets the last input received by the filter.
 *	Its arguments will be untouched if no input has yet been received.
 *
 *	@param	time		Will be set to the time stamp of the last input.
 *	@param	spaceID		Will be set to the space ID of the last input.
 *	@param	vehicleID	Will be set to the vehicle ID provided in the last
 *						input.
 *	@param	position	The position received in the last input, with its
 *						height fully resolved.
 *	@param	positionError	The amount of uncertainty in the position.
 *	@param	direction	The direction received in the last input.
 *
 *	@return				Returns true if an input existed and the values in the
 *						parameters were set.
 */
bool AvatarFilterHelper::getLastInput( double & time,
		SpaceID & spaceID, EntityID & vehicleID, Position3D & position,
		Vector3 & positionError, Direction3D & direction ) const
{
	BW_GUARD;
	if (!reset_)
	{
		const StoredInput & storedInput = this->getStoredInput( 0 );
		time = storedInput.time_;
		spaceID = storedInput.spaceID_;
		vehicleID = storedInput.vehicleID_;
		position = storedInput.position_;
		positionError = storedInput.positionError_;
		direction = storedInput.direction_;
	}
	return !reset_;
}


/**
 *	Queries if the AvatarFilter class is 'active'. In the case that it is not,
 *	all AvatarFilters should behave like the DumbFilter and simply return their
 *	most recently received input.
 *	This is part of the filter demonstration accessed using DEBUG-H in
 *	fantasydemo.
 *
 *	@return	Returns true if the AvatarFilter class is currently active.
 */
/*static*/ bool AvatarFilterHelper::isActive()
{
	return AvatarFilterSettings::s_isActive_;
}


/**
 *	This function sets the AvatarFilter class to active or inactive. When
 *	AvatarFilter is 'inactive' all AvatarFilters behave like the DumbFilter
 *	and simply return their most recently received input.
 *	This is part of a demonstration of the filters accessed using
 *	DEBUG-H in fantasydemo.
 *
 *	@param	value	The new state of the AvatarFilter class. 'true' == active
 */
/*static*/ void AvatarFilterHelper::isActive( bool value )
{
	AvatarFilterSettings::s_isActive_ = value;
}

BW_END_NAMESPACE

// avatar_filter_helper.cpp

