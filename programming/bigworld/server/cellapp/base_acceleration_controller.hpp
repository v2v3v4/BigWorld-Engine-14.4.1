#ifndef ACCELERATING_MOVE_CONTROLLER_HPP
#define ACCELERATING_MOVE_CONTROLLER_HPP


#include "controller.hpp"
#include "network/basictypes.hpp"
#include "server/updatable.hpp"


BW_BEGIN_NAMESPACE

typedef SmartPointer< Entity > EntityPtr;


/**
 *	This class forms the base of the accelerating move controllers. It
 *	samples the velocity for the entity and manipulates it by applying
 *	acceleration in order to arrive at a given destination. 
 */
class BaseAccelerationController : public Controller, public Updatable
{
public:
	enum Facing
	{
		FACING_NONE			= 0,
		FACING_VELOCITY		= 1,
		FACING_ACCELERATION	= 2,

		FACING_MAX			= 3
	};

public:
	BaseAccelerationController(	float acceleration,
								float maxSpeed,
								Facing facing = FACING_NONE );

	virtual void startReal( bool isInitialStart );
	virtual void stopReal( bool isFinalStop );

	bool move( const Position3D & destination, bool stopAtDestination );

protected:

	static Vector3	calculateDesiredVelocity(
											const Position3D & currentPosition,
											const Position3D & desiredPosition,
											float acceleration,
											float maxSpeed,
											bool stopAtDestination );

	static Vector3	calculateAccelerationVector(
											const Vector3 & currentVelocity,
											const Vector3 & desiredVelocity );

	virtual void	writeRealToStream( BinaryOStream & stream );
	virtual bool	readRealFromStream( BinaryIStream & stream );

protected:
	float		acceleration_;
	float		maxSpeed_;
	Facing		facing_;	

private:
	//for velocity calculation in local coordinate,
	//since entity().pReal()->velocity() is global velocity, 
	//but we need local velocity here
	Vector3			currVelocity_;
	Vector3			positionSample_;
	GameTime		positionSampleTime_;
	Entity*			pVehicle_;
	void refreshCurrentVelocity();
	const Vector3 & currVelocity() const			{ return currVelocity_; }

};

BW_END_NAMESPACE


#endif // ACCELERATING_MOVE_CONTROLLER_HPP
