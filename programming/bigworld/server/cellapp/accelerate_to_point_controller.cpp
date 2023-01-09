#include "script/first_include.hpp"

#include "accelerate_to_point_controller.hpp"
#include "entity.hpp"
#include "cellapp.hpp"
#include "cell.hpp"
#include "real_entity.hpp"


DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

IMPLEMENT_EXCLUSIVE_CONTROLLER_TYPE(
		AccelerateToPointController, DOMAIN_REAL, "Movement")


/**
 *	Constructor.
 *
 *	@param destination			The point in world space that the entity will
 *								move toward.
 *	@param acceleration			The rate at which the controller will adjust the
 *								speed of the entity in units per second.
 *	@param maxSpeed				The maximum speed the controller will
 *								accelerate the entity to.
 *	@param facing				Determines in which direction the controller
 *								should face the entity.
 *	@param stopAtDestination	Indicates whether the controller should bring
 *								the entity to a halt at the destination.
 */
AccelerateToPointController::AccelerateToPointController(
													Vector3 destination,
													float acceleration,
													float maxSpeed,
													Facing facing,
													bool stopAtDestination) :
	BaseAccelerationController(	acceleration,
								maxSpeed,
								facing),
	destination_( destination ),
	stopAtDestination_( stopAtDestination )
{

}

/**
 *	This method writes our state to a stream.
 *
 *	@param stream		Stream to which we should write
 */
void AccelerateToPointController::writeRealToStream( BinaryOStream & stream )
{
	this->BaseAccelerationController::writeRealToStream( stream );
	stream << destination_ << stopAtDestination_;
}


/**
 *	This method reads our state from a stream.
 *
 *	@param stream		Stream from which to read
 *	@return				true if successful, false otherwise
 */
bool AccelerateToPointController::readRealFromStream( BinaryIStream & stream )
{
	this->BaseAccelerationController::readRealFromStream( stream );
	stream >> destination_ >> stopAtDestination_;
	return true;
}


/**
 *	This is the update function of the AccelerateToPointController
 *	every tick. It accelerates the entity toward the destination given when the
 *	controller was created.
 *
 *	If the destination is reached the controller will cancel its self and
 *	initiate the onMove callback.
 *
 *	@see	Entity.onMove
 */
void AccelerateToPointController::update()
{
	bool destinationReached = this->move( destination_, stopAtDestination_ );

	if (!destinationReached)
	{
		return;
	}

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

BW_END_NAMESPACE

// accelerate_to_point_controller.cpp
