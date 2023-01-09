#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include "pyscript/pyobject_plus.hpp"

#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "math/angle.hpp"
#include "script/script_object.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"
#include "network/basictypes.hpp"

#include "input/input.hpp"

#include "path_seeker.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class Scene;
class CollisionScene;

//#include "entity.hpp"
class Entity;
class ChunkSpace;
class PyEntity;
typedef ScriptObjectPtr< PyEntity > PyEntityPtr;

struct MotionConstants;

typedef SmartPointer<Entity> EntityPtr;
typedef SmartPointer<ChunkSpace> ChunkSpacePtr;


#ifndef Entity_CONVERTERS
#define Entity_CONVERTERS
PY_SCRIPT_CONVERTERS_DECLARE( Entity )
#endif

struct MotionConstants;


/*~ class BigWorld.Physics
 *
 *	This class controls the movement of a controlled entity. It
 *	applies physics to the entity, applying its velocity, geometry checks and
 *	 gravity.
 *
 *	An entity is controlled if it is the player entity, or it has the
 *	BigWorld.controlEntity function called on it.
 *
 *	Physics objects are created by assigning a value to an entity's physics
 *	attribute.  The value is an integer, indicating which physics object type to
 *	create.  For example
 *
 *	@{
 *	>>> entity.physics = 1
 *	>>>	entity.physics
 *	Physics at 0x00eafe87
 *	@}
 *	causes a new physics of type 1 to be created
 */
/**
 *	This Python accessible class applies physics to the pose of the
 *	player's entity, based on the forces, constraints and
 *	(large number of) settings controlled by the player's script.
 */
class Physics : public PyObjectPlus, InputHandler, PathSeeker::SeekListener
{
	Py_Header( Physics, PyObjectPlus )

public:
	Physics( Entity * pSlave, int style,
		PyTypeObject * pType = &Physics::s_type_ );
	~Physics();

	enum SeekState
	{
		LINE,
		PATH,
		UNKNOWN
	};

	enum
	{
		DUMMY_PHYSICS = -1,
		STANDARD_PHYSICS = 0,
		HOVER_PHYSICS = 1,
		CHASE_PHYSICS = 2,
		TURRET_PHYSICS = 3
	};

	static void tickAll( double timeNow, double timeLast );
	void tick( double timeNow, double timeLast );

	PY_AUTO_METHOD_DECLARE( RETOWN, teleportSpace, ARG( SpaceID, END ) )
	PY_AUTO_METHOD_DECLARE( RETOK, teleportVehicle, ARG( PyEntityPtr, END ) )
	// TODO: teleport's second parameter should be Direction3D but Python
	// doesn't know about Direction3D
	PY_AUTO_METHOD_DECLARE( RETVOID, teleport, ARG( Vector3,
		OPTARG( Vector3, Vector3(FLT_MAX,0,0), END ) ) )

	PY_METHOD_DECLARE( py_seek )
	PY_AUTO_METHOD_DECLARE( RETVOID, chase,
		ARG( PyEntityPtr,
			ARG( float,
				OPTARG( float, 0.f, END )
			)
		)
	)
	PY_AUTO_METHOD_DECLARE( RETVOID, seekPath,
		ARG( BW::vector<Vector3>,
			ARG( float,
				ARG( float,
					OPTARG( float, 0.0f,
						OPTARG( float, 0.1f,
							OPTARG( PyObjectPtr, NULL, END )
						)
					)
				)
			)
		)
	)
	PY_AUTO_METHOD_DECLARE( RETVOID, cancelSeek, END )

	PY_AUTO_METHOD_DECLARE( RETVOID, stop, OPTARG( bool, false, END ) )

	PY_AUTO_METHOD_DECLARE( RETDATA, movingForward, ARG( float,
		OPTARG( Vector4ProviderPtr, NULL, END ) ) )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, velocity, velocity )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, smoothingTime, smoothingTime )
	PY_RO_ATTRIBUTE_DECLARE( uncorrectedVelocity_.getObject(), uncorrectedVelocity )
	PY_RW_ATTRIBUTE_DECLARE( joystickEnabled_, joystickEnabled )
	PY_RW_ATTRIBUTE_DECLARE( nudge_, nudge )
	PyObject * pyGet_velocityMouse();
	int pySet_velocityMouse( PyObject * value );
	PY_RW_ATTRIBUTE_DECLARE( userDriven_, userDriven )

	PY_RW_ATTRIBUTE_DECLARE( angular_, angular )
	PY_RW_ATTRIBUTE_DECLARE( turn_, turn )
	PyObject * pyGet_angularMouse();
	int pySet_angularMouse( PyObject * value );
	PY_RW_ATTRIBUTE_DECLARE( userDirected_, userDirected )
	PY_RW_ATTRIBUTE_DECLARE( dcLocked_, dcLocked )

	PY_RW_ATTRIBUTE_DECLARE( oldStyleCollision_, oldStyleCollision )
	PY_RW_ATTRIBUTE_DECLARE( collide_, collide )
	PY_RW_ATTRIBUTE_DECLARE( collideTerrain_, collideTerrain )
	PY_RW_ATTRIBUTE_DECLARE( collideObjects_, collideObjects )
	PY_DEFERRED_ATTRIBUTE_DECLARE( fall )
	PY_RO_ATTRIBUTE_DECLARE( fallingVelocity_, fallingVelocity )
	PY_RW_ATTRIBUTE_DECLARE( gravity_, gravity )

	PY_RW_ATTRIBUTE_DECLARE( thrust_, thrust )
	PY_RW_ATTRIBUTE_DECLARE( brake_, brake )
	PY_RW_ATTRIBUTE_DECLARE( thrustAxis_, thrustAxis )

	PY_RW_ATTRIBUTE_DECLARE( runFwdSpeed_, joystickFwdSpeed )
	PY_RW_ATTRIBUTE_DECLARE( runBackSpeed_, joystickBackSpeed )

	PY_READABLE_ATTRIBUTE_GET( this->isSeeking(), seeking )
	PY_RO_ATTRIBUTE_SET( seeking )

	PY_RW_ATTRIBUTE_DECLARE( targetSource_, targetSource )
	PY_RW_ATTRIBUTE_DECLARE( targetDest_, targetDest )

	PY_RW_ATTRIBUTE_DECLARE( modelHeight_, modelHeight )
	PY_RW_ATTRIBUTE_DECLARE( modelWidth_, modelWidth )
	PY_RW_ATTRIBUTE_DECLARE( modelDepth_, modelDepth )

	PY_RW_ATTRIBUTE_DECLARE( inWater_, inWater )
	PY_RW_ATTRIBUTE_DECLARE( waterSurfaceHeight_, waterSurfaceHeight )
	PY_RW_ATTRIBUTE_DECLARE( buoyancy_, buoyancy )
	PY_RW_ATTRIBUTE_DECLARE( viscosity_, viscosity )	

	PY_RW_ATTRIBUTE_DECLARE( scrambleHeight_, scrambleHeight )
	PY_RW_ATTRIBUTE_DECLARE( maximumSlope_, maximumSlope )	

	PY_RO_ATTRIBUTE_DECLARE( chaseTarget_ != NULL, chasing )
	PY_RO_ATTRIBUTE_DECLARE( moving_, moving )

	PY_RW_ATTRIBUTE_DECLARE( ripper_.desiredHeightFromGround_, ripperDesiredHeightFromGround )
	PY_RW_ATTRIBUTE_DECLARE( ripper_.minHeightFromGround_, ripperMinHeightFromGround )
	PY_RW_ATTRIBUTE_DECLARE( ripper_.maxThrust_, ripperMaxThrust )
	PY_RW_ATTRIBUTE_DECLARE( ripper_.gravity_, ripperGravity )
	PY_RW_ATTRIBUTE_DECLARE( ripper_.xDrag_, ripperXDrag )
	PY_RW_ATTRIBUTE_DECLARE( ripper_.yUpDrag_, ripperYUpDrag )
	PY_RW_ATTRIBUTE_DECLARE( ripper_.yDownDrag_, ripperYDownDrag )
	PY_RW_ATTRIBUTE_DECLARE( ripper_.zDrag_, ripperZDrag )
	PY_RW_ATTRIBUTE_DECLARE( ripper_.brakeDrag_, ripperBrakeDrag )
	PY_RW_ATTRIBUTE_DECLARE( ripper_.thrustDropOffHeight_, ripperThrustDropOffHeight )
	PY_RW_ATTRIBUTE_DECLARE( ripper_.thrustDropOffRate_, ripperThrustDropOffRate )
	PY_RW_ATTRIBUTE_DECLARE( ripper_.turnRate_, ripperTurnRate )
	PY_RW_ATTRIBUTE_DECLARE( ripper_.elasticity_, ripperElasticity )

	PY_RW_ATTRIBUTE_DECLARE( pAxisEventNotifier_, axisEventNotifier )
	PY_RW_ATTRIBUTE_DECLARE( axisEventNotifierThreshold_, axisEventNotifierThreshold )
	PY_RW_ATTRIBUTE_DECLARE( pIsMovingNotifier_, isMovingNotifier )

	Vector3 & getDesiredPosition()		{ return desiredPosition_; }

	bool collide() const				{ return collide_; }

	void disown()						{ pSlave_ = NULL; }

	bool fall() const;

	void fall( bool fall );

	/**
	 * TODO: to be documented.
	 */
	class Data
	{
	public:
		Data();
		Data(const Vector3 & momentum,
			const Vector3 & position);

		Vector3 momentum_;
		Vector3 position_;

		void operator+=(const Data & data);
	};

	/**
	 * TODO: to be documented.
	 */
	struct HoverSetting
	{
		HoverSetting() :
			desiredHeightFromGround_( 1.0f ),
			minHeightFromGround_( 0.4f ),
			maxThrust_( 40.0f ),
			gravity_( 30.0f ),
			xDrag_( 1.0f ),
			yUpDrag_( 0.7f ),
			yDownDrag_( 0.1f ),
			zDrag_( 0.2f ),
			brakeDrag_( 1.0f ),
			thrustDropOffHeight_( 2.0f ),
			thrustDropOffRate_( 2.0f ),
			turnRate_( 1.0f ),
			elasticity_( 0.5f )
		{
		}

		float desiredHeightFromGround_;
		float minHeightFromGround_;
		float maxThrust_;
		float gravity_;
		float xDrag_;
		float yUpDrag_;
		float yDownDrag_;
		float zDrag_;
		float brakeDrag_;
		float thrustDropOffHeight_;
		float thrustDropOffRate_;
		float turnRate_;
		float elasticity_;
	};

	HoverSetting &hoverSetting()			{ return ripper_; }

	Vector3 movedDirection() const				{ return movedDirection_; }
	float movedDistance() const				{ return movedDistance_; }

	enum MouseAxis
	{
		MA_None = -1,

		MA_MouseX = 0,
		MA_MouseY = 1,
		MA_Direction = 2,

		MA_INVALID = 0x7fffffff
	};

	static bool handleAxisEventAll( const AxisEvent & event );
	virtual bool handleAxisEvent( const AxisEvent & event );

	static void setMovementThreshold(float movementThreshhold) { movementThreshold_ = movementThreshhold; }

	PyObject * teleportSpace( SpaceID spaceID );
	bool teleportVehicle( PyEntityPtr pVehicle );
	void teleport( const Vector3 & pos, const Vector3 & dir );
	void cancelTeleport();

	Vector3 movingForward( float maxDistance, Vector4ProviderPtr destinationPtr ) const;

	void chase( PyEntityPtr pTarget, float distance, float tolerance = 0.f );

	void seekPath( const BW::vector<Vector3>& pathPoints,
		float yawAtEnd,
		float speed,
		float timeout,
		float verticalRange,
		PyObjectPtr callback );

	void cancelSeek();

	void seekFinished( bool successfully );

	bool isSeeking() const;

private:
	void setIsMoving( bool newValue );

	void avatarStyleTick( double timeNow, double timeLast );
	void weaponEmplacementStyleTick( double timeNow, double timeLast );


	void transformCommonToLocal( SpaceID spaceID, EntityID vehicleID,
		Vector3 & pos, Direction3D & dir ) const;
	void transformLocalToCommon( SpaceID spaceID, EntityID vehicleID,
		Vector3 & pos, Direction3D & dir ) const;

	void stop( bool noslide );

	void calculateInputVelocity(const Angle& velocityYaw,
		const Angle& thisPitch,
		Vector3& useVelocity,
		float dTime);

	void doSeek( const MotionConstants& mc,
		const Vector3& useVelocity,
		bool& disableVelocitySmoothing,
		Angle& newHeadYaw,
		Vector3& newPos,
		float dTime );

	void doChaseTarget( const MotionConstants& mc,
		bool& disableVelocitySmoothing,
		Angle& newHeadYaw,
		Vector3& newPos,
		float dTime );

	void doTargetDestination( const MotionConstants& mc,
		Angle& desiredHeadPitch,
		Angle& desiredHeadYaw );

	void applyVelocitySmoothing( bool disableVelocitySmoothing,
		const Vector3& oldPos,
		Vector3& newPos,
		Vector3& thisDisplacement,
		float dTime,
		double timeNow,
		double timeLast );

	void ensureAboveTerrain( const IEntityEmbodiment * pEmb,
		const MotionConstants& mc,
		Vector3& newPos,
		bool& posNotLoaded );

	float scaleUphillSpeed( float maxDistance,
		const Vector3& oldPos,
		Vector3& newPos );

	void doTeleport( IEntityEmbodiment* pEmb,
		MotionConstants& mc,
		Vector3& newPos,
		Angle& newHeadYaw,
		Angle& newHeadPitch,
		SpaceID& spaceID,
		EntityID& vehicleID );

	/**
	 *	This class holds information about the collisions that physics
	 *	will perform.
	 */
	class CollisionInfo
	{
	public:
		Vector3 delta_;
		Vector3 flatDir_;
		Vector3 bottomLeft_;
		Vector3 dest_;
		Vector3 widthVector_;
		Vector3 halfWidthVector_;
		Vector3 heightVector_;
		Vector3 frontBuffer_;
		float frontDistance_;
		float maxAdvance_;
	};

	bool collisionSetup( const Vector3& newPos,
		const Vector3& oldPos,
		const MotionConstants& mc,
		bool shouldSlide,
		bool reverseCheck,
		CollisionInfo& ci ) const;

	bool preventCollision( const Vector3 &oldPos, Vector3 &newPos,
		const MotionConstants & mc, bool shouldSlide = true,
		bool reverseCheck = false ) const;

	bool preventSteepSlope( const Vector3& origDesiredDir,
		const Vector3 &oldPos, Vector3 &newPos,
		const MotionConstants& mc, float dTime) const;

	float applyGravityAndBuoyancy( EntityID & vehicleID, const Vector3 & oldPos, Vector3 & newPos,
		const MotionConstants & mc, float deltaTime ) const;

	void calculateJoystickVelocity();


	void hoverStyleTick( double timeNow, double timeLast );

	void integrateHoverPosition( float elapsedTime, Vector3 & position,
		float & yaw, float & pitch, float & roll );

	Entity *	pSlave_;
	int			style_;

	const Vector3& velocity() const
	{
		return velocity_;
	}

	void velocity( const Vector3& v)
	{
		velocity_ = v;
	}

	float smoothingTime() const
	{
		return smoothingTime_;
	}

	void smoothingTime( float time )
	{
		smoothingTime_ = max(time,0.01f);
		blendingReset_ = true;
	}

	Vector3		velocity_;
	float		smoothingTime_;
	bool		blendingReset_;
	Vector3		nudge_;
	MouseAxis	velocityMouse_;
	bool		userDriven_;
	bool		joystickEnabled_;

	//This vector4 provider records the entity
	//velocity, minus some collision corrections.
	//It can be used to drive animations.
	SmartPointer<Vector4Basic> uncorrectedVelocity_;

	double		angular_;
	double		turn_;
	MouseAxis	angularMouse_;
	bool		userDirected_;
	bool		dcLocked_;

	bool		oldStyleCollision_;
	bool		collide_;
	bool		collideTerrain_;
	bool		collideObjects_;
	bool		fall_;
	float		fallingVelocity_;
	float		gravity_;

	bool		collided_;
	Vector3		collisionVector_;
	Vector3		collisionPosition_;
	float		collisionSeverity_;
	int			collisionTriangleFlags_;

	float		maximumSlope_;

	double		thrust_;
	double		brake_;
	Vector3		thrustAxis_;

	SeekState	seekState_;
	PathSeeker		seeker_;
	Vector4		seek_;
	double		seekDeadline_;
	float		seekHeightTolerance_;
	PyObject	*seekCallback_;

	Angle		lastYaw_;
	Angle		lastPitch_;

	Vector3		lastVelocity_;

	float		runFwdSpeed_;
	float		runBackSpeed_;
	Vector2		joystickPos_;
	Vector3		joystickVelocity_;

	Vector3		desiredPosition_;

	float		blendingFromSpeed_;
	double		blendingFromFinish_;
	Vector3		blendingFromDirection_;
	float		lastBlendedSpeed_;

	ClientSpacePtr	teleportSpace_;
	EntityID		teleportVehicle_;
	Vector3			teleportPos_;
	Vector3			teleportDir_;

	MatrixProviderPtr	targetSource_;
	MatrixProviderPtr	targetDest_;

	float		modelHeight_;
	float		modelWidth_;
	float		modelDepth_;

	float		scrambleHeight_;

	bool		inWater_;
	float		waterSurfaceHeight_;
	float		buoyancy_;
	float		viscosity_;

	EntityPtr	chaseTarget_;
	float		chaseDistance_;
	float		chaseTolerance_;

	Vector3		movedDirection_;
	float		movedDistance_;
	bool		moving_;
	static float movementThreshold_;

	Data		data_;

	HoverSetting	ripper_;

	SmartPointer<PyObject>	pAxisEventNotifier_;
	float axisEventNotifierThreshold_;
	SmartPointer<PyObject>	pIsMovingNotifier_;

	typedef BW::vector<Physics*> Physicians;
	static Physicians		s_physicians_;

	friend class WaypointPhysicsHandler;
	friend struct MotionConstants;
};


int setData( PyObject * pObject, Physics::MouseAxis & rCaps,
	const char * varName = "" );

PyObject * getData( const Physics::MouseAxis data );





inline Physics::Data::Data()
: momentum_( 0, 0, 0 ),
  position_( 0, 0, 0 )
{
}

inline Physics::Data::Data(const Vector3& momentum, const Vector3& position) :
	momentum_(momentum),
	position_(position)
{
}

inline Physics::Data operator *(float t, const Physics::Data & d)
{
	return Physics::Data(t * d.momentum_,
		t * d.position_);
}


inline Physics::Data operator +(const Physics::Data & d1, const Physics::Data & d2)
{
	return Physics::Data( d1.momentum_ + d2.momentum_,
		d1.position_ + d2.position_ );
}

inline void Physics::Data::operator+=(const Physics::Data & data)
{
	momentum_ += data.momentum_;
	position_ += data.position_;
}

BW_END_NAMESPACE

#endif // PHYSICS_HPP
