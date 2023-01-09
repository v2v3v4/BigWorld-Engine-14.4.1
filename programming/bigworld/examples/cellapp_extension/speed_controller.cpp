#include "script/first_include.hpp"

#include "speed_controller.hpp"
#include "speed_extra.hpp"

#include "cellapp/cellapp.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

IMPLEMENT_CONTROLLER_TYPE( SpeedController,
		ControllerDomain( DOMAIN_GHOST | DOMAIN_REAL ) );

// -----------------------------------------------------------------------------
// Section: SpeedController
// -----------------------------------------------------------------------------

/**
 *	This method is an override from Controller. It is called when this
 *	controller is started on a real or ghost entity.
 */
void SpeedController::startGhost()
{
	// Create the SpeedExtra.
	SpeedExtra::instance( this->entity() );
}


/**
 *	This method is called when this controller is started on a real entity.
 */
void SpeedController::startReal( bool isInitialStart )
{
	SpeedExtra::instance( this->entity() ).setController( this );
}


/**
 *	This method is called when this controller is stopped on a real entity.
 */
void SpeedController::stopReal( bool isFinalStop )
{
	SpeedExtra::instance( this->entity() ).setController( NULL );
}


/**
 *	This method is called when this controller is stopped on a real or ghost
 *	entity.
 */
void SpeedController::stopGhost()
{
	// Destroy the SpeedExtra
	SpeedExtra::instance.clear( this->entity() );
}


/**
 *	This method streams the necessary state to create a ghost controller.
 */
void SpeedController::writeGhostToStream( BinaryOStream & stream )
{
	this->Controller::writeGhostToStream( stream );

	SpeedExtra::instance( this->entity() ).writeToStream( stream );
}


/**
 *	This method reads the ghost controller data from the input stream.
 */
bool SpeedController::readGhostFromStream( BinaryIStream & stream )
{
	this->Controller::readGhostFromStream( stream );

	SpeedExtra::instance( this->entity() ).readFromStream( stream );

	return true;
}

BW_END_NAMESPACE

// speed_controller.cpp
