#ifndef GuardSpawnerGameLogic_HPP
#define GuardSpawnerGameLogic_HPP

#include "EntityExtensions/GuardSpawnerExtension.hpp"


/**
 *	This class implements the app-specific logic for the GuardSpawner entity.
 */
class GuardSpawnerGameLogic :
		public GuardSpawnerExtension
{
public:
	GuardSpawnerGameLogic( const GuardSpawner * pEntity ) :
			GuardSpawnerExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_numberOfGuards( const BW::uint32 & oldValue );

	virtual void set_spawning( const BW::uint8 & oldValue );

	virtual void set_nameProperty( const BW::string & oldValue );
};

#endif // GuardSpawnerGameLogic_HPP
