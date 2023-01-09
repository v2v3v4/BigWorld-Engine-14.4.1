#include "pch.hpp"
#include "orbitor.hpp"
#include "pymodel.hpp"
#include "pyscript/py_callback.hpp"

DECLARE_DEBUG_COMPONENT2( "Motor", 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( Orbitor )

PY_BEGIN_METHODS( Orbitor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Orbitor )
	/*~ attribute Orbitor.target
	 *	This attribute specifies a target MatrixProvider. The Orbitor Motor will
	 *	trigger an optional callbacks when the model comes within proximity
	 *	distance of the target.
	 *	@type MatrixProvider.
	 */
	PY_ATTRIBUTE( target )
	/*~ attribute Orbitor.speed
	 *  The attribute speed specifies the speed at which the Orbitor will move the
	 *	model towards its destination. 
	 *	@type Float (units/s). Default is 0.1.
	 */
	PY_ATTRIBUTE( speed )
	/*~ attribute Orbitor.startSpin
	 *	The attribute startSpin specifies the distance from the orbiting radius 
	 *	when the trajectory starts to move into an orbiting path. 
	 */
	PY_ATTRIBUTE( startSpin )
	/*~ attribute Orbitor.spinRadius
	 *	The attribute spinRadius specifies the orbit radius of the object controlled
	 *	by the motor
	 *
	 *	@type Float. Default is 5.0.
	 */
	PY_ATTRIBUTE( spinRadius )
	/*~ attribute Orbitor.maxSpeed
	 *	The attribute specifies the maximum speed that the motor will travel
	 *	@type Float. Default is 4.0
	 */
	 PY_ATTRIBUTE( maxSpeed )
	/*~ attribute Orbitor.endSpeed
	 *	The attribute specifies the end speed that the motor will travel (when spinning).
	 *	@type Float. Default is 4.0
	 */
	 PY_ATTRIBUTE( endSpeed )
	 /*~ attribute Orbitor.maxAccel
	 *	The attribute specifies the maximum acceleration of the motor
	 *	@type Float. Default is 5.0
	 */
	 PY_ATTRIBUTE( maxAccel)
	 /*~ attribute Orbitor.slowHeigth
	 *	The slowHeight Attribute sets a verticle distance at which the verticle motion
	 *	towards a target becomes damped and slows to zero
	 *	@type Float. Default is 2.0.
	 */
	 PY_ATTRIBUTE( slowHeigth)
	 /*~ attribute Orbitor.proximity
	 *	Proximity specifies how close to the "target" the model must get before
	 *	the Orbitor Motor calls proximityCallback.
	 *	@type Float. Default is 0.1.
	 */
	PY_ATTRIBUTE( proximity )
	/*~ attribute Orbitor.proximityCallback
	 *	The attribute proximityCallback is used to assign a callback function
	 *	to the Orbitor Motor that is triggered once the model that the Orbitor
	 *	is moving gets within "proximity" distance of "target".
	 *	@type Callback Function (python function model, or an instance of a
	 *	python class that implements the __call__ interface). Default is None.
	 */
	PY_ATTRIBUTE( proximityCallback )
	/*~ attribute Orbitor.weightingFunction
	 *	The attribute weightingFunction is an index that selects from a list of
	 *	possible functions that define how the orbitors behaviour moves from
	 *	purely linear to orbiting. Mainly for testing.
	 */
	PY_ATTRIBUTE( weightingFunction )
	/*~ attribute Orbitor.distanceScaler
	 *	The attribute distanceScaler used in comination with a weigthng function
	 *	to change relative weigths of spin and attraction with distance, can be used
	 *  to create a spiriling effect around the target.
	 */
	PY_ATTRIBUTE( distanceScaler )
	/*~ attribute Orbitor.wobble
	 *	The attribute wobble is a bool that determines weaher orbitor will
	 *  oscillate in the verticle axis.
 	 *	@type bool. Default is true.
	 */
	PY_ATTRIBUTE( wobble)
	/*~ attribute Orbitor.wobbleFreq
	 *	The attribute wobbleFreq sets the frequency of oscillation.
	 *	@type Float. Default is 1.0.
	 */
	PY_ATTRIBUTE( wobbleFreq )
	/*~ attribute Orbitor.wobbleMax
	 *	The attribute wobbleMax sets the maximum vertcle heigth of oscillation
	 *	@type Float. Default is 0.5.
	 */
	PY_ATTRIBUTE( wobbleMax )



PY_END_ATTRIBUTES()

/*~ function BigWorld.Orbitor
 *	Orbitor is a factory function to create a Orbitor Motor. A Orbitor is a Motor
 *	that moves a model towards a target (MatrixProvider) and orbits.
 *	@return A new Orbitor object.
 */
PY_FACTORY( Orbitor, BigWorld )


/**
 *	Constructor
 */
Orbitor::Orbitor( PyTypeObject * pType ):
	Motor( pType ),
	target_( NULL ),
	speed_( 0.0f ),
	startSpin_( 5.0f ),
	spinRadius_( 2.0f ),
	maxSpeed_( 4.0f ),
	endSpeed_( 4.0f ),
	maxAccel_( 1.0f ),
	slowHeigth_( 2.0f ),
	proximity_( 0.1f ),
	proximityCallback_( NULL ),
	distanceScaler_(10.0),
	weightingFunction_(0),
	spinning_(false),
	wobble_(false),
	spinTime_(0.0f),
	wobbleFreq_(3.0f),
	wobbleMax_(0.5f)
{
}

/**
 *	Destructor
 */
Orbitor::~Orbitor()
{
	BW_GUARD;
	Py_XDECREF( proximityCallback_ );
}


/**
 *	Static python factory method
 */
PyObject * Orbitor::pyNew( PyObject * args )
{
	BW_GUARD;
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_Format( PyExc_TypeError,
			"BigWorld.Orbitor brooks no arguments (%d given)",
			PyTuple_Size( args ) );
		return NULL;
	}

	return new Orbitor();
}


/**
 *	Run this motor
 */

void Orbitor::rev( float dTime )
{
	BW_GUARD;
	if (!target_)
	{
		this->makePCBHappen();
		return;
	}
	float inDTime = dTime;

	Matrix world = pOwner_->worldTransform();
	Vector3 currentPos = world.applyToOrigin();

	Matrix tmx;
	target_->matrix( tmx );
	Vector3 targetPos = tmx.applyToOrigin();

	//Create vector that represents horizontal plane motion.
	//Depending on distance to target make this a weighted avarage of an 
	//attraction vector (aiming at target) and a spin vector (orbiting target).
	Vector3 displacement3D = targetPos - currentPos;
	Vector2 disp(displacement3D.x, displacement3D.z);
	float dist = disp.length();
	Vector3 dir = displacement3D;

	//If close enough call proximity callback
	if (displacement3D.length() < proximity_)
	{
		this->makePCBHappen();
	}

	//Check if horizontal displacement is zero
	if (dir.x == 0.0 &&  dir.z == 0.0)
		dir.x = 1.0;

	//dir.normalise();


	Vector2 attraction(disp.x, disp.y);
	Vector2 spin(-disp.y, disp.x);

	//Invert attarction if inside spin radius
	if (dist < spinRadius_)
		attraction = -1.0f * attraction;

	dist -= spinRadius_;

	float spinRatio = 0.0f;
	//Inside orbiting region
	if (fabs(dist) < startSpin_)
	{	
		//Set spinning flag
		spinning_ = true;

		//Normalise range
		float spinDist = 1.0f - (fabsf(dist)/(startSpin_));

		//Weigthing function 
		//Idealy increase from 0 to 1.0 in range [0,1] with dy/dx 0.0 at extremes.
		switch(weightingFunction_)
		{
			case 0:		spinRatio = 0.5f * (sinf((-0.5f + spinDist) * MATH_PI) + 1.0f);
						break;

			case 1:		spinRatio = 0.5f * (tanhf(-distanceScaler_ + 2.0f * distanceScaler_ * spinDist) + 1.0f);
						break;

			default: 	spinRatio = spinDist; 
		}
	}

	//Horizontal motion
	Vector2 moveDir = (spinRatio * spin ) + ((1.f - spinRatio) * attraction);

	//Verticle motion
	float vert = displacement3D.y;
	float incline = vert / dist;

	if (incline > 1.0f)
		incline = 1.0f;
	else if (incline < -1.0f)
		incline = -1.0f;

	if (fabs(vert) < slowHeigth_)
		incline = incline * (fabsf(vert) / slowHeigth_);

	float horiz = moveDir.length();
	vert = horiz * incline;

	//Resulting direction
	Vector3 orbitor(moveDir.x, vert, moveDir.y); 

	//If spinning then aim for end velocity
	if (spinning_)
		maxSpeed_ = endSpeed_;

	//Speed
	if (maxSpeed_ < 0.1f)
		maxSpeed_ = 0.1f;
	float accel = (maxSpeed_ - speed_) * maxAccel_ / maxSpeed_;
	speed_ = speed_ + accel * dTime;




	orbitor.normalise();
	orbitor = speed_ * orbitor;

	//Wobble facter:
	if (spinning_ && wobble_)
	{
		if (fabs(wobbleFreq_) <= 0.01)
			wobbleFreq_ = 1.0;

		float angFreq = wobbleFreq_ * 2.0f * MATH_PI;
		float wobbleVelMax = wobbleMax_ * angFreq;
		float spring = wobbleVelMax * sinf(angFreq * spinTime_);
		Vector3 wobble(0, spring, 0);
		spinTime_ += dTime;
		orbitor += wobble;
	}


	//Simple Euler. 
	//If unstable may need to use more data (prev values or current accel)
	world.translation(currentPos + orbitor * dTime);

	pOwner_->worldTransform( world );

}



/**
 *	This internal method makes the proximity callback happen if it's set.
 */
void Orbitor::makePCBHappen()
{
	BW_GUARD;	
#ifndef EDITOR_ENABLED
	if (proximityCallback_ != NULL)
	{
		// if proximityCallback_ isn't callable then when the callback
		//  happens (next frame) it'll complain about it for us
		Script::callNextFrame(	proximityCallback_,
								PyTuple_New(0),
								"Orbitor Proximity Callback: " );
		proximityCallback_ = NULL;
	}
#endif
}

BW_END_NAMESPACE

// orbitor.cpp
