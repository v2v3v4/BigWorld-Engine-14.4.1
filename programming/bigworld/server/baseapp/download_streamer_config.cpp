#include "script/first_include.hpp"

#include "download_streamer_config.hpp"

#include "baseapp_config.hpp"

#include "server/util.hpp"

#include "data_downloads.hpp"

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS DownloadStreamerConfig
#define BW_CONFIG_PREFIX "baseApp/"
#define BW_COMMON_PREFIX "downloadStreaming/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: DownloadStreamerConfig
// -----------------------------------------------------------------------------

BW_OPTION_RO( int, bitsPerSecondTotal, 10000000 );
BW_OPTION_RO( int, bitsPerSecondPerClient, 100000 );
BW_OPTION_RO( int, rampUpRate, 5000 );
BW_OPTION( int, backlogLimit, 5 );

DERIVED_BW_OPTION( int, bytesPerTickTotal );
DERIVED_BW_OPTION( int, bytesPerTickPerClient );
DERIVED_BW_OPTION( int, rampUpRateBytesPerTick );


/**
 *	This method is called after the configuration options have been read but
 *	before they have been printed.
 */
bool DownloadStreamerConfig::postInit()
{
	bytesPerTickTotal.set( ServerUtil::bpsToBytesPerTick( bitsPerSecondTotal(),
			BaseAppConfig::updateHertz() ) );

	bytesPerTickPerClient.set(
			ServerUtil::bpsToBytesPerTick( bitsPerSecondPerClient(),
			BaseAppConfig::updateHertz() ) );

	rampUpRateBytesPerTick.set(
		ServerUtil::bpsToBytesPerTick( rampUpRate(), BaseAppConfig::updateHertz() ) );

	if (bytesPerTickPerClient() <= DataDownload::packetOverhead())
	{
		ERROR_MSG( "DownloadStreamerConfig::postInit: Download streaming will "
			"not operate, as only %u bytes can be sent per tick per client, "
			"but download streaming has a %u bytes-per-packet overhead\n",
			bytesPerTickPerClient(), DataDownload::packetOverhead() );
		return false;
	}
	else if (bytesPerTickTotal() <= DataDownload::packetOverhead())
	{
		ERROR_MSG( "DownloadStreamerConfig::postInit: Download streaming will "
			"not operate, as only %u bytes can be sent per tick, "
			"but download streaming has a %u bytes-per-packet overhead\n",
			bytesPerTickTotal(), DataDownload::packetOverhead() );
		return false;
	}
	else if (bytesPerTickTotal() < bytesPerTickPerClient())
	{
		WARNING_MSG( "DownloadStreamerConfig::postInit: Download streaming has "
			"been configured for up to %u bytes per tick per client, but "
			"only %u bytes per tick in total\n",
			bytesPerTickPerClient(), bytesPerTickTotal() );
	}

	return true;
}

BW_END_NAMESPACE

// download_streamer_config.cpp
