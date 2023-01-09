#include "pch.hpp"
#include "servo.hpp"
#include "pymodel.hpp"
#include "duplo/pymodelnode.hpp"

BW_BEGIN_NAMESPACE

/*~	class BigWorld.Servo
 *	The Servo class is a Motor that sets the transform 
 *	of a model to the given MatrixProvider.  As the 
 *	MatrixProvider is updated, the model will move.
 */
PY_TYPEOBJECT( Servo )

PY_BEGIN_METHODS( Servo )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Servo )
	/*~ attribute Servo.signal
	 *
	 *	This attribute is the MatrixProvider that sets the model's transform.
	 *	As this MatrixProvider is updated, the model will be transformed accordingly.
	 *
	 *	@type	Read-Write MatrixProvider
	 */
	PY_ATTRIBUTE( signal )
PY_END_ATTRIBUTES()

/*~	function BigWorld.Servo
 *
 *	Creates and returns a new Servo Motor, which sets the 
 *	transform of a model to the given MatrixProvider.  As 
 *	the MatrixProvider is updated, the model will move.
 *
 *	@param signal	MatrixProvider that model will follow
 */
PY_FACTORY( Servo, BigWorld )


/**
 *	Constructor.
 */
Servo::Servo( MatrixProviderPtr s, PyTypeObject * pType ):
	Motor( pType ),
	signal_( s )
{
}

/**
 *	Destructor.
 */
Servo::~Servo()
{
}


/**
 *	This method runs this motor.
 */
void Servo::rev( float dTime )
{
	BW_GUARD;
	if ( signal_ )
	{
		Matrix newWorld;
		{
			SCOPED_DISABLE_FRAME_BEHIND_WARNING;
			signal_->matrix( newWorld );
		}
		pOwner_->worldTransform( newWorld );
	}
}


/**
 *	Static python factory method
 */
PyObject * Servo::pyNew( PyObject * args )
{
	BW_GUARD;
	PyObject* pObject;

	if (!PyArg_ParseTuple( args, "O", &pObject ) || !MatrixProvider::Check( pObject ))
	{
		PyErr_SetString(PyExc_TypeError, "Servo takes a matrix provider" );
		return NULL;
	}

	return new Servo( (MatrixProvider*)pObject );
}

BW_END_NAMESPACE

// servo.cpp
