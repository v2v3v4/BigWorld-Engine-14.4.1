#include "script/first_include.hpp"

#include "turn_controller.hpp"

#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "cell.hpp"
#include "entity.hpp"
#include "real_entity.hpp"
#include "passengers.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

/*~ function Entity addYawRotator
 *  @components{ cell }
 *  The addYawRotator function causes the Entity to turn towards a specified
 *  direction around the world's vertical axis. This is achieved by creating a
 *  new controller and adding it to the Entity. It throws a TypeError if the
 *  Entity is not real. The controller can be removed via the Entity's cancel
 *  function. For example, Entity.cancel( "Movement" ) or
 *	Entity.cancel( controllerID ).
 *  The controller will determine the shortest direction (clockwise or anti-clockwise)
 *  to turn
 *  On reaching its target yaw, the controller will call Entity.onTurn
 *  then delete itself. The Entity.onTurn method is not defined by default,
 *  and is called with two integer arguments, those being the ID of the
 *  controller, and the value specified as "userArg". Errors thrown by the
 *  attempt to call onTurn are silently ignored.
 *
 *	@see cancel
 *
 *  @param targetYaw targetYaw is a float representing the yaw to which the
 *  entity is to be turned, in radians and world space.
 *  @param velocity velocity is a float specifying the rate at which the entity
 *  should turn, in radians per second.
 *  @param userArg=0 userArg is an integer which is passed as the second
 *  parameter to onTurn.
 *
 *  @return The ID of the new controller. This can later be used with the
 *  Entity's cancel function to remove the controller and stop the Entity
 *  from turning. Entity.cancel( "Movement" ) can also be used to stop this
 *	controller.
 */
/*~ callback Entity.onTurn
 *  @components{ cell }
 *	This method is called on the completion of a call to Entity.addYawRotator.
 *	@param controllerID The id of the controller.
 *	@param userData The user data passed in to the Entity.addYawRotator call.
 */
IMPLEMENT_EXCLUSIVE_CONTROLLER_TYPE_WITH_PY_FACTORY(
		YawRotatorController, DOMAIN_REAL, "Movement" )

/**
 *	This method is exposed to scripting. It creates a controller that
 *	will turn an entity to the given yaaw. The arguments below are passed
 *	via a Python tuple.
 *
 *	@param targetYaw 		    Yaw
 *	@param velocity				Angular velocity in radians/s
 *	@param userArg				User data to be passed to the callback
 *
 *	@return		The integer ID of the newly created controller.
 */
Controller::FactoryFnRet YawRotatorController::New(
	float targetYaw, float velocity, int userArg )
{
	if( velocity <= 0.f )
	{
		PyErr_SetString( PyExc_AttributeError,
			"Can't add YawRotator controller with 0 or negative velocity" );
		return NULL;
	}

	return FactoryFnRet(
		new YawRotatorController( targetYaw, velocity ), userArg );
}

/**
 * 	YawRotatorController constructor
 *
 *	@param targetYaw		The target yaw in radians.
 *	@param velocity			Velocity in metres per second
 */
YawRotatorController::YawRotatorController( float targetYaw, float velocity )
{
	radiansPerTick_ = velocity / CellAppConfig::updateHertz();
	targetYaw_ = Angle(targetYaw);
}


/**
 * 	This method rotates the entity towards the target yaw by radiansPerTick_.
 */
bool YawRotatorController::turn()
{
	Direction3D direction = this->entity().direction();
	bool reachedYaw = false;
	Angle currentYaw = direction.yaw;
	Angle newYaw	 = direction.yaw + radiansPerTick_;

	if (radiansPerTick_ > 0.f ?
			targetYaw_.isBetween( currentYaw, newYaw ) :
			targetYaw_.isBetween( newYaw, currentYaw ))
	{
		newYaw = targetYaw_;
		reachedYaw = true;
	}

	direction.yaw = newYaw;

	if( this->entity().pVehicle() )
		Passengers::instance( *this->entity().pVehicle() ).transformToVehicleDir( direction );

	this->entity().setPositionAndDirection(
		this->entity().localPosition(), direction );

	return reachedYaw;
}


/**
 *	This method overrides the Controller method.
 */
void YawRotatorController::startReal( bool /*isInitialStart*/ )
{
	CellApp::instance().registerForUpdate( this );

	//check if anti-clockwise is shortest direction
	Angle yaw = this->entity().direction().yaw;
	if (targetYaw_.isBetween( yaw - MATH_PI, yaw ))
	{
		radiansPerTick_ = -radiansPerTick_;
	}

	// Turn at least once without checking so that you start turning.
	// This is needed in the case that you want to do a full circle and
	// specify the same yaw as the one you are facing.
	this->turn();
}


/**
 *	This method overrides the Controller method.
 */
void YawRotatorController::stopReal( bool /*isFinalStop*/ )
{
	MF_VERIFY( CellApp::instance().deregisterForUpdate( this ) );
}


/**
 *	This method is called every tick.
 */
void YawRotatorController::update()
{
	if (this->turn())
	{
		// Keep ourselves alive until we have finished cleaning up,
		// with an extra reference count from a smart pointer.
		ControllerPtr pController = this;
		this->standardCallback( "onTurn" );
		if (this->isAttached())
		{
			this->cancel();
		}
	}
}


/**
 *	Write our state to a stream
 *
 *	@param stream		Stream to which we should write
 */
void YawRotatorController::writeRealToStream( BinaryOStream & stream )
{
	this->Controller::writeRealToStream( stream );
	stream << targetYaw_ << radiansPerTick_;
}


/**
 *	Read our state from a stream
 *
 *	@param stream		Stream from which to read
 *	@return				true if successful, false otherwise
 */
bool YawRotatorController::readRealFromStream( BinaryIStream & stream )
{
	this->Controller::readRealFromStream( stream );
	stream >> targetYaw_ >> radiansPerTick_;
	return true;
}

BW_END_NAMESPACE

// turn_controller.cpp
