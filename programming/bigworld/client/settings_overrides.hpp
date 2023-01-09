#ifndef SETTINGS_OVERRIDES_HPP
#define SETTINGS_OVERRIDES_HPP

BW_BEGIN_NAMESPACE
	
// support for command line overriding of settings

struct ScreenSettingsOverrides
{
	// screen resolution
	uint32 resolutionX_;
	uint32 resolutionY_; 
	bool resolutionForced_;

	// screen mode
	bool fullscreen_;
	bool screenModeForced_;

	ScreenSettingsOverrides()
		: resolutionForced_( false )
		, screenModeForced_( false )
	{}
};


// helper function for reading screen override params from command line
ScreenSettingsOverrides readCommandLineScreenSettingsOverrides();

BW_END_NAMESPACE

#endif // SETTINGS_OVERRIDES_HPP
