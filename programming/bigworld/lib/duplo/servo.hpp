#ifndef SERVO_HPP
#define SERVO_HPP

#include "motor.hpp"
#include "pyscript/script_math.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is a motor for a model that sets the position
 *	of the model based on a signal.
 *
 *	This class actually sets the transform of a model to the
 *	given MatrixProvider.
 */
class Servo : public Motor
{
	Py_Header( Servo, Motor )

public:
	Servo( MatrixProviderPtr s, PyTypeObject * pType = &Servo::s_type_ );
	~Servo();

	PY_FACTORY_DECLARE()
	PY_RW_ATTRIBUTE_DECLARE( signal_, signal )

	virtual void rev( float dTime );
private:
	MatrixProviderPtr	signal_;
};

BW_END_NAMESPACE

#endif
