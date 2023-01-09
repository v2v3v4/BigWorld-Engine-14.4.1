#ifndef TeleportSourceGameLogic_HPP
#define TeleportSourceGameLogic_HPP

#include "EntityExtensions/TeleportSourceExtension.hpp"


/**
 *	This class implements the app-specific logic for the TeleportSource entity.
 */
class TeleportSourceGameLogic :
		public TeleportSourceExtension
{
public:
	TeleportSourceGameLogic( const TeleportSource * pEntity ) :
			TeleportSourceExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_modelType( const BW::uint8 & oldValue );

	virtual void set_spaceLabel( const BW::string & oldValue );

	virtual void set_destination( const BW::string & oldValue );
};

#endif // TeleportSourceGameLogic_HPP
