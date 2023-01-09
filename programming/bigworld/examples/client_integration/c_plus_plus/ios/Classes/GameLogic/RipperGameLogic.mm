#include "RipperGameLogic.hpp"


// -----------------------------------------------------------------------------
// Section: ClientMethod implementations 
// -----------------------------------------------------------------------------

/**
 *	This method implements the ClientMethod mountVehicle.
 */
void RipperGameLogic::mountVehicle( 
			const BW::uint8 & succeeded,
			const BW::int32 & pilotAvatarID )
{
	// DEBUG_MSG( "RipperGameLogic::set_mountVehicle\n" );
}



/**
 *	This method implements the ClientMethod dismountVehicle.
 */
void RipperGameLogic::dismountVehicle( 
			const BW::int32 & pilotAvatarID )
{
	// DEBUG_MSG( "RipperGameLogic::set_dismountVehicle\n" );
}

// -----------------------------------------------------------------------------
// Section: Property setter callbacks
// -----------------------------------------------------------------------------

/**
 *	This method implements the setter callback for the property pilotID.
 */
void RipperGameLogic::set_pilotID( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "RipperGameLogic::set_pilotID\n" );
}

// RipperGameLogic.mm
