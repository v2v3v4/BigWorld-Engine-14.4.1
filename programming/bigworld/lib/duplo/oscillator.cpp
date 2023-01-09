#include "pch.hpp"

#include "oscillator.hpp"
#include "pymodel.hpp"
#include "pyscript/py_callback.hpp"

DECLARE_DEBUG_COMPONENT2( "Motor", 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( Oscillator )

PY_BEGIN_METHODS( Oscillator )
	/*~ function Oscillator.canSee
	 *	The canSee function determines whether the input position is within
	 *	"line of sight" of the object being oscillated. Line of sight is defined
	 *	as within +/- 0.5 radians of yaw in the viewing direction and within a
	 *	distance of 60 units.
	 *	@param position Sequence of 3 floats ( x, y, z ).
	 *	@return Integer as boolean. 1 (true) if the position is within "line of
	 *	sight", 0 (false) otherwise.
	 */
	PY_METHOD( canSee )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Oscillator )
	/*~ attribute Oscillator.period
	 *	The attribute period specifies the period of oscillation (in seconds).
	 *	@type Float.
	 */
	PY_ATTRIBUTE( period )
	/*~ attribute Oscillator.amplitude
	 *	The attribute amplitude is a scaling factor to the magnitude
	 *	of the oscillation, resulting in a rotational oscillation from
	 *	-amplitude to +amplitude in radians. If amplitude is negative then
	 *	the Oscillator will make the object rotate instead of oscillate.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( amplitude )
	/*~ attribute Oscillator.offset
	 *	The attribute offset is applied as an addition to the start of the
	 *	period, effectively offsetting the starting angle of oscillation.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( offset )
PY_END_ATTRIBUTES()

/*~ function BigWorld.Oscillator
 *	Oscillator is a factory function that creates and returns a new Oscillator
 *	object. An Oscillator is a Motor that applies
 *	an oscillating rotational component to an objects world transform.
 *
 *	See the Oscillator class for more information on the parameters below.
 *	@param yaw Float. Specifies the initial yaw (in radians) of the rotation.
 *	@param period Float. Specifies the period (in seconds) of rotation.
 *	@param amplitude Float. Specifies the maximum angle (in radians) of rotation.
 *	@param offset Float. Specifies the offset (in seconds) to the start of the
 *	period of oscillation.
 *	@return A new Oscillator object.
 */
PY_FACTORY( Oscillator, BigWorld )


/**
 *	Constructor
 */
Oscillator::Oscillator( float yaw, float period, float amplitude, float offset,
		PyTypeObject * pType ):
	Motor( pType ),
	period_( period ),
	amplitude_( amplitude ),
	offset_( offset ),
	yaw_( yaw )
{
}


/**
 *	Destructor
 */
Oscillator::~Oscillator()
{
}


/**
 *	Static python factory method
 */
PyObject * Oscillator::pyNew( PyObject * args )
{
	BW_GUARD;
	float yaw;
	float period = 5.f;
	float amplitude = MATH_PI / 2.f;
	float offset = 0.f;

	if (!PyArg_ParseTuple( args, "f|fff",
		&yaw, &period, &amplitude, &offset ))
	{
		return NULL;
	}

	return new Oscillator( yaw, period, amplitude, offset );
}


/**
 *	This method runs this motor.
 */
void Oscillator::rev( float dTime )
{
	BW_GUARD;
	double timeNow = Script::getTotalGameTime();

	float yawOffset = float( 2 * MATH_PI * fmod( timeNow / period_ + offset_, 1.0 ) );
	if (amplitude_ >= 0.f) yawOffset = amplitude_ * sinf( yawOffset );

	const Matrix & oldWorld = pOwner_->worldTransform();
	Matrix newWorld;

	newWorld.setRotate( yaw_ + yawOffset, oldWorld.pitch(), oldWorld.roll() );
	newWorld[0] *= oldWorld[0].length();
	newWorld[1] *= oldWorld[1].length();
	newWorld[2] *= oldWorld[2].length();
	newWorld.translation( oldWorld.applyToOrigin() );

	pOwner_->worldTransform( newWorld );
}


/**
 *	This method overrides the Motor method so that we can get the initial
 *	position and direction.
 */
void Oscillator::attach( PyModel * pOwner )
{
	BW_GUARD;
	Motor::attach( pOwner );

/*
	const Matrix & world = pOwner->worldTransform();

	yaw_ = world.yaw();
	pitch_ = world.pitch();
	roll_ = world.roll();
	*/
}


/**
 *	This method overrides the Motor method so that we can restore any values
 *	that we need to.
 */
void Oscillator::detach()
{
	BW_GUARD;
	// TODO: Do we want to reset the yaw when we detach?
	/*
	const Matrix & oldWorld = pOwner_->worldTransform();
	Matrix newWorld;
	newWorld.setRotate( yaw_, pitch_, roll_ );
	newWorld.translation( oldWorld.applyToOrigin() );

	pOwner_->worldTransform( newWorld );
	*/
}


// -----------------------------------------------------------------------------
// Section: Script
// -----------------------------------------------------------------------------

/**
 *	This method is used to test whether the oscillator can see the input entity.
 */
bool Oscillator::canSee( const Vector3 & entityPos ) const
{
	BW_GUARD;
	// TODO: This is no longer used. The Spotter is used instead.
	const Matrix & world = pOwner_->worldTransform();
	const Vector3 & turretPos = world.applyToOrigin();
	Vector3 diff = entityPos - turretPos;

	Angle turretYaw = world.yaw();
	Angle entityYaw = diff.yaw();

	float yawDiff = turretYaw - entityYaw;

	const float YAW_RANGE = 0.5f;
	const float DIST_RANGE_SQR = 60.f * 60.f;

	bool canSee = (-YAW_RANGE < yawDiff) && (yawDiff < YAW_RANGE) &&
		(diff.lengthSquared() < DIST_RANGE_SQR);

	return canSee;
}






PY_TYPEOBJECT( Oscillator2 )

PY_BEGIN_METHODS( Oscillator2 )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Oscillator2 )
PY_END_ATTRIBUTES()

/*~ function BigWorld.Oscillator2
 *	Oscillator2 is a factory function that creates and returns a new Oscillator2
 *	object. An Oscillator2 extends Oscillator (Motor) and applies
 *	an oscillating rotational component around the origin to a model's world
 *	transform, instead of rotating it around its own centre.
 *
 *	See the Oscillator class for more information on the parameters below.
 *	@param yaw Float. Specifies the initial yaw (in radians) of the rotation.
 *	@param period Float. Specifies the period (in seconds) of rotation.
 *	@param amplitude Float. Specifies the maximum angle (in radians) of rotation.
 *	@param offset Float. Specifies the offset to the start of the period of
 *	oscillation.
 *	@return A new Oscillator2 object.
 */
PY_FACTORY( Oscillator2, BigWorld )


/**
 *	Constructor
 */
Oscillator2::Oscillator2( float yaw, float period, float amplitude, float offset,
		PyTypeObject * pType ):
	Oscillator( yaw, period, amplitude, offset, pType )
{
}


/**
 *	Destructor
 */
Oscillator2::~Oscillator2()
{
}


/**
 *	Static python factory method
 */
PyObject * Oscillator2::pyNew( PyObject * args )
{
	BW_GUARD;
	float yaw;
	float period = 5.f;
	float amplitude = MATH_PI / 2.f;
	float offset = 0.f;

	if (!PyArg_ParseTuple( args, "f|fff",
		&yaw, &period, &amplitude, &offset ))
	{
		return NULL;
	}

	return new Oscillator2( yaw, period, amplitude, offset );
}


/**
 *	This method runs this motor.
 */
void Oscillator2::rev( float dTime )
{
	BW_GUARD;
	double timeNow = Script::getTotalGameTime();

	float yawOffset = float( 2 * MATH_PI * fmod( dTime / period_, 1.f ) );
	if (amplitude_ >= 0.f) yawOffset = amplitude_ * sinf( yawOffset );

	const Matrix & oldWorld = pOwner_->worldTransform();
	Matrix newWorld;
	newWorld.setRotateY( yawOffset );
	newWorld.preMultiply( oldWorld );
	pOwner_->worldTransform( newWorld );
}

BW_END_NAMESPACE

// oscillator.cpp
