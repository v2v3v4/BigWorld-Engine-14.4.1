#ifndef RESCHEDULED_SENDER_HPP
#define RESCHEDULED_SENDER_HPP

#include "basictypes.hpp"
#include "channel.hpp"
#include "packet.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class UDPChannel;
class EventDispatcher;
class PacketSender;

typedef SmartPointer< Channel > ChannelPtr;


/**
 *	This class is responsible for sending packets that have artificial latency.
 */
class RescheduledSender : public TimerHandler
{
public:
	RescheduledSender( EventDispatcher & eventDispatcher,
			PacketSender & packetSender, const Address & addr,
			Packet * pPacket, UDPChannel * pChannel, int latencyMilli );

	virtual void handleTimeout( TimerHandle handle, void * arg );
	virtual void onRelease( TimerHandle handle, void * arg );

private:
	UDPChannel * pUDPChannel();

	PacketSender & packetSender_;
	Address addr_;
	PacketPtr pPacket_;
	ChannelPtr pChannel_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // RESCHEDULED_SENDER_HPP
