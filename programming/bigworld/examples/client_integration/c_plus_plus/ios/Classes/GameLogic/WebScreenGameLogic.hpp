#ifndef WebScreenGameLogic_HPP
#define WebScreenGameLogic_HPP

#include "EntityExtensions/WebScreenExtension.hpp"


/**
 *	This class implements the app-specific logic for the WebScreen entity.
 */
class WebScreenGameLogic :
		public WebScreenExtension
{
public:
	WebScreenGameLogic( const WebScreen * pEntity ) :
			WebScreenExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_webScreenType( const BW::int8 & oldValue );

	virtual void set_textureFeedName( const BW::string & oldValue );

	virtual void set_triggerRadius( const float & oldValue );

	virtual void set_feedSourcesInput( const BW::string & oldValue );

	virtual void set_feedUpdateDelay( const BW::uint16 & oldValue );

	virtual void set_ratePerSecond( const float & oldValue );

	virtual void set_interactiveRatePerSecond( const BW::uint16 & oldValue );

	virtual void set_dynamic( const BW::uint8 & oldValue );

	virtual void set_interactive( const BW::uint8 & oldValue );

	virtual void set_usedTint( const BW::string & oldValue );

	virtual void set_scale( const BW::Vector3 & oldValue );

	virtual void setNested_scale( const BW::NestedPropertyChange::Path & path, 
			const BW::Vector3 & oldValue );

	virtual void setSlice_scale( const BW::NestedPropertyChange::Path & path,
			int startIndex, int endIndex, 
			const BW::Vector3 & oldValue );

	virtual void set_width( const BW::uint16 & oldValue );

	virtual void set_height( const BW::uint16 & oldValue );

	virtual void set_mipmap( const BW::uint8 & oldValue );

	virtual void set_graphicsSettingsBehaviour( const BW::int8 & oldValue );

	virtual void set_mozillaHandlesKeyboard( const BW::uint8 & oldValue );
};

#endif // WebScreenGameLogic_HPP
