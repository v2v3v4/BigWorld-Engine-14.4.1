#ifndef PlaneGameLogic_HPP
#define PlaneGameLogic_HPP

#include "EntityExtensions/PlaneExtension.hpp"

/**
 *	This class implements the app-specific logic for the Plane entity.
 */
class PlaneGameLogic : public PlaneExtension
{
public:
	PlaneGameLogic( const Plane * pEntity ) : PlaneExtension( pEntity ) {}

	// Client methods

	virtual void mountVehicle(
			const uint8 & succeeded,
			const int32 & pilotAvatarID );

	virtual void dismountVehicle(
			const int32 & pilotAvatarID );



	// Client property callbacks (optional)


	virtual void set_pilotID( const int32 & oldValue );
};

#endif // PlaneGameLogic_HPP
