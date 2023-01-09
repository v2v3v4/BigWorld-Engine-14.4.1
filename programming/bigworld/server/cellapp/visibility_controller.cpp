#include "script/first_include.hpp"

#include "visibility_controller.hpp"

#include "entity_vision.hpp"


DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: VisibilityController
// -----------------------------------------------------------------------------

/*~ function Entity addVisibility
 *  @components{ cell }
 *  The addVisibility function creates a controller that handles whether an Entity
 *  can be seen by any active vision controllers.
 *  These vision controllers make use of the visibleHeight provided by this controller.
 *  This should be updated whenever the visible height changes (eg, Entity crouches or jumps).
 *
 *  @see addVision
 *  @see addScanVision
 *  @see cancel
 *
 *  @param visibleHeight visibleHeight is a float which gives the current height of the model.
 *  @param userData userData is not used in this controller.
 *  @return The id of the created controller.
*/
IMPLEMENT_CONTROLLER_TYPE_WITH_PY_FACTORY( VisibilityController, DOMAIN_GHOST )


/**
 *	Constructor.
 */
VisibilityController::VisibilityController( float visibleHeight ) :
	visibleHeight_( visibleHeight )
{
}


/**
 *	Destructor.
 */
VisibilityController::~VisibilityController()
{
}


/**
 *	Stream on ghosted data
 */
void VisibilityController::writeGhostToStream( BinaryOStream & stream )
{
	this->Controller::writeGhostToStream( stream );

	stream << visibleHeight_;
}


/**
 *	Stream off ghosted data
 */
bool VisibilityController::readGhostFromStream( BinaryIStream & stream )
{
	this->Controller::readGhostFromStream( stream );

	stream >> visibleHeight_;
	// do we need to do anything when this changes and we are running?

	return true;
}


/**
 *	Ghost startup method
 */
void VisibilityController::startGhost()
{
	// add ourselves to the EntityVision EntityExtra
	EntityVision::instance( this->entity() ).setVisibility( this );
}


/**
 *	Ghost shut down method
 */
void VisibilityController::stopGhost()
{
	// remove ourselves from the EntityVision EntityExtra
	EntityVision::instance( this->entity() ).setVisibility( NULL );
}


/**
 *	Python factory method
 */
Controller::FactoryFnRet VisibilityController::New(
	float visibleHeight, int userArg )
{
	// Controller has already checked that entity is real.

	if (EntityVision::instance( *s_factoryFnEntity_ ).getVisibility() != NULL)
	{
		PyErr_SetString( PyExc_TypeError, "Entity.addVisibilityController: "
			"Entity already has a visibility controller" );
		return FactoryFnRet();
	}

	return FactoryFnRet( new VisibilityController( visibleHeight ), userArg );
}


/**
 *	Set the visible height
 */
void VisibilityController::visibleHeight( float h )
{
	MF_ASSERT( entity().isReal() );

	visibleHeight_ = h;
	// do we need to do anything when this changes and we are running?

	this->ghost();
}

BW_END_NAMESPACE

// visibility_controller.cpp
