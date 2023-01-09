#include "script/first_include.hpp"

#include "accelerate_to_entity_controller.hpp"
#include "entity.hpp"
#include "cellapp.hpp"
#include "cell.hpp"
#include "real_entity.hpp"
#include "passengers.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

IMPLEMENT_EXCLUSIVE_CONTROLLER_TYPE(
		AccelerateToEntityController, DOMAIN_REAL, "Movement")


/**
 *	Constructor.
 *
 *	@param destEntityID		Entity to follow
 *	@param acceleration		The rate at which the controller will adjust the
 *							speed of the entity in units per second.
 *	@param maxSpeed			The maximum speed the controller will
 *							accelerate the entity to.
 *	@param range			Stop once we are within this range
 *	@param facing			Determines in which direction the controller
 *							should face the entity.
 */
AccelerateToEntityController::AccelerateToEntityController(	
														EntityID destEntityID,
														float acceleration,
														float maxSpeed,
														float range,
														Facing facing ) :
	BaseAccelerationController(	acceleration, maxSpeed, facing ),
	destEntityID_( destEntityID ),
	pDestEntity_( NULL ),
	destination_( 0.f, 0.f, 0.f ),
	offset_( 0.f, 0.f, 0.f ),
	range_( range )
{
}


/**
 *	This method writes our state to a stream.
 *
 *	@param stream		Stream to which we should write
 */
void AccelerateToEntityController::writeRealToStream( BinaryOStream & stream )
{
	this->BaseAccelerationController::writeRealToStream( stream );
	stream << destEntityID_ << range_;
}


/**
 *	This method reads our state from a stream.
 *
 *	@param stream		Stream from which to read
 *
 *	@return				true if successful, false otherwise
 */
bool AccelerateToEntityController::readRealFromStream(BinaryIStream & stream )
{
	this->BaseAccelerationController::readRealFromStream( stream );
	stream >> destEntityID_ >> range_;
	return true;
}


/**
 *	Make sure we decrement the reference count of the destination
 *	entity when we stop.
 */
void AccelerateToEntityController::stopReal( bool isFinalStop )
{
	this->BaseAccelerationController::stopReal( isFinalStop );
	pDestEntity_ = NULL;
}


/**
 *  Pick a random offset, within range of the destination.
 */
void AccelerateToEntityController::recalcOffset()
{
	float angle = (rand() % 360) * (MATH_PI / 180.0f);
	offset_.x = range_ * cosf(angle);
	offset_.z = range_ * sinf(angle);
}


/**
 *	This is the update function of the AccelerateToEntityController
 *	every tick. It accelerates the entity toward the entity given when the
 *	controller was created, stopping when it arrives.
 *
 *	If the destination is reached the controller will cancel its self and
 *	initiate the onMove callback.
 *
 *	If the target entity move out of range or is otherwise unattainable the
 *	controller will cancel and the callback onMoveFailure will be called.
 *
 *	@see	Entity.onMove
 *	@see	Entity.onMoveFailure
 */
void AccelerateToEntityController::update()
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

	// calculate remaining distance: don't take into account y-position when we
	// are not moving vertically; instead take 2D dist projected to x-z plane
	Vector3 remaining = destination - this->entity().position();

	bool closeEnough = remaining.length() < range_;

	// Keep ourselves alive until we have finished cleaning up,
	// with an extra reference count from a smart pointer.
	ControllerPtr pController = this;

	// move(..) only happens in local space, so if we are on vehicle, convert
	// the destination to local space,
	// Talked with Paul: it makes sense to be "move the src entity towards the
	// destination entity". 
	// To a script writer, it doesn't matter. The other option is to make this
	// fail but why not allow it?
	Position3D destLocal = destination + offset_;

	if (this->entity().pVehicle())
	{
		Passengers::instance( *this->entity().pVehicle() ).
			transformToVehiclePos( destLocal );
	}

	if (closeEnough || this->move(destLocal, true))
	{
		if (closeEnough)
		{
			// In case we were moving towards a point near the entity,
			// make sure that we are now facing it exactly.

			const Position3D & position = this->entity().position();
			Vector3 vector = destination - position;

			Direction3D direction = this->entity().direction();
			if (facing_ != FACING_NONE)
			{
				direction.yaw = vector.yaw();
			}

			if (this->entity().pVehicle())
			{
				Passengers::instance( *this->entity().pVehicle() ).
					transformToVehicleDir( direction );
			}

			this->entity().setPositionAndDirection( 
				this->entity().localPosition(), direction );
		}

		if (this->isAttached())
		{
			this->standardCallback( "onMove" );
			if (this->isAttached())
			{
				this->cancel();
			}
		}
		return;
	}
}

BW_END_NAMESPACE

// accelerate_to_entity_controller.cpp
