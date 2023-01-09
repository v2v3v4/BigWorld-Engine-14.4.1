#include "MenuScreenAvatarGameLogic.hpp"


// -----------------------------------------------------------------------------
// Section: ClientMethod implementations 
// -----------------------------------------------------------------------------// -----------------------------------------------------------------------------
// Section: Property setter callbacks
// -----------------------------------------------------------------------------

/**
 *	This method implements the setter callback for the property avatarModel.
 */
void MenuScreenAvatarGameLogic::set_avatarModel( const PersistentAvatarModel & oldValue )
{
	// DEBUG_MSG( "MenuScreenAvatarGameLogic::set_avatarModel\n" );
}

/**
 *	This method implements the property setter callback method for the
 *	property avatarModel. 
 *	It is called when a single sub-property element has changed. The
 *	location of the element is described in the given change path.
 */
void MenuScreenAvatarGameLogic::setNested_avatarModel( const BW::NestedPropertyChange::Path & path, 
		const PersistentAvatarModel & oldValue )
{
	this->set_avatarModel( oldValue );
}

/**
 *	This method implements the property setter callback method for the
 *	property avatarModel. 
 *	It is called when a single sub-property slice has changed. The
 *	location of the ARRAY element is described in the given change path,
 *	and the slice within that element is described in the two given slice
 *	indices. 
 */
void MenuScreenAvatarGameLogic::setSlice_avatarModel( const BW::NestedPropertyChange::Path & path,
		int startIndex, int endIndex, 
		const PersistentAvatarModel & oldValue )
{
	this->set_avatarModel( oldValue );
}


/**
 *	This method implements the setter callback for the property realm.
 */
void MenuScreenAvatarGameLogic::set_realm( const BW::string & oldValue )
{
	// DEBUG_MSG( "MenuScreenAvatarGameLogic::set_realm\n" );
}

// MenuScreenAvatarGameLogic.mm
