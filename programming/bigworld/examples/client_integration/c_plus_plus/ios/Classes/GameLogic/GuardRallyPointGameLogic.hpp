#ifndef GuardRallyPointGameLogic_HPP
#define GuardRallyPointGameLogic_HPP

#include "EntityExtensions/GuardRallyPointExtension.hpp"


/**
 *	This class implements the app-specific logic for the GuardRallyPoint entity.
 */
class GuardRallyPointGameLogic :
		public GuardRallyPointExtension
{
public:
	GuardRallyPointGameLogic( const GuardRallyPoint * pEntity ) :
			GuardRallyPointExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_modelName( const BW::string & oldValue );
};

#endif // GuardRallyPointGameLogic_HPP
