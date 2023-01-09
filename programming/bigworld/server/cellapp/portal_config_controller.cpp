#include "script/first_include.hpp"

#include "portal_config_controller.hpp"
#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "entity.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_space.hpp"
#include "pyscript/pyobject_plus.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

IMPLEMENT_EXCLUSIVE_CONTROLLER_TYPE( PortalConfigController,
		DOMAIN_GHOST, "Portal" )


// -----------------------------------------------------------------------------
// Section: PortalConfigController
// -----------------------------------------------------------------------------

/**
 *	Construct the PortalConfigController.
 *	We zero all our members here because we expect whoever created us to
 *	either set them directly (above) or get us to read them from a stream
 *	(below).
 */
PortalConfigController::PortalConfigController( bool permissive,
		WorldTriangle::Flags collisionFlags ) :
	pSpace_( NULL ),
	point_( 0.f, 0.f, 0.f ),
	permissive_( permissive ),
	collisionFlags_( collisionFlags ),
	started_( false ),
	timerHandle_()
{
}


/**
 *	Start affecting the world.
 */
void PortalConfigController::startGhost()
{
	point_ = this->entity().position();

	this->apply();

	started_ = true;
}


/**
 *	Stop affecting the world.
 */
void PortalConfigController::stopGhost()
{
	started_ = false;

	if (timerHandle_.isSet())
	{
		// Give up attempting to apply
		timerHandle_.cancel();
	}
	else
	{
		// Revert the apply
		this->attemptToApply( true, 0 );
	}
}


/**
 *	Write out our current state.
 */
void PortalConfigController::writeGhostToStream( BinaryOStream & stream )
{
	this->Controller::writeGhostToStream( stream );
	stream << permissive_ << collisionFlags_;
}


/**
 *	Read our state from the stream.
 */
bool PortalConfigController::readGhostFromStream( BinaryIStream & stream )
{
	this->Controller::readGhostFromStream( stream );
	stream >> permissive_ >> collisionFlags_;

	if (started_ && !timerHandle_.isSet())
	{
		this->apply();
	}

	return true;
}


/**
 *	Apply ourselves to the world
 */
bool PortalConfigController::attemptToApply( bool permissive,
		WorldTriangle::Flags collisionFlags )
{
	if (pSpace_ == NULL)
	{
		pSpace_ = this->entity().pChunkSpace();

		if (pSpace_ == NULL)
		{
			return false;
		}
	}

	return pSpace_->setClosestPortalState( point_,
			permissive, collisionFlags );
}


/**
 *	Our timer has expired; try to apply ourselves to the world again.
 */
void PortalConfigController::handleTimeout( TimerHandle handle, void * pUser )
{
	if (this->attemptToApply( permissive_, collisionFlags_ ))
	{
		timerHandle_.cancel();
	}
}


/**
 *	This method starts a timer that is used to keep retrying to apply the state
 *	associated with this controller.
 */
void PortalConfigController::apply()
{
	MF_ASSERT( !timerHandle_.isSet() );
	const int RETRY_PERIOD = 1;

	if (!this->attemptToApply( permissive_, collisionFlags_ ))
	{
		// and wait for the chunks to reload
		int ticks = CellAppConfig::updateHertz() * RETRY_PERIOD;

		CellApp & app = app.instance();
		timerHandle_ = app.timeQueue().add(
				app.time() + ticks, ticks, this, NULL, 
				"PortalConfigController" );
	}
}

BW_END_NAMESPACE

// portal_config_controller.cpp
