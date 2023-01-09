#include "script/first_include.hpp"

#include "base_acceleration_controller.hpp"
#include "entity.hpp"
#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "cell.hpp"
#include "real_entity.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

/**
 *	@enum BaseAccelerationController::Facing
 *
 *	This enumeration is used to describe different direction behaviour of the
 *	movement controller.
 */

/**
 *	@var BaseAccelerationController::Facing BaseAccelerationController::FACING_NONE
 *
 *	This value means that the movement controller will not modify the direction
 *	of the entity.
 */

/**
 *	@var BaseAccelerationController::Facing BaseAccelerationController::FACING_VELOCITY
 *
 *	This value means that the entity will be rotated along the z and x axis to
 *	face in the direction it is heading.
 */

/**
 *	@var BaseAccelerationController::Facing BaseAccelerationController::FACING_ACCELERATION
 *
 *	This value means that the entity will be rotated along the z and x axis to
 *	face in the direction it is accelerating.
 */




/**
 *	@property	float BaseAccelerationController::acceleration_
 *
 *	This member holds the speed at which the entity will accelerate.\n
 *	units/second^2
 */

/**
 *	@property	float BaseAccelerationController::maxSpeed_
 *
 *	This member holds the speed at which the entity will accelerate.\n
 *	units/second^2
 */

/**
 *	@property	Facing BaseAccelerationController::facing_
 *
 *	The direction in which to face the entity while it is being moved.
 */




//-----------------------------------------------------------------------------
// Section: BaseAccelerationController
//-----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param acceleration Velocity in metres per second
 *  @param maxSpeed			The speed at which acceleration should stop.
 *  @param facing			Enumerated value indicating which direction to
 *                          face while accelerating.
 */
BaseAccelerationController::BaseAccelerationController(	
													float acceleration,
													float maxSpeed,
													Facing facing) :
	acceleration_( acceleration ),
	maxSpeed_( maxSpeed ),
	facing_( facing ),
	currVelocity_( 0.f, 0.f, 0.f ),
	pVehicle_( NULL )
{

}


/**
 *	This method overrides the Controller method.
 */
void BaseAccelerationController::startReal( bool /*isInitialStart*/ )
{
	MF_ASSERT( entity().isReal() );
	CellApp::instance().registerForUpdate( this );

	pVehicle_ = this->entity().pVehicle();
	if (pVehicle_)
	{
		//restart to sample position
		positionSample_ = this->entity().localPosition();
		positionSampleTime_ = CellApp::instance().time();
	}
}


/**
 *	This method overrides the Controller method.
 */
void BaseAccelerationController::stopReal( bool /*isFinalStop*/ )
{
	MF_VERIFY( CellApp::instance().deregisterForUpdate( this ) );
}


/**
 *	Private method to calculate the current velocity in local coordinate.
 */
void BaseAccelerationController::refreshCurrentVelocity()
{
	Entity * pVehicle = this->entity().pVehicle();
	bool changedVehicle = (pVehicle_ != pVehicle);

	if (changedVehicle)
		pVehicle_ = pVehicle;

	if (!pVehicle)
	{
		currVelocity_ = this->entity().pReal()->velocity();
	}
	else if (!changedVehicle)
	{

		GameTime now = CellApp::instance().time();
		uint32 dur = now - positionSampleTime_;
		if (dur > 1)
		{   
			Vector3 position = this->entity().localPosition();
			float tinv = float(CellAppConfig::updateHertz()) / float(dur);
			currVelocity_ = ( position - positionSample_) * tinv;

			positionSample_ = position; 
			positionSampleTime_ = now;
		}   
	}
	else
	{

		positionSample_ = this->entity().localPosition();
		positionSampleTime_ = CellApp::instance().time();
		currVelocity_ = Vector3( 0.f, 0.f, 0.f );
	}   

}

/**
 *	This method moves the entity towards the destination by metresPerTick_, in
 *	local space.
 *
 *	@param destination			The position to move to, in local space.
 *	@param stopAtDestination	Should velocity be zero upon reaching the
 *								destination.
 */
bool BaseAccelerationController::move(	const Position3D & destination,
										bool stopAtDestination )
{
	MF_ASSERT( this->entity().pReal() );

	this->refreshCurrentVelocity();

	Position3D position = this->entity().localPosition();


	Direction3D direction = this->entity().localDirection();
	const Vector3 currentVelocity = this->currVelocity();


	Vector3 desiredVelocity = calculateDesiredVelocity(	position,
														destination,
														acceleration_, 
														maxSpeed_,
														stopAtDestination);

	if (desiredVelocity.length() > maxSpeed_)
	{
		desiredVelocity.normalise();
		desiredVelocity *= maxSpeed_;
	}

	const Vector3 accelerationVector = calculateAccelerationVector(	
															currentVelocity,
															desiredVelocity);

	const Vector3 destinationVector( destination - position );
	const Vector3 velocityDifference = currentVelocity - desiredVelocity;
	const float accelerationPerTick = acceleration_ /
										CellAppConfig::updateHertz();


	Vector3 newVelocity;

	if (velocityDifference.length() > accelerationPerTick)
	{
		newVelocity = currentVelocity + 
						(accelerationVector * accelerationPerTick);
	}
	else
	{
		newVelocity = desiredVelocity;
	}

	// If we are real close, then finish up.
	const float stepDistance =	newVelocity.length() /
								CellAppConfig::updateHertz();
	if (stepDistance > destinationVector.length())
	{
		position = destination;
	}
	else
	{
		position += newVelocity / CellAppConfig::updateHertz();
	}


	// Make sure we are facing the right direction.
	if (facing_ != FACING_NONE)
	{
		Vector3 facingVector(0, 0, 0);

		if (facing_ == FACING_VELOCITY &&
			newVelocity.lengthSquared() > 0.0f)
		{
			facingVector = newVelocity.unitVector();
		}
		else if (facing_ == FACING_ACCELERATION)
		{
			// Avoid facing in odd angles when our current velocity is almost
			// perfect.
			if (velocityDifference.length() > accelerationPerTick)
			{
				facingVector = accelerationVector;
			}
			else
			{
				facingVector = newVelocity.unitVector();
			}
		}


		if (!isZero( facingVector.x ) || !isZero( facingVector.z ))
		{
			direction.yaw = facingVector.yaw();
		}

		if (!isZero( facingVector.y ))
		{
			direction.pitch = facingVector.pitch();
		}
	}


	// Keep ourselves alive until we have finished cleaning up,
	// with an extra reference count from a smart pointer.
	ControllerPtr pController = this;

	// No longer on the ground...
	// Might want to make this changeable from script
	// for entities that want to be on the ground.
	this->entity().isOnGround( false );
	this->entity().setPositionAndDirection( position, direction );

	if (!this->isAttached())
		return false;


	if (stopAtDestination)
	{
		return	almostEqual(position, destination) &&
				almostZero(currentVelocity.lengthSquared());
	}
	else
	{
		return almostEqual(position, destination);
	}
}


/**
 *	This function calculates the desired velocity to reach a given destination.
 *
 *	@param	currentPosition		The position of the entity at the current time.
 *	@param	desiredPosition		The destination the entity is trying to reach.
 *	@param	acceleration		The speed at which the entity changes
 *								velocity in units/second^2
 *	@param	maxSpeed			The maximum velocity the entity wishes to
 *								travel.
 *	@param	stopAtDestination	Defines whether the entity wishes to have a
 *								speed of zero upon reaching its destination.
 *
 *	@return		The estimated desirable speed of the entity in order for it to
 *				reach its destination.
 */
Vector3 BaseAccelerationController::calculateDesiredVelocity(	
											const Position3D & currentPosition,
											const Position3D & desiredPosition,
											float acceleration, 
											float maxSpeed,
											bool stopAtDestination )
{
	Vector3 destinationVector( desiredPosition - currentPosition );
	Vector3 desiredVelocity( destinationVector.unitVector() );

	if (stopAtDestination)
	{
		// This is the speed that would be reached by an object that
		// accelerated from zero over the distance from the destination. 
		float speedAtDistance = sqrtf(	2.0f *
										destinationVector.length() *
										acceleration);

		desiredVelocity *=  std::min(maxSpeed, speedAtDistance);
	}
	else
	{
		desiredVelocity *= maxSpeed;
	}

	return desiredVelocity;
}


/**
 *	This function calculates the direction in which acceleration should be
 *	applied in order to move the current velocity toward that desired.
 *
 *	@param	currentVelocity		The current velocity of the entity.
 *	@param	desiredVelocity		The velocity it is desired for the entity to
 *								reach.
 *
 *	@return		The direction in which acceleration should be applied in order
 *				to reach the desired velocity.
 */
Vector3 BaseAccelerationController::calculateAccelerationVector(
											const Vector3 & currentVelocity,
											const Vector3 & desiredVelocity )
{
	if (almostEqual( currentVelocity, desiredVelocity ))
	{
		return desiredVelocity.unitVector();
	}

	if (almostZero( desiredVelocity.lengthSquared() ))
	{
		return -currentVelocity.unitVector();
	}

	Vector3 accelerationVector;
	if (currentVelocity.lengthSquared() > 0.0f)
	{
		Vector3 unitCurrentVelocity( currentVelocity.unitVector() );

		Vector3 velocityDifference = desiredVelocity - currentVelocity;

		accelerationVector.projectOnto( velocityDifference, desiredVelocity );

		accelerationVector += 2.0f * (velocityDifference - accelerationVector);
	}
	else
	{
		accelerationVector = desiredVelocity;
	}

	return accelerationVector.unitVector();
}


/**
 *	This method writes our state to a stream.
 *
 *	@param stream		Stream to which we should write
 */
void BaseAccelerationController::writeRealToStream(BinaryOStream& stream)
{
	this->Controller::writeRealToStream(stream);
	stream << acceleration_ << maxSpeed_ << facing_ << currVelocity_;
}


/**
 *	This method reads our state from a stream.
 *
 *	@param stream		Stream from which to read
 *
 *	@return				true if successful, false otherwise
 */
bool BaseAccelerationController::readRealFromStream( BinaryIStream & stream )
{
	this->Controller::readRealFromStream( stream );
	stream >> acceleration_ >> maxSpeed_ >> facing_ >> currVelocity_;
	return true;
}

BW_END_NAMESPACE

// base_acceleration_controller.cpp
