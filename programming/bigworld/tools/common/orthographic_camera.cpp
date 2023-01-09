#include "pch.hpp"

#include "orthographic_camera.hpp"
#include "resmgr/string_provider.hpp"

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
OrthographicCamera::OrthographicCamera()
	: up_( 0.f ),
	  right_( 0.f ),
	  isCursorHidden_( false )
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

	// looking straight down
	Vector3 initialPosition(0.f, 100.f, 0.f);
	view_.setRotateX( 1.570796326794f );
	view_.postTranslateBy( initialPosition );
	view_.invert();
}


/**
 *	Destructor
 */
OrthographicCamera::~OrthographicCamera()
{
}


/**
 *	This method sets the view matrix for the mouse look camera.
 *	The polar coordinates are calculated thereafter.
 *
 *	@param v	The new view matrix for the camera.
 */
void OrthographicCamera::view( const Matrix & v )
{
	BW_GUARD;

	view_ = v;
	viewToPolar();
}


/**
 * This method updates the camera.
 *
 * If keys were not handled by the camera, then we reset our state.
 * Because the mouse look camera tests the keyboard states directly,
 * (which it must do because it interprets continuously held down keys ),
 * it must handle the case where the focus has been lost to the camera.
 *
 * @param dTime the change in time since the last frame
 */
void OrthographicCamera::update( float dTime, bool activeInputHandler )
{
	BW_GUARD;

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
        
    polarToView();

	if ( InputDevices::hasFocus() )
	{
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
	else if (isCursorHidden_)
	{
		isCursorHidden_ = false;
		while (::ShowCursor( TRUE ) < 0) {}
	}
}


/**
 * This method handles key events.  Here, we update the keyDown_ array,
 * indicating that we got a key down, and therefore should process repeats.
 */
bool OrthographicCamera::handleKeyEvent( const KeyEvent& event )
{
	BW_GUARD;

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

	// Show and hide the mouse cursor
	if (event.key() == KeyCode::KEY_RIGHTMOUSE)
		::ShowCursor( !event.isKeyDown() );

	return handled;
}


/**
 * This method handles mouse events.  Here, we update the pitch and yaw
 * information.
 */
bool OrthographicCamera::handleMouseEvent( const MouseEvent & event )
{
	BW_GUARD;

	bool handled = false;

    if ( InputDevices::isKeyDown( KeyCode::KEY_RIGHTMOUSE ) )
    {
		if ( event.dy() != 0 )
		{
			if (invert_)
	           	up_ += ( event.dy() * 0.01f );
			else
	           	up_ -= ( event.dy() * 0.01f );
			handled = true;
		}

		if ( event.dx() != 0 )
		{
			if (invert_)
				right_ -= ( event.dx() * 0.01f );
			else
				right_ += ( event.dx() * 0.01f );
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
void OrthographicCamera::handleInput( float dTime )
{
	BW_GUARD;

	view_.invert();

	Vector3 forward = view_.applyToUnitAxisVector( 2 );
	Vector3 up = view_.applyToUnitAxisVector( 1 );
	Vector3 right = view_.applyToUnitAxisVector( 0 );

	Vector3 & position = view_[3];

	float movementSpeed;

	//if ( !InputDevices::isKeyDown( KeyCode::KEY_CAPSLOCK ) )
	if ( (::GetKeyState( VK_CAPITAL ) & 0x0001) == 0 )
		movementSpeed = speed();
	else
		movementSpeed = turboSpeed();

	//frame rate independent speed, but capped
	if ( dTime < 0.1f )
		movementSpeed *= dTime;
	else
		movementSpeed *= 0.1f;


	if ( InputDevices::isKeyDown( KeyCode::KEY_W ) )
	{
		if ( keyDown_[KeyCode::KEY_W] )
		{
			//move forward
			position += up * movementSpeed;
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
			position -= up * movementSpeed;
		}
	}
	else
	{
		keyDown_[KeyCode::KEY_S] = false;
	}

	if ( InputDevices::isKeyDown( KeyCode::KEY_A ) )
	{
		if ( keyDown_[KeyCode::KEY_A] )
		{
			//move left
			position -= right * movementSpeed;
		}
	}
	else
	{
		keyDown_[KeyCode::KEY_A] = false;
	}

	if ( InputDevices::isKeyDown( KeyCode::KEY_D ) )
	{
		if ( keyDown_[KeyCode::KEY_D] )
		{
			//move right
			position += right * movementSpeed;
		}
	}
	else
	{
		keyDown_[KeyCode::KEY_D] = false;
	}

	if ( InputDevices::isKeyDown( KeyCode::KEY_E ) )
	{
		if ( keyDown_[KeyCode::KEY_E] )
		{
			//move up
			position += forward * movementSpeed;
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
			position -= forward * movementSpeed;
		}
	}
	else
	{
		keyDown_[KeyCode::KEY_Q] = false;
	}


	// add in the mouse inputs
	static float mouseSpeedAdjust = 100.f;
	position += up * (up_ * movementSpeed * mouseSpeedAdjust);
	position += right * (right_ * movementSpeed * mouseSpeedAdjust);
	up_ = 0.f;
	right_ = 0.f;


	// Cap the camera position
	position.x = Math::clamp( limit().minBounds().x, position.x, limit().maxBounds().x );
	position.y = Math::clamp( limit().minBounds().y, position.y, limit().maxBounds().y );
	position.z = Math::clamp( limit().minBounds().z, position.z, limit().maxBounds().z );


	if ( InputDevices::hasFocus() )
		if ( InputDevices::isKeyDown( KeyCode::KEY_RIGHTMOUSE ) )
		{
			// Record the cursor position if we just started holding the button down
			if (lastCursorPosition_.x == -1 && lastCursorPosition_.y == -1)
				::GetCursorPos( &lastCursorPosition_ );

			// Keep mouse click pos
			::SetCursorPos( lastCursorPosition_.x, lastCursorPosition_.y );
		}
		else
		{
			// Restore the cursor position if we just released the button
			if (lastCursorPosition_.x != -1 || lastCursorPosition_.y != -1)
			{
				lastCursorPosition_.x = -1;
				lastCursorPosition_.y = -1;
			}
		}
	else
		if ( lastCursorPosition_.x != -1 || lastCursorPosition_.y != -1 )
		{
			lastCursorPosition_.x = -1;
			lastCursorPosition_.y = -1;
		}

	view_.invert();
}


/**
 *	This method calculates the pitch and yaw from the inverse view matrix.
 */
void OrthographicCamera::viewToPolar()
{
	BW_GUARD;

	Matrix invView( view_ );
    invView.invert();
	Vector3 * dir = (Vector3*)&invView.applyToUnitAxisVector(2);

//	pitch_ = -atan2f( dir->y,	sqrtf( dir->z * dir->z + dir->x * dir->x ) );
//	yaw_ = atan2f( dir->x, dir->z );
}


/**
 *	This method calculates the view matrix from our pitch and yaw,
 *	and the current camera position
 */
void OrthographicCamera::polarToView()
{
	//calculate the view matrix
//	view_.invert();
//
//	Vector3 pos( view_.applyToOrigin() );
//
//	view_.setIdentity();

//	Matrix rot;
//	XPMatrixRotationYawPitchRoll( &rot, yaw_, pitch_, 0.f );

//	view_.setTranslate( pos );
//	view_.preMultiply( rot );

//	view_.invert();
}


/**
 *	This method copies view_, pitch_ and yaw_ from another camera
 */
void OrthographicCamera::view( const OrthographicCamera& other )
{
	view_ = other.view_;
//	pitch_ = other.pitch_;
//	yaw_ = other.yaw_;
}


/**
 *	This function is the output stream operator for OrthographicCamera.
 */
std::ostream& operator<<(std::ostream& o, const OrthographicCamera& t)
{
	BW_GUARD;

	// TODO:UNICODE wchar_t?

	o << Localise(L"COMMON/ORTHOGRAPHIC_CAMERA/OUTPUT");
	return o;
}
BW_END_NAMESPACE

