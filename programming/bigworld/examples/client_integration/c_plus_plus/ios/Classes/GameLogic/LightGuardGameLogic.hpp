#ifndef LightGuardGameLogic_HPP
#define LightGuardGameLogic_HPP

#include "EntityExtensions/LightGuardExtension.hpp"


/**
 *	This class implements the app-specific logic for the LightGuard entity.
 */
class LightGuardGameLogic :
		public LightGuardExtension
{
public:
	LightGuardGameLogic( const LightGuard * pEntity ) :
			LightGuardExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_modelNumber( const BW::uint16 & oldValue );
};

#endif // LightGuardGameLogic_HPP
