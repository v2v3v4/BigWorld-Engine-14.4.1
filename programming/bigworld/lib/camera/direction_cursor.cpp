#include "pch.hpp"
#include "direction_cursor.hpp"

#include "moo/render_context.hpp"	// for fov. lame, I know.

#include "base_camera.hpp"
#include "annal.hpp"
#include <algorithm>

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Camera", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "direction_cursor.ipp"
#endif


// -----------------------------------------------------------------------------
// SimpleSpeedProvider
// -----------------------------------------------------------------------------

void SimpleSpeedProvider::readInitialValues( DataSectionPtr section )
{
	BW_GUARD;
	// Read in joystick behaviour defaults.
	horizontalSpeed_ = section->readFloat( "joystickHorizontalTurnSpeed", 2.f );
	verticalSpeed_ = section->readFloat( "joystickVerticalTurnSpeed", 1.f );
}

float SimpleSpeedProvider::value( const AxisEvent &event )
{
	BW_GUARD;
	switch (event.axis())
	{
		default:
			return 0.0f;

		case AxisEvent::AXIS_RX:
			return event.value() * horizontalSpeed_;

		case AxisEvent::AXIS_RY:
			return event.value() * verticalSpeed_;
	}
}

void SimpleSpeedProvider::adjustDirection( float dTime, Angle& pitch, Angle& yaw, Angle& roll )
{
	// No adjustments
}


DirectionCursor * DirectionCursor::pInstance_ = NULL;


// -----------------------------------------------------------------------------
// Constructor and Destructor for DirectionCursor.
// -----------------------------------------------------------------------------

/**
 *	The constructor for the DirectionCursor class. The parameters after the
 *	'section' parameter all have default arguments. These arguments are used
 *	as default catch-values should their corresponding values not be found in
 *	the resource.
 *
 *	@param	rootSection				The Root Section of the configuration file.
 *	@param	speedProvider			An object to provide the cursor's speed.
 *	@param	pitchInRadians			The pitch angle in radians.
 *	@param	yawInRadians			The yaw angle in radians.
 *	@param	invertVertical			The flag for vertical movement inversion.
 *	@param	initialMouseSensitivity	Mouse sensitivity multiplier.
 *	@param	initialMouseHVBias		Mouse horizontal/vertical bias.
 *	@param	maxPitchInRadians		The maximum value of the cursor pitch.
 *	@param	minPitchInRadians		The minimum value of the cursor pitch.
 *	@param	pType					The derived Python object type.
 */
DirectionCursor::DirectionCursor( DataSectionPtr rootSection,
		SpeedProvider* speedProvider,
		const Angle &pitchInRadians,
		const Angle &yawInRadians,
		bool invertVertical,
		float initialMouseSensitivity,
		float initialMouseHVBias,
		const Angle &maxPitchInRadians,
		const Angle &minPitchInRadians,
		PyTypeObject * pType ) :
	InputCursor( pType ),
	pitch_( pitchInRadians ),
	yaw_( yawInRadians ),
	roll_( 0.0f ),
	direction_( 0.0f, 0.0f, 0.0f ),
	refreshDirection_( false ),
	invertVerticalMovement_( invertVertical ),
	mouseSensitivity_( initialMouseSensitivity ),
	mouseHVBias_( initialMouseHVBias ),
	maxPitch_( maxPitchInRadians ),
	minPitch_( minPitchInRadians ),
	speedProvider_( speedProvider ),
	maxYaw_( 0.f ),
	minYaw_( 0.f ),
	lookSpring_ ( true ),
	lookSpringRate_( 0.90f ),
	lookSpringIdleTime_( 1.0 ),
	lookSpringOnMove_( true ),
	forcedLookSpring_( false ),
	elapsedTimeForLookSpring_( 0.0 )
{
	BW_GUARD;
	pInstance_ = this;

	MF_ASSERT( speedProvider );

	DataSectionPtr section = rootSection->openSection( "directionCursor" );

	if (section) {
		// Read in the direction defaults.
		this->pitch( DEG_TO_RAD( section->readFloat( "pitch",
			RAD_TO_DEG( this->pitch() ) ) ) );
		this->yaw( DEG_TO_RAD( section->readFloat( "yaw",
			RAD_TO_DEG( this->yaw() ) ) ) );
		direction_.setPitchYaw( -this->pitch(), this->yaw() );
		refreshDirection_ = false;

		// Read in general behaviour defaults.
		this->invertVerticalMovement( section->readBool(
			"invertVerticalMovement",
			this->invertVerticalMovement() ) );

		// Read in mouse behaviour defaults.
		this->mouseSensitivity( section->readFloat( "mouseSensitivity",
			this->mouseSensitivity() ) );
		this->mouseHVBias( section->readFloat( "mouseHVBias",
			this->mouseHVBias() ) );

		// Read in cursor limiting defaults.
		this->maxPitch( DEG_TO_RAD( section->readFloat( "maxPitch",
			RAD_TO_DEG( this->maxPitch() ) ) ) );
		this->minPitch( DEG_TO_RAD( section->readFloat( "minPitch",
			RAD_TO_DEG( this->minPitch() ) ) ) );

		// Read in look-spring behaviour.
		this->lookSpring( section->readBool( "lookSpring",
			this->lookSpring() ) );
		this->lookSpringRate( section->readFloat( "lookSpringRate",
			this->lookSpringRate() ) );
		this->lookSpringIdleTime( section->readDouble( "lookSpringIdleTime",
			this->lookSpringIdleTime() ) );
		this->lookSpringOnMove( section->readBool( "lookSpringOnMove",
			this->lookSpringOnMove() ) );
	}
}


/**
 *	Destructor for the DirectionCursor class.
 */
DirectionCursor::~DirectionCursor()
{
	pInstance_ = NULL;
}


/**
 *	Helper class turning a direction cursor into a matrix
 */
class DCMProv : public MatrixProvider
{
public:
	DCMProv( const DirectionCursor & dc ) :
		MatrixProvider( false, &s_type_ ),
		dc_( dc ) { }

	virtual void matrix( Matrix & m ) const
	{
		m.setRotate( dc_.yaw(), dc_.pitch(), dc_.roll() );
	};

private:
	const DirectionCursor & dc_;
};


/**
 *	Gets an object which provides a matrix representation of
 *	this direction cursor
 */
MatrixProviderPtr DirectionCursor::provider()
{
	BW_GUARD;
	return MatrixProviderPtr(
		new DCMProv( *this ), PyObjectPtr::STEAL_REFERENCE );
}


/**
 *	Updates the directionCursor, allowing it to keep track of elapsed time
 *	since last movement update.
 */
void DirectionCursor::tick( float deltaTime )
{
	BW_GUARD;
	// This is the countdown time for returning to neutral pitch.
	elapsedTimeForLookSpring_ += deltaTime;

	if ( this->lookSpring() )
	{
		// Slide the pitch towards neutral if enough idle time has elapsed.
		if ( elapsedTimeForLookSpring_ > this->lookSpringIdleTime() ||
			this->forcedLookSpring_ )
		{
			// Cannot use pitch() as it will be treated as external change,
			// which resets the elapsed time counter.
			pitch_ = Math::decay( pitch_,
				0.f, this->lookSpringRate(), deltaTime );

			// Set dirty flag.
			refreshDirection_ = true;

			// Reset forced lookSpring if at neutral position, counting as one
			// completed application of the lookSpring.
			if ( this->pitch() - 0.0f <= 0.001f )
			{
				this->forcedLookSpring_ = false;
			}
		}
		pitch_ = Math::clamp( this->minPitch(), pitch_, this->maxPitch() );
	}

	// Slowly shift any non-zero yaw back towards zero.
	if ( this->roll() != 0.0f )
	{
		const float r = this->roll();
		const float RADIANS_PER_SECOND = 0.5f;
		this->roll( r > 0.f ?
			max(0.f,r-RADIANS_PER_SECOND*deltaTime) :
			min(0.f,r+RADIANS_PER_SECOND*deltaTime) );
	}

	speedProvider_->adjustDirection( deltaTime, pitch_, yaw_, roll_ );
}

// -----------------------------------------------------------------------------
// InputHandler Overrides for DirectionCursor.
// -----------------------------------------------------------------------------

/**
 *	Handles a key event. If there are any key events that would affect the
 *	DirectionCursor, they are processed here. A key event is defined as a key
 *	or button press, regardless of the device.
 *
 *	@param	event		The key event.
 *
 *	@return	True, if the event was processed. False, if the event was ignored.
 */
bool DirectionCursor::handleKeyEvent( const KeyEvent &event )
{
	return false;
}

/**
 *	Handles a mouse event. The majority of changes to the DirectionCursor will
 *	come from events from positional devices, such as joysticks and mouses.
 *
 *	@param	event		The mouse event.
 *
 *	@return	True, if the event was processed. False, if the event was ignored.
 */
bool DirectionCursor::handleMouseEvent( const MouseEvent &event )
{
	BW_GUARD;
	// Find the mouse sensitivity
	float msens = this->mouseSensitivity() *
		Moo::rc().camera().fov() / DEG_TO_RAD( 60.f );

	// Adjust raw mouse values into changes in angle.
	Angle dYaw = event.dx() * this->mouseHVBias() * msens;
	Angle dPitch = event.dy() * ( 1.0f - this->mouseHVBias() ) * msens;

	// Apply the changes to current cursor direction.
	Angle newYaw( this->yaw() + dYaw );
	if ( yawReference_ )
	{
		Matrix m;
		yawReference_->matrix( m );
		float yawRefVal = m.yaw();
		newYaw.clampBetween( yawRefVal+minYaw_, yawRefVal+maxYaw_ );
	}
	this->yaw( newYaw );

	if ( this->invertVerticalMovement() )
	{
		this->pitch( this->pitch() - dPitch );
	}
	else
	{
		this->pitch( this->pitch() + dPitch );
	}

	// Limit the pitch values.
	this->pitch( Math::clamp( this->minPitch(),
		this->pitch(),
		this->maxPitch() ) );

	// Set the dirty flag for direction.
	refreshDirection_ = true;

	// Reset elapsed time for kicking in the look-spring behaviour.
	elapsedTimeForLookSpring_ = 0.0;

	// Event has been handled.
	return true;
}


/**
 *	Handles an axis event. When a joystick is being used, the direction
 *	cursor takes all its input from its position every frame instead of using
 *	mouse movement events. If the axis is within the dead zone, this function
 *	will not be called.
 */
bool DirectionCursor::handleAxisEvent( const AxisEvent &event )
{
	BW_GUARD;
	bool handled = true;

	//adjustment for binoculars etc.
	float sens = Moo::rc().camera().fov() / DEG_TO_RAD( 60.f );

	switch (event.axis())
	{
	default:
		handled = false;
		break;

	case AxisEvent::AXIS_RX:
		{
			float yawRate = speedProvider_->value( event );
			this->yaw( this->yaw() + yawRate * event.dTime() * sens );
			// Reset elapsed time for kicking in the look-spring behaviour.
			elapsedTimeForLookSpring_ = 0.0;
		}
		break;

	case AxisEvent::AXIS_RY:
		{
			float pitchRate = speedProvider_->value( event );
			if (this->invertVerticalMovement())
				pitchRate *= -1.0f;
			Angle pitch = this->pitch() + pitchRate * event.dTime() * sens;
			this->pitch( Math::clamp( this->minPitch(), pitch, this->maxPitch() ) );
			// Reset elapsed time for kicking in the look-spring behaviour.
			elapsedTimeForLookSpring_ = 0.0;
		}
		break;
	}

	return handled;
}


// -----------------------------------------------------------------------------
// Python stuff
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( DirectionCursor )

PY_BEGIN_METHODS( DirectionCursor )
	PY_METHOD( yawPitch )
	PY_METHOD( forceLookSpring )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( DirectionCursor )
	/*~ attribute DirectionCursor.pitch
	 *
	 *	The pitch (whether it is tipped up or down) of the direction cursor.
	 *	Positive numbers are looking upwards, negative are looking downwards.
	 *	If this is set, then the direction cursor moves to the new pitch at 0.5
	 *	radians per second, rather than just popping.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( pitch )
	/*~ attribute DirectionCursor.yaw
	 *
	 *	The yaw (rotation around the Y-axis) of the direction cursor.
	 *	If this is set, then the direction cursor moves to the new pitch at 0.5
	 *	radians per second, rather than just popping.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( yaw )
	/*~ attribute DirectionCursor.roll
	 *
	 *	The roll (whether it is tipped to the left or the right) of the
	 *	direction cursor.  If this is set, then the direction cursor moves to
	 *	the new pitch at 0.5 radians per second, rather than just popping.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( roll )
	/*~ attribute DirectionCursor.matrix
	 *
	 *	The transformation matrix which sums up the location and orientation of
	 *	the direction cursor.  The pitch, yaw and roll, as well as the origin
	 *	are all stored inside this.
	 *
	 *	@type	Read-Only MatrixProvider
	 */
	PY_ATTRIBUTE( matrix )

	/*~ attribute DirectionCursor.invertVerticalMovement
	 *
	 *	If this attribute is set to 0, then moving the mouse upward causes the
	 *	pitch of the DirectionCursor to increase (ie look upwards more), and
	 *	down causes it to decrease.  For any other value, moving the mouse up
	 *	causes the pitch of the DirectionCursor to decrease (ie look downwards
	 *	more), and down to increase.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( invertVerticalMovement )
	/*~ attribute DirectionCursor.mouseSensitivity
	 *
	 *	How sensitive the DirectionCursor is to mouse movements.  Higher
	 *	numbers mean more sensative.  This effects both horizontal and vertical
	 *	sensitivity.  In order to change the ratio of sensativity between
	 *	horizontal and vertical, use the mouseHVBias.
	 *
	 *	Like all DirectionCursor attributes, its default is read in from
	 *	engine_config.xml
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( mouseSensitivity )
	/*~ attribute DirectionCursor.mouseHVBias
	 *
	 *	This effects whether the mouse is more sensitive in the Horizontal or
	 *	Vertical directions.  It is clamped to be between 0 and 1, with 1
	 *	meaning horizontal motion is greatest, and vertical is completely
	 *	disregarded, and 0 meaning vertical motion is most sensative, with
	 *	horizontal being disregarded.  To make both axes more
	 *	sensitive use the mouseSensitivity attribute.
	 *
	 *	Like all DirectionCursor attributes, its default is read in from
	 *	engine_config.xml
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( mouseHVBias )
	/*~ attribute DirectionCursor.maxPitch
	 *
	 *	This is an upper bound for pitch.  It is only checked when a mouse
	 *	movement attempts to change the pitch.  That is, if maxPitch is
	 *	assigned to be less than the current pitch, then pitch will not
	 *	automatically change to be within bounds.  Similarly, if pitch is
	 *	assigned a value above maxPitch, the limit will not be applied until
	 *	the user moves the mouse.
	 *
	 *	Like all DirectionCursor attributes, its default is read in from
	 *	engine_config.xml.  However, in the cofig file, it is specified in degrees,
	 *	while in python, it is in radians.
	 *
	 *	@type Float
	 */
	PY_ATTRIBUTE( maxPitch )
	/*~ attribute DirectionCursor.minPitch
	 *
	 *	This is a lower bound for pitch.  It is only checked when a mouse
	 *	movement attempts to change the pitch.  That is, if minPitch is
	 *	assigned to be greater than the current pitch, then pitch will not
	 *	automatically change to be within bounds.  Similarly, if pitch is
	 *	assigned a value below minPitch, the limit will not be applied until
	 *	the user moves the mouse.
	 *
	 *	Like all DirectionCursor attributes, its default is read in from
	 *	engine_config.xml.  However, in the cofig file, it is specified in degrees,
	 *	while in python, it is in radians.
	 *
	 *	@type Float
	 */
	PY_ATTRIBUTE( minPitch )
	/*~ attribute DirectionCursor.minYaw
	 *
	 *	This attribute is the minimum yaw that the direction cursor will
	 *	go to. Only checked if yawReference is set. Negative yaw values are
	 *	on the left of yawReference as seen from above.
	 */
	PY_ATTRIBUTE( minYaw )
	/*~ attribute DirectionCursor.maxYaw
	 *
	 *	This attribute is the maximum yaw that the direction cursor will
	 *	go to. Only checked if yawReference is set. Positive yaw values are
	 *	on the right of yawReference as seen from above.
	 */
	PY_ATTRIBUTE( maxYaw )
	/*~ attribute DirectionCursor.yawReference
	 *
	 *	This attribute is the matrix whose direction is used as a reference
	 *	when enforcing the min/max yaw limits.
	 */
	PY_ATTRIBUTE( yawReference )
	/*~ attribute DirectionCursor.lookSpring
	 *
	 *	This attribute determines whether or not the pitch reverts to 0 once
	 *	lookSpringIdleTime has elapsed, with no further changes to the
	 *	DirectionCursor.  If it is set non-zero, then, once lookSpringIdleTime
	 *	has elapsed, it will move towards zero at lookSpringRate.  If it is
	 *	zero, then the DirectionCursor's pitch will remain where it was set,
	 *	unless lookSpringOnMove is non-zero at which point, the pitch reverts
	 *	to zero.
	 *
	 *	Like all DirectionCursor attributes, its default is read in from
	 *	engine_config.xml.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( lookSpring )
	/*~ attribute DirectionCursor.lookSpringRate
	 *
	 *	This attribute is the rate (in radians per second) at which the pitch
	 *	of the DirectionCursor restores itself to zero, if it decides to do
	 *	lookSpring behaviour.  This will happen either if lookSpring is true,
	 *	and lookSpringIdleTime has elapsed, or if lookSpringOnMove is true, and
	 *	the DirectionCursor's origin is changed.
	 *
	 *	Like all DirectionCursor attributes, its default is read in from
	 *	engine_config.xml.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( lookSpringRate )
	/*~ attribute DirectionCursor.lookSpringIdleTime
	 *
	 *	This is the time (in seconds) that the rotation of the DirectionCursor
	 *	must remain unchanged before it starts restoring its pitch
	 *	to zero.  This will only happen if the lookSpring flag is non-zero.
	 *
	 *	Like all DirectionCursor attributes, its default is read in from
	 *	engine_config.xml.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( lookSpringIdleTime )
	/*~ attribute DirectionCursor.lookSpringOnMove
	 *
	 *	If this attribute is set to true, then if the origin of the
	 *	DirectionCursor is changed, the pitch will start moving towards zero at
	 *	lookSpringRate radians per second.
	 *
	 *	Like all DirectionCursor attributes, its default is read in from
	 *	engine_config.xml.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( lookSpringOnMove )
PY_END_ATTRIBUTES()


/*~ function DirectionCursor.yawPitch
 *
 *	This function gets and optionally sets the yaw and pitch of the direction
 *	cursor.  It takes 0, 1 or 2 arguments.  If one argument is supplied, it
 *  is the new yaw.  If Two arguments are supplied, they are the yaw and pitch.
 *	It returns a list of the yaw and the pitch in all cases.
 *
 *	@param	yaw		(optional) The new yaw for the direction cursor
 *	@param	pitch	(optional) The new pitch for the direction cursor
 *
 *	@return			a list with the first element being the yaw, and the second
 *					being the pitch
 */
/**
 *	Gets or sets direction cursor info
 */
PyObject * DirectionCursor::py_yawPitch( PyObject * args )
{
	BW_GUARD;
	Py_ssize_t nargs = PyTuple_Size( args );

	float nyaw = 0, npitch = 0;
	if (!PyArg_ParseTuple( args, "|ff", &nyaw, &npitch ))
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.directionCursor "
			"expects an optional pitch and yaw as floats" );
		return NULL;
	}

	if (nargs >= 1)
	{
		this->yaw( nyaw );
		if (nargs >= 2)
		{
			this->pitch( npitch );
		}
	}

	return Py_BuildValue( "(ff)", (float)this->yaw(), (float)this->pitch() );
};


/*~ function BigWorld.dcursor
 *
 *	This function returns the direction cursor which is used to control which
 *  way the user is looking with the mouse.
 *
 *	@return the current direction cursor.
 */
/**
 *	Gets a reference to the direction cursor
 */
PyObject * DirectionCursor::py_dcursor( PyObject * args )
{
	BW_GUARD;
	DirectionCursor * pDC = &DirectionCursor::instance();
	Py_INCREF( pDC );
	return pDC;
}
PY_MODULE_STATIC_METHOD( DirectionCursor, dcursor, BigWorld )

BW_END_NAMESPACE

// direction_cursor.cpp
