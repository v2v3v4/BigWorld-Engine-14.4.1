#ifndef MovingPlatformGameLogic_HPP
#define MovingPlatformGameLogic_HPP

#include "EntityExtensions/MovingPlatformExtension.hpp"


/**
 *	This class implements the app-specific logic for the MovingPlatform entity.
 */
class MovingPlatformGameLogic :
		public MovingPlatformExtension
{
public:
	MovingPlatformGameLogic( const MovingPlatform * pEntity ) :
			MovingPlatformExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_platformType( const BW::int8 & oldValue );

	virtual void set_platformName( const BW::string & oldValue );
};

#endif // MovingPlatformGameLogic_HPP
