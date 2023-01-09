#ifndef RipperGameLogic_HPP
#define RipperGameLogic_HPP

#include "EntityExtensions/RipperExtension.hpp"


/**
 *	This class implements the app-specific logic for the Ripper entity.
 */
class RipperGameLogic : public RipperExtension
{
public:
	RipperGameLogic( const Ripper * pEntity ) :
			RipperExtension( pEntity )
	{}

	// Client methods

	virtual void mountVehicle(
			const BW::uint8 & succeeded,
			const BW::int32 & pilotAvatarID );

	virtual void dismountVehicle(
			const BW::int32 & pilotAvatarID );



	// Client property callbacks (optional)


	virtual void set_pilotID( const BW::int32 & oldValue );
};

#endif // RipperGameLogic_HPP
