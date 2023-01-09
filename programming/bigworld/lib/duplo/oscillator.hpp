#ifndef OSCILLATOR_HPP
#define OSCILLATOR_HPP

#include "motor.hpp"
#include "pyscript/script.hpp"
#include "math/vector3.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.Oscillator
 *	The Oscillator class is a Motor that "oscillates" a model. It applies
 *	an oscillating rotational component to the models world transform.
 *
 *	A sine function is applied across the period of oscillation, producing
 *	oscillations in radians from -1 to 1. Amplitude is applied as a scaling
 *	factor to this result, producing an oscillation from -amplitude to
 *	+amplitude in radians. Offset is applied as an addition to offset the start
 *	of the period (in seconds).
 *
 *	Example:
 *	@{
 *	model = BigWorld.Model( "path/to/model/mymodel.model" )
 *	oscillator = BigWorld.Oscillator( 0, 4, -1 )
 *	model.addMotor( oscillator )
 *	@}
 */

/**
 *	This class is a Motor that oscillates a camera or turret.
 */
class Oscillator : public Motor
{
	Py_Header( Oscillator, Motor )

public:
	Oscillator( float yaw, float period, float amplitude, float offset,
		PyTypeObject * pType = &Oscillator::s_type_ );
	~Oscillator();

	virtual void rev( float dTime );
	virtual void attach( PyModel * pOwner );
	virtual void detach();

	PY_FACTORY_DECLARE()

	PyObject * pyGet_target();
	int pySet_target( PyObject * pObject );

	PY_RW_ATTRIBUTE_DECLARE( yaw_, yaw )

	PY_RW_ATTRIBUTE_DECLARE( period_, period )
	PY_RW_ATTRIBUTE_DECLARE( amplitude_, amplitude )
	PY_RW_ATTRIBUTE_DECLARE( offset_, offset )

	bool canSee( const Vector3 & entityPos ) const;
	PY_AUTO_METHOD_DECLARE( RETDATA, canSee, ARG( Vector3, END ) )

protected:
	float period_;
	float amplitude_;
	float offset_;

	float yaw_;
};


/*~ class BigWorld.Oscillator2
 *	The Oscillator2 class is a Motor that "oscillates" a model. It applies
 *	an oscillating rotational component around the world origin to the model's
 *	world transform (this is in contrast to the Oscillator which applies the
 *	rotational component around the model's origin).
 *	
 *	Oscillator2 is an extension of the Oscillator class. See Oscillator for more
 *	information. 
 *
 *	Example:
 *	@{
 *	model = BigWorld.Model( "path/to/model/mymodel.model" )
 *	oscillator2 = BigWorld.Oscillator2( 0, 4, -1 )
 *	model.addMotor( oscillator2 )
 *	@}
 */

/**
 *	This is a very special type of oscillator, that post-multiplies rotations
 *	onto the original transform.
 */
class Oscillator2 : public Oscillator
{
	Py_Header( Oscillator2, Oscillator )

public:
	Oscillator2( float yaw, float period, float amplitude, float offset,
		PyTypeObject * pType = &Oscillator::s_type_ );
	~Oscillator2();

	PY_FACTORY_DECLARE()

	virtual void rev( float dTime );
};

BW_END_NAMESPACE

#endif // OSCILLATOR_HPP
