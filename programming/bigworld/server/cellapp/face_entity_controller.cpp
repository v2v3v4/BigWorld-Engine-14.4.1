#include "script/first_include.hpp"

#include "face_entity_controller.hpp"
#include "entity.hpp"
#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "cell.hpp"
#include "real_entity.hpp"
#include "passengers.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

IMPLEMENT_EXCLUSIVE_CONTROLLER_TYPE(
		FaceEntityController, DOMAIN_REAL, "Movement" )

/**
 * 	FaceEntityController constructor
 *
 *	@param entityId  The entity to track
 *	@param velocity  Velocity in metres per second
 *	@param period 	 This is how much in between checking vision(in ticks)
 *					 Default period = 10 tick(1 second)
 */
FaceEntityController::FaceEntityController( int entityId, float velocity,
		int period ) :
	pTargetEntity_( NULL ),
	targetEntityId_( entityId ),
	radiansPerTick_( velocity / CellAppConfig::updateHertz() ),
	ticksToNextUpdate_( period ),
	period_( period )
{
}


/**
 *	This method overrides the Controller method.
 */
void FaceEntityController::startReal( bool /*isInitialStart*/ )
{
	CellApp::instance().registerForUpdate( this );
}


/**
 *	This method overrides the Controller method.
 */
void FaceEntityController::stopReal( bool /*isFinalStop*/ )
{
	MF_VERIFY( CellApp::instance().deregisterForUpdate( this ) );
}


/**
 *	This method is called every tick.
 */
void FaceEntityController::update()
{
	ticksToNextUpdate_--;

	if (ticksToNextUpdate_ > 0)
	{
		return;
	}

	ticksToNextUpdate_ = period_;

	if (!pTargetEntity_)
	{
		pTargetEntity_ = CellApp::instance().findEntity( targetEntityId_ );
	}
	else if (pTargetEntity_->isDestroyed())
	{
		pTargetEntity_ = NULL;
	}

	if (!pTargetEntity_)
	{
		return;
	}

	Vector3 position = this->entity().position();
	Direction3D direction = this->entity().direction();

	Vector3 targetPos = pTargetEntity_->position();
	Vector3 targetDir = targetPos - position;

	float targetYaw = atan2f( targetDir.x, targetDir.z );
	float deltaYaw = targetYaw - direction.yaw;

	if (deltaYaw > MATH_PI)
	{
		deltaYaw -= MATH_2PI;
	}
	else if (deltaYaw < - MATH_PI)
	{
		deltaYaw += MATH_2PI;
	}

	if (fabs( deltaYaw ) < 0.01f)
	{
		deltaYaw = 0.f;
	}
	else if (fabs(deltaYaw) > radiansPerTick_)
	{
		deltaYaw = Math::clamp( -radiansPerTick_, deltaYaw,
						radiansPerTick_ );
	}

	direction.yaw += deltaYaw ;
	if (direction.yaw > MATH_PI)
	{
		direction.yaw -= MATH_2PI ;
	} 
	else if (direction.yaw < - MATH_PI)
	{
		direction.yaw += MATH_2PI ;
	}

	if( this->entity().pVehicle() )
		Passengers::instance( *this->entity().pVehicle() ).transformToVehicleDir( direction );

	this->entity().
		setPositionAndDirection( this->entity().localPosition(), direction );
}


/**
 *	This method writes our state to a stream.
 *
 *	@param stream		Stream to which we should write
 */
void FaceEntityController::writeRealToStream( BinaryOStream & stream )
{
	this->Controller::writeRealToStream( stream );
	stream << targetEntityId_ << radiansPerTick_ << period_;
}

/**
 *	This method reads our state from a stream.
 *
 *	@param stream		Stream from which to read
 *	@return				true if successful, false otherwise
 */
bool FaceEntityController::readRealFromStream( BinaryIStream & stream )
{
	this->Controller::readRealFromStream( stream );
	stream >> targetEntityId_ >> radiansPerTick_ >> period_;

	// TODO: Should we stream this or is a guess good enough.
	ticksToNextUpdate_ = period_/2;

	return true;
}

BW_END_NAMESPACE

// face_entity_controller.cpp
