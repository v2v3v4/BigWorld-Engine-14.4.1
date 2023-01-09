#include "pch.hpp"
#include "propellor.hpp"

#include "pymodel.hpp"

#include "math/integrat.hpp"

DECLARE_DEBUG_COMPONENT2( "Motor", 0 )


BW_BEGIN_NAMESPACE

/*~ class BigWorld.Propellor
 *
 *	This class moves an object with the effect of a propellor or other point source of force.
 *	A throttle value is used to determine the desired level of force.  A wheelPosition is used
 *	to determine a desired turn rate, which is then adjusted over time for smoother transition.
 *	With the throttle and WheelPosition driving the motor, other values can be set, such as drag,
 *	etc, to affect the overall behaviour of the motor.
 *
 *	A new Propellor Motor is created using BigWorld.Propellor function.
 */
PY_TYPEOBJECT( Propellor )

PY_BEGIN_METHODS( Propellor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Propellor )

	/*~	attribute Propellor.throttle
	 *	Determines the current level of force for this motor.
	 *	@type	float
	 */
	PY_ATTRIBUTE( throttle )

	/*~	attribute Propellor.wheelPosition
	 *	This value represents the desired rate of turn.
	 *	Higher values will turn more, lower values turn less.
	 *	@type	float
	 */
	PY_ATTRIBUTE( wheelPosition )

	/*~	attribute Propellor.momentum
	 *	The current momentum being applied to the model.
	 *	@type	Vector3
	 */
	PY_ATTRIBUTE( momentum )

	/*~	attribute Propellor.xDrag
	 *	Drag along the x-axis.  Defaults to 0.2.
	 *	@type	float
	 */
	PY_ATTRIBUTE( xDrag )

	/*~	attribute Propellor.yDrag
	 *	Drag along the y-axis.  Defaults to 1.0.
	 *	@type	float
	 */
	PY_ATTRIBUTE( yDrag )

	/*~	attribute Propellor.zDrag
	 *	Drag along the z-axis.  Defaults to 0.01.
	 *	@type	float
	 */
	PY_ATTRIBUTE( zDrag )

	/*~	attribute Propellor.turnRate
	 *	The degree to which the motor can turn over time.  Defaults to 0.005.
	 *	@type	float
	 */
	PY_ATTRIBUTE( turnRate )

	/*~	attribute Propellor.maxThrust
	 *	This is the maximum thrust that can be applied to the model.  Defaults to 0.075.
	 *	@type	float
	 */
	PY_ATTRIBUTE( maxThrust )

	/*~	attribute Propellor.timeScale
	 *	Can be used to scale the integration time of the motor.  Defaults to 1.0 (no change - using real-time).
	 *	@type	float
	 */
	PY_ATTRIBUTE( timeScale )

	/*~	attribute Propellor.actualTurn
	 *	Used to dampen the wheel's response.  Each update, the actualTurn rate will be 'decayed' from
	 *	wheelPosition to this value based upon the turnHalfLife and current time step (which can be
	 *	adjusted with timeScale) and stored for the next update.  Defaults to 0.0.
	 *	@type	float
	 */
	PY_ATTRIBUTE( actualTurn )

	/*~	attribute Propellor.turnHalflife
	 *	How quickly the desired turn is approached.  Defaults to 10.
	 *	@type	float
	 */
	PY_ATTRIBUTE( turnHalflife )

PY_END_ATTRIBUTES()

/*~ function BigWorld.Propellor
 *	Creates and returns a new Propellor Motor, which is used
 *	to move a model by applying a point source of force.
 */

PY_FACTORY( Propellor, BigWorld )


/**
 *	Constructor
 */
Propellor::Propellor( PyTypeObject * pType ) : Motor( pType ),
	xDrag_( 0.2f ),
	yDrag_( 1.f ),
	zDrag_( 0.01f ),
	turnRate_( 0.005f ),
	maxThrust_( 0.075f ),
	timeScale_( 1.f ),
	actualTurn_( 0.f ),
	turnHalflife_( 10.f )
{
	BW_GUARD;
	MF_WATCH( "Propellor/throttle", throttle_, Watcher::WT_READ_WRITE, "Propellor throttle" );
	MF_WATCH( "Propellor/wheelPosition", wheelPosition_, Watcher::WT_READ_WRITE, "Propellor wheel position" );
	MF_WATCH( "Propellor/momentum", momentum_, Watcher::WT_READ_WRITE, "Propellor momentum" );
	MF_WATCH( "Propellor/xDrag", xDrag_, Watcher::WT_READ_WRITE, "Propellor drag on the X axis" );
	MF_WATCH( "Propellor/yDrag", yDrag_,Watcher::WT_READ_WRITE, "Propellor drag on the Y axis" );
	MF_WATCH( "Propellor/zDrag", zDrag_, Watcher::WT_READ_WRITE, "Propellor drag on the Z axis" );

	MF_WATCH( "Propellor/turnRate", turnRate_, Watcher::WT_READ_WRITE, "Propellor turn rate" );
	MF_WATCH( "Propellor/maxThrust", maxThrust_, Watcher::WT_READ_WRITE, "Maximum propellor thrust" );
	MF_WATCH( "Propellor/timeScale", timeScale_, Watcher::WT_READ_WRITE, "Propellor time scale" );

	MF_WATCH( "Propellor/actualTurn", actualTurn_, Watcher::WT_READ_WRITE, "Propellor actual turn" );
	MF_WATCH( "Propellor/turnHalflife", turnHalflife_, Watcher::WT_READ_WRITE, "Propellor turn half life" );
}


/**
 *	Destructor
 */
Propellor::~Propellor()
{
}


/**
 *	Static python factory method
 */
PyObject * Propellor::pyNew( PyObject * args )
{
	BW_GUARD;
	Py_ssize_t i = PyTuple_Size( args );
	if (i != 0)
	{
		PyErr_Format( PyExc_TypeError,
			"BigWorld.Propellor brooks no arguments (%d given)", i );
		return NULL;
	}

	return new Propellor();
}


/**
 * This class maintains the momentum and position of a Propellor Motor.
 */
class PhysData
{
public:
	PhysData();
	PhysData(const Vector3 & momentum,
		const Vector3 & position);

	Vector3 momentum_;
	Vector3 position_;

	void operator+=(const PhysData & data);
};

inline PhysData::PhysData()
: momentum_( 0, 0, 0 ),
  position_( 0, 0, 0 )
{
}

inline PhysData::PhysData(const Vector3& momentum, const Vector3& position) :
	momentum_(momentum),
	position_(position)
{
}

inline PhysData operator *(float t, const PhysData & d)
{
	return PhysData(t * d.momentum_,
		t * d.position_);
}


inline PhysData operator +(const PhysData & d1, const PhysData & d2)
{
	return PhysData( d1.momentum_ + d2.momentum_,
		d1.position_ + d2.position_ );
}

inline void PhysData::operator+=(const PhysData & data)
{
	momentum_ += data.momentum_;
	position_ += data.position_;
}


/**
 *	This functor is used in calculating the motion of the boat. The operator()
 *	function calculates the derivative of the momentum and position. This is
 *	then integrated using our integrator.
 */
class BoatFunction
{
public:
	BoatFunction( const Propellor & propellor, const Matrix & matrix ) :
		propellor_( propellor ),
		matrix_( matrix )
	{};


	PhysData operator()(float t, const PhysData & data ) const
	{
		BW_GUARD;
		PhysData retVal = data;
		retVal.position_ = data.momentum_;

		Vector3 momentum(
			data.momentum_.x,
			data.momentum_.y,
			data.momentum_.z );

		// Find the components of the momentum in each direction of the vehicle. We want
		// more drag on sideways momentum than forward momentum, for example.

		float xMomentum = matrix_.applyToUnitAxisVector(X_AXIS).dotProduct( momentum );
		float yMomentum = matrix_.applyToUnitAxisVector(Y_AXIS).dotProduct( momentum );
		float zMomentum = matrix_.applyToUnitAxisVector(Z_AXIS).dotProduct( momentum );

		// Calculate the drag on each component of the momentum.
		const float X_DRAG = propellor_.xDrag();
		const float Y_DRAG = propellor_.yDrag();
		const float Z_DRAG = propellor_.zDrag();

		// Create the new momentum after considering drag.
		retVal.momentum_ =
			-X_DRAG * xMomentum * matrix_.applyToUnitAxisVector(X_AXIS) +
			-Y_DRAG * yMomentum * matrix_.applyToUnitAxisVector(Y_AXIS) +
			-Z_DRAG * zMomentum * matrix_.applyToUnitAxisVector(Z_AXIS);

		const Vector3 & thrustAxis = matrix_.applyToUnitAxisVector(Z_AXIS);

		// Add the engine's thrust to the momentum.
		retVal.momentum_ += propellor_.thrust() * thrustAxis;

		return retVal;
	}

private:
	const Propellor & propellor_;
	const Matrix & matrix_;
};


/**
 *	Run this motor
 */
void Propellor::rev( float dTime )
{
	BW_GUARD;
	const float MAX_DTIME = 0.05f;
	dTime *= timeScale_;

	while (dTime > 0.f)
	{
		this->runPhysics( min( dTime, MAX_DTIME ) );
		dTime -= MAX_DTIME;
	}
}


/**
 *	This method runs a step of the physics based on the input step size.
 */
void Propellor::runPhysics( float dTime )
{
	BW_GUARD;
	Matrix world = pOwner_->worldTransform();

	Vector3 position = world.applyToOrigin();
	Vector3 direction = world.applyToUnitAxisVector( Z_AXIS );
	float yaw = atan2f( direction.x, direction.z );

	float desiredTurn = wheelPosition_ * momentum_.dotProduct( direction );
	actualTurn_ = Math::decay( actualTurn_, desiredTurn, turnHalflife_, dTime );

	yaw += dTime * actualTurn_ * turnRate_;

	float actualThrust = throttle_ * maxThrust_;

	PhysData data;
	data.position_ = position;
	data.momentum_ = momentum_;
	BoatFunction F( *this, world );
	integrate( F, data, dTime );

	momentum_ = data.momentum_;
	position.set( data.position_.x, data.position_.y, data.position_.z );

	world.setRotateY( yaw );
	world.translation( data.position_ );

	Vector3 newDirection = world.applyToUnitAxisVector( Z_AXIS );

	pOwner_->worldTransform( world );
}

BW_END_NAMESPACE

// propellor.cpp
