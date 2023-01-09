#ifndef DustDevilGameLogic_HPP
#define DustDevilGameLogic_HPP

#include "EntityExtensions/DustDevilExtension.hpp"


/**
 *	This class implements the app-specific logic for the DustDevil entity.
 */
class DustDevilGameLogic :
		public DustDevilExtension
{
public:
	DustDevilGameLogic( const DustDevil * pEntity ) :
			DustDevilExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)

};

#endif // DustDevilGameLogic_HPP
