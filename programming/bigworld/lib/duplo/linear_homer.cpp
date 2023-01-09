#include "pch.hpp"
#include "linear_homer.hpp"
#include "pymodel.hpp"
#include "math/blend_transform.hpp"
#include "pyscript/py_callback.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.LinearHomer
 *
 *	A LinearHomer is a Motor that accelerates a model towards a target (MatrixProvider).
 *	Additionally an optional proximity callback can be set for the LinearHomer to 
 *	call once the model it is moving gets within a specified (proximity) distance 
 *	of its target.  It has a different set of features to a Homer Motor.
 *
 *	A new LinearHomer Motor is created using BigWorld.LinearHomer function.
 */
PY_TYPEOBJECT( LinearHomer )

PY_BEGIN_METHODS( LinearHomer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( LinearHomer )

	/*~ attribute LinearHomer.target
	 *	This attribute specifies a target MatrixProvider. The Homer Motor will
	 *	trigger an optional callback when the model comes within proximity
	 *	distance of the target.
	 *	@type MatrixProvider
	 */
	PY_ATTRIBUTE( target )

	/*~	attribute LinearHomer.velocity
	 *	Velocity is the current direction and speed of the Model the Motor is driving.
	 *	Changing the velocity will cause a change in momentum.
	 *	@type 3-tuple
	 */
	PY_ATTRIBUTE( velocity )

	/*~ attribute LinearHomer.acceleration
	 *  The attribute acceleration specifies the rate of acceleration at which the 
	 *	LinearHomer will move the model towards its destination.  Default is 0.1
	 *	@type float
	 */
	PY_ATTRIBUTE( acceleration )

	/*~ attribute LinearHomer.proximity
	 *	Proximity specifies how close to the "target" the model must get before
	 *	the Homer Motor calls proximityCallback.  Default is 0.1
	 *	@type float
	 */
	PY_ATTRIBUTE( proximity )

	/*~ attribute LinearHomer.proximityCallback
	 *	The attribute proximityCallback is used to assign a callback function
	 *	to the LinearHomer Motor that is triggered once the model that the LinearHomer
	 *	is moving gets within "proximity" distance of "target".  Default is None.
	 *	@type CallbackFunction
	 *	Note:  type CallbackFunction is a python function or an instance of a
	 *	python class that implements the __call__ interface.
	 */
	PY_ATTRIBUTE( proximityCallback )

	/*~	attribute LinearHomer.align
	 *	If provided, aligns this motor to the given matrix.
	 *	@type MatrixProvider
	 */
	PY_ATTRIBUTE( align )

	/*~	attribute LinearHomer.up
	 *	Specifies which axis is up.
	 *	@type 3-tuple
	 */
	PY_ATTRIBUTE( up )

	/*~	attribute LinearHomer.revDelay
	 *	Provides an initial delay for the motor in ticks.
	 *	@type int
	 */
	PY_ATTRIBUTE( revDelay )

	/*~	attribute LinearHomer.offsetProvider
	 *	If provided, this will offset Motor from the Model by the given amount
	 *	@type MatrixProvider
	 */
	PY_ATTRIBUTE( offsetProvider )

	/*~	attribute LinearHomer.pitchRollBlendInTime
	 *	Time in seconds it should take to blend in pitch and roll animations.
	 *	@type float
	 */
	PY_ATTRIBUTE( pitchRollBlendInTime )

	/*~	attribute LinearHomer.blendOutTime
	 *	Time in seconds it should take to blend out pitch and roll animations.
	 *	@type float
	 */
	PY_ATTRIBUTE( blendOutTime )

PY_END_ATTRIBUTES()

/*~	function BigWorld.LinearHomer
 *
 *	This function returns a new LinearHomer object, which is Motor designed to drive a Model towards it's 
 *	target in a straight line.  It has a different set of features to the Homer Motor.
 *
 *	@return	A new LinearHomer object
 */
PY_FACTORY( LinearHomer, BigWorld )


/**
 *	Constructor.
 */
LinearHomer::LinearHomer(PyTypeObject * pType ):
	Motor( pType ),
	velocity_( 0,0,0 ),
	acceleration_( 1 ),
	proximity_( 0 ),
	proximityCallback_( NULL ),
	up_( 0, 1, 0 ),
	revDelay_( 0 ),
	pitchRollBlendInTime_( 0 ),
	pitchRollBlendIn_( 1 ),
	updatePitchRollRef_( true ),
	pitchRollRef_( Matrix::identity ),
	blendOutTime_( 0 ),
	blendOut_( 1 )
{
}

/**
 *	Destructor.
 */
LinearHomer::~LinearHomer()
{
}


/**
 *	This method runs this motor.
 */
void LinearHomer::rev( float dTime )
{
	BW_GUARD;
	if (!target_ || !pOwner_)
		makePCBHappen();

	if (revDelay_ > 0)
	{
		revDelay_--;
		return;
	}


	Matrix world = pOwner_->worldTransform();

	if (updatePitchRollRef_)
	{
		pitchRollRef_ = world;
		updatePitchRollRef_ = false;
	}

	Matrix target;
	target_->matrix(target);

	if (offsetProvider_)
	{
		Matrix offset;
		offsetProvider_->matrix(offset);

	}

	Vector3 pos = world.applyToOrigin();
	Vector3 tgt = target.applyToOrigin();

	Matrix offset(Matrix::identity);
	if (offsetProvider_)
	{
		offsetProvider_->matrix(offset);
        tgt += pos - offset.applyToOrigin();
	}

	Vector3 tgtOffset = tgt - pos;

	if (tgtOffset.lengthSquared() >= proximity_ )
	{
		Vector3 dir = tgtOffset;
		dir.normalise();
		Vector3 acc = dir * acceleration_;

		Vector3 moveDir = velocity_ * dTime + acc * ( dTime * dTime );
		Vector3 newPos = pos + moveDir;
		velocity_ += dTime * acc;

		Vector3 velDir = velocity_;
		if (velDir.lengthSquared())
			velDir.normalise();
		else
			velDir.set( 0,0,1);

		float travelDist = moveDir.length();
		if (travelDist)
			moveDir *= 1.f/travelDist;

        float dot = moveDir.dotProduct( tgtOffset );
		if (dot < travelDist)
			travelDist = dot;

		moveDir *= travelDist;

		float distSq = (moveDir - tgtOffset).lengthSquared();

		if (distSq < (proximity_ * proximity_))
		{
			newPos = pos + moveDir;
			if (!blendOutTime_)
				makePCBHappen();
		}

		world.lookAt( newPos, velDir, up_ );
		world.invert();
		if (align_)
		{
			Matrix align;
			align_->matrix( align );
			world.preMultiply( align );
		}

		if (pitchRollBlendInTime_ && (pitchRollBlendIn_ > 0))
		{
			pitchRollBlendIn_ -= dTime / pitchRollBlendInTime_;
			pitchRollBlendIn_ = max( 0.f, min( 1.f, pitchRollBlendIn_ ) );
			if (pitchRollBlendIn_ > 0)
			{
				Matrix m = pitchRollRef_;
				m.translation( world.applyToOrigin() );
				BlendTransform b( m );
				BlendTransform w( world );
				w.blend( pitchRollBlendIn_, b );
				w.output( world );
			}
		}

		pOwner_->worldTransform( world );
	}
	else if ( (blendOutTime_ > 0) && blendOut_ )
	{
		blendOut_ -= dTime / blendOutTime_;
		blendOut_ = max( 0.f, min( 1.f, blendOut_ ) );
		Vector3 off(offset.applyToOrigin());
		Vector3 offAlign( off - pos );

		Matrix invWorld = world;
		invWorld.invertOrthonormal();
		offAlign = invWorld.applyVector( offAlign );

		Matrix m = pitchRollRef_;
		m.translation( world.applyToOrigin() );
		BlendTransform b( m );
		BlendTransform w( world );
		b.blend( blendOut_, w );
		b.output( world );

		offAlign = world.applyVector( offAlign );
		world.translation( off - offAlign );
		pOwner_->worldTransform( world );
	}
	else
	{
		makePCBHappen();
	}
}

void LinearHomer::makePCBHappen()
{
	BW_GUARD;
	acceleration_ = 0;
	velocity_.set(0,0,0);

#ifndef EDITOR_ENABLED
	if (proximityCallback_ != NULL)
	{
		// if proximityCallback_ isn't callable then when the callback
		//  happens (next frame) it'll complain about it for us
		Script::callNextFrame(
			proximityCallback_,
			PyTuple_New(0),
			"Homer Proximity Callback: " );
		proximityCallback_ = NULL;
	}
#endif
}


/**
 *	Static python factory method
 */
PyObject * LinearHomer::pyNew( PyObject * args )
{
	BW_GUARD;
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_Format( PyExc_TypeError,
			"BigWorld.LinearHomer brooks no arguments (%d given)",
			PyTuple_Size( args ) );
		return NULL;
	}

	return new LinearHomer( );
}

BW_END_NAMESPACE

// linear_homer.cpp
