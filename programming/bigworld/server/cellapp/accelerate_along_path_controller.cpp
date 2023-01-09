#include "script/first_include.hpp"

#include "accelerate_along_path_controller.hpp"
#include "entity.hpp"
#include "cellapp.hpp"
#include "cell.hpp"
#include "real_entity.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

IMPLEMENT_EXCLUSIVE_CONTROLLER_TYPE(
		AccelerateAlongPathController, DOMAIN_REAL, "Movement");




/**
 *	Default constructor for AccelerateAlongPathController
 */
AccelerateAlongPathController::AccelerateAlongPathController() :
	BaseAccelerationController(	0.0f,
								0.0f,
								FACING_NONE),
	progress_(0)
{
	waypoints_.clear();
}


/**
 *	Constructor for AccelerateAlongPathController.
 *
 *	@param waypoints		The list of waypoints to traverse. NOTE:
 *							They are in local spaces, and the given vector 
 *							will be empty after this function call.
 *	@param acceleration		The amount of acceleration to apply.
 *							(units/second^2)
 *	@param maxSpeed			The maximum speed to travel at. (units/second)
 *	@param facing			The direction to face while travelling.
 *
 *	@note	To avoid an unnecessary vector copy, the contents of 'waypoints' is
 *			std::swapped with the member. Be aware that waypoints is not a
 *			const reference and will be empty after the function completes.
 */
AccelerateAlongPathController::AccelerateAlongPathController(
										BW::vector< Position3D > & waypoints,
										float acceleration,
										float maxSpeed,
										Facing facing) :
	BaseAccelerationController(	acceleration,
								maxSpeed,
								facing),
	progress_(0)
{
	std::swap(waypoints, waypoints_);

	MF_ASSERT( !waypoints_.empty() );
}


/**
 *	This method writes our state to a stream.
 *
 *	@param stream		Stream to which we should write
 */
void AccelerateAlongPathController::writeRealToStream( BinaryOStream & stream )
{
	this->BaseAccelerationController::writeRealToStream( stream );
	stream << waypoints_ << progress_;
}


/**
 *	This method reads our state from a stream.
 *
 *	@param stream		Stream from which to read
 *
 *	@return				true if successful, false otherwise
 */
bool AccelerateAlongPathController::readRealFromStream( BinaryIStream & stream )
{
	this->BaseAccelerationController::readRealFromStream( stream );
	stream >> waypoints_ >> progress_;
	return true;
}


/**
 *	This is the update function of the AccelerateAlongPathController
 *	every tick. It accelerates the entity toward the next waypoint from the
 *	list given when the controller was created.
 *
 *	If the destination is reached the controller will cancel its self and
 *	initiate the onMove callback.
 *
 *	@see	Entity.onMove
 */
void AccelerateAlongPathController::update()
{
	bool isLastWaypoint = (progress_ == waypoints_.size() - 1);

	bool waypointReached = this->move( waypoints_[ progress_ ],
										isLastWaypoint );

	if (!waypointReached)
	{
		return;
	}

	progress_++;

	if (isLastWaypoint)
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

BW_END_NAMESPACE

// accelerate_along_path_controller.cpp
