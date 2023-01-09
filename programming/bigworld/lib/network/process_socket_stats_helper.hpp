#ifndef PROCESS_SOCKET_STATS_HPP
#define PROCESS_SOCKET_STATS_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{
class PacketReceiverStats;


/**
 *	This class is used to help collect statistics from the
 *	PacketReceiver::processSocket method.
 */
class ProcessSocketStatsHelper
{
public:
	ProcessSocketStatsHelper( PacketReceiverStats & stats );
	~ProcessSocketStatsHelper();

	void startMessageHandling( int messageLength );
	void stopMessageHandling();

	void socketReadFinished( int length );

	void onBundleFinished();
	void onCorruptedBundle();

private:
	PacketReceiverStats & stats_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // PROCESS_SOCKET_STATS_HPP
