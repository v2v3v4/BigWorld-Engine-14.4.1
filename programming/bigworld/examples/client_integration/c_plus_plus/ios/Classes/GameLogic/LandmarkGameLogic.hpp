#ifndef LandmarkGameLogic_HPP
#define LandmarkGameLogic_HPP

#include "EntityExtensions/LandmarkExtension.hpp"


/**
 *	This class implements the app-specific logic for the Landmark entity.
 */
class LandmarkGameLogic :
		public LandmarkExtension
{
public:
	LandmarkGameLogic( const Landmark * pEntity ) :
			LandmarkExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_initialTriggerDelay( const BW::int8 & oldValue );

	virtual void set_retriggerDelay( const BW::uint8 & oldValue );

	virtual void set_radius( const float & oldValue );

	virtual void set_description( const BW::string & oldValue );
};

#endif // LandmarkGameLogic_HPP
