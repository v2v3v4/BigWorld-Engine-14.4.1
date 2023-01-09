#include "pch.hpp"
#include "flexicam.hpp"

// Standard MF Library Headers.
#include "moo/camera.hpp"
#include "moo/render_context.hpp"


// TODO: Make flexicam work properly when chunking!

DECLARE_DEBUG_COMPONENT2( "Camera", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "flexicam.ipp"
#endif


// -----------------------------------------------------------------------------
// Constructor(s) for FlexiCam.
// -----------------------------------------------------------------------------

/**
 *	The constructor for the FlexiCam class. It simply initialises the various
 *	data members to a default value.
 *
 *	@return None.
 */
FlexiCam::FlexiCam( MatrixProviderPtr pMProv, PyTypeObject * pType ) :
	BaseCamera( pType ),
	positionAcceleration_(),
	pTarget_( pMProv ),
	timeMultiplier_( 1.f ),
	trackingAcceleration_()
{
	BW_GUARD;
	this->init();
}


/**
 *	Init method.
 */
void FlexiCam::init()
{
	BW_GUARD;
	// Camera position defaults set here.
	preferredPos_.set(0.0f, 0.0f, 0.0f);
	actualPos_.set(0.0f, 0.0f, 0.0f);
	positionAcceleration_ = 0.4f;

	// Camera direction defaults set here.
	uprightDir_.set(0.0f, 1.0f, 0.0f);
	actualDir_.set(0.0f, 0.0f, 1.0f);
	viewOffset_.set(0.0f, 0.0f, 0.0f);
	trackingAcceleration_ = 0.2f;
}

// -----------------------------------------------------------------------------
// Methods associated with transforms and the 3D world.
// -----------------------------------------------------------------------------

/**
 *	Updates the camera to the position and direction of the camera supplied.
 *
 *	@param	viewMatrix	The camera to world transform.
 */
void FlexiCam::set( const Matrix & viewMatrix )
{
	BW_GUARD;
	// Invert camera to world matrix.
	Matrix worldToCamera( viewMatrix );
	worldToCamera.invert();

	actualPos_ = worldToCamera.applyToOrigin();
	viewDir_ = worldToCamera.applyToUnitAxisVector( 2 );
	actualDir_ = viewDir_ + actualPos_;
}


/**
 *	Updates the camera position and viewing direction based on time. The
 *	parameters include the scene information and terrain information which
 *	is used to compute collision detection.
 *
 *	@param	deltaTime	The time passed in seconds since last update.
 */
void FlexiCam::update( float deltaTime )
{
	BW_GUARD;
	deltaTime *= timeMultiplier_;

	// TODO: Change variable names.
	// NOTE: In this method, variables that have the name dir should actually be
	// something like lookAtPos. It is the position that the camera is pointing
	// towards. The direction is the vector from the camera position to this
	// lookAtPos.

	Moo::Camera * pCamera = &Moo::rc().camera();

	if (deltaTime <= 0.f)
	{
		return;
	}

	Vector3 finalDesiredPos;
	Vector3 finalDesiredDir;

	Matrix matrix;
	if (pTarget_)
	{
		pTarget_->matrix( matrix );
		finalDesiredPos = matrix.applyPoint( preferredPos_ );
		finalDesiredDir = matrix.applyPoint( viewOffset_ );
	}

	// We want to make sure that the step size is not too big. We want to assume
	// that the position the camera wants to be at and the position the camera
	// wants to look at are interpolated linearly since the previous frame. This
	// makes the flexicam less dependant on framerate.

	const float MAX_ELAPSED_TIME = 0.01f;

	float remainingTime = deltaTime;


	const Vector3 deltaPosPerSecond =
		1.0f/deltaTime * ( finalDesiredPos - actualPos_ );
	const Vector3 deltaDirPerSecond =
		1.0f/deltaTime * ( finalDesiredDir - actualDir_ );


	while ( remainingTime > 0.f )
	{
		float stepSize = min(remainingTime, MAX_ELAPSED_TIME);
		remainingTime -= stepSize;

		// Linear interpolate the desired positions.
		Vector3 currDesiredPos = finalDesiredPos -
			remainingTime * deltaPosPerSecond;
		Vector3 currDesiredDir = finalDesiredDir -
			remainingTime * deltaDirPerSecond;

		// Move the camera towards the preferred position.

		actualPos_ = currDesiredPos +
			powf( 0.5f, stepSize/positionAcceleration_ ) *
			( actualPos_ - currDesiredPos );

		// Turn the camera towards the preferred direction.
		actualDir_ = currDesiredDir +
			powf( 0.5f, stepSize/trackingAcceleration_ ) *
			( actualDir_ - currDesiredDir );
	}


	Vector3 finalPos = actualPos_;

	// Check the terrain for collisions.
	// TODO: This simply forces a minimum height above the height map.
	if (pCamera)
	{
			Vector3 direction = actualPos_ - actualDir_;
			direction.normalise();

			BaseCamera::sceneCheck( finalPos,
									actualDir_,
									direction,
									uprightDir_);
	}

	// Make sure we don't get an undefined direction if camera position
	// happens to be also the target position.
	viewDir_ = actualDir_ - actualPos_;
	if(viewDir_ == Vector3(0.0f, 0.0f, 0.0f))
	{
		viewDir_.set(0.0f, 0.0f, 1.0f);
	}

	// Apply changes to the camera.
	view_.lookAt(finalPos, viewDir_, uprightDir_ );
	invView_.invert( view_ );
}



// -----------------------------------------------------------------------------
// Python stuff
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( FlexiCam )

PY_BEGIN_METHODS( FlexiCam )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( FlexiCam )
	/*~ attribute FlexiCam.preferredPos
	 *
	 *	This attribute specifies where the camera positions itself, relative
	 *	to the targets frame of reference.  It is a Vector3.  Positive
	 *	x-component places the camera to the right of the target, negative to
	 *	the left.  Positive y component places the camera above the target
	 *	negative below.  Positive z component places the camera in front of
	 *	the target, negative behind.  Where ever the camera is positioned,
	 *	it will orient itself to point towards the target.
	 *
	 *	This attrtibute specifies the camera's "preferred" position.  If
	 *	the target suddenly moves, or preferred position is assigned to,
	 *	it will move towards this position over positionAcceleration seconds.
	 *
	 *	Its actual position at all times, whether in the preferred position
	 *	or not, is specified by actualPos.
	 *
	 *	@type	Vector3
	 */
	PY_ATTRIBUTE( preferredPos )
	/*~ attribute FlexiCam.actualPos
	 *
	 *	This attribute specifies where, in world coordinates the camera is
	 *	at the current time.  If it is assigned to, it moves the camera to
	 *	the specified position.
	 *
	 *	The camera will not neccessarily remain at this position, as it moves
	 *	towards the position specified by preferredPos relative to its target.
	 *	It will move at a rate specified by positionAcceleration.
	 *
	 *	@type	Vector3
	 */
	PY_ATTRIBUTE( actualPos )
	/*~ attribute FlexiCam.preferredYaw
	 *
	 *	This read-only attribute specifies the camera's preferred yaw, relative
	 *	to its target.  This is calculated directly from the preferredPos
	 *	attribute.
	 *
	 *	For example:
	 *	@{
	 *	>>>	flex.preferredPos = (0,1,-1)
	 *	>>>	flex.preferredYaw
	 *	0.0
	 *	@}
	 *	In the example, we place the camera one unit above and one unit behind
	 *	 the target, which gives it a yaw of zero when looking at the target.
	 *
	 *	@type	Read-Only Float
	 */
	PY_ATTRIBUTE( preferredYaw )
	/*~ attribute FlexiCam.preferredPitch
	 *
	 *	This read-only attribute specifies the camera's preferred pitch,
	 *	relative to its target.  This is calculated directly from the
	 *	preferredPos attribute.
	 *
	 *	For example:
	 *	@{
	 *	>>>	flex.preferredPos = (0,1,-1)
	 *	>>>	flex.preferredPitch
	 *	-0.78539816339
	 *	@}
	 *	In the example, we place the camera one unit above and one unit behind
	 *	 the target, which gives it a pitch of -pi/4.
	 *
	 *	@type	Read-Only Float
	 */
	PY_ATTRIBUTE( preferredPitch )
	/*~ attribute FlexiCam.positionAcceleration
	 *
	 *	This attribute is the half-life for movement from the camera's
	 *	actualPos to it's desiredPos.  That is, it will move half the
	 *	distance remaining in this time, causing it to slow down as it
	 *	approaches its desiredPos.
	 *
	 *	If this is set to zero, then actualPos is always at the desired
	 *	position.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( positionAcceleration )

	/*~ attribute FlexiCam.target
	 *
	 *	This attribute is the the MatrixProvider that specifies what position
	 *	in world coordinates that the FlexiCam should look at, and also specifies
	 *	the frame of reference that the preferredPos is specified in.
	 *
	 *	The camera actually looks at the point specified by viewOffset, relative
	 *	to target.
	 *
	 *	If the target MatrixProvider is updated, then the camera will move from
	 *	its actualPos to its preferred position with a half life of
	 *	positionAcceleration seconds.  If it needs to turn to look at the target
	 *	it will turn with a half life of trackingAcceleration seconds.
	 *
	 *	@type	MatrixProvider
	 */
	PY_ATTRIBUTE( target )
	/*~ attribute FlexiCam.timeMultiplier
	 *
	 *	Before the FlexiCam is updated, the time delta is scaled by this
	 *	attribute.  This scales the speed at which the camera moves and turns.
	 *
	 *	@type	float
	 */
	PY_ATTRIBUTE( timeMultiplier )
	/*~ attribute FlexiCam.uprightDir
	 *
	 *	This attribute specifies the axis, in world coordinates that will be
	 *	vertical on the camera.
	 *
	 *	@type	Vector3
	 */
	PY_ATTRIBUTE( uprightDir )
	/*~ attribute FlexiCam.actualDir
	 *
	 *	This attribute is the forward direction, in world coordinates, that the
	 *	camera is facing at the current moment.  If it is set, then
	 *	the camera will move back to pointing at the target (modified by the
	 *	viewOffset) with a half life of trackingAcceleration.
	 *
	 *	@type Vector3
	 */
	PY_ATTRIBUTE( actualDir )
	/*~ attribute FlexiCam.viewOffset
	 *
	 *	This attribute is the offset from target, specified in target coordinates,
	 *	that the camera moves towards looking at.
	 *
	 *	It turns towards this target, if it is moved away, with a half-life of
	 *	trackingAcceleration seconds.
	 *
	 *	@type	Vector3
	 */
	PY_ATTRIBUTE( viewOffset )
	/*~ attribute FlexiCam.trackingAcceleration
	 *
	 *	This attribute is the half-life for turning towards the target point.
	 *	The target point is specified by viewOffset, interpreted in the
	 *	frame of reference specified by the target attribute.
	 *
	 *	Half life means that it turns half the angle between its current
	 *	direction and the desired direction in this time.  This causes it to
	 *	slow its rate of turn as it approaches the desired direction.
	 *
	 *	@type float
	 */
	PY_ATTRIBUTE( trackingAcceleration )
PY_END_ATTRIBUTES()

/*~ function BigWorld.FlexiCam
 *
 *	This function creates a new FlexiCam, which can be used to look at a
 *	target from a specified point relative to the target.
 *
 *	@return a new FlexiCam
 */
PY_FACTORY( FlexiCam, BigWorld )

/**
 *	Python factory method
 */
PyObject * FlexiCam::pyNew( PyObject * args )
{
	BW_GUARD;
	return new FlexiCam();
}

BW_END_NAMESPACE

// flexicam.cpp
