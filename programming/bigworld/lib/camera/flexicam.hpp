#ifndef FLEXICAM_HPP
#define FLEXICAM_HPP

#include "base_camera.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.FlexiCam
 *
 *	The camera object that follows a particular target. The camera has an
 *	actual position and a preferred position relative to the target. The
 *	camera's preferred viewing direction is centered on the target minus
 *	some offset vector. There is also a current viewing direction for the
 *	camera.
 *
 *	As the camera's view and position are not immediately bound to the
 *	preferred position and direction, they have accelerations associated with
 *	them. These values can also be adjusted.
 *
 *	The default position is the default preferred position for the camera
 *	when at rest, that is, it is not being influenced by the character turning
 *	or looking around.
 *
 *	A new FlexiCam is created using BigWorld.FlexiCam function.
 */
/**
 *	The camera object that follows a particular target. The camera has an
 *	actual position and a preferred position relative to the target. The
 *	camera's preferred viewing direction is centered on the target minus
 *	some offset vector. There is also a current viewing direction for the
 *	camera.
 *
 *	As the camera's view and position are not immediately bound to the
 *	preferred position and direction, they have accelerations associated with
 *	them. These values can also be adjusted.
 *
 *	The camera position is affected by the collision scene and terrain.
 *
 *	The default position is the default preferred position for the camera
 *	when at rest, that is, it is not being influenced by the character turning
 *	or looking around.
 *
 *	Multiple Target Matrices can be set: If the targetMatrix is set, it
 *	becomes the camera target, taking precedence over all others. If the
 *	targetEntity is set, it is targeted only if targetMatrix is set to NULL.
 *	If both targets are set to NULL, the player entity is queried specifically
 *	as a target.
 */
class FlexiCam : public BaseCamera
{
	Py_Header( FlexiCam, BaseCamera )

public:
	///	@name Constructor and destructor
	//@{
	explicit FlexiCam( MatrixProviderPtr pMProv = NULL,
		PyTypeObject * pType = &s_type_ );

	~FlexiCam();
	//@}


	///	@name Methods associated with camera position relative to the target.
	//@{
	const Vector3 &preferredPos(void) const;
	void preferredPos(const Vector3 &newPos);

	const Vector3 &actualPos(void) const;
	void actualPos(const Vector3 &newPos);

	const float preferredYaw(void) const;
	void changePreferredYawBy(float deltaDegree);

	const float preferredPitch(void) const;
	void changePreferredPitchBy(float deltaDegree);

	float positionAcceleration(void) const;
	void positionAcceleration(float newAcceleration);
	//@}


	///	@name Methods associated with camera view relative to the target.
	//@{
	MatrixProviderPtr pTarget() const;
	void pTarget( MatrixProviderPtr pNewTarget );

	const Vector3 &uprightDir(void) const;
	void uprightDir(const Vector3 &newDir);

	const Vector3 &actualDir(void) const;
	void actualDir(const Vector3 &newDir);

	const Vector3 &viewOffset(void) const;
	void viewOffset(const Vector3 &newOffset);

	float trackingAcceleration(void) const;
	void trackingAcceleration(float newAcceleration);
	//@}


	///	@name Methods associated with transforms and the 3D world.
	//@{
	const Vector3 &posInWS( void ) const;
	const Vector3 &dirInWS( void ) const;

	virtual void set( const Matrix & viewMatrix );

	virtual void update( float deltaTime );
	//@}

	///	@name Python stuff
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ATTRIBUTE_REF_DECLARE( preferredPos_, preferredPos )
	PY_RW_ATTRIBUTE_REF_DECLARE( actualPos_, actualPos )
	PY_RO_ATTRIBUTE_DECLARE( preferredYaw(), preferredYaw )
	PY_RO_ATTRIBUTE_DECLARE( preferredPitch(), preferredPitch )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, positionAcceleration,
		positionAcceleration )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( MatrixProviderPtr, pTarget, target )
	PY_RW_ATTRIBUTE_DECLARE( timeMultiplier_, timeMultiplier )
	PY_RW_ATTRIBUTE_DECLARE( uprightDir_, uprightDir )
	PY_RW_ATTRIBUTE_DECLARE( actualDir_, actualDir )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, viewOffset, viewOffset )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, trackingAcceleration,
		trackingAcceleration )
	//@}

private:
	void init();

	///	@name Data associated with camera position.
	//@{
	Vector3 preferredPos_;
	Vector3 actualPos_;
	float positionAcceleration_;
	Vector3 prevDesiredPos_;
	//@}

	///	@name Data associated with camera direction.
	//@{
	MatrixProviderPtr	pTarget_;
	float				timeMultiplier_;

	Vector3 uprightDir_;
	Vector3 actualDir_;
	Vector3 viewOffset_;
	float trackingAcceleration_;
	Vector3 prevDesiredDir_;

	Vector3 viewDir_;
	//@}

	///	@name Private Methods that are made private to reserve their use.
	//@{
	///	The copy constructor for FlexiCam.
	FlexiCam(const FlexiCam&);

	///	The assignment operator for FlexiCam.
	FlexiCam &operator=(const FlexiCam&);
	//@}
};

#ifdef CODE_INLINE
#include "flexicam.ipp"
#endif

BW_END_NAMESPACE

#endif // FLEXICAM_HPP
/* flexicam.hpp */
