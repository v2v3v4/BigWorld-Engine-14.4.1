#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "movement_controller.hpp"

#include "main_app.hpp"

DECLARE_DEBUG_COMPONENT2( "Bots", 0 );


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: MovementFactory
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
MovementFactory::MovementFactory( const char * name )
{
	MainApp::addFactory( name, *this );
}

BW_END_NAMESPACE

// movement_controller.cpp
