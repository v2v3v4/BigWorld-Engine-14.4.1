#ifndef DOWNLOAD_STREAMER_CONFIG_HPP
#define DOWNLOAD_STREAMER_CONFIG_HPP

#include "server/server_app_option.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This contains configuration options for DownloadStreamer.
 */
class DownloadStreamerConfig
{
public:
	static ServerAppOption< int > bitsPerSecondTotal;
	static ServerAppOption< int > bitsPerSecondPerClient;
	static ServerAppOption< int > rampUpRate;
	static ServerAppOption< int > backlogLimit;

	static ServerAppOption< int > bytesPerTickTotal;
	static ServerAppOption< int > bytesPerTickPerClient;
	static ServerAppOption< int > rampUpRateBytesPerTick;

	static bool postInit();

private:
	DownloadStreamerConfig();
};

BW_END_NAMESPACE

#endif // DOWNLOAD_STREAMER_CONFIG_HPP
