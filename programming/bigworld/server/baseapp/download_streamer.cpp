#include "download_streamer.hpp"

#include "download_streamer_config.hpp"

#include <limits.h>


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
DownloadStreamer::DownloadStreamer() :
	curDownloadRate_( 0 )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors
// -----------------------------------------------------------------------------

/**
 *	Maximum downstream bitrate for stream*ToClient() stuff, in bytes/tick.
 */
int DownloadStreamer::maxDownloadRate() const
{
	int rate = DownloadStreamerConfig::bytesPerTickTotal();
	return rate ? rate : INT_MAX;
}


/**
 *	Actual downstream bandwidth in use by all proxies, in bytes/tick.
 */
int DownloadStreamer::curDownloadRate() const
{
	return curDownloadRate_;
}


/**
 *	Maximum downstream bitrate per client, in bytes/tick.
 */
int DownloadStreamer::maxClientDownloadRate() const
{
	int rate = DownloadStreamerConfig::bytesPerTickPerClient();
	return rate ? rate : INT_MAX;
}


/**
 *	Maximum increase in download rate to a single client per tick, in bytes.
 */
int DownloadStreamer::downloadRampUpRate() const
{
	return DownloadStreamerConfig::rampUpRateBytesPerTick();
}


/**
 *	Unacked packet age at which we throttle downloads.
 */
int DownloadStreamer::downloadBacklogLimit() const
{
	return DownloadStreamerConfig::backlogLimit();
}


float DownloadStreamer::downloadScaleBack() const
{
	return std::min( 1.f, float( this->maxDownloadRate() ) / curDownloadRate_ );
}


void DownloadStreamer::modifyDownloadRate( int delta )
{
	curDownloadRate_ += delta;
}

BW_END_NAMESPACE

// download_stream.cpp
