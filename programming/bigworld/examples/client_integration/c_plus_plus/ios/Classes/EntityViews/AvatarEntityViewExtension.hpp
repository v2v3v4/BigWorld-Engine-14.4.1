#ifndef AVATAR_ENTITY_VIEW_EXTENSION_HPP
#define AVATAR_ENTITY_VIEW_EXTENSION_HPP

#include "EntityExtensions/AvatarExtension.hpp"
#include "GenericEntityViewExtension.hpp"

class AvatarEntityViewExtension :
		public GenericEntityViewExtension< AvatarExtension >
{
public:
	AvatarEntityViewExtension( const Avatar * pEntity ) :
			GenericEntityViewExtension( pEntity )
	{
	}
	
	virtual ~AvatarEntityViewExtension() {}
	
	// Overrides from EntityViewDataProvider
	
	virtual BW::string spriteName() const;
	virtual BW::string popoverString() const;
	
	virtual void onClick();
	
};


#endif // AVATAR_ENTITY_VIEW_EXTENSION_HPP