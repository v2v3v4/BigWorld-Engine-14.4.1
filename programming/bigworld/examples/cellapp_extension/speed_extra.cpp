#include "script/first_include.hpp"

#include "speed_extra.hpp"
#include "speed_controller.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( SpeedExtra )

PY_BEGIN_METHODS( SpeedExtra )
	PY_METHOD( trackSpeed )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( SpeedExtra )
	PY_ATTRIBUTE( mySpeed )
	PY_ATTRIBUTE( myVelocity )
PY_END_ATTRIBUTES()

const SpeedExtra::Instance< SpeedExtra >
		SpeedExtra::instance( SpeedExtra::s_getMethodDefs(),
			SpeedExtra::s_getAttributeDefs() );

// -----------------------------------------------------------------------------
// Section: SpeedExtra
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
SpeedExtra::SpeedExtra( Entity& e ) :
	EntityExtra( e ),
	speed_( 0.f ),
	velocity_( Vector3::zero() ),
	lastTime_( 0 ),
	pController_( NULL )
{
	// DEBUG_MSG( "SpeedExtra::SpeedExtra:\n" );
}


/**
 *	Destructor.
 */
SpeedExtra::~SpeedExtra()
{
	// DEBUG_MSG( "SpeedExtra::~SpeedExtra:\n" );
}


/**
 *	This method is exposed to Python and allows enabling and disabling speed.
 */
bool SpeedExtra::trackSpeed( bool enable )
{
	// DEBUG_MSG( "SpeedExtra::trackSpeed: %d\n", enable );

	if (!entity_.isReal())
	{
		PyErr_SetString( PyExc_TypeError,
				"Entity.trackSpeed can only be called on a real entity" );
		return false;
	}

	if (enable == this->isEnabled())
	{
		// Could consider raising a Python exception here.
		WARNING_MSG( "SpeedExtra::trackSpeed: "
					"Tracking for %d already %senabled\n",
				entity_.id(),
				enable ? "" : "dis" );
		return true;
	}

	if (enable)
	{
		MF_ASSERT( pController_ == NULL );

		entity_.addController( new SpeedController, 0 );
	}
	else
	{
		MF_ASSERT( pController_ != NULL );

		pController_->stop();
		MF_ASSERT( pController_ == NULL );
	}

	return true;
}


/**
 *	This method sets the controller associated with the extra. This allows
 *	trackSpeed( False ) to be able to cancel the controller.
 */
void SpeedExtra::setController( SpeedController * pController )
{
	MF_ASSERT( (pController_ == NULL) || (pController == NULL) );

	pController_ = pController;
}


/**
 *	This method write this object's state to the input stream.
 */
void SpeedExtra::writeToStream( BinaryOStream & stream )
{
	uint64 timeOffset = 0;

	if (lastTime_ != 0)
	{
		// Need to consider timestamp being different on different machines.
		timeOffset = timestamp() - lastTime_;
	}

	stream << speed_ << velocity_ << timeOffset;
}


/**
 *	This method reads this object's state from the input stream.
 */
void SpeedExtra::readFromStream( BinaryIStream & stream )
{
	uint64 timeOffset = 0;

	stream >> speed_ >> velocity_ >> timeOffset;

	if (timeOffset != 0)
	{
		lastTime_ = timestamp() + timeOffset;
	}
}


/**
 *	This method is called whenever an entity that is being track moves.
 */
void SpeedExtra::onMovement( const Vector3 & oldPosition )
{
	// DEBUG_MSG( "SpeedExtra::onMovement:\n" );
	uint64 thisTime = timestamp();

	if ((lastTime_ != 0) && (thisTime > lastTime_))
	{
		double delta = (thisTime - lastTime_) / stampsPerSecondD();

		// TODO: Do a proper calculation that filters the value etc.
		// Should also consider whether it is better to use clock time or
		// game time. Get game time with: CellApp::instance().time(). This is
		// in game ticks.

		// No need for both velocity_ and speed_ but this is just as an
		// example.
		velocity_ = (entity_.position() - oldPosition)/delta;
		speed_ = velocity_.length()/delta;
	}

	lastTime_ = thisTime;
}


// -----------------------------------------------------------------------------
// Section: Callback registration
// -----------------------------------------------------------------------------

namespace
{

/**
 *	This function will be called whenever an entity's position is updated.
 */
void movementCallback( const Vector3 & oldPosition,
		Entity * pEntity )
{
	// If it has a SpeedExtra, inform it of the new position.
	if (SpeedExtra::instance.exists( *pEntity ))
	{
		SpeedExtra::instance( *pEntity ).onMovement( oldPosition );
	}
}

/**
 *	This class is used to set g_entityMovementCallback.
 */
class StaticIniter
{
public:
	StaticIniter()
	{
		// DEBUG_MSG( "StaticIniter::StaticIniter:\n" );
		g_entityMovementCallback = &movementCallback;
	}
};

StaticIniter g_initer;

}

BW_END_NAMESPACE

// speed_extra.cpp
