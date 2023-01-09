#ifndef PointOfInterestGameLogic_HPP
#define PointOfInterestGameLogic_HPP

#include "EntityExtensions/PointOfInterestExtension.hpp"


/**
 *	This class implements the app-specific logic for the PointOfInterest entity.
 */
class PointOfInterestGameLogic :
		public PointOfInterestExtension
{
public:
	PointOfInterestGameLogic( const PointOfInterest * pEntity ) :
			PointOfInterestExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)

};

#endif // PointOfInterestGameLogic_HPP
