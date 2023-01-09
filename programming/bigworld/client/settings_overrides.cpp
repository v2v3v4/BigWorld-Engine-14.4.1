#include "pch.hpp"

#include "settings_overrides.hpp"
#include "cstdmf/command_line.hpp"

BW_BEGIN_NAMESPACE

namespace {

	// helper function for parsing uint32 values from string (command line)
	uint32 atouint32( const char * s, uint32 failedResult )
	{
		char * endChar;
		const int decimalRadix = 10;
		long parsedVal = strtoul( s, &endChar, decimalRadix );
		if (endChar != s && (parsedVal != LONG_MAX || parsedVal != LONG_MIN))
		{
			return static_cast<uint32>(parsedVal);
		}
		return failedResult;
	}

}	// namespace anonymous


ScreenSettingsOverrides readCommandLineScreenSettingsOverrides()
{
	ScreenSettingsOverrides sso;

	// determine if user requested to force screen resolution/mode
	LPSTR commandLine = ::GetCommandLineA();
	CommandLine cmd( commandLine );

	const char * CMD_PARAM_FAILURE_PARSING_MSG = 
		"DeviceApp: Parsing command line value for parameter '-%s' failed\n";

	// screen resolution --------------------------------------
	const char * CMD_PARAM_SCREEN_RES_X = "screen-resolution-x";
	const char * CMD_PARAM_SCREEN_RES_Y = "screen-resolution-y";

	bool setX = false;
	bool setY = false;

	if (cmd.hasParam( CMD_PARAM_SCREEN_RES_X ))
	{
		// get screen x resolution override
		sso.resolutionX_ = atouint32( cmd.getParam( CMD_PARAM_SCREEN_RES_X ), 0 );
		if (sso.resolutionX_)
		{
			setX = true;
		}
		else
		{
			ERROR_MSG( CMD_PARAM_FAILURE_PARSING_MSG, CMD_PARAM_SCREEN_RES_X );
		}
	}

	if (cmd.hasParam( CMD_PARAM_SCREEN_RES_Y ))
	{
		// get screen y resolution override
		sso.resolutionY_ = atouint32( cmd.getParam( CMD_PARAM_SCREEN_RES_Y ), 0 );
		if (sso.resolutionY_)
		{
			setY = true;
		}
		else
		{
			ERROR_MSG( CMD_PARAM_FAILURE_PARSING_MSG, CMD_PARAM_SCREEN_RES_Y );
		}
	}

	if (setX && setY)
	{
		// both x and y are set
		sso.resolutionForced_ = true;
	}
	else if (setX || setY)
	{
		// only one was set
		ERROR_MSG( 
			"DeviceApp: parsing command line arguments failed. "
			"Must supply valid '%s' and '%s' if either one is specified.\n",
			CMD_PARAM_SCREEN_RES_X, CMD_PARAM_SCREEN_RES_Y );
	}


	// window mode --------------------------------------
	const char * CMD_PARAM_FULLSCREEN = "fullscreen";
	if (cmd.hasParam( CMD_PARAM_FULLSCREEN ))
	{
		const uint32 CMD_FULLSCREEN_BAD_PARAM = uint32(-1);
		uint32 forcedFullscreen = atouint32( 
			cmd.getParam( CMD_PARAM_FULLSCREEN ), CMD_FULLSCREEN_BAD_PARAM );
		if (forcedFullscreen == 0 || forcedFullscreen == 1)
		{
			sso.fullscreen_ = (forcedFullscreen != 0);
			sso.screenModeForced_ = true;
		}
		else
		{
			ERROR_MSG( CMD_PARAM_FAILURE_PARSING_MSG, CMD_PARAM_FULLSCREEN );
			ERROR_MSG( "DeviceApp: Value should should be 0 or 1 for param '-%s'\n", 
				CMD_PARAM_FULLSCREEN );
		}
	}

	return sso;
}


BW_END_NAMESPACE
