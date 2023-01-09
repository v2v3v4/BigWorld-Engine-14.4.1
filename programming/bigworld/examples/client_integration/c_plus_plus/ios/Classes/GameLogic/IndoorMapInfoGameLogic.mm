#include "IndoorMapInfoGameLogic.hpp"


// -----------------------------------------------------------------------------
// Section: ClientMethod implementations 
// -----------------------------------------------------------------------------// -----------------------------------------------------------------------------
// Section: Property setter callbacks
// -----------------------------------------------------------------------------

/**
 *	This method implements the setter callback for the property radius.
 */
void IndoorMapInfoGameLogic::set_radius( const float & oldValue )
{
	// DEBUG_MSG( "IndoorMapInfoGameLogic::set_radius\n" );
}

/**
 *	This method implements the setter callback for the property mapName.
 */
void IndoorMapInfoGameLogic::set_mapName( const BW::string & oldValue )
{
	// DEBUG_MSG( "IndoorMapInfoGameLogic::set_mapName\n" );
}

/**
 *	This method implements the setter callback for the property worldMapWidth.
 */
void IndoorMapInfoGameLogic::set_worldMapWidth( const float & oldValue )
{
	// DEBUG_MSG( "IndoorMapInfoGameLogic::set_worldMapWidth\n" );
}

/**
 *	This method implements the setter callback for the property worldMapHeight.
 */
void IndoorMapInfoGameLogic::set_worldMapHeight( const float & oldValue )
{
	// DEBUG_MSG( "IndoorMapInfoGameLogic::set_worldMapHeight\n" );
}

/**
 *	This method implements the setter callback for the property worldMapAnchor.
 */
void IndoorMapInfoGameLogic::set_worldMapAnchor( const BW::Vector2 & oldValue )
{
	// DEBUG_MSG( "IndoorMapInfoGameLogic::set_worldMapAnchor\n" );
}

/**
 *	This method implements the property setter callback method for the
 *	property worldMapAnchor. 
 *	It is called when a single sub-property element has changed. The
 *	location of the element is described in the given change path.
 */
void IndoorMapInfoGameLogic::setNested_worldMapAnchor( const BW::NestedPropertyChange::Path & path, 
		const BW::Vector2 & oldValue )
{
	this->set_worldMapAnchor( oldValue );
}

/**
 *	This method implements the property setter callback method for the
 *	property worldMapAnchor. 
 *	It is called when a single sub-property slice has changed. The
 *	location of the ARRAY element is described in the given change path,
 *	and the slice within that element is described in the two given slice
 *	indices. 
 */
void IndoorMapInfoGameLogic::setSlice_worldMapAnchor( const BW::NestedPropertyChange::Path & path,
		int startIndex, int endIndex, 
		const BW::Vector2 & oldValue )
{
	this->set_worldMapAnchor( oldValue );
}


/**
 *	This method implements the setter callback for the property minimapRange.
 */
void IndoorMapInfoGameLogic::set_minimapRange( const float & oldValue )
{
	// DEBUG_MSG( "IndoorMapInfoGameLogic::set_minimapRange\n" );
}

/**
 *	This method implements the setter callback for the property minimapRotate.
 */
void IndoorMapInfoGameLogic::set_minimapRotate( const BW::uint8 & oldValue )
{
	// DEBUG_MSG( "IndoorMapInfoGameLogic::set_minimapRotate\n" );
}

// IndoorMapInfoGameLogic.mm
