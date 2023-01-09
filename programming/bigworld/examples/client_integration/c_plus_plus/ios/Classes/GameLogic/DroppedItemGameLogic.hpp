#ifndef DroppedItemGameLogic_HPP
#define DroppedItemGameLogic_HPP

#include "EntityExtensions/DroppedItemExtension.hpp"


/**
 *	This class implements the app-specific logic for the DroppedItem entity.
 */
class DroppedItemGameLogic :
		public DroppedItemExtension
{
public:
	DroppedItemGameLogic( const DroppedItem * pEntity ) :
			DroppedItemExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_classType( const BW::int32 & oldValue );

	virtual void set_itemSerial( const BW::int32 & oldValue );

	virtual void set_onGround( const BW::int8 & oldValue );

	virtual void set_dropperID( const BW::int32 & oldValue );
};

#endif // DroppedItemGameLogic_HPP
