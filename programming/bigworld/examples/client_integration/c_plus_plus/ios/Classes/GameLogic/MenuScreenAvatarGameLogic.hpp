#ifndef MenuScreenAvatarGameLogic_HPP
#define MenuScreenAvatarGameLogic_HPP

#include "EntityExtensions/MenuScreenAvatarExtension.hpp"


/**
 *	This class implements the app-specific logic for the MenuScreenAvatar entity.
 */
class MenuScreenAvatarGameLogic :
		public MenuScreenAvatarExtension
{
public:
	MenuScreenAvatarGameLogic( const MenuScreenAvatar * pEntity ) :
			MenuScreenAvatarExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_avatarModel( const PersistentAvatarModel & oldValue );

	virtual void setNested_avatarModel( const BW::NestedPropertyChange::Path & path, 
			const PersistentAvatarModel & oldValue );

	virtual void setSlice_avatarModel( const BW::NestedPropertyChange::Path & path,
			int startIndex, int endIndex, 
			const PersistentAvatarModel & oldValue );

	virtual void set_realm( const BW::string & oldValue );
};

#endif // MenuScreenAvatarGameLogic_HPP
