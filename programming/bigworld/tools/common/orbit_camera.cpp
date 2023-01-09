#include "pch.hpp"

#include "orbit_camera.hpp"
#include "math/mathdef.hpp"
#include "resmgr/string_provider.hpp"
#include "editor_shared/cursor/cursor.hpp"

BW_BEGIN_NAMESPACE

const float MAX_PITCH = (MATH_PI / 2.f) * 0.75f;
const float MIN_PITCH = (-MATH_PI / 2.f) * 0.75f;

/**
 *	Constructor.
 */
OrbitCamera::OrbitCamera() :
	interactive_( false ),
	rotDir_( -1 ),
	orbitSpeed_( 1.f ),
    pitch_( 0.f ),
    yaw_( 0.f ),
	radius_( 10.f ),
    origin_( 0,0,0 )
{
	BW_GUARD;

	lastCursorPosition_.x = -1;
	lastCursorPosition_.y = -1;

	keyDown_.insert( std::make_pair( KeyCode::KEY_W, false ) );
	keyDown_.insert( std::make_pair( KeyCode::KEY_S, false ) );
	keyDown_.insert( std::make_pair( KeyCode::KEY_A, false ) );
	keyDown_.insert( std::make_pair( KeyCode::KEY_D, false ) );
	keyDown_.insert( std::make_pair( KeyCode::KEY_Q, false ) );
	keyDown_.insert( std::make_pair( KeyCode::KEY_E, false ) );
}


/**
 *	Destructor
 */
OrbitCamera::~OrbitCamera()
{
}


/**
 *	This method sets the view matrix for the orbit camera.
 *	The polar coordinates are calculated thereafter.
 *
 *	@param v	The new view matrix for the camera.
 */
void OrbitCamera::view( const Matrix & v )
{
	BW_GUARD;

	view_ = v;
	viewToPolar();
}


/**
 *	This method gets the view matrix for the orbit camera.
 *
 *	@returns	The view matrix of the camera.
 */
const Matrix & OrbitCamera::view() const
{
	return view_;
}


/**
 * This method updates the camera.
 *
 * @param dTime the change in time since the last frame
 */
void OrbitCamera::update( float dTime, bool activeInputHandler )
{
	BW_GUARD;

    //update position
    if ( activeInputHandler )
		handleInput( dTime );

	if ( !InputDevices::hasFocus() )
	{
		KeyDownMap::iterator it = keyDown_.begin();
		KeyDownMap::iterator end = keyDown_.end();
		while ( it != end )
		{
			(it++)->second = false;
		}
	}

	if ( !interactive_ )
    {
		yaw_ += dTime * orbitSpeed_ * rotDir_;

		if (yaw_ < 0.f)
			yaw_ += MATH_2PI;
		else if (yaw_ > MATH_2PI)
			yaw_ -= MATH_2PI;
	}

	//calculate the view matrix
	polarToView();

	if (isCursorHidden_ && !InputDevices::isKeyDown( KeyCode::KEY_RIGHTMOUSE ))
	{
		isCursorHidden_ = false;
		while (::ShowCursor( TRUE ) < 0) {}
	}
	else if (!isCursorHidden_ && InputDevices::isKeyDown( KeyCode::KEY_RIGHTMOUSE ))
	{
		isCursorHidden_ = true;
		while (::ShowCursor( FALSE ) > -1) {}
	}
}

/**
 * This method handles key events.  Here, we update the keyDown_ array,
 * indicating that we got a key down, and therefore should process repeats.
 */
bool OrbitCamera::handleKeyEvent( const KeyEvent& event )
{
	BW_GUARD;

	// only move if control is not depressed
	if (event.isCtrlDown())
		return false;

	//update the key down map.
	bool handled = false;

	KeyDownMap::iterator found = keyDown_.find( event.key() );

	if ( found != keyDown_.end() )
	{
		if ( event.isKeyDown() )
		{
			found->second = true;
			handled = true;
		}
		else
		{
			if ( found->second )
			{
				found->second = false;
				handled = true;
			}
		}
	}

	return handled;
}

/**
 * This method handles mouse events.  Here, we update the pitch and yaw
 * information.
 */
bool OrbitCamera::handleMouseEvent( const MouseEvent & event )
{
	BW_GUARD;

    bool handled = false;

    if ( interactive_ )
    {
		//read the mouse y
		if ( event.dy() != 0 )
		{
			//adjust pitch in polar coords
            if ( invert_ )
				adjustPitch( event.dy() * -0.01f );
            else
            	adjustPitch( event.dy() * 0.01f );
			handled = true;
		}

	    //read the mouse x for interactive control

		if ( event.dx() != 0 )
		{
			//adjust yaw in polar coords
			yaw_ += event.dx() * 0.01f;
			handled = true;
		}
    }

	return handled;
}


/**
 *	This method reads the keyboard and moves the camera around.
 *
 *	@param dTime	The change in time since the last frame.
 */
void OrbitCamera::handleInput( float dTime )
{
	BW_GUARD;

	float movementSpeed = speed_[0] * dTime;

	if ( InputDevices::isKeyDown( KeyCode::KEY_LCONTROL ) )
		movementSpeed = speed_[1] * dTime;


	if ( InputDevices::isKeyDown( KeyCode::KEY_W ) )
	{
		if ( keyDown_[KeyCode::KEY_W] )
		{
			//move forward
			radius_ -= movementSpeed;
		}
	}
	else
	{
		keyDown_[KeyCode::KEY_W] = false;
	}

	if ( InputDevices::isKeyDown( KeyCode::KEY_S ) )
	{
		if ( keyDown_[KeyCode::KEY_S] )
		{
			//move back
			radius_ += movementSpeed;
		}
	}
	else
	{
		keyDown_[KeyCode::KEY_S] = false;
	}

	static bool s_leftJustDown = false;
	if ( InputDevices::isKeyDown( KeyCode::KEY_A ) )
	{
		if ( keyDown_[KeyCode::KEY_A] )
		{
			//move left
			if (!s_leftJustDown)
			{
				if (rotDir_ > -1)
					rotDir_ -= 1;
				s_leftJustDown = true;
			}
		}
	}
	else
	{
		s_leftJustDown = false;
		keyDown_[KeyCode::KEY_A] = false;
	}

	static bool s_rightJustDown = false;
	if ( InputDevices::isKeyDown( KeyCode::KEY_D ) )
	{
		if ( keyDown_[KeyCode::KEY_D] )
		{
			//move right
			if (!s_rightJustDown)
			{
				if (rotDir_ < 1)
					rotDir_ += 1;
				s_rightJustDown = true;
			}
		}
	}
	else
	{
		s_rightJustDown = false;
		keyDown_[KeyCode::KEY_D] = false;
	}

	if ( InputDevices::isKeyDown( KeyCode::KEY_E ) )
	{
		if ( keyDown_[KeyCode::KEY_E] )
		{
			//move up
			origin_.y += movementSpeed;
		}
	}
	else
	{
		keyDown_[KeyCode::KEY_E] = false;
	}

	if ( InputDevices::isKeyDown( KeyCode::KEY_Q ) )
	{
		if ( keyDown_[KeyCode::KEY_Q] )
		{
			//move down
			origin_.y -= movementSpeed;
		}
	}
	else
	{
		keyDown_[KeyCode::KEY_Q] = false;
	}


	if ( InputDevices::isKeyDown( KeyCode::KEY_RIGHTMOUSE ) )
	{
		interactive_ = true;
		// Keep cursor's click position
		if ( (lastCursorPosition_.x == -1) && (lastCursorPosition_.y == -1) )
			::GetCursorPos( &lastCursorPosition_ );
		Cursor::setPosition( lastCursorPosition_.x, lastCursorPosition_.y );
	}
	else
	{
		//Make sure we stop the camera spin if we just moved it
		if (interactive_)
		{
			rotDir_ = 0;
			interactive_ = false;
		}
		if ( (lastCursorPosition_.x != -1) || (lastCursorPosition_.y != -1) )
		{
			lastCursorPosition_.x = -1;
			lastCursorPosition_.y = -1;
		}
	}

	view_.invert();
}


/**
 *	This method calculates the pitch and yaw from the inverse view matrix.
 */
void OrbitCamera::viewToPolar()
{
	BW_GUARD;

	Matrix invView( view_ );
    invView.invert();
	Vector3 * dir = (Vector3*)&invView.applyToUnitAxisVector(2);

	pitch_ = -(float)atan2( dir->y,	sqrt( dir->z * dir->z + dir->x * dir->x ) );
	yaw_ = atan2f( dir->x, dir->z );
	invView[3] -= origin_;
	radius_ = invView.applyToOrigin().length();

	//Now recalculate the view using the new polar coords
	polarToView();
}


/**
 *	This method calculates the view matrix from the polar coordinates.
 */
void OrbitCamera::polarToView()
{
	BW_GUARD;

	//build view matrix
	D3DXMatrixRotationYawPitchRoll( &view_, yaw_, pitch_, 0.f );
    Matrix t;
    Vector3 offset( 0,0,-radius_ );
    t.setTranslate( offset );
    view_.preMultiply( t );
	view_[3] += origin_;
    view_.invert();
}


/**
 *	This method safely updates the pitch.
 */
void OrbitCamera::adjustPitch( float dPitch )
{
	BW_GUARD;

	pitch_ += dPitch;

	constrainPitch();
}


/**
 *	This method constrains the pitch to reasonable values.
 */
void OrbitCamera::constrainPitch()
{
	if ( pitch_ < MIN_PITCH )
	{
		pitch_ = MIN_PITCH;
	}
	else if ( pitch_ > MAX_PITCH )
	{
		pitch_ = MAX_PITCH;
	}
}


/**
 *	This function is the output stream operator for OrbitCamera.
 */
std::ostream& operator<<(std::ostream& o, const OrbitCamera& t)
{
	o << Localise(L"COMMON/ORBIT_CAMERA/OUTPUT");
	return o;
}


BW_END_NAMESPACE
/*orbit_camera.cpp*/
