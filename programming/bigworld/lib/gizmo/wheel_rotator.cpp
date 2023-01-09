#include "pch.hpp"
#include "wheel_rotator.hpp"
#include "current_general_properties.hpp"
#include "resmgr/string_provider.hpp"
#include "snap_provider.hpp"
#include "tool_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: WheelRotator
// -----------------------------------------------------------------------------


PY_TYPEOBJECT( WheelRotator )

PY_BEGIN_METHODS( WheelRotator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( WheelRotator )
PY_END_ATTRIBUTES()

PY_FACTORY( WheelRotator, Functor )

WheelRotator::WheelRotator( bool allowedToDiscardChanges,
	PyTypeObject * pType ) :

	AlwaysApplyingFunctor( allowedToDiscardChanges, pType ),
	timeSinceInput_( 0.0f ),
	rotAmount_( 0.0f ),
	needsInit_( true )
{
	BW_GUARD;

	curProperties_ = CurrentRotationProperties::properties();
	undoName_ = LocaliseUTF8( L"GIZMO/UNDO/WHEEL_ROTATOR" );
}


bool WheelRotator::arePropertiesValid() const
{
	BW_GUARD;

	return curProperties_ == CurrentRotationProperties::properties();
}


void WheelRotator::update( float dTime, Tool& tool )
{
	BW_GUARD;

	if (this->checkStopApplying())
	{
		this->stopApplying( tool, true );
		return;
	}

	this->doApply( dTime, tool );
}


bool WheelRotator::checkStopApplying() const
{
	BW_GUARD;
	// Stop if properties are not valid
	return (!this->arePropertiesValid());
}


void WheelRotator::doApply( float dTime, Tool & tool )
{
	BW_GUARD;

	if (needsInit_)
	{
		needsInit_ = false;

		// Get the initial picked position on the tool
		Matrix grabOffset = tool.locator()->transform();
		rotaterHelper_.init( grabOffset );
	}

	// The wheelrotator always rotates around the y-axis
	Vector3 axis( 0, 1, 0 );

	float angle = DEG_TO_RAD( rotAmount_ );

	// Snap the angle
	float snapAmount = DEG_TO_RAD( SnapProvider::instance()->angleSnapAmount() );

	if (snapAmount != 0.0f)
	{
		float snapAngle = int( angle / snapAmount ) * snapAmount;
		if( angle - snapAngle >= snapAmount / 2 )
		{
			angle = snapAngle + snapAmount;
		}
		else
		{
			angle = snapAngle;
		}
	}

	// Update the rotation
	rotaterHelper_.updateRotation( angle, axis );

	// Automatically commit after 750ms of no input
	if (timeSinceInput_ > 0.75f)
	{
		this->stopApplying( tool, true );
	}
	
	// update the time since input value
	timeSinceInput_ += dTime;
}


bool WheelRotator::handleMouseEvent( const MouseEvent & event, Tool& tool )
{
	BW_GUARD;

	if (!this->arePropertiesValid())
	{
		return false;
	}


	if (event.dz() != 0)
	{
		timeSinceInput_ = 0.0f;

		// Get the direction only, we don't want the magnitude
		float amnt = (event.dz() > 0) ? -1.0f : 1.0f;

		// Move at 1deg/click with the button down, 15 degs/click otherwise
		if (!InputDevices::instance().isKeyDown( KeyCode::KEY_MIDDLEMOUSE ))
			amnt *= 15.0f;

		rotAmount_ += amnt;

		return true;
	}
	else
	{
		// commit the rotation now, so we don't have to wait for the timeout
		this->stopApplying( tool, true );
	}

	return false;
}

bool WheelRotator::handleKeyEvent( const KeyEvent & event, Tool& tool )
{
	BW_GUARD;

	if (!this->arePropertiesValid())
	{
		return false;
	}

	if (!event.isKeyDown())
	{
		return false;
	}

	if (event.key() == KeyCode::KEY_LEFTMOUSE ||
		event.key() == KeyCode::KEY_RIGHTMOUSE)
	{
		this->stopApplying( tool, true );
	}

	return false;
}

/**
 *	This helper method commits the changes that have been done 
 *	to the rotation and sets up the undo barrier
 */
void WheelRotator::stopApplyCommitChanges( Tool& tool, bool addUndoBarrier )
{
	BW_GUARD;

	if (applying_)
	{
		rotaterHelper_.fini(true);

		needsInit_ = true;
		rotAmount_ = 0;

		AlwaysApplyingFunctor::stopApplyCommitChanges( tool, addUndoBarrier );
	}
}


void WheelRotator::stopApplyDiscardChanges( Tool& tool )
{
	BW_GUARD;

	if (applying_)
	{
		rotaterHelper_.fini( false );

		AlwaysApplyingFunctor::stopApplyDiscardChanges( tool );
	}
}


PyObject * WheelRotator::pyNew( PyObject * args )
{
	BW_GUARD;

	return new WheelRotator();
}

BW_END_NAMESPACE
// wheel_rotator.cpp
