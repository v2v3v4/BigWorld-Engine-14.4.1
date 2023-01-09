#ifndef CreatureGameLogic_HPP
#define CreatureGameLogic_HPP

#include "EntityExtensions/CreatureExtension.hpp"


/**
 *	This class implements the app-specific logic for the Creature entity.
 */
class CreatureGameLogic :
		public CreatureExtension
{
public:
	CreatureGameLogic( const Creature * pEntity ) :
			CreatureExtension( pEntity )
	{}

	// Client methods

	virtual void performAction(
			const BW::int8 & actionID );



	// Client property callbacks (optional)


	virtual void set_creatureName( const BW::string & oldValue );

	virtual void set_creatureType( const BW::int8 & oldValue );

	virtual void set_creatureMonthOfBirth( const BW::int8 & oldValue );

	virtual void set_creatureRecoverRate( const float & oldValue );

	virtual void set_creatureState( const BW::int8 & oldValue );

	virtual void set_lookAtTarget( const BW::int32 & oldValue );

	virtual void set_healthPercent( const BW::int8 & oldValue );

	virtual void set_ownerId( const BW::int32 & oldValue );

	virtual void set_moving( const BW::int8 & oldValue );
};

#endif // CreatureGameLogic_HPP
