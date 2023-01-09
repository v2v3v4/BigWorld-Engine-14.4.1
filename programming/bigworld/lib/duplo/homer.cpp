#include "pch.hpp"
#include "homer.hpp"
#include "pymodel.hpp"
#include "pyscript/py_callback.hpp"

DECLARE_DEBUG_COMPONENT2( "Motor", 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( Homer )

PY_BEGIN_METHODS( Homer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Homer )
	/*~ attribute Homer.target
	 *	This attribute specifies a target MatrixProvider. The Homer Motor will
	 *	trigger an optional callback when the model comes within proximity
	 *	distance of the target.
	 *	@type MatrixProvider.
	 */
	PY_ATTRIBUTE( target )
	/*~ attribute Homer.speed
	 *  The attribute speed specifies the speed at which the Homer will move the
	 *	model towards its destination. 
	 *	@type Float (units/s). Default is 0.1.
	 */
	PY_ATTRIBUTE( speed )
	/*~ attribute Homer.turnRate
	 *	The attribute turnRate specifies the maximum rate (rad/s) at which the
	 *	Homer can turn to face "turnAxis" at the "target".
	 *	@type Float. Default is 0.f.
	 */
	PY_ATTRIBUTE( turnRate )
	/*~ attribute Homer.turnAxis
	 *	The attribute turnAxis specifies the axis that will be be pointed
	 *	towards the model as the Homer turns and moves towards its target.
	 *	@type A sequence of 3 floats ( x, y, z ). Default is ( 1, 0, 0 ).
	 */
	PY_ATTRIBUTE( turnAxis )
	/*~ attribute Homer.tripTime
	 *	The attribute tripTime specifies how long (in seconds) the Homer will
	 *	take to reach its target. If the speed and tripTime are not in agreement
	 *	for the time it should take travelling directly towards the target, then
	 *	the Homer will arc the model towards its target, keeping the speed and
	 *	tripTime valid by effectively increasing the distance the Homer has to
	 *	travel towards the target.
	 *
	 *	If tripTime &lt;= 0 then it is ignored and the Homer makes the model
	 *	travel directly towards its target.
	 *
	 *	If tripTime &gt; 0 then it is reduced by the frame duration at the end of
	 *	each frame.
	 *
	 *	@type Float. Default is 0.
	 */
	PY_ATTRIBUTE( tripTime )
	/*~ attribute Homer.goneTime
	 *	The attribute goneTime is the cumulative amount of time that the Homer
	 *	has been acting on the model (in seconds). It is increased by the frame
	 *	duration at
	 *	the end of every frame, provided that tripTime > 0. It is 0 otherwise.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( goneTime )
	/*~ attribute Homer.proximity
	 *	Proximity specifies how close to the "target" the model must get before
	 *	the Homer Motor calls proximityCallback.
	 *	@type Float. Default is 0.1.
	 */
	PY_ATTRIBUTE( proximity )
	/*~ attribute Homer.proximityCallback
	 *	The attribute proximityCallback is used to assign a callback function
	 *	to the Homer Motor that is triggered once the model that the Homer
	 *	is moving gets within "proximity" distance of "target".
	 *	@type Callback Function (python function model, or an instance of a
	 *	python class that implements the __call__ interface). Default is None.
	 */
	PY_ATTRIBUTE( proximityCallback )
	/*~ attribute Homer.arrivalCountLimit
	 *	The attribute arrivalCountLimit is used to limit the number of times an
	 *	model may need to move a smaller distance than that required by the
	 *	frame duration and speed to reach the target (ie the last frame before
	 *	it reaches the target). This acts to trigger the proximity callback,
	 *	even if the model is not within proximity distance of the target. This
	 *	is a failsafe so that if the proximity is too small it acts to stop the
	 *	Homer from "buzzing" around its target - in effect never reaching it
	 *	target because the model's position cannot move close enough to the
	 *	target to get inside its proximity radius.
	 *	@type Integer. Default is 1.
	 */
	PY_ATTRIBUTE( arrivalCountLimit )
	/*~ attribute Homer.scale
	 *	The attribute scale is used to apply a scaling factor to the model over
	 *	time. If scale > 1 then the model will start at its original size and
	 *	be scaled linearly over scaleTime seconds to reach the specified scale.
	 *	When the model reaches half-way point of its trip (based on its
	 *	movement duration as a fraction of tripTime), it scales back to its
	 *	original size, regardless of whether or not it has reach its maximum
	 *	scale.
	 *	@type Float. Default is 0.
	 */
	PY_ATTRIBUTE( scale )
	/*~ attribute Homer.scaleTime
	 *	The attribute scaleTime is the duration over which the model will be
	 *	scaled linearly to the relative size of "scale".
	 *	When the model reaches half-way (based on its
	 *	movement duration as a fraction of tripTime), it scales back to its 
	 *	original size, regardless of whether or not it has reach its maximum
	 *	scale.
	 *	@type Float. Default is 1.
	 */
	PY_ATTRIBUTE( scaleTime )
PY_END_ATTRIBUTES()

/*~ function BigWorld.Homer
 *	Homer is a factory function to create a Homer Motor. A Homer is a Motor
 *	that moves a model towards a target (MatrixProvider).
 *	@return A new Homer object.
 */
PY_FACTORY( Homer, BigWorld )


/**
 *	Constructor
 */
Homer::Homer( PyTypeObject * pType ):
	Motor( pType ),
	target_( NULL ),
	speed_( 0.1f ),
	turnRate_( 0.f ),
	turnAxis_( 1, 0, 0 ),
	tripTime_( -1.f ),
	goneTime_( 0.f ),
	zenithed_( false ),
	proximity_( 0.1f ),
	proximityCallback_( NULL ),
	arrivalCount_( 0 ),
	arrivalCountLimit_( 1 ),
	scale_( 0.f ),
	scaleTime_( 1.f )
{
}

/**
 *	Destructor
 */
Homer::~Homer()
{
	BW_GUARD;
	Py_XDECREF( proximityCallback_ );
}


/**
 *	Static python factory method
 */
PyObject * Homer::pyNew( PyObject * args )
{
	BW_GUARD;
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_Format( PyExc_TypeError,
			"BigWorld.Homer brooks no arguments (%d given)",
			PyTuple_Size( args ) );
		return NULL;
	}

	return new Homer();
}


/*
 *	For arrows and other things, you simply specify the amount
 *	of time you want it to take to get there, and it bends
 *	the line up appropriately.
 */

/**
 *	Run this motor
 */
void Homer::rev( float dTime )
{
	BW_GUARD;
	if (!target_)
	{
		this->makePCBHappen();
		return;
	}

	float inDTime = dTime;

	Matrix world = pOwner_->worldTransform();

	Vector3 worldPos = world.applyToOrigin();
	Matrix tmx;
	target_->matrix( tmx );
	Vector3 worldTgt = tmx.applyToOrigin();
	Vector3 diff = worldTgt - worldPos;
	float diffLength = diff.length();

	// move the target back by proximity_ towards us
	Vector3 diffDir( diff );
	if (diffLength != 0.f) diffDir /= diffLength;
	Vector3 toSub = diffDir * proximity_;
	worldTgt -= toSub;
	diff -= toSub;
	diffLength -= proximity_;

	// get out now if we're close enough
	if (diffLength <= 0.f || arrivalCount_ > arrivalCountLimit_)
	{
		this->makePCBHappen();
		return;
	}

	Vector3 dir( 0, 0, 0 );
	float yaccel = 0.f;

	// if we want to get there in a certain time, figure out
	//  how much we have to bend our trajectory to do it
	if (tripTime_ > 0.f)
	{
		float sx = sqrtf( diff.x*diff.x + diff.z*diff.z );
		float sy = diff.y;
		//INFO_MSG( "Homer: sx is %f, sy is %f; ", sx, sy );

		float ux = sx / tripTime_;
		// ux is the speed we need to allocate to movement in the XZ plane

		if (diff.y > 0.f) zenithed_ = false;
		float uy = sqrtf( speed_*speed_ - ux*ux ) * (zenithed_?-1.f:1.f);
		// uy is our initial speed along the Y axis

		float ay = (2.f * (sy - uy * tripTime_)) / (tripTime_*tripTime_);
		// ay is the acceleration along the y axis
		//INFO_MSG( "Homer: ux is %f, uy is %f, ay is %f\n", ux, uy, ay );

		if (ux < speed_)
		{
			// split ux in ratio diff.x:diff.z, s.t. dir.x*dir.x+dir.z*dir.z = ux
			dir.x = ux * diff.x / sx;
			dir.y = uy;
			dir.z = ux * diff.z / sx;
			dir /= speed_;
			yaccel = ay;
		}
		else
		{
			dir = diff / diffLength;
		}

		// shorten dTime to make us hit our target
		dTime = min( dTime, tripTime_ );

		// take dTime off tripTime_
		tripTime_ = max( 0.f, tripTime_ - dTime );

		// update the time that has gone ... only for scaling :-(
		goneTime_ += dTime;
	}
	else
	{
		dir = diff / diffLength;

		// shorten dTime to make us hit our target
		dTime = min( dTime, diffLength / speed_ );

		// gone time is not valid when tripTime is negative
		goneTime_ = 0.f;
	}

	if (turnRate_ > 0.f)
	{
		Quaternion qOrig = Quaternion( world );
		qOrig.normalise();

		Vector3 vAxis = turnAxis_.crossProduct( dir );
		Quaternion qNew;
		qNew.fromAngleAxis( acosf( Math::clamp( -1.f, turnAxis_.dotProduct( dir ), 1.f ) ), vAxis );
		qNew.normalise();

		float wantTurn = acosf( Math::clamp( -1.f, qOrig.dotProduct( qNew ), 1.f ) );
		float haveTurn = Math::clamp( turnRate_*dTime, wantTurn );

		if (fabs(wantTurn) > 0.001f)
		{
			qNew.slerp( qOrig, qNew, haveTurn/wantTurn );
		}

		// preserve scale
		float scale = world[0].length();
		world.setRotate( qNew );
		world[0] *= scale;
		world[1] *= scale;
		world[2] *= scale;
	}

	if (scale_ > 1.f)
	{
		float eprox = min( goneTime_, tripTime_) / scaleTime_;
		if (eprox > 1.f) eprox = 1.f;
		float scale = 1.f + (scale_ - 1.f) * eprox;
		world[0] *= scale / world[0].length();
		world[1] *= scale / world[1].length();
		world[2] *= scale / world[2].length();
	}

	Vector3 vel( dir * speed_ );
	Vector3 acc( 0, yaccel, 0 );	// yaccel includes speed weighting

	world.translation(
		worldPos +
		vel * dTime +
		acc * (0.5f*dTime*dTime) );

	Vector3 nextvel = vel + (acc * dTime);
	speed_ = nextvel.length();
	if (vel.y >= 0.f && nextvel.y <= 0.f) zenithed_ = true;

	pOwner_->worldTransform( world );

	if (dTime != inDTime)
		arrivalCount_++;
	else
		arrivalCount_ = 0;
}


/**
 *	This internal method makes the proximity callback happen if it's set.
 */
void Homer::makePCBHappen()
{
	BW_GUARD;	
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

BW_END_NAMESPACE

// homer.cpp
