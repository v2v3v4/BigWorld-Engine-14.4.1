#include "pch.hpp"

#include "process_socket_stats_helper.hpp"

#include "basictypes.hpp"
#include "packet_receiver_stats.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	Constructor.
 */
ProcessSocketStatsHelper::ProcessSocketStatsHelper(
		PacketReceiverStats & stats ) :
	stats_( stats )
{
#if ENABLE_WATCHERS
	stats_.mercuryTimer_.start();
	stats_.systemTimer_.start();
#endif // ENABLE_WATCHERS
}


/**
 *	Destructor.
 */
ProcessSocketStatsHelper::~ProcessSocketStatsHelper()
{
#if ENABLE_WATCHERS
	stats_.mercuryTimer_.stop();
#endif // ENABLE_WATCHERS
}


/**
 *	 This method is called just before a message is dispatched to its handler.
 */
void ProcessSocketStatsHelper::startMessageHandling( int messageLength )
{
	++stats_.numMessagesReceived_;
	// Do not account for it in the overhead anymore.
	stats_.numOverheadBytesReceived_ -= messageLength;
#if ENABLE_WATCHERS
	stats_.mercuryTimer_.stop();
#endif // ENABLE_WATCHERS
}


/**
 *	 This method is called just after a message has been processed by its
 *	 handler.
 */
void ProcessSocketStatsHelper::stopMessageHandling()
{
#if ENABLE_WATCHERS
	stats_.mercuryTimer_.start();
#endif // ENABLE_WATCHERS
}


/**
 *	This method is called when an attempt to read the socket (successful or not)
 *	has finished.
 *
 *	@param length	The size of the data read.
 */
void ProcessSocketStatsHelper::socketReadFinished( int length )
{
#if ENABLE_WATCHERS
	stats_.systemTimer_.stop( length > 0 );
#endif // ENABLE_WATCHERS

	if (length > 0)
	{
		++stats_.numPacketsReceived_;

		int totalLength = length + UDP_OVERHEAD;

		// Payload subtracted later
		stats_.numOverheadBytesReceived_ += totalLength;
		stats_.numBytesReceived_ += totalLength;
	}
}


/**
 *	This method is called when a bundle has successful been processed.
 */
void ProcessSocketStatsHelper::onBundleFinished()
{
	++stats_.numBundlesReceived_;
}


/**
 *	This method is called if a bundle is corrupted.
 */
void ProcessSocketStatsHelper::onCorruptedBundle()
{
	++stats_.numCorruptedBundlesReceived_;
}

} // namespace Mercury

BW_END_NAMESPACE

// process_socket_stats.cpp
