#include "script/first_include.hpp"

#include "move_controller.hpp"

#include "entity.hpp"
#include "entity_navigate.hpp"
#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "cell.hpp"
#include "passengers.hpp"
#include "real_entity.hpp"
#include "space.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

/*~ callback Entity.onMove
 *  @components{ cell }
 *	This method is related to the navigation methods Entity.accelerateToEntity,
 *	Entity.accelerateToPoint, Entity.moveToPoint, Entity.moveToEntity,
 *	Entity.navigateFollow and Entity.navigateStep.
 *
 *	It is called when the entity has reached its destination or
 *	for Entity.navigateStep when the entity reaches the next waypoint.
 *	@param controllerID The id of the controller that triggered this callback.
 *	@param userData The user data that was passed to the navigation method
 */
/*~ callback Entity.onMoveFailure
 *  @components{ cell }
 *	This method is related to the navigation methods Entity.accelerateToEntity,
 *	Entity.moveToEntity and Entity.navigateStep methods.
 *
 *	It is called if the entity fails to reach its destination entity due to 
 *	it no longer being avaliable, and is also called when the entity has not
 *	moved enough during a single tick from Entity.navigateStep.
 *
 *	@see accelerateToEntity
 *	@see moveToEntity
 *	@see navigateStep
 *
 *	@param controllerID The id of the controller that triggered this callback.
 *	@param userData The user data that was passed to the navigation method
 *
 */


// -----------------------------------------------------------------------------
// Section: MoveController
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param velocity			Velocity in metres per second
 *	@param faceMovement		Whether the entity should face the direction of
 *							movement.
 *	@param moveVertically	Indicates whether or not the entity should be moved
 *							vertically (i.e. along the y-axis).
 */
MoveController::MoveController( float velocity, bool faceMovement,
		bool moveVertically ):
	faceMovement_( faceMovement ),
	moveVertically_( moveVertically )
{
	metresPerTick_ = velocity / CellAppConfig::updateHertz();
}


/**
 *	This method moves the entity towards the destination by metresPerTick_,
 *	the destination is in local space.
 *
 *	@param destination The position to move to.
 *	@param pRemainingTicks The number of remaining ticks (if provided).
 *
 */
bool MoveController::move( const Position3D & destination,
						  float * pRemainingTicks )
{
	Position3D position = this->entity().localPosition();
	Direction3D direction = this->entity().localDirection();
	float metresToGo = metresPerTick_;

	if (pRemainingTicks)
	{
		metresToGo = *pRemainingTicks * metresPerTick_;
	}

	Vector3 movement = destination - position;
	if (!moveVertically_)
	{
		movement.y = 0.f;
	}

	// If we are real close, then finish up.

	if(movement.length() < metresToGo)
	{
		position = destination;

		if (pRemainingTicks)
		{
			*pRemainingTicks *= ( metresToGo - movement.length() ) / metresPerTick_;
		}
	}
	else
	{
		movement.normalise();
		movement *= metresToGo;
		position += movement;

		if (pRemainingTicks)
		{
			*pRemainingTicks = 0.f;
		}
	}

	// Make sure we are facing the right direction.

	if (faceMovement_)
	{
		if (!isZero( movement.x ) || !isZero( movement.z ))
		{
			direction.yaw = movement.yaw();
		}
		if (!isZero( movement.y ))
		{
			direction.pitch = movement.pitch();
		}
	}

	// Keep ourselves alive until we have finished cleaning up,
	// with an extra reference count from a smart pointer.
	ControllerPtr pController = this;

	if (CellAppConfig::shouldNavigationDropPosition())
	{
		position.y += CellAppConfig::navigationMaxClimb() + movement.length() *
			CellAppConfig::navigationMaxSlopeGradient();
	}

	// No longer on the ground...
	// Might want to make this changeable from script
	// for entities that want to be on the ground.
	this->entity().isOnGround( false );
	this->setEntityPosition( position, direction );

	if (!this->isAttached())
	{
		return false;
	}

	return almostEqual( position.x, destination.x ) &&
			almostEqual( position.z, destination.z ) &&
			(almostEqual( position.y, destination.y ) || !moveVertically_);
}


void MoveController::writeRealToStream( BinaryOStream & stream )
{
	this->Controller::writeRealToStream( stream );
	stream << metresPerTick_ << faceMovement_ << moveVertically_;
}


bool MoveController::readRealFromStream( BinaryIStream & stream )
{
	bool result = this->Controller::readRealFromStream( stream );
	stream >> metresPerTick_ >> faceMovement_ >> moveVertically_;

	return result;
}


/**
 *	This method overrides the Controller method.
 */
void MoveController::startReal( bool /*isInitialStart*/ )
{
	MF_ASSERT( entity().isReal() );
	CellApp::instance().registerForUpdate( this );
}


/**
 *	This method overrides the Controller method.
 */
void MoveController::stopReal( bool /*isFinalStop*/ )
{
	MF_VERIFY( CellApp::instance().deregisterForUpdate( this ) );
}


/**
 *	This method sets the entity position and direction during a movement.
 */
void MoveController::setEntityPosition( const Position3D & position,
								const Direction3D & direction )
{
	this->entity().setPositionAndDirection( position, direction );
}

// -----------------------------------------------------------------------------
// Section: MoveToPointController
// -----------------------------------------------------------------------------

IMPLEMENT_EXCLUSIVE_CONTROLLER_TYPE(
		MoveToPointController, DOMAIN_REAL, "Movement" )

/**
 *	Constructor for MoveToPointController.
 *
 *	@param destination		The location to which we should move
 *	@param destinationChunk	The destination chunk.
 *	@param destinationWaypoint The id of the destination waypoint.
 *	@param velocity			Velocity in metres per second
 *	@param faceMovement		Whether or not the entity should face in the
 *							direction of movement.
 *	@param moveVertically	Whether or not the entity should move vertically.
 */
MoveToPointController::MoveToPointController( const Position3D & destination,
	   const BW::string & destinationChunk, int32 destinationWaypoint,
		float velocity, bool faceMovement, bool moveVertically ) :
	MoveController( velocity, faceMovement, moveVertically ),
	destination_( destination ),
	destinationChunk_( destinationChunk ),
	destinationWaypoint_( destinationWaypoint )
{
}


/**
 *	This method writes our state to a stream.
 *
 *	@param stream		Stream to which we should write
 */
void MoveToPointController::writeRealToStream( BinaryOStream & stream )
{
	this->MoveController::writeRealToStream( stream );
	stream << destination_ << destinationChunk_ << destinationWaypoint_;
}


/**
 *	This method reads our state from a stream.
 *
 *	@param stream		Stream from which to read
 *	@return				true if successful, false otherwise
 */
bool MoveToPointController::readRealFromStream( BinaryIStream & stream )
{
	this->MoveController::readRealFromStream( stream );
	stream >> destination_ >> destinationChunk_ >> destinationWaypoint_;
	return true;
}



/**
 *	This method is called every tick.
 */
void MoveToPointController::update()
{
	if (this->move( destination_ ))
	{
		// Keep ourselves alive until we have finished cleaning up,
		// with an extra reference count from a smart pointer.
		ControllerPtr pController = this;

		{
			SCOPED_PROFILE( ON_MOVE_PROFILE );
			this->standardCallback( "onMove" );
		}

		if (this->isAttached())
		{
			this->cancel();
		}
	}
}


IMPLEMENT_EXCLUSIVE_CONTROLLER_TYPE(
		NavigateStepController, DOMAIN_REAL, "Movement" )


/**
 *	Default constructor for NavigateStepController.
 */
 NavigateStepController::NavigateStepController() :
	MoveController( 0.f ), destination_( Vector3::zero() )
{
}


/**
 *	Constructor for NavigateStepController.
 *
 *	@param destination		The location to which we should move
 *	@param velocity			Velocity in metres per second
 *	@param faceMovement		Whether or not the entity should face in the
 *							direction of movement.
 */
NavigateStepController::NavigateStepController( const Position3D & destination,
		float velocity, bool faceMovement ) :
	MoveController( velocity, faceMovement, false ),
	destination_( destination )
{
}


void NavigateStepController::startReal( bool isInitialStart )
{
	MoveController::startReal( isInitialStart );

	EntityNavigate& en = EntityNavigate::instance( this->entity() );

	en.registerNavigateStepController( this );
}


void NavigateStepController::stopReal( bool isFinalStop )
{
	MoveController::stopReal( isFinalStop );

	EntityNavigate& en = EntityNavigate::instance( this->entity() );

	en.deregisterNavigateStepController( this );
}


void NavigateStepController::setDestination( const Position3D& destination )
{
	destination_ = destination;
}


/**
 *	This method writes our state to a stream.
 *
 *	@param stream		Stream to which we should write
 */
void NavigateStepController::writeRealToStream( BinaryOStream & stream )
{
	this->MoveController::writeRealToStream( stream );
	stream << destination_;
}


/**
 *	This method reads our state from a stream.
 *
 *	@param stream		Stream from which to read
 *	@return				true if successful, false otherwise
 */
bool NavigateStepController::readRealFromStream( BinaryIStream & stream )
{
	bool result = this->MoveController::readRealFromStream( stream );

	stream >> destination_;

	return result && !stream.error();
}


/**
 *	This method is called every tick.
 */
void NavigateStepController::update()
{
	float tick = 1.f;
	const Vector3 invalidDestination( FLT_MAX, FLT_MAX, FLT_MAX );

	ControllerPtr pController = this;
	int numOfLoops = 0;

	while (!almostZero( tick ) && this->isAttached() &&
		this->move( destination_, &tick ))
	{
		destination_ = invalidDestination;

		if (++numOfLoops < 100)
		{
			SCOPED_PROFILE( ON_MOVE_PROFILE );
			this->standardCallback( "onMove" );
		}
		else
		{
			WARNING_MSG( "NavigateStepController::update: Entity %d: "
					"destination = (%.2f, %.2f, %.2f). Space = %d. "
					"numOfLoops = %d and tick still %.3f. This is likely "
					"caused by script navigating to the same point in onMove "
					"callback. Quitting loop.\n",
				this->entity().id(),
				this->entity().position().x,
				this->entity().position().y,
				this->entity().position().z,
				this->entity().space().id(),
				numOfLoops, tick );

			{
				SCOPED_PROFILE( ON_MOVE_PROFILE );
				this->standardCallback( "onMoveFailure" );
			}

			if (this->isAttached())
			{
				this->cancel();
			}

			return;
		}

		if (this->isAttached() && destination_ == invalidDestination)
		{
			this->cancel();
		}
	}
}


/**
 *	Override from MoveController.
 */
void NavigateStepController::setEntityPosition( const Position3D & position,
	const Direction3D & direction )
{
	this->entity().setAndDropGlobalPositionAndDirection( position, direction );
}


// -----------------------------------------------------------------------------
// Section: MoveToEntityController
// -----------------------------------------------------------------------------

IMPLEMENT_EXCLUSIVE_CONTROLLER_TYPE(
		MoveToEntityController, DOMAIN_REAL, "Movement" )

/**
 *	Constructor for MoveToEntityController.
 *
 *	@param destEntityID		Entity to follow
 *	@param velocity			Velocity in metres per second
 *	@param range			Stop once we are within this range
 *	@param faceMovement		Whether or not the entity should face the direction
 *							of movement.
 *	@param moveVertically	Whether or not the entity should move vertically.
 */
MoveToEntityController::MoveToEntityController( EntityID destEntityID,
			float velocity, float range, bool faceMovement,
			bool moveVertically ) :
	MoveController( velocity, faceMovement, moveVertically ),
	destEntityID_( destEntityID ),
	pDestEntity_( NULL ),
	offset_( 0.f, 0.f, 0.f ),
	range_( range )
{
}


/**
 *	This method writes our state to a stream.
 *
 *	@param stream		Stream to which we should write
 */
void MoveToEntityController::writeRealToStream( BinaryOStream & stream )
{
	this->MoveController::writeRealToStream( stream );
	stream << destEntityID_ << range_;
}

/**
 *	This method reads our state from a stream.
 *
 *	@param stream		Stream from which to read
 *	@return				true if successful, false otherwise
 */
bool MoveToEntityController::readRealFromStream( BinaryIStream & stream )
{
	this->MoveController::readRealFromStream( stream );
	stream >> destEntityID_ >> range_;
	return true;
}



/**
 *	Make sure we decrement the reference count of the destination
 *	entity when we stop.
 */
void MoveToEntityController::stopReal( bool isFinalStop )
{
	this->MoveController::stopReal( isFinalStop );
	pDestEntity_ = NULL;
}


/**
 *  Pick a random offset, within range of the destination.
 */
void MoveToEntityController::recalcOffset()
{
	float angle = (rand() % 360) * (MATH_PI / 180.0f);
	offset_.x = range_ * cosf(angle);
	offset_.z = range_ * sinf(angle);
}


/**
 *	This method is called every tick.
 */
void MoveToEntityController::update()
{
	// Resolve the entity ID to an entity pointer.

	if (!pDestEntity_)
	{
		pDestEntity_ = CellApp::instance().findEntity( destEntityID_ );
		this->recalcOffset();
	}

	if (pDestEntity_ && pDestEntity_->isDestroyed())
	{
		pDestEntity_ = NULL;
	}

	if (!pDestEntity_)
	{
		ControllerPtr pController = this;
		this->standardCallback( "onMoveFailure" );
		if (this->isAttached())
		{
			this->cancel();
		}
		return;
	}

	Position3D destination = this->pDestEntity_->position();
	Position3D currentPosition = this->entity().position();

	// calculate remaining distance: don't take into account y-position when we
	// are not moving vertically; instead take 2D dist projected to x-z plane
	Vector3 remaining = destination - this->entity().position();

	if (!this->moveVertically())
	{
		remaining.y = 0.f;
		destination.y = currentPosition.y;
	}
	bool closeEnough = remaining.length() < range_;

	// Keep ourselves alive until we have finished cleaning up,
	// with an extra reference count from a smart pointer.
	ControllerPtr pController = this;

	//move(..) only happens in local space, so if we are on vehicle, convert the destination to local space,
	//talked with Paul: it makes sense to be "move the src entity towards the destination entity". 
	//To a script writer, it doesn't matter. The other option is to make this fail but why not allow it??
	Position3D destLocal = destination + offset_;
	if (this->entity().pVehicle())
		Passengers::instance( *this->entity().pVehicle() ).transformToVehiclePos( destLocal );
	if (closeEnough || this->move( destLocal ))
	{
		if (closeEnough)
		{
			// In case we were moving towards a point near the entity,
			// make sure that we are now facing it exactly.

			const Position3D & position = this->entity().position();
			Vector3 vector = destination - position;

			Direction3D direction = this->entity().direction();
			if (this->faceMovement())
			{
				direction.yaw = vector.yaw();
			}

			if (this->entity().pVehicle())
			{
				Passengers::instance( *this->entity().pVehicle() ).transformToVehicleDir( direction );
			}
			this->entity().setPositionAndDirection( this->entity().localPosition(), direction );
		}

		if (this->isAttached())
		{
			this->standardCallback( "onMove" );
			if (this->isAttached())
			{
				this->cancel();
			}
		}
	}
}

BW_END_NAMESPACE

// move_controller.cpp

