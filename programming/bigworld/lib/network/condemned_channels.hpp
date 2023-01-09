#ifndef CONDEMNED_CHANNELS_HPP
#define CONDEMNED_CHANNELS_HPP

#include "network/interfaces.hpp"
#include "misc.hpp"

#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class UDPChannel;

/**
 *	This class is used handle channels that will be deleted once they have no
 *	unacked packets.
 */
class CondemnedChannels : public TimerHandler
{
public:
	CondemnedChannels() : timerHandle_() {}
	~CondemnedChannels();

	void add( UDPChannel * pChannel );

	UDPChannel * find( ChannelID channelID ) const;

	bool deleteFinishedChannels();

	int numCriticalChannels() const;

	bool hasUnackedPackets() const;

private:
	static const int AGE_LIMIT = 60;

	virtual void handleTimeout( TimerHandle, void * );
	bool shouldDelete( UDPChannel * pChannel, uint64 now ) const;

	typedef BW::list< UDPChannel * > NonIndexedChannels;
	typedef BW::map< ChannelID, UDPChannel * > IndexedChannels;

	NonIndexedChannels	nonIndexedChannels_;
	IndexedChannels		indexedChannels_;

	TimerHandle timerHandle_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // CONDEMNED_CHANNELS_HPP
