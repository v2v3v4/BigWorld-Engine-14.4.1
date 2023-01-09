#ifndef DROPPED_ITEM_ENTITY_VIEW_EXTENSION_HPP
#define DROPPED_ITEM_ENTITY_VIEW_EXTENSION_HPP

#include "EntityExtensions/DroppedItemExtension.hpp"
#include "GenericEntityViewExtension.hpp"


class DroppedItemEntityViewExtension :
		public GenericEntityViewExtension< DroppedItemExtension >
{
public:
	DroppedItemEntityViewExtension( const DroppedItem * pEntity ) :
			GenericEntityViewExtension( pEntity )
	{}

	virtual ~DroppedItemEntityViewExtension() {}
	
	// Override from EntityViewDataProvider
	virtual BW::string spriteName() const;
};


#endif // DROPPED_ITEM_ENTITY_VIEW_EXTENSION_HPP