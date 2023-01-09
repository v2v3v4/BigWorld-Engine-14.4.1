#include "WebScreenGameLogic.hpp"


// -----------------------------------------------------------------------------
// Section: ClientMethod implementations 
// -----------------------------------------------------------------------------// -----------------------------------------------------------------------------
// Section: Property setter callbacks
// -----------------------------------------------------------------------------

/**
 *	This method implements the setter callback for the property webScreenType.
 */
void WebScreenGameLogic::set_webScreenType( const BW::int8 & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_webScreenType\n" );
}

/**
 *	This method implements the setter callback for the property textureFeedName.
 */
void WebScreenGameLogic::set_textureFeedName( const BW::string & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_textureFeedName\n" );
}

/**
 *	This method implements the setter callback for the property triggerRadius.
 */
void WebScreenGameLogic::set_triggerRadius( const float & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_triggerRadius\n" );
}

/**
 *	This method implements the setter callback for the property feedSourcesInput.
 */
void WebScreenGameLogic::set_feedSourcesInput( const BW::string & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_feedSourcesInput\n" );
}

/**
 *	This method implements the setter callback for the property feedUpdateDelay.
 */
void WebScreenGameLogic::set_feedUpdateDelay( const BW::uint16 & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_feedUpdateDelay\n" );
}

/**
 *	This method implements the setter callback for the property ratePerSecond.
 */
void WebScreenGameLogic::set_ratePerSecond( const float & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_ratePerSecond\n" );
}

/**
 *	This method implements the setter callback for the property interactiveRatePerSecond.
 */
void WebScreenGameLogic::set_interactiveRatePerSecond( const BW::uint16 & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_interactiveRatePerSecond\n" );
}

/**
 *	This method implements the setter callback for the property dynamic.
 */
void WebScreenGameLogic::set_dynamic( const BW::uint8 & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_dynamic\n" );
}

/**
 *	This method implements the setter callback for the property interactive.
 */
void WebScreenGameLogic::set_interactive( const BW::uint8 & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_interactive\n" );
}

/**
 *	This method implements the setter callback for the property usedTint.
 */
void WebScreenGameLogic::set_usedTint( const BW::string & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_usedTint\n" );
}

/**
 *	This method implements the setter callback for the property scale.
 */
void WebScreenGameLogic::set_scale( const BW::Vector3 & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_scale\n" );
}

/**
 *	This method implements the property setter callback method for the
 *	property scale. 
 *	It is called when a single sub-property element has changed. The
 *	location of the element is described in the given change path.
 */
void WebScreenGameLogic::setNested_scale( const BW::NestedPropertyChange::Path & path, 
		const BW::Vector3 & oldValue )
{
	this->set_scale( oldValue );
}

/**
 *	This method implements the property setter callback method for the
 *	property scale. 
 *	It is called when a single sub-property slice has changed. The
 *	location of the ARRAY element is described in the given change path,
 *	and the slice within that element is described in the two given slice
 *	indices. 
 */
void WebScreenGameLogic::setSlice_scale( const BW::NestedPropertyChange::Path & path,
		int startIndex, int endIndex, 
		const BW::Vector3 & oldValue )
{
	this->set_scale( oldValue );
}


/**
 *	This method implements the setter callback for the property width.
 */
void WebScreenGameLogic::set_width( const BW::uint16 & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_width\n" );
}

/**
 *	This method implements the setter callback for the property height.
 */
void WebScreenGameLogic::set_height( const BW::uint16 & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_height\n" );
}

/**
 *	This method implements the setter callback for the property mipmap.
 */
void WebScreenGameLogic::set_mipmap( const BW::uint8 & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_mipmap\n" );
}

/**
 *	This method implements the setter callback for the property graphicsSettingsBehaviour.
 */
void WebScreenGameLogic::set_graphicsSettingsBehaviour( const BW::int8 & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_graphicsSettingsBehaviour\n" );
}

/**
 *	This method implements the setter callback for the property mozillaHandlesKeyboard.
 */
void WebScreenGameLogic::set_mozillaHandlesKeyboard( const BW::uint8 & oldValue )
{
	// DEBUG_MSG( "WebScreenGameLogic::set_mozillaHandlesKeyboard\n" );
}

// WebScreenGameLogic.mm
