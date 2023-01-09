#include "timer_controller.hpp"

#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "pyscript/pyobject_plus.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

/*~ function Entity.addTimer
 *  @components{ cell }
 *  The addTimer function starts a timer after the given time period (rounded to
 *	the nearest tick).
 *  The timer can be set to repeat, or execute once.  Each time an interval
 *  expires, the Entity is notified by the onTimer function, which can be
 *  defined as follows:
 *
 *	@{
 *		def onTimer( self, controllerID, userData ):
 *	@}
 *
 *	A timer can be cancelled with Entity.cancel( controllerID ).
 *
 * 	@see cancel
 *
 *  @param start Number of seconds until the first callback.
 *  @param interval=0.0 Duration in seconds between subsequent callbacks.
 *  Set to zero for single timed event.
 *  @param userData=0 userData is an optional integer to be passed to the onTimer function.
 *  @return The id of the newly created controller.
*/
IMPLEMENT_CONTROLLER_TYPE_WITH_PY_FACTORY( TimerController, DOMAIN_REAL )

Controller::FactoryFnRet TimerController::New(
	float initialOffset, float repeatOffset, int userArg )
{
	GameTime repeatTicks = 0;

	GameTime initialTicks =
		(GameTime)( initialOffset * CellAppConfig::updateHertz() + 0.5f );
	if (int(initialTicks) <= 0)
	{
		// WARNING_MSG( "TimerController::New: "
		// 	"Rounding up initial offset to 1 from %d (initialOffset %f)\n",
		// 	initialTicks, initialOffset );
		initialTicks = 1;
	}
	initialTicks += CellApp::instance().time();

	if (repeatOffset > 0.0f)
	{
		repeatTicks =
			(GameTime)(repeatOffset * CellAppConfig::updateHertz() + 0.5f);
		if (repeatTicks < 1)
		{
			WARNING_MSG( "TimerController::New: "
					"Rounding up repeatTicks to 1 (repeatOffset %f)\n",
				repeatOffset );
			repeatTicks = 1;
		}
	}

	return FactoryFnRet(
		new TimerController( initialTicks, repeatTicks ), userArg );
}


/**
 *	Construct the TimerController.
 *	is zero, there will be only a single callback, and the controller will
 *	destroy itself automatically after that callback.
 *
 *	@param start		Timestamp of the first callback
 *	@param interval		Duration in game ticks between subsequent callbacks
 */
TimerController::TimerController( GameTime start, GameTime interval ) :
	pHandler_( NULL ),
	start_( start ),
	interval_( interval ),
	timerHandle_()
{
}


/**
 *	Start the timer.
 */
void TimerController::startReal( bool isInitialStart )
{
	// Make sure it is not already running
	MF_ASSERT( !timerHandle_.isSet() );

	pHandler_ = new Handler( this );

	// if we were offloaded just as our timer was going off - we set the timer
	// start time to be now
	if (!isInitialStart)
	{
		if (start_ < CellApp::instance().time())
		{
			start_ = CellApp::instance().time();
		}
	}

	timerHandle_ = CellApp::instance().timeQueue().add( start_, interval_,
			pHandler_, NULL, "TimerController" );
}


/**
 *	Stop the timer.
 */
void TimerController::stopReal( bool /*isFinalStop*/ )
{
	MF_ASSERT( timerHandle_.isSet() );

	if (pHandler_)
	{
		// Detach from the handler so that cancel does not call stop again.
		pHandler_->pController( NULL );
		pHandler_ = NULL;

		timerHandle_.cancel();
	}
}


/**
 *	Write out our current state.
 */
void TimerController::writeRealToStream( BinaryOStream & stream )
{
	this->Controller::writeRealToStream( stream );
	stream << start_ << interval_;
}


/**
 *	Read our state from the stream.
 */
bool TimerController::readRealFromStream( BinaryIStream & stream )
{
	this->Controller::readRealFromStream( stream );
	stream >> start_ >> interval_;
	return true;
}


/*~	callback Entity.onTimer
 *  @components{ cell }
 *	This method is called when a timer associated with this entity is triggered.
 *	A timer can be added with the Entity.addTimer method.
 *	@param timerHandle	The id of the timer.
 *	@param userData	The user data passed in to Entity.addTimer.
 */
/**
 *	Handle timer callbacks from TimeQueue and pass them on.
 */
void TimerController::handleTimeout()
{
	AUTO_SCOPED_PROFILE( "onTimer" );

	// Update our start time, so it is correct if we are streamed
	// across the network.
	start_ += interval_;

	// Keep ourselves alive until we have finished cleaning up,
	// with an extra reference count from a smart pointer.
	ControllerPtr pController = this;

	this->standardCallback( "onTimer" );
}


/**
 *	This method is called when the handler releases us
 */
void TimerController::onHandlerRelease()
{
	MF_ASSERT( pHandler_ );
	pHandler_ = NULL;
	this->cancel();
}


// -----------------------------------------------------------------------------
// Section: TimerController::Handler
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
TimerController::Handler::Handler( TimerController * pController ) :
	pController_( pController )
{
}


/**
 *	Handle timer callbacks from TimeQueue and pass them on.
 */
void TimerController::Handler::handleTimeout( TimerHandle, void* )
{
	if (pController_)
	{
		pController_->handleTimeout();
	}
	else
	{
		WARNING_MSG( "TimerController::Handler::handleTimeout: "
			"pController_ is NULL\n" );
	}
}


/**
 *	This method is called when this timer is no longer used.
 */
void TimerController::Handler::onRelease( TimerHandle /*handle*/,
		void * /*pUser*/ )
{
	if (pController_)
	{
		// This call deletes this object.
		pController_->onHandlerRelease();
	}
	delete this;
}

BW_END_NAMESPACE

// timer_controller.cpp
