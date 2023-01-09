#ifndef FogGameLogic_HPP
#define FogGameLogic_HPP

#include "EntityExtensions/FogExtension.hpp"

/**
 *	This class implements the app-specific logic for the Fog entity.
 */
class FogGameLogic : public FogExtension
{
public:
	FogGameLogic( const Fog * pEntity ) :
			FogExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_density( const BW::int8 & oldValue );

	virtual void set_innerRadius( const float & oldValue );

	virtual void set_outerRadius( const float & oldValue );

	virtual void set_red( const BW::uint8 & oldValue );

	virtual void set_green( const BW::uint8 & oldValue );

	virtual void set_blue( const BW::uint8 & oldValue );
};

#endif // FogGameLogic_HPP
