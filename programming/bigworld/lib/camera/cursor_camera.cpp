#include "pch.hpp"
#include "cursor_camera.hpp"

#include "moo/camera.hpp"
#include "moo/render_context.hpp"

#include "physics2/worldtri.hpp"
#include "physics2/worldpoly.hpp"

#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"

#include "romp/water.hpp"

DECLARE_DEBUG_COMPONENT2( "Camera", 0 )


BW_BEGIN_NAMESPACE

extern void s_dpolyadd( const WorldPolygon & wp, uint32 col );

#ifndef CODE_INLINE
#include "cursor_camera.ipp"
#endif

/*
// Private Definitions:
float CAMERA_OVERSHOOT_ANGLE = DEG_TO_RAD( 30.0f );
float CAMERA_OVERSHOOT_CUTOFF_ANGLE = DEG_TO_RAD( -10.0f );
float CAMERA_OVERSHOOT_FALLOFF_RATIO = 0.8f;
*/


// -----------------------------------------------------------------------------
// Constructor and Destructor.
// -----------------------------------------------------------------------------


/**
 *	Constructor for CursorCamera that targets an Entity.
 */
CursorCamera::CursorCamera( PyTypeObject * pType ) :
	BaseCamera( pType ),
	pivotPosition_( 0.0f, 2.0f, 0.0f ),
	targetPivotPosition_( 0.0f, 2.0f, 0.0f ),
	maxDistanceFromPivot_( 3.5f ),
	targetMaxDistanceFromPivot_( 3.5f ),
	maxDistHalfLife_( 1.5f ),
	minDistanceFromPivot_( 1.5f ),
	minDistanceFromTerrain_( 0.4f ),
	movementHalfLife_( 0.1f ),
	turningHalfLife_( 0.1f ),
	uprightDirection_( 0.0f, 1.0f, 0.0f ),
	cameraPosInWS_( 0.0f, 0.0f, 0.0f ),
	cameraDirInWS_( 0.0f, 0.0f, 1.0f ),
	pTarget_( NULL ),
	pSource_( NULL ),
	firstPersonPerspective_( false ),
	reverseView_( false ),
	inPosition_( true ),
	shakeAmount_( 0.f, 0.f, 0.f ),
	shakeTime_( 0.f ),
	shakeLeft_( 0.f ),
	yaw_( MATH_PI / 2.0f ),
	pitch_( 0.0f ),
	roll_( 0.0f ),
	lastDesiredKnown_( false ),
	lastDesiredYaw_( MATH_PI / 2.0f ),
	lastDesiredPitch_( 0.0f ),
	lastDesiredRoll_( 0.0f ),
	limitVelocity_( false ),
	maxVelocity_( 2.f ),
	inaccuracyProvider_( NULL )
{
	BW_GUARD;
	// Check that minimum camera distance is sensible.
	if ( this->minDistanceFromPivot() < 0.0f )
	{
		this->minDistanceFromPivot( 0.0f );
	}


	// Check that maximum camera distance is sensible.
	if ( this->maxDistanceFromPivot() < this->minDistanceFromPivot() )
	{
		this->maxDistanceFromPivot( this->minDistanceFromPivot() );
	}
}

/**
 *	Destructor for CursorCamera.
 */
CursorCamera::~CursorCamera()
{
}


// -----------------------------------------------------------------------------
// Static Data Member Initialisations
// -----------------------------------------------------------------------------

//const int MAXIMUM_TERRAIN_SUBSET_TRIANGLES = 1024;
//WorldTriangle CursorCamera::s_terrain_[MAXIMUM_TERRAIN_SUBSET_TRIANGLES];


// -----------------------------------------------------------------------------
// Methods associated with Camera Behaviour.
// -----------------------------------------------------------------------------

/**
 *	Sets the maximum camera distance from the pivot offset.
 *
 *	@param	newDistance		The maximum camera offset from the pivot offset.
 */
void CursorCamera::maxDistanceFromPivot( float newDistance )
{
	BW_GUARD;
	if ( newDistance < this->minDistanceFromPivot() )
	{
		minDistanceFromPivot( newDistance );
	}
	targetMaxDistanceFromPivot_ = newDistance;
}

/**
 *	Returns a transform matrix to the current target. If there is no target,
 *	the transform matrix returned is the Identity matrix.
 *
 *	@return	The targeting Matrix for the lookAt position.
 */
/*
const Matrix &CursorCamera::targetMatrix( void ) const
{
	static Matrix	entityM;

	if ( targetPlayer_ && Player::entity() != NULL )
	{
		entityM = Player::entity()->fallbackTransform();
		return entityM;
	}
	else
	{
		if ( pEntity_ )
		{
			entityM = pEntity_->fallbackTransform();
			return entityM;
		}
		else if ( pTarget_ )
		{
			return pTarget_->transform();
		}
		else
		{
			return targetMatrix_;
		}
	}
}
*/

// -----------------------------------------------------------------------------
// Methods associated with Camera Position.
// -----------------------------------------------------------------------------


/**
 *	Sets the camera to the position and direction of the camera supplied.
 *
 *	@param	viewMatrix		The matrix describing the position and orientation
 *							of the camera.
 */
void CursorCamera::set( const Matrix & viewMatrix )
{
	BW_GUARD;
	// Invert camera to world matrix.
	Matrix worldToCamera( viewMatrix );
	worldToCamera.invert();

	cameraPosInWS_ = worldToCamera.applyToOrigin();
	cameraDirInWS_ = worldToCamera.applyToUnitAxisVector( 2 );

	pitch_ = cameraDirInWS_.pitch();
	yaw_ = cameraDirInWS_.yaw();

	inPosition_ = false;
	lastDesiredKnown_ = false;
}


/**
 *	Updates the camera position and viewing direction based on time.
 *
 *	@param	deltaTime	The time passed in seconds since last update.
 *
 *	@return None.
 */
void CursorCamera::update( float deltaTime )
{
	BW_GUARD;
	bool collidedWithScene = false;

	Vector3 finalPosInWS = cameraPosInWS_;
	if ( pTarget_ && pSource_ )
	{
		// pWorldDirectionCursor_ the transformation matrix from local space to world space.
		Matrix tmatrix( Matrix::identity );
		pTarget_->matrix( tmatrix );

		// Calculate target camera yaw and pitch in world space.
		Matrix smatrix( Matrix::identity );
		pSource_->matrix( smatrix );
		Angle targetYaw = smatrix.yaw();
		Angle targetPitch = smatrix.pitch();
		Angle targetRoll = smatrix.roll();
		if (this->reverseView()) targetPitch += DEG_TO_RAD(180.0f);
		/*
		Angle targetYaw = pWorldDirectionCursor_->yaw();
		Angle targetPitch = pWorldDirectionCursor_->pitch() +
			( this->reverseView() ? DEG_TO_RAD(180.0f) : 0.0f );
		Angle targetRoll = pWorldDirectionCursor_->roll();
		*/

		//dprintf( "CC: Source is at (%f,%f,%f)\n",
		//	float(targetYaw), float(targetPitch), float(targetRoll) );
		//MF_ASSERT_DEV(targetRoll == 0);
		//targetRoll = Matrix_saferoll( smatrix );


		// Move camera pivot position towards target position.
		if ( pivotPosition_ != targetPivotPosition_ )
		{
			pivotPosition_ = Vector3(
				Math::decay( pivotPosition_.x, targetPivotPosition_.x,
					this->movementHalfLife() * 2.0f, deltaTime ),
				Math::decay( pivotPosition_.y, targetPivotPosition_.y,
					this->movementHalfLife() * 2.0f, deltaTime ),
				Math::decay( pivotPosition_.z, targetPivotPosition_.z,
					this->movementHalfLife() * 2.0f, deltaTime ) );
		}

		// Move camera distance towards the target camera distance.
		if (!MF_FLOAT_EQUAL(maxDistanceFromPivot_, targetMaxDistanceFromPivot_))
		{
			maxDistanceFromPivot_ = Math::decay(
				maxDistanceFromPivot_,
				targetMaxDistanceFromPivot_,
				maxDistHalfLife_, deltaTime );
		}

		// Adjust for pitch change in third-person mode.
		/*
		if ( !this->firstPersonPerspective() )
		{
			if ( targetPitch < CAMERA_OVERSHOOT_CUTOFF_ANGLE )
			{
				targetPitch -= CAMERA_OVERSHOOT_ANGLE +
					( targetPitch - CAMERA_OVERSHOOT_CUTOFF_ANGLE ) *
					CAMERA_OVERSHOOT_FALLOFF_RATIO;
			}
			else
			{
				targetPitch -= CAMERA_OVERSHOOT_ANGLE;
			}
		}
		*/

#if 0
		if ( pSourceEntity_ )
		{
			// If we also have a source Entity, adjust our Yaw values to
			// that Entity instead - we're using the source Entity's position
			// to control our yaw instead of the DirectionCursor.

			// Get the target in world space for the source Entity.
			Vector3 targetPosInWS = pSourceEntity_->pos();

			// Calculate the direction Vector and convert it a yaw value.
			Vector3 targetDirection = targetPosInWS - cameraPosInWS_;
			targetYaw = atan2( targetDirection.x, targetDirection.z );
		}
#endif

		// Now that target angles are determined, see if they have changed.
		if ( !inPosition_ && lastDesiredKnown_ )
		{
			if ( ( targetYaw != lastDesiredYaw_ ) ||
				( targetPitch != lastDesiredPitch_ ) ||
				( targetRoll != lastDesiredRoll_ ) )
			{
				inPosition_ = true;
			}
		}


		// Rotate the camera angles to the target angles.
		this->yaw( Angle::decay( this->yaw(),
			targetYaw,
			this->turningHalfLife(),
			deltaTime ) );

		this->pitch( Angle::decay( this->pitch(),
			targetPitch,
			this->turningHalfLife(),
			deltaTime ) );

		this->roll( Angle::decay( this->roll(),
			targetRoll,
			this->turningHalfLife(),
			deltaTime ) );


		// Set camera to its proper direction in world space.
		cameraDirInWS_.setPitchYaw( this->pitch(), this->yaw() );

		// TODO:PM We could/should split pivotPosition into two values.
		// Now figure out its position
		Vector3 positionOnEntity =
			tmatrix.applyPoint( Vector3( 0.f, this->pivotPosition().y, 0.f ) );

		// Note: The pivotOffset is now relative to the camera for the x and
		// z-coordinates.
		const Vector3 zOffset( sinf( this->yaw() ), 0.f, cosf( this->yaw() ) );
		const Vector3 xOffset( zOffset.z, 0.f, -zOffset.x );

		Vector3 pivotPosInWS =  positionOnEntity +
			this->pivotPosition().x * xOffset +
			this->pivotPosition().z * zOffset;

		// Shake the position up if we ought to
		if (shakeLeft_ > 0.f)
		{
			Vector3 amt = shakeAmount_ * (shakeLeft_ / shakeTime_);

			float yaw = ((float(rand()) / RAND_MAX) - 0.5f) * amt.x;
			float pitch = ((float(rand()) / RAND_MAX) - 0.5f) * amt.y;
			float roll = ((float(rand()) / RAND_MAX) - 0.5f) * amt.z;

			Matrix m;
			m.setRotate( yaw, pitch, roll );
			cameraDirInWS_ = m.applyVector(cameraDirInWS_);

			shakeLeft_ -= deltaTime;
		}

		// Make the camera shift due to unskillful camera person
		// (e.g. for sniper rifle)
		if (inaccuracyProvider_)
		{
			Vector4 amt;
			inaccuracyProvider_->output(amt);

			float yaw = amt.x;
			float pitch = amt.y;
			float roll = 0.f;

			Matrix m;
			m.setRotate( yaw, pitch, roll );
			cameraDirInWS_ = m.applyVector(cameraDirInWS_);
		}

		// Slide it to its distance and make sure it doesn't hit anything
		Vector3 idealPosInWS = pivotPosInWS;
		if (!this->firstPersonPerspective())
		{
			// Calculate the new position of the camera in world space.
			float cameraDist = this->maxDistanceFromPivot();
			idealPosInWS = pivotPosInWS - cameraDist * cameraDirInWS_;

			// Calculate the position on the sphere by which the camera is
			// restricted.
			Vector3 positionOnSphere = pivotPosInWS -
				( cameraPosInWS_ - pivotPosInWS ).length() * cameraDirInWS_;
			if ( inPosition_ && !limitVelocity_ )
			{
				// Snap camera to the correct position on the sphere.
				cameraPosInWS_ = positionOnSphere;
			}
			else
			{
				float distSquared = (cameraPosInWS_ -
					positionOnSphere).lengthSquared();
				if ( distSquared < 0.01f )
				{
					// Glide camera to the correct position on the sphere.
					cameraPosInWS_.x = Math::decay( cameraPosInWS_.x,
						positionOnSphere.x,
						this->movementHalfLife(), deltaTime );
					cameraPosInWS_.y = Math::decay( cameraPosInWS_.y,
						positionOnSphere.y,
						this->movementHalfLife(), deltaTime );
					cameraPosInWS_.z = Math::decay( cameraPosInWS_.z,
						positionOnSphere.z,
						this->movementHalfLife(), deltaTime );

					// If the distance is really close, consider the camera
					// to be locked and in position.
					if ( distSquared < 0.0001f )
					{
						inPosition_ = true;
					}
				}
			}

			// Check for scene collision and obstruction.
			collidedWithScene =
				BaseCamera::sceneCheck( idealPosInWS, positionOnEntity, cameraDirInWS_, uprightDirection_ );
		}

		float oldDist = (cameraPosInWS_ - positionOnEntity).length();
		float newDist = (idealPosInWS - positionOnEntity).length();

		if ((!collidedWithScene && !inPosition_) ||
			oldDist < newDist ||
				limitVelocity_)
		{
			// Glide camera to its position in world space if we're moving
			// backwards.
			if ( !limitVelocity_ )
			{
				float range = newDist - this->minDistanceFromPivot();
				float dist = 0.0f;
				if ( range != 0.0f )
				{
					float ratio = (oldDist - this->minDistanceFromPivot()) / range;
					float div = ( ( 1.0f - ratio ) + 1.0f ) * 2.0f;
					dist = Math::decay( oldDist, newDist,
						this->movementHalfLife() / div, deltaTime );
				}
				else
				{
					dist = Math::decay( oldDist, newDist,
						this->movementHalfLife(), deltaTime );
				}

				if (newDist != 0.f)
				{
					cameraPosInWS_ = positionOnEntity + 
								 (dist / newDist) * (idealPosInWS - positionOnEntity);
				}
				else
				{
					cameraPosInWS_ = positionOnEntity;
				}
			}
			else
			{
				//Move camera to the desired position, given a fixed velocity.
				Vector3 move = idealPosInWS - cameraPosInWS_;
				float distSq = move.lengthSquared();
				float maxMove = maxVelocity_ * deltaTime;
				if ( distSq > maxMove )
				{
					move.normalise();
					move *= maxMove;
					cameraPosInWS_ += move;
				}
				else
				{
					cameraPosInWS_.x = Math::decay( cameraPosInWS_.x, idealPosInWS.x,
						this->movementHalfLife(), deltaTime );
					cameraPosInWS_.y = Math::decay( cameraPosInWS_.y, idealPosInWS.y,
						this->movementHalfLife(), deltaTime );
					cameraPosInWS_.z = Math::decay( cameraPosInWS_.z, idealPosInWS.z,
						this->movementHalfLife(), deltaTime );
					limitVelocity_ = false;
				}
			}
		}
		else
		{
			// Else teleport camera directly there to avoid crashing through
			// the collision scene.
			cameraPosInWS_ = idealPosInWS;
			inPosition_ = true;
		}

		// check if on top of the water surface before any snapping of the camera can be done

		Matrix m;
		m.setTranslate( cameraPosInWS_ );
		bool overWater = false;
		float waterSurfaceHeight = 0.f;
		for(uint i = 0; i < Waters::instance().size(); i++)
		{
			Water * water = Waters::instance()[i];

			if ( water->isInsideBoundary( m ) )
			{
				overWater = true;
				waterSurfaceHeight = water->position().y;
				break;
			}
		}

		if(overWater)
		{
			const Moo::Camera * pCamera = &Moo::rc().camera();

			// compute the camera's axes
			Vector3 zAxis = cameraDirInWS_;
			Vector3 xAxis = uprightDirection_.crossProduct( zAxis );
			xAxis.normalise();
			Vector3 yAxis = zAxis.crossProduct( xAxis );

			// get the position of the near plane
			Vector3 planeSize = pCamera->nearPlanePoint(1, 1);
			Vector3 planePoint = cameraPosInWS_ + zAxis * planeSize[2];

			// compute the snap height (which is the height of the cameras clip
			// height projected onto the waters normal axis or y axis)
			float snapHeight = ::abs( yAxis[1] * planeSize[1] );

			// compute the near planes displacement off the surface of the water
			float waterDispHeight = planePoint.y - waterSurfaceHeight;

			// snap the camera's position up or down depending on which is closer
			if ( waterDispHeight > 0 && waterDispHeight < snapHeight ) 
			{
				planePoint.y = waterSurfaceHeight + snapHeight;

				Vector3 dir = planePoint - pivotPosInWS;
				float length = dir.length();
				dir.normalise();
				cameraPosInWS_ = pivotPosInWS + dir * (length + planeSize[2]);
			}
			else if ( waterDispHeight < 0 && waterDispHeight > -snapHeight )
			{
				planePoint.y = waterSurfaceHeight - snapHeight;

				Vector3 dir = planePoint - pivotPosInWS;
				float length = dir.length();
				dir.normalise();
				cameraPosInWS_ = pivotPosInWS + dir * (length + planeSize[2]);
			}
		}	

		// Check that camera is a certain minimum distance from the terrain.
		finalPosInWS = cameraPosInWS_;

		// Store previous desired direction.
		if ( !inPosition_ )
		{
			lastDesiredYaw_ = targetYaw;
			lastDesiredPitch_ = targetPitch;
			lastDesiredRoll_ = targetRoll;
			lastDesiredKnown_ = true;
		}
	}

	Vector3 useCDIWS = cameraDirInWS_;

	// Apply changes to the camera.
	view_.lookAt( finalPosInWS, useCDIWS, this->uprightDirection() );

	// Roll the camera.
	Matrix rollAdjust;
	rollAdjust.setRotateZ( roll() );
	view_.postMultiply( rollAdjust );

	// And calculate invView_ too.
	invView_.invert( view_ );

#if 0
	// Calculate whether the near plane intersects the bounding box.
	Entity * pEntity = Player::entity();
	if (pEntity != NULL &&
		pEntity->pPrimaryModel() != NULL)
	{
		const BoundingBox & bb =
						pEntity->pPrimaryModel()->boundingBox();
		{
			const Matrix & transform = this->targetMatrix();
			const Vector3 minV = bb.minBounds();
			const Vector3 maxV = bb.maxBounds();
			const Vector3 r = maxV - minV;
			Vector3 xR( r.x, 0.f, 0.f );
			Vector3 yR( 0.f, r.y, 0.f );
			Vector3 zR( 0.f, 0.f, r.z );
			WorldPolygon poly;

			poly.push_back( transform.applyPoint( minV ) );
			poly.push_back( transform.applyPoint( minV + xR ) );
			poly.push_back( transform.applyPoint( minV + xR + zR ) );
			poly.push_back( transform.applyPoint( minV + zR ) );

			poly.push_back( transform.applyPoint( minV + yR + zR ) );
			poly.push_back( transform.applyPoint( minV + yR + xR + zR ) );
			poly.push_back( transform.applyPoint( minV + yR + xR ) );
			poly.push_back( transform.applyPoint( minV + yR ) );
			s_dpolyadd( poly, 0x0100ff00 );
		}
	}
#endif
}


// -----------------------------------------------------------------------------
// Protected Helper Methods.
// -----------------------------------------------------------------------------



/**
 *	This method accepts an array of triangles and tests the array against a
 *	triangle supplied by its three points. If the triangle intersects with any
 *	of the triangles in the array, the return value is true.
 *
 *	@param	triangles		The array of triangles.
 *	@param	numTriangles	The size of the array.
 *	@param	pointA			First point of the test triangle.
 *	@param	pointB			Second point of the test triangle.
 *	@param	pointC			Third point of the test triangle.
 *
 *	@return	True, if the test triangle intersects any of the triangles in
 *			the array; false, otherwise.
 */
bool CursorCamera::intersects( WorldTriangle *triangles, int numTriangles,
	const Vector3 &pointA,
	const Vector3 &pointB,
	const Vector3 &pointC )
{
	BW_GUARD;
	WorldTriangle testTriangle( pointA, pointB, pointC );

	for ( int i = 0; i < numTriangles; i++ )
	{
		if ( testTriangle.intersects( triangles[i] ) )
		{
			return true;
		}
	}

	return false;
}

/**
 *	Accepts an array of triangles and tests if the line-segment supplied
 *	intersects with any of them.
 *
 *	@param	triangles		The array of triangles.
 *	@param	numTriangles	The size of the array.
 *	@param	intersectDist	The distance to the end point of the intersection.
 *							This is a value between 0 and 1. If it is set to 0,
 *							no intersection occurred.
 *	@param	start			The start of the line segment.
 *	@param	end				The end of the line segment.
 *
 *	@return	True, if the line segment intersected any of the triangles;
 *			false, otherwise.
 */
bool CursorCamera::intersects( WorldTriangle *triangles, int numTriangles,
	float &intersectDist,
	const Vector3 &start,
	const Vector3 &end )
{
	BW_GUARD;
	float closestIntersectDist = -1.0f;
	bool intersected = false;

	Vector3 dir = end - start;
	float length = dir.length();
	if ( length > 0.0f )
	{
		dir.normalise();
		for ( int i = 0; i < numTriangles; i++ )
		{
			intersectDist = length;
			if ( triangles[i].intersects( start, dir, intersectDist ) )
			{
				// Keep the closest intersect distance to the end position.
				if ( closestIntersectDist < intersectDist )
				{
					closestIntersectDist = intersectDist;
				}
				intersected = true;
			}
		}

		if ( intersected )
		{
			intersectDist = closestIntersectDist / length;
		}
		else
		{
			intersectDist = 0.0f;
		}
	}

	return intersected;
}


/*~ function CursorCamera.shake
 *
 *	This function causes the camera to start shaking for the specified period.
 *	The amount vector specifies the direction and magnitude (in source space)
 *	to move the camera.
 *
 *	If shake is called before a previous shake has finished, it overwrites the
 *  existing shake.
 *
 *	@param	duration	a float specifying the time (in seconds) to shake for.
 *	@param	amount		a vector3 specifying the direction and magnitude of the
 *						shaking.
 *
 *	@return				None
 */
/**
 *	This method starts the camera a-shaking in its boots.
 */
void CursorCamera::shake( float duration, Vector3 amount )
{
	shakeAmount_ = amount;
	shakeTime_ = duration;
	shakeLeft_ = duration;
}



// -----------------------------------------------------------------------------
// Python stuff
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( CursorCamera )

PY_BEGIN_METHODS( CursorCamera )
	PY_METHOD( shake )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( CursorCamera )
	/*~ attribute CursorCamera.pivotPosition
	 *
	 *	The pivotPosition is an offset from the position pivotMaxDistance
	 *	behind the target on the  source line.  The calculation of the desired
	 *	camera position is done as follows.  The source MatrixProvider
	 *	specifies a direction.  If pivotPosition is (0,0,0) then the camera
	 *	position is pivotMaxDistance back along this direction from the target.
	 *	If pivotPosition in something else, then it is used as an offset from
	 *	this point, with increasing x moving it to the right of the target,
	 *	increasing y moving it up from the target and increasing z moving it
	 *	towards the target.
	 *
	 *	After the desired position has been offset by pivotPosition, it is then
	 *	clipped to the geometry, and moved forwards along the z-axis if necessary
	 *	to give it a clear line of site to the target position.
	 *
	 *	The desired position is where the camera will move towards being.  If
	 *	either the target moves, or one of the parameters to the CursorCamera
	 *	change, then it may no longer be in this position, at that point, it
	 *	will start moving towards it.  If the maxVelocity attribute is
	 *	non-zero, then its movement speed is limited to be no more than
	 *	maxVelocity.  In addition the movementHalfLife attribute specifies how
	 *	long it will take to move half the distance from its current position
	 *	to the desired position.  Thus, it will slow down as it approaches the
	 *	desired position, approaching it smoothely.
	 *
	 *	@type	Vector3
	 */
	PY_ATTRIBUTE( pivotPosition )
	/*~ attribute CursorCamera.pivotMaxDist
	 *
	 *	This attribute specifies how far behind the target the camera wants to
	 *	sit.  It is measured backwards along the line specified by the source
	 *	attribute. When it is set to a new value, that value will not actually
	 *	be reached immediately, rather it will be smoothly approached with a
	 *	half life of maxDistHalfLife.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( pivotMaxDist )
	/*~ attribute CursorCamera.targetMaxDist
	 *
	 *	This attribute returns the target maximum distance from the pivot,
	 *	set the last time that pivotMaxDist was changed.
	 *
	 *	@type	Read-Only Float
	 */
	PY_ATTRIBUTE( targetMaxDist )
	/*~ attribute CursorCamera.maxDistHalfLife
	 *
	 *	This attribute is the half life with which the difference between
	 *	targetMaxDist and pivotMaxDist is decayed.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( maxDistHalfLife )
	/*~ attribute CursorCamera.pivotMinDist
	 *
	 *	This attribute has been deprecated, and no longer performs a useful
	 *	function.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( pivotMinDist )
	/*~ attribute CursorCamera.terrainMinDist
	 *
	 *	This attribute has been deprecated, and no longer performs a useful
	 *	function.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( terrainMinDist )
	/*~ attribute CursorCamera.movementHalfLife
	 *
	 *	The time it takes for the camera to move half the distance from its
	 *	current position to its desired position.  This is used every tick to
	 *	update the velocity the camera is moving at so that it slows down as it
	 *	approaches its desired position.
	 *
	 *	The desired position is pivotMaxDistance behind the target back along
	 *	the direction specified by source.  This point is offset by
	 *	pivotPosition, and then clipped to geometry, and moved forwards
	 *	along the z-axis if neccessary to give a clear view of the target
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( movementHalfLife )
	/*~ attribute CursorCamera.turningHalfLife
	 *
	 *	The time in seconds it takes for the camera to turn half the angle from its
	 *	current line behind the target to the line specified by source.
	 *
	 *	If this is zero, there is no momentum, and the camera will always point
	 *	exactly where it's told.
	 *	A value of 1.0 would mean that the camera would get
	 *	half-way to the desired direction each second.
	 *	A value of 0.5 would mean that the camera would get 70%
	 *	of the way to the desired direction each second.
	 *	Other values would be:
	 *	@{
	 *		0.25  84%
	 *		0.10  93%
	 *		0.08  95%
	 *		0.05  97%
	 *		0.03  98%
	 *		0.01  99%
	 *	@}
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( turningHalfLife )
	/*~ attribute CursorCamera.uprightDirection
	 *
	 *	The direction which the camera treats as up, relative to the source
	 *	frame of reference.  By default it is (0,1,0).
	 *
	 *	@type	Vector3
	 */
	PY_ATTRIBUTE( uprightDirection )

	/*~ attribute CursorCamera.limitVelocity
	 *
	 *	This attribute, if set to non-zero prevents the camera moving faster
	 *	than maxVelocity when trying to return to its desired position.  Once
	 *	it reaches its desired position, limitVelocity is automatically set to
	 *	0.
	 *
	 *	The movementHalfLife attribute specifies how long the camera should take
	 *	to move half the distance between its current position and its desired
	 *	position.  As long as this gives a speed which is less than
	 *	maxVelocity then limitVelocity has no efect.  However, if the
	 *	limitVelocity is non-zero, and maxVelocity is less than this desired speed,
	 *	then the camera speed will be limited to maxVelocity.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( limitVelocity )
	/*~ attribute CursorCamera.maxVelocity
	 *
	 *	The maximum speed that the camera can move at when trying to return
	 *	to its desired position.  This is only used if limitVelocity is
	 *	non-zero.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( maxVelocity )

	/*~ attribute CursorCamera.target
	 *
	 *	The target is a MatrixProvider, which supplies the location of the
	 *	object to look at.  The rest of the matrix is not used, only the
	 *	position components.
	 *
	 *	@type	MatrixProvider
	 */
	PY_ATTRIBUTE( target )
	/*~ attribute CursorCamera.source
	 *
	 *	The source is a MatrixProvider, which supplies the direction for the
	 *	camera to look in.  It will look at the position specified in the
	 *	target attribute, along the orientation specified in this attribute.
	 *
	 *	@type	MatrixProvider
	 */
	PY_ATTRIBUTE( source )

	/*~ attribute CursorCamera.firstPerson
	 *
	 *	If this is set to non-zero, then the camera will sit at the target
	 *	position, rather than behind it.  This will make it look like first
	 *	person view, rather than the usual third person.  It defaults to 0
	 *	(third person view).
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( firstPerson )
	/*~ attribute CursorCamera.reverseView
	 *
	 *	If this is set to non-zero, then the camera will be in front of the
	 *	target looking backwards, rather than behind looking forwards.  It
	 *	defaults to zero.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( reverseView )
	/*~ attribute CursorCamera.inaccuracyProvider
	 *	If this is set to non-NULL, then the camera will tick the given
	 *	Vector4Provider and use the x,y values as yaw,pitch adjustments.
	 *	an example use for this is implementing an unsteady sniper rifle scope.
	 *
	 *	@type	Vector4Provider
	 */
	PY_ATTRIBUTE( inaccuracyProvider )
PY_END_ATTRIBUTES()

/*~ function BigWorld.CursorCamera
 *
 *	This function creates a new CursorCamera.  A CursorCamera looks at a
 *	specified target, in the direction of a specified source.
 *
 *	@return a new CursorCamera object.
 */
PY_FACTORY( CursorCamera, BigWorld )


/**
 *	Python factory method
 */
PyObject * CursorCamera::pyNew( PyObject * args )
{
	BW_GUARD;
	return new CursorCamera();
}

BW_END_NAMESPACE

// cursor_camera.cpp
