#include "pch.hpp"
#include "base_camera.hpp"

#include "moo/render_context.hpp"
#include "input/py_input.hpp"

#include "physics2/worldtri.hpp"
#include "physics2/worldpoly.hpp"

#include "space/deprecated_space_helpers.hpp"
#include "space/client_space.hpp"

#include "collision_advance.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "base_camera.ipp"
#endif

bool BaseCamera::checkCameraTooClose_;


/**
 *	Constructor.
 */
BaseCamera::BaseCamera( PyTypeObject * pType ) :
	PyObjectPlusWithWeakReference( pType ),
	view_( Matrix::identity ),
	invView_( Matrix::identity ),
	viewMatrixProvider_( NULL ),
	invViewMatrixProvider_( NULL ),
	spaceID_( 0 )
{
}

/**
 *	Destructor.
 */
BaseCamera::~BaseCamera()
{
}

/*~ function BaseCamera.render
 *
 *	This function has no effect
 */
/**
 *	Render method. Simply sets the view transform to our one.
 */
void BaseCamera::render()
{
	Moo::rc().view( view_ );
}


// -----------------------------------------------------------------------------
// Section: Python stuff
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( BaseCamera )

PY_BEGIN_METHODS( BaseCamera )
	PY_METHOD( set )
	PY_METHOD( update )
	PY_METHOD( render )
	/*~ function BaseCamera.handleKeyEvent
	 *
	 *	This function allows the camera to handle the given key event. If
	 *	the camera handles the event it returns True, otherwise it returns False.
	 *	
	 *	@param	event	A PyKeyEvent representing the event.
	 *
	 *	@return			True if the event was handled, False if it was not.
	 */
	PY_METHOD( handleKeyEvent )
	/*~ function BaseCamera.handleMouseEvent
	 *
	 *	This function allows the camera to handle the given mouse movement event. If
	 *	the camera handles the event it returns True, otherwise it returns False.
	 *	
	 *	@param	event	A PyMouseEvent representing the event.
	 *
	 *	@return			True if the event was handled, False if it was not.
	 */
	PY_METHOD( handleMouseEvent )
	/*~ function BaseCamera.handleAxisEvent
	 *
	 *	This function allows the camera to handle the given joystick axis event. If
	 *	the camera handles the event it returns True, otherwise it returns False.
	 *	
	 *	@param	event	A PyAxisEvent representing the event.
	 *
	 *	@return			True if the event was handled, False if it was not.
	 */
	PY_METHOD( handleAxisEvent )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( BaseCamera )
	/*~ attribute BaseCamera.position
	 *
	 *	The current location of the camera within the world
	 *
	 *	@type Read-Only Vector3
	 */
	PY_ATTRIBUTE( position )
	/*~ attribute BaseCamera.direction
	 *
	 *	The current facing of the camera within the world
	 *
	 *	@type	Read-Only Vector3
	 */
	PY_ATTRIBUTE( direction )
	/*~ attribute BaseCamera.matrix
	 *
	 *	The MatrixProvider which specifies the current transform of the camera 
	 *	within the world
	 *
	 *	@type	Read-Only MatrixProvider
	 */
	PY_ATTRIBUTE( matrix )
	/*~ attribute BaseCamera.invViewMatrix
	 *
	 *	The MatrixProvider which specifies the current inverse transform of the camera 
	 *	within the world ( i.e. the camera's transform as a world object )
	 *
	 *	@type	Read-Only MatrixProvider
	 */
	PY_ATTRIBUTE( invViewMatrix )
	/*~	attribute BaseCamera.spaceID
	 *	
	 *	This is the ID of the space in which the camera resides. If zero the
	 *	camera derives its space from that of the player. Default is zero.
	 *
	 *	@type	SpaceID (int)
	 */
	PY_ATTRIBUTE( spaceID )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( BaseCamera )


/*~ function BaseCamera.set
 *
 *	This function sets the transformation matrix for the camera to
 *	the specified MatrixProvider.  It doesn't actually use the specified
 *  MatrixProvider directly but copies it.
 *	
 *	@param	matrix	The MatrixProvider which the camera is being moved to.
 */
/**
 *	This method sets the matrix for this camera from a matrix provider
 */
void BaseCamera::set( ConstSmartPointer<MatrixProvider> pMP )
{
	Matrix m;
	pMP->matrix( m );
	this->set( m );
}

/**
 *	This helper class provides a matrix from a camera
 */
class CameraMatrixProvider : public MatrixProvider
{
public:
	typedef WeakPyPtr<BaseCamera> CameraWPtr;

	CameraMatrixProvider( BaseCamera * pCam,
			Matrix BaseCamera::*pMatrix ) :
		MatrixProvider( false, &s_type_ ),
		pCam_( pCam ),
		pMatrix_( pMatrix )		{ }

	~CameraMatrixProvider()		{ }

private:
	virtual void matrix( Matrix & m ) const
	{
		if (!pCam_)
		{
			m.setIdentity();
		}
		else
		{
			m = (*pCam_).*pMatrix_;
		}
	}

	CameraWPtr pCam_;
	Matrix BaseCamera::*pMatrix_;
};

/**
 *	This method returns a matrix provider for this camera's view matrix
 */
MatrixProviderPtr BaseCamera::viewMatrixProvider() const
{
	BW_GUARD;
	if (!viewMatrixProvider_)
	{
		viewMatrixProvider_ = MatrixProviderPtr(
			new CameraMatrixProvider( const_cast<BaseCamera*>(this), 
				&BaseCamera::view_ ),
			MatrixProviderPtr::STEAL_REFERENCE
		);
	}
	return viewMatrixProvider_;
}


/**
 *	This method returns a matrix provider for this camera's view matrix
 */
MatrixProviderPtr BaseCamera::invViewMatrixProvider() const
{
	BW_GUARD;
	if (!invViewMatrixProvider_)
	{
		invViewMatrixProvider_ = MatrixProviderPtr(
			new CameraMatrixProvider( const_cast<BaseCamera*>(this),
				&BaseCamera::invView_ ),
			MatrixProviderPtr::STEAL_REFERENCE
		);
	}
	return invViewMatrixProvider_;
}


/**
 *	This function returns the spaceID of the camera.
 *
 *	@return		Returns the ID of the camera's current space or zero if none
 *				is set. 
 */
SpaceID	BaseCamera::spaceID() const
{
	return spaceID_;
}


/**
 *	This method sets the spaceID of the camera.
 *
 *	@param	spaceID		The new spaceID for the camera or zero.
 */
void BaseCamera::spaceID( SpaceID spaceID )
{
	spaceID_ = spaceID;
}


/**
 *	This method repositions the camera position so that there is nothing between
 *	it and the valid position.
 *
 *	@param	cameraPosInWS		The camera position in world coordinates.
 *	@param	validPosInWS		The position to slide from.
 *	@param	direction			The direction of the camera.
 *	@param	uprightDirection	The up direction of the camera.
 *
 *	@return	True if the camera collided with the scene. False, otherwise.
 */
bool BaseCamera::sceneCheck(
		Vector3 & cameraPosInWS,
		const Vector3 & validPosInWS,
		const Vector3 & direction,
		const Vector3 & uprightDirection )
{
	BW_GUARD;
	bool collided = false;

	const Moo::Camera * pCamera = &Moo::rc().camera();
	const ClientSpacePtr& pSpace = DeprecatedSpaceHelpers::cameraSpace();
	if (pSpace == NULL) return false;

	// "Near plane" is used to mean the intersection of the near plane
	// with the clip cone (pyramid).

	// zAxis is the vector from the camera position to the centre of the
	// near plane of the camera.
//	Vector3 zAxis = cameraLookAtInWS - cameraPosInWS;
//	zAxis.normalise();
	Vector3 zAxis = direction;

	// xAxis is the vector from the centre of the near plane to its right
	// edge.
	Vector3 xAxis = uprightDirection.crossProduct( zAxis );
	xAxis.normalise();

	// yAxis is the vector from the centre of the near plane to its top
	// edge.
	Vector3 yAxis = zAxis.crossProduct( xAxis );

	const float fov = pCamera->fov();
	const float nearPlane = pCamera->nearPlane();
	const float aspectRatio = pCamera->aspectRatio();

	const float yLength = nearPlane * tanf( fov / 2.0f );
	const float xLength = yLength * aspectRatio;

	xAxis *= xLength;
	yAxis *= yLength;
	zAxis *= nearPlane;

	float dist = 1.0f;
	// The centre of the near plane at the desired location.
	Vector3 nearPlaneCentre( cameraPosInWS + zAxis );

	// Construct near plane at the valid position.
	const Vector3 v0 = validPosInWS - xAxis - yAxis;
	const Vector3 v1 = validPosInWS - xAxis + yAxis;
	const Vector3 v2 = validPosInWS + xAxis + yAxis;
	const Vector3 v3 = validPosInWS + xAxis - yAxis;

	// Vector of valid position to desired position
	const Vector3 delta = nearPlaneCentre - validPosInWS;
	const float length = delta.length();
	const Vector3 dir = delta/length;

	CollisionAdvance collisionAdvance( v0,
		2.f * xAxis, 2.f * yAxis, dir, length );
	collisionAdvance.ignoreFlags( TRIANGLE_TRANSPARENT | TRIANGLE_BLENDED | TRIANGLE_CAMERANOCOLLIDE );

	WorldTriangle tri1( v0, v1, v2 );
	WorldTriangle tri2( v0, v3, v2 );

	pSpace->collide( tri1, v0 + delta, collisionAdvance );
	pSpace->collide( tri2, v0 + delta, collisionAdvance );

	const float advance = collisionAdvance.advance();

	if (!MF_FLOAT_EQUAL( advance, length ))
	{
		collided = true;
		cameraPosInWS += (advance - length) * dir;
	}
	else
	{
		// Cap the camera position
		AABB cameraBounds = pSpace->bounds();
		if (!cameraBounds.intersects( cameraPosInWS, -.5f ))
		{
			collided = true;
			Vector3 cameraExtentInWS = cameraPosInWS -
				( cameraBounds.width() + cameraBounds.depth() ) * dir;
			cameraBounds.clip( cameraPosInWS, cameraExtentInWS, -.5f );
		}
	}

	BaseCamera::checkCameraTooClose( collided );

	return collided;
}


// ----------------------------------------------------------------------------
// Section: InvViewMatrixProvider
// ----------------------------------------------------------------------------
class InvViewMatrixProvider : public MatrixProvider
{
	Py_Header( InvViewMatrixProvider, MatrixProvider )
public:
	InvViewMatrixProvider() : MatrixProvider( false, &s_type_ ) {}

	virtual void matrix( Matrix & m ) const
	{
		m = Moo::rc().invView();
	}
	PY_AUTO_CONSTRUCTOR_FACTORY_DECLARE( InvViewMatrixProvider, END )
};


PY_TYPEOBJECT( InvViewMatrixProvider )

PY_BEGIN_METHODS( InvViewMatrixProvider )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( InvViewMatrixProvider )
PY_END_ATTRIBUTES()

/*~ function BigWorld.InvViewMatrix
 *
 *	This function returns a new InvViewMatrixProvider which is a MatrixProvider
 *	that can be used to access the rendering engine's inverse view matrix. 
 */
PY_FACTORY_NAMED( InvViewMatrixProvider, "InvViewMatrix", BigWorld )

BW_END_NAMESPACE

// base_camera.cpp
