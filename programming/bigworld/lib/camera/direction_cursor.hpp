#ifndef DIRECTION_CURSOR_HPP
#define DIRECTION_CURSOR_HPP


#include "math/angle.hpp"
#include "moo/moo_math.hpp"

#include "resmgr/datasection.hpp"

#include "input/input_cursor.hpp"

#include "pyscript/script.hpp"

#include "base_camera.hpp"


BW_BEGIN_NAMESPACE

// Default Constants.
const float INITIAL_PITCH				= 0.0f;
const float INITIAL_YAW					= 0.0f;
const bool	INITIAL_VERTICAL_POLARITY	= false;
const float INITIAL_MOUSE_SENSITIVITY	= 0.01f;
const float INITIAL_MOUSE_HV_BIAS		= 0.33f;
const float INITIAL_MAX_PITCH			= DEG_TO_RAD(  35.0f );
const float INITIAL_MIN_PITCH			= DEG_TO_RAD( -35.0f );

/**
 *	The SpeedProvider object provides speed control over a
 *	DirectionCursor.
 */
class SpeedProvider
{
public:
	SpeedProvider() {}

	virtual void readInitialValues( DataSectionPtr section ) = 0;

	virtual float value( const AxisEvent &event ) = 0;
	virtual void adjustDirection( float dTime, Angle& pitch, Angle& yaw, Angle& roll ) = 0;
};


/**
 *	The SimpleSpeedProvider object provides linear speed control to a
 *	DirectionCursor.
 */
class SimpleSpeedProvider : public SpeedProvider
{
public:
	SimpleSpeedProvider( float horizontalSpeed,  float verticalSpeed );

	virtual void readInitialValues( DataSectionPtr section );

	virtual float value( const AxisEvent &event );
	virtual void adjustDirection( float dTime, Angle& pitch, Angle& yaw, Angle& roll );

protected:
	float horizontalSpeed_;
	float verticalSpeed_;
};


/*~	class BigWorld.DirectionCursor
 *
 *	The DirectionCursor object acts as a view target for the player entity.  It
 *  accepts	mouse events and represents its direction based on the update.  It
 *  is represented as a direction vector from the origin intersecting a point
 *	on a sphere centered on the origin.
 *
 *	The origin represents the observer's eyes in local coordinate space. The
 *	direction represents the line of sight along observer's view direction.
 *
 *	The attributes of the direction cursor receive their default values from 
 *  the &lt;directioncursor> section of engine_config.xml.
 *
 *	You can obtain the direction cursor using the function BigWorld.dcursor()
 */
/**
 *	The DirectionCursor object acts as a view target for an object. It accepts
 *	device events and represents its direction based on the update. It is
 *	represented as a direction vector from the origin intersecting a point
 *	on a sphere centered on the origin.
 *
 *	The origin represents the observer's eyes in local coordinate space. The
 *	direction represents the line of sight along observer's view direction.
 *
 *	You can obtain the direction cursor using the BigWorld.dcursor function.
 */
class DirectionCursor : public InputCursor
{
	Py_Header( DirectionCursor, InputCursor )

public:
	///	@name Constructors and Destructor.
	//@{
	DirectionCursor( DataSectionPtr rootSection,
		SpeedProvider* speedProvider = NULL,
		const Angle &pitchInRadians = INITIAL_PITCH,
		const Angle &yawInRadians = INITIAL_YAW,
		bool invertVertical = INITIAL_VERTICAL_POLARITY,
		float initialMouseSensitivity = INITIAL_MOUSE_SENSITIVITY,
		float initialMouseHVBias = INITIAL_MOUSE_HV_BIAS,
		const Angle &maxPitchInRadians = INITIAL_MAX_PITCH,
		const Angle &minPitchInRadians = INITIAL_MIN_PITCH,
		PyTypeObject * pType = &s_type_ );
   ~DirectionCursor();
	//@}


	///	@name Methods associated with DirectionCursor Representation.
	//@{
	const Vector3& direction() const;
	void direction( const Angle& pitchInRadians, const Angle &yawInRadians );

	const Angle& pitch() const;
	void pitch( const Angle &pitchInRadians );

	const Angle& yaw() const;
	void yaw( const Angle &yawInRadians );

	const Angle& roll() const;
	void roll( const Angle &rollInRadians );

	MatrixProviderPtr provider();
	//@}


	///	@name Methods associated with DirectionCursor Behaviour.
	//@{
	bool invertVerticalMovement( void ) const;
	void invertVerticalMovement( bool flag );

	float mouseSensitivity( void ) const;
	void mouseSensitivity( float newMultiplier );

	float mouseHVBias( void ) const;
	void mouseHVBias( float newHVBias );

	const Angle &maxPitch( void ) const;
	void maxPitch ( const Angle &newAngleInRadians );

	const Angle &minPitch( void ) const;
	void minPitch ( const Angle &newAngleInRadians );

	bool lookSpring( void ) const;
	void lookSpring( bool flag );

	float lookSpringRate( void ) const;
	void lookSpringRate( float newRate );

	double lookSpringIdleTime( void ) const;
	void lookSpringIdleTime( double newTimeInSeconds );

	bool lookSpringOnMove( void ) const;
	void lookSpringOnMove( bool flag );

	void forceLookSpring();

	void tick( float deltaTime );
	//@}

	///	@name Python stuff
	//@{
	PY_METHOD_DECLARE( py_yawPitch )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, pitch, pitch )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, yaw, yaw )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, roll, roll )
	PY_RO_ATTRIBUTE_DECLARE( provider(), matrix )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(
		bool, invertVerticalMovement, invertVerticalMovement )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(
		float, mouseSensitivity, mouseSensitivity )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, mouseHVBias, mouseHVBias )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, maxPitch, maxPitch )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, minPitch, minPitch )

	PY_RW_ATTRIBUTE_DECLARE( minYaw_, minYaw )
	PY_RW_ATTRIBUTE_DECLARE( maxYaw_, maxYaw )
	PY_RW_ATTRIBUTE_DECLARE( yawReference_, yawReference )

	//PY_RW_ATTRIBUTE_DECLARE( joystickTurnSpeed_, joystickTurnSpeed )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, lookSpring, lookSpring )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, lookSpringRate, lookSpringRate )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(
		double, lookSpringIdleTime, lookSpringIdleTime )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, lookSpringOnMove, lookSpringOnMove )

	PY_AUTO_METHOD_DECLARE( RETVOID, forceLookSpring, END )

	PY_MODULE_STATIC_METHOD_DECLARE( py_dcursor )
	//@}

	/// This method returns the singleton instance of this class.
	static DirectionCursor & instance()	{ return *pInstance_; }

protected:
	/// @name InputHandler Overrides.
	//@{
	virtual bool handleKeyEvent( const KeyEvent &event );
	virtual bool handleMouseEvent( const MouseEvent &event );
	virtual bool handleAxisEvent( const AxisEvent &event );
	//@}

private:
	///	@name Data Members associated with DirectionCursor Representation.
	//@{
	mutable Vector3 direction_;
	mutable bool refreshDirection_;
	Angle pitch_;
	Angle yaw_;
	Angle roll_;
	//@}

	///	@name Data Members associated with DirectionCursor Behaviour.
	//@{
	bool invertVerticalMovement_;
	float mouseSensitivity_;
	float mouseHVBias_;
	Angle maxPitch_;
	Angle minPitch_;
	SpeedProvider* speedProvider_;

	float minYaw_;
	float maxYaw_;
	MatrixProviderPtr	yawReference_;

	//float joystickTurnSpeed_;

	bool lookSpring_;
	float lookSpringRate_;
	double lookSpringIdleTime_;
	bool lookSpringOnMove_;
	bool forcedLookSpring_;
	double elapsedTimeForLookSpring_;
	//@}

	///	@name Unimplemented Methods made private to restrict accidental usage.
	//@{
	///	The Copy Constructor.
	DirectionCursor( const DirectionCursor &rhs );

	///	The Assignment Operator.
	DirectionCursor &operator = ( const DirectionCursor &rhs );
	//@}

	static DirectionCursor * pInstance_;
};

typedef SmartPointer<DirectionCursor> DirectionCursorPtr;

#ifdef CODE_INLINE
#include "direction_cursor.ipp"
#endif

BW_END_NAMESPACE

#endif // DIRECTION_CURSOR_HPP
/* direction_cursor.hpp */
