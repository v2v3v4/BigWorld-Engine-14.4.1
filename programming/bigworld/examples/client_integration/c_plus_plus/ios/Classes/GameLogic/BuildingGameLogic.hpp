#ifndef BuildingGameLogic_HPP
#define BuildingGameLogic_HPP

#include "EntityExtensions/BuildingExtension.hpp"


/**
 *	This class implements the app-specific logic for the Building entity.
 */
class BuildingGameLogic :
		public BuildingExtension
{
public:
	BuildingGameLogic( const Building * pEntity ) :
			BuildingExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_modelName( const BW::string & oldValue );
};

#endif // BuildingGameLogic_HPP
