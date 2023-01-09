#ifndef WeatherSystemGameLogic_HPP
#define WeatherSystemGameLogic_HPP

#include "EntityExtensions/WeatherSystemExtension.hpp"

/**
 *	This class implements the app-specific logic for the WeatherSystem entity.
 */
class WeatherSystemGameLogic :
		public WeatherSystemExtension
{
public:
	WeatherSystemGameLogic( const WeatherSystem * pEntity ) :
			WeatherSystemExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_name( const BW::string & oldValue );

	virtual void set_radius( const float & oldValue );
};

#endif // WeatherSystemGameLogic_HPP
