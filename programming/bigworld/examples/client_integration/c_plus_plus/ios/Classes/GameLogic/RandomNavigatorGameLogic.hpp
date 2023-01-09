#ifndef RandomNavigatorGameLogic_HPP
#define RandomNavigatorGameLogic_HPP

#include "EntityExtensions/RandomNavigatorExtension.hpp"


/**
 *	This class implements the app-specific logic for the RandomNavigator entity.
 */
class RandomNavigatorGameLogic :
		public RandomNavigatorExtension
{
public:
	RandomNavigatorGameLogic( const RandomNavigator * pEntity ) :
			RandomNavigatorExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)

};

#endif // RandomNavigatorGameLogic_HPP
