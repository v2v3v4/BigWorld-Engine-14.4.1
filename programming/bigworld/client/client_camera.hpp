#ifndef CLIENT_CAMERA_HPP
#define CLIENT_CAMERA_HPP

#include "input/input.hpp"

#include "cstdmf/smartpointer.hpp"
#include "camera/direction_cursor.hpp"
#include "pyscript/stl_to_py.hpp"
#include "math/sma.hpp"
#include "entity_picker.hpp"


BW_BEGIN_NAMESPACE

typedef SmartPointer< class DataSection > DataSectionPtr;

class BaseCamera;
class ClientCamera;
class ProjectionAccess;
class DirectionCursor;
class Entity;

typedef BW::vector< Vector2 > V2Vector;
typedef ScriptObjectPtr<BaseCamera> BaseCameraPtr;

/**
 *  AutoAim is the python interface to the targeting values.
 *  It is used exclusively by ClientSpeedProvider.
 */
class AutoAim : public PyObjectPlus
{
	Py_Header( AutoAim, PyObjectPlus )
public:
	AutoAim( EntityPicker& entityPicker, PyTypeObject * pType = &s_type_ );
	virtual ~AutoAim();

	virtual void readInitialValues( DataSectionPtr rootSection );

	PY_RW_ATTRIBUTE_DECLARE( friction_, friction )
	PY_RW_ATTRIBUTE_DECLARE( forwardAdhesion_, forwardAdhesion )
	PY_RW_ATTRIBUTE_DECLARE( strafeAdhesion_, strafeAdhesion )
	PY_RW_ATTRIBUTE_DECLARE( turnAdhesion_, turnAdhesion )
	PY_RW_ATTRIBUTE_DECLARE( adhesionPitchToYawRatio_, adhesionPitchToYawRatio )
	PY_RW_ATTRIBUTE_DECLARE( reverseAdhesionStyle_, reverseAdhesionStyle )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, frictionHorizontalAngle, frictionHorizontalAngle )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, frictionVerticalAngle, frictionVerticalAngle )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, frictionDistance, frictionDistance )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, frictionHorizontalFalloffAngle, frictionHorizontalFalloffAngle )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, frictionVerticalFalloffAngle, frictionVerticalFalloffAngle )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, frictionMinimumDistance, frictionMinimumDistance )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, frictionFalloffDistance, frictionFalloffDistance )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, adhesionHorizontalAngle, adhesionHorizontalAngle )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, adhesionVerticalAngle, adhesionVerticalAngle )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, adhesionDistance, adhesionDistance )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, adhesionHorizontalFalloffAngle, adhesionHorizontalFalloffAngle )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, adhesionVerticalFalloffAngle, adhesionVerticalFalloffAngle )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, adhesionFalloffDistance, adhesionFalloffDistance )


	float frictionHorizontalAngle() const;
	void frictionHorizontalAngle( float angle );
	float frictionVerticalAngle() const;
	void frictionVerticalAngle( float angle );
	float frictionDistance() const;
	void frictionDistance( float distance );
	float frictionHorizontalFalloffAngle() const;
	void frictionHorizontalFalloffAngle( float angle );
	float frictionVerticalFalloffAngle() const;
	void frictionVerticalFalloffAngle( float angle );
	float frictionMinimumDistance() const;
	void frictionMinimumDistance( float distance );
	float frictionFalloffDistance() const;
	void frictionFalloffDistance( float distance );

	float adhesionHorizontalAngle() const;
	void adhesionHorizontalAngle( float angle );
	float adhesionVerticalAngle() const;
	void adhesionVerticalAngle( float angle );
	float adhesionDistance() const;
	void adhesionDistance( float distance );
	float adhesionHorizontalFalloffAngle() const;
	void adhesionHorizontalFalloffAngle( float angle );
	float adhesionVerticalFalloffAngle() const;
	void adhesionVerticalFalloffAngle( float angle );
	float adhesionFalloffDistance() const;
	void adhesionFalloffDistance( float distance );

	EntityPicker& entityPicker_;
	float friction_;
	float forwardAdhesion_;
	float strafeAdhesion_;
	float turnAdhesion_;
	float adhesionPitchToYawRatio_;
	int reverseAdhesionStyle_;

protected:
	AutoAim( const AutoAim& );
	AutoAim& operator=( const AutoAim& );
};

PY_SCRIPT_CONVERTERS_DECLARE( AutoAim )

typedef ScriptObjectPtr<AutoAim> AutoAimPtr;


/**
 *  Targeting is the python interface to the targeting values.
 *  It is used exclusively by ClientSpeedProvider.
 */
class Targeting : public PyObjectPlus
{
	Py_Header( Targeting, PyObjectPlus )
public:
	Targeting( EntityPicker & entityPicker,
		const AutoAimPtr & autoAim,
		PyTypeObject * pType = &s_type_ );
	virtual ~Targeting() {}

	virtual void readInitialValues( DataSectionPtr rootSection );

	Entity*	hasAnAutoAimTarget( float& autoAimTargetDistance, float& autoAimTargetAngle,
								bool useFriction, bool wantHorizontalAngle );

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, selectionAngle, selectionAngle )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, deselectionAngle, deselectionAngle )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, selectionDistance, selectionDistance )
	PY_RO_ATTRIBUTE_DECLARE( this->defaultSelectionAngle(), defaultSelectionAngle )
	PY_RO_ATTRIBUTE_DECLARE( this->defaultDeselectionAngle(), defaultDeselectionAngle )
	PY_RO_ATTRIBUTE_DECLARE( this->defaultSelectionDistance(), defaultSelectionDistance )

	PY_RW_ATTRIBUTE_DECLARE( autoAimOn_, autoAimOn )
	PY_RO_ATTRIBUTE_DECLARE( autoAim_, autoAim )

	bool autoAimOn() const;
	const AutoAimPtr autoAim() const;
	AutoAimPtr autoAim();

	float selectionAngle() const;
	void selectionAngle( float angle );
	float deselectionAngle() const;
	void deselectionAngle( float angle );
	float selectionDistance() const;
	void selectionDistance( float distance );
	float defaultSelectionAngle() const;
	float defaultDeselectionAngle() const;
	float defaultSelectionDistance() const;

protected:
	Targeting( const Targeting& );
	Targeting& operator=( const Targeting& );

	bool autoAimOn_;
	AutoAimPtr autoAim_;
	EntityPicker& entityPicker_;
};


typedef ScriptObjectPtr<Targeting> TargetingPtr;

/**
 *  CameraSpeed is the python interface to the camera movement values.
 *  It is used exclusively by ClientSpeedProvider.
 */
class CameraSpeed : public PyObjectPlus
{
	Py_Header( CameraSpeed, PyObjectPlus )
public:
	CameraSpeed( PyTypeObject * pType = &s_type_ );
	virtual ~CameraSpeed() {}

	virtual void readInitialValues( DataSectionPtr rootSection );

	PY_RW_ATTRIBUTE_DECLARE( lookAxisMin_, lookAxisMin )
	PY_RW_ATTRIBUTE_DECLARE( lookAxisMax_, lookAxisMax )
	PY_RW_ATTRIBUTE_DECLARE( accelerateStart_, accelerateStart )
	PY_RW_ATTRIBUTE_DECLARE( accelerateRate_, accelerateRate )
	PY_RW_ATTRIBUTE_DECLARE( accelerateEnd_, accelerateEnd )
	PY_RW_ATTRIBUTE_DECLARE( accelerateWhileTargeting_, accelerateWhileTargeting )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, cameraYawSpeed, cameraYawSpeed )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, cameraPitchSpeed, cameraPitchSpeed )

	PY_RW_ATTRIBUTE_DECLARE( lookTableHolder_, lookTable )

	float cameraYawSpeed() const;
	void cameraYawSpeed( float speed );
	float cameraPitchSpeed() const;
	void cameraPitchSpeed( float speed );

	V2Vector lookTable_;
	float lookAxisMin_;
	float lookAxisMax_;
	float cameraYawSpeed_;
	float cameraPitchSpeed_;
	float accelerateStart_ ;
	float accelerateRate_ ;
	float accelerateEnd_;
	bool accelerateWhileTargeting_;

	PySTLSequenceHolder<V2Vector>	lookTableHolder_;

protected:
	CameraSpeed( const CameraSpeed& );
	CameraSpeed& operator=( const CameraSpeed& );
};

typedef ScriptObjectPtr<CameraSpeed> CameraSpeedPtr;


/**
 *	The ClientSpeedProvider object provides control of the
 *	DirectionCursor that is non-linear, accelerates and
 *	has friction when near targets.
 */
class ClientSpeedProvider : public SpeedProvider
{
	enum { kNumSamples = 50 };
public:
	ClientSpeedProvider( const TargetingPtr & targeting,
		const CameraSpeedPtr & cameraSpeed );
	virtual ~ClientSpeedProvider();

	virtual void readInitialValues( DataSectionPtr rootSection );

	virtual float value( const AxisEvent &event );
	virtual void adjustDirection( float dTime, Angle& pitch, Angle& yaw, Angle& roll );

	void drawDebugStuff();

protected:
	TargetingPtr targeting_;
	CameraSpeedPtr cameraSpeed_;

	Entity* target_;
	float previousPitch_;
	float previousYaw_;
	bool previousHadATarget_;
	float yawAccelerationTime_;
	SMA<float> yawStrafeSMA_;
	SMA<float> yawTurningSMA_;

	// Debug stuff
	bool showTotalYaw_;
	bool showForwardYaw_;
	bool showStrafeYaw_;
	bool showTurnYaw_;
	bool showTotalPitch_;
	bool showFriction_;
	int yawPitchScale_;
	unsigned int numSamples_;
	typedef BW::vector<float> GraphInfo;
	float lastFriction_;
	GraphInfo friction_;
	GraphInfo yawForwardChange_;
	GraphInfo yawStrafeChange_;
	GraphInfo yawTurnChange_;
	GraphInfo yawChange_;
	GraphInfo pitchChange_;

	ClientSpeedProvider( const ClientSpeedProvider& );
	ClientSpeedProvider& operator=( const ClientSpeedProvider& );
};


/**
 *	This class maintains the current camera-related objects
 */
class ClientCamera
{
public:
	ClientCamera( ClientSpeedProvider& speedProvider );
	~ClientCamera();

	void init( DataSectionPtr configSection );

	BaseCameraPtr	camera() const { return pCamera_; };
	void			camera( BaseCameraPtr pCamera );

	ProjectionAccess*	projAccess() const { return pProjAccess_; };
	void			projAccess( ProjectionAccess* pProjAccess ) { pProjAccess_ = pProjAccess; };

	DirectionCursor* directionCursor() const { return pDirectionCursor_; };

	void			update( float dTime );

	void			drawDebugStuff();

private:
	BaseCameraPtr pCamera_;
	ProjectionAccess* pProjAccess_;
	ClientSpeedProvider& speedProvider_;
	DirectionCursor* pDirectionCursor_;

	ClientCamera( const ClientCamera& );
	ClientCamera& operator=( const ClientCamera& );
};


#ifdef CODE_INLINE
#include "client_camera.ipp"
#endif

BW_END_NAMESPACE

#endif // CLIENT_CAMERA_HPP
