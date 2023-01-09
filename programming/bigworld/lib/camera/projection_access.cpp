#include "pch.hpp"
#include "projection_access.hpp"

#include "moo/render_context.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"

#include "direction_cursor.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ProjectionAccess
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ProjectionAccess::ProjectionAccess( PyTypeObject * pType ) :
	PyObjectPlusWithWeakReference( pType ),
	smoothFovTransition_( false ),
	fovTransitionStart_(),
	fovTransitionEnd_(),
	fovTransitionTime_(),
	fovTransitionTimeElapsed_()
{
}


/**
 *	Destructor.
 */
ProjectionAccess::~ProjectionAccess()
{
}

/**
 *	Update any timed effects on the camera
 */
void ProjectionAccess::update( float dTime )
{
	BW_GUARD;
	if (!smoothFovTransition_) return;

	// All angles in this method are in radians
	Moo::Camera & camera = Moo::rc().camera();
	float newFOV;

	// Update the FOV and elapsed time
	fovTransitionTimeElapsed_ += dTime;
	if ( fovTransitionTimeElapsed_ < fovTransitionTime_ )
	{
		float percent = fovTransitionTimeElapsed_ / fovTransitionTime_;
		newFOV = fovTransitionStart_ +
			percent * ( fovTransitionEnd_ - fovTransitionStart_ );
	}
	else
	{
		newFOV = fovTransitionEnd_;
		smoothFovTransition_ = false;
	}

	// Set the new FOV.
	camera.fov( newFOV );
}


/*~ function ProjectionAccess.rampFov
 *
 *	This function changes from the current fov (field of view, in radians) to
 *  the specified one over the specified time period.  It does this
 *	using linear interpolation.  Note the fov is measured vertically.
 *
 *	@param	newFov		The fov to move toward
 *	@param	time		The time in seconds to take to move between the current
 *						fov and the specified fov
 */
/**
 *	Start ramping the fov between the given values
 */
void ProjectionAccess::rampFov( float newFOV, float timeAllowed )
{
	BW_GUARD;
	smoothFovTransition_ = true;

	fovTransitionStart_ = this->fov();
	fovTransitionEnd_ = newFOV;
	fovTransitionTime_= timeAllowed;
	fovTransitionTimeElapsed_ = 0.0f;
}


/**
 *	Get the near plane
 */
float ProjectionAccess::nearPlane() const
{
	BW_GUARD;
	return Moo::rc().camera().nearPlane();
}

/**
 *	Set the near plane
 */
void ProjectionAccess::nearPlane( float val )
{
	BW_GUARD;
	Moo::rc().camera().nearPlane( val );
}


/**
 *	Get the far plane
 */
float ProjectionAccess::farPlane() const
{
	BW_GUARD;
	return Moo::rc().camera().farPlane();
}

/**
 *	Set the far plane
 */
void ProjectionAccess::farPlane( float val )
{
	BW_GUARD;
	ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
	if (space.exists())
	{
		space->enviro().setFarPlane( val );
	}
}


/**
 *	Get the vertical field of view angle
 */
float ProjectionAccess::fov() const
{
	BW_GUARD;
	return Moo::rc().camera().fov();
}

/**
 *	Set the vertical field of view angle
 */
void ProjectionAccess::fov( float val )
{
	BW_GUARD;
	smoothFovTransition_ = false;

	Moo::rc().camera().fov( val );
}





// -----------------------------------------------------------------------------
// Section: Python stuff
// -----------------------------------------------------------------------------



PY_TYPEOBJECT( ProjectionAccess )

PY_BEGIN_METHODS( ProjectionAccess )
	PY_METHOD( rampFov )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ProjectionAccess )
	/*~ attribute ProjectionAccess.nearPlane
	 *
	 *	This attribute is the distance to the near clipping plane for the
	 *	current camera.  All geometry closer to the	eye than this will be
	 *	clipped out.
	 *	
	 *	@type	Float
	 */
	PY_ATTRIBUTE( nearPlane )
	/*~ attribute ProjectionAccess.farPlane
	 *
	 *	This attribute is the distance to the far clipping plane for the
	 *	current camera.  All geometry further from the eye than this will be
	 *	clipped out.
	 *	
	 *	@type	Float
	 */
	PY_ATTRIBUTE( farPlane )
	/*~ attribute ProjectionAccess.fov
	 *
	 *	The vertical field of view of the current camera, measured in radians. This
	 *	number must be between 0 and pi.  Setting to a number outside of this
	 *	is likely to cause video driver crashes.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( fov )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( ProjectionAccess )

BW_END_NAMESPACE

// projection_access.cpp
