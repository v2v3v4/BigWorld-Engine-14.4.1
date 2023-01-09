#ifndef CURSOR_CAMERA_HPP
#define CURSOR_CAMERA_HPP

// Standard MF Library Headers.
#include "math/angle.hpp"
#include "math/mathdef.hpp"
#include "math/matrix.hpp"
#include "math/vector3.hpp"

#include "base_camera.hpp"


BW_BEGIN_NAMESPACE

class WorldTriangle;

/*~ class BigWorld.CursorCamera
 *
 *	The CursorCamera is a camera that looks at a particular object (specified 
 *	by the target MatrixProvider attribute), looking in a direction specfied 
 *	by the source MatrixProvider atrribute.  The camera's desired position is
 *	pivotMaxDistance behind the target offset by pivotPosition, which is a Vector3
 *	which gets interpreted in the source-space.  After the 
 *	desired position has been offset by pivotPosition, it is then clipped to
 *  the geometry, and moved forwards along the source z-axis if necessary to give 
 *	it a clear line of site to the target position.
 *
 *	The desired position is where the camera will move towards being.  If 
 *	either the target moves, or one	of the parameters to the CursorCamera 
 *	change, then it may no longer be in this position, at that point, it will
 *	start moving towards it.  If the maxVelocity attribute is non-zero, then 
 *	its movement speed is limited to be no more than maxVelocity.  In addition
 *	the movementHalfLife attribute specifies how long it will take to move half
 *  the distance from its current position to the desired position.  Thus, it 
 *	will slow down as it approaches the desired position, approaching it 
 *	smoothely.
 *
 *	It is also possible to set the camera to firstPerson mode, where it
 *	sits exactly on the target position and looks in the source direction.
 *
 *	A new CursorCamera is created using BigWorld.CursorCamera function.
 */
/**
 *	The CursorCamera acts as a roaming camera that tracks a target while
 *	looking in the direction of a DirectionCursor. The CursorCamera has an
 *	position and direction, which is controlled by the pivot and the
 *	distanceFromPivot.
 *
 *	The camera pivots around the pivot which is defined as an offset from 
 *	local origin. The distanceFromPivot is the distance of the camera from
 *	the pivot offset. The camera direction is the line from the camera through
 *	the offset intersecting the DirectionCursor's direction vector.
 *
 *	The pivot is can also be defined as the look-at position because the camera
 *	line of sight does pass through it.
 *
 *	The position and direction are bounded by the collision scene as well as
 *	acceleration. There is acceleration for both camera movement and turning;
 *	these are defined as a HalfLife period.
 *
 *	The CursorCamera may have multiple targets. Taking priority is the Entity.
 *	If there is an Entity target selected, it will always target the Entity.
 *	Next is the Node target. If there is a Node target, it will target the
 *	Node only if the Entity is not NULL. Lastly is a matrix transform target.
 *	It will only target the matrix position if both other targets are NULL.
 *
 *	The exception to the above rule is if targetPlayer() is set to true. This
 *	will force the CursorCamera to query the Player class for the player Entity.
 *	The CursorCamera will prioritise the Player Entity as a target until this
 *	flag is set to false.
 */
class CursorCamera : public BaseCamera
{
	Py_Header( CursorCamera, BaseCamera )

public:

	///	@name Constructor and Destructor.
	//@{
	CursorCamera( PyTypeObject * pType = &s_type_ );
	~CursorCamera();
	//@}


	///	@name Methods associated with Camera Behaviour.
	//@{
	const Vector3 &pivotPosition( void ) const;
	void pivotPosition( const Vector3 &newPosition );
	void setPivotPosition( const Vector3 &pos );
	void scalePivotPosition( const Vector3 &factor );

	float maxDistanceFromPivot( void ) const;
	void maxDistanceFromPivot( float newDistance );
	float targetMaxDistanceFromPivot( void ) const;


	float minDistanceFromPivot( void ) const;
	void minDistanceFromPivot( float newDistance );

	float minDistanceFromTerrain( void ) const;
	void minDistanceFromTerrain( float newDistance );

	float movementHalfLife( void ) const;
	void movementHalfLife( float halfLifeInSeconds );

	float turningHalfLife( void ) const;
	void turningHalfLife( float halfLifeInSeconds );

	const Vector3 &uprightDirection( void ) const;
	void uprightDirection( const Vector3 &newDirection );

	void target( MatrixProviderPtr pMProv );
	void source( MatrixProviderPtr pMProv );

	const bool firstPersonPerspective( void ) const;
	void firstPersonPerspective( bool flag );

	bool reverseView( void ) const;
	void reverseView( bool flag );

	void shake( float duration, Vector3 amount );
	//@}


	///	@name Methods associated with Camera Position.
	//@{
	const Vector3 &dirInWS( void ) const;
	const Vector3 &posInWS( void ) const;

	virtual void set( const Matrix & viewMatrix );

	virtual void update( float deltaTime );
	//@}

	/// @name Python stuff
	//@{

	MatrixProviderPtr pTarget() { return pTarget_; }

	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, pivotPosition, pivotPosition )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, maxDistanceFromPivot, pivotMaxDist )
	PY_RO_ATTRIBUTE_DECLARE( this->targetMaxDistanceFromPivot(), targetMaxDist )
//	PY_READABLE_ATTRIBUTE_GET( this->targetMaxDistanceFromPivot(), targetMaxDist )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, minDistanceFromPivot, pivotMinDist )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, minDistanceFromTerrain, terrainMinDist )
	PY_RW_ATTRIBUTE_DECLARE( maxDistHalfLife_, maxDistHalfLife )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, movementHalfLife, movementHalfLife )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, turningHalfLife, turningHalfLife )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, uprightDirection, uprightDirection )

	PY_RW_ATTRIBUTE_DECLARE( pTarget_, target )
	PY_RW_ATTRIBUTE_DECLARE( pSource_, source )

	PY_RW_ATTRIBUTE_DECLARE( limitVelocity_, limitVelocity )
	PY_RW_ATTRIBUTE_DECLARE( maxVelocity_, maxVelocity )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, firstPersonPerspective, firstPerson )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, reverseView, reverseView )

	PY_AUTO_METHOD_DECLARE( RETVOID, shake, ARG( float, ARG( Vector3, END ) ) )

	PY_RW_ATTRIBUTE_DECLARE( inaccuracyProvider_, inaccuracyProvider )
	//@}

protected:
	///	@name Protected Methods associated with Camera Direction.
	//@{
	const Angle &yaw( void ) const;
	void yaw( const Angle &yawInRadians );
	const Angle &pitch( void ) const;
	void pitch( const Angle &pitchInRadians );
	const Angle &roll( void ) const;
	void roll( const Angle &rollInRadians );
	//@}


	///	@name Protected Helper Methods.
	//@{
	bool intersects( WorldTriangle *triangles, int numTriangles,
		const Vector3 &pointA,
		const Vector3 &pointB,
		const Vector3 &pointC );

	bool intersects( WorldTriangle *triangles, int numTriangles,
		float &intersectDist,
		const Vector3 &start,
		const Vector3 &end );
	//@}

private:
	///	@name Data associated with Camera Behaviour.
	//@{
	Vector3 pivotPosition_;
	Vector3 targetPivotPosition_;
	float maxDistanceFromPivot_;
	float targetMaxDistanceFromPivot_;
	float maxDistHalfLife_;
	float minDistanceFromPivot_;
	float minDistanceFromTerrain_;
	float movementHalfLife_;
	float turningHalfLife_;
	Vector3 uprightDirection_;

	MatrixProviderPtr	pTarget_;
	MatrixProviderPtr	pSource_;

	bool firstPersonPerspective_;
	bool reverseView_;
	bool inPosition_;
	bool limitVelocity_;
	float maxVelocity_;

	Vector3	shakeAmount_;
	float	shakeTime_;
	float	shakeLeft_;

	Vector4ProviderPtr inaccuracyProvider_;
	//@}


	///	@name Data associated with Camera Position and Direction.
	//@{
	Angle yaw_;
	Angle pitch_;
	Angle roll_;

	bool lastDesiredKnown_;
	Angle lastDesiredYaw_;
	Angle lastDesiredPitch_;
	Angle lastDesiredRoll_;

	Vector3 cameraPosInWS_;
	Vector3 cameraDirInWS_;
	//@}


	///	@name Static Data.
	//@{
	//static WorldTriangle s_terrain_[MAXIMUM_TERRAIN_SUBSET_TRIANGLES];
	//@}


	///	@name Unimplemented methods made private to restrict implicit usage.
	//@{
	CursorCamera( const CursorCamera &rhs );
	CursorCamera &operator=( const CursorCamera &rhs );
	//@}
};


#ifdef CODE_INLINE
#include "cursor_camera.ipp"
#endif

BW_END_NAMESPACE

#endif // CURSOR_CAMERA_HPP

