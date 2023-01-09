#include "pch.hpp"

#include "rescheduled_sender.hpp"

#include "event_dispatcher.hpp"
#include "packet_sender.hpp"
#include "udp_channel.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	Constructor for RescheduledSender.
 */
RescheduledSender::RescheduledSender( EventDispatcher & dispatcher,
		PacketSender & packetSender,
		const Address & addr, Packet * pPacket, UDPChannel * pChannel,
		int latencyMilli ) :
	packetSender_( packetSender ),
	addr_( addr ),
	pPacket_( pPacket ),
	pChannel_( pChannel )
{
	dispatcher.addOnceOffTimer( latencyMilli*1000, this );
}


/**
 *
 */
void RescheduledSender::handleTimeout( TimerHandle, void * )
{
	packetSender_.sendRescheduledPacket( addr_, pPacket_.get(),
			this->pUDPChannel() );
}


/**
 *
 */
void RescheduledSender::onRelease( TimerHandle, void * )
{
	delete this;
}


UDPChannel * RescheduledSender::pUDPChannel()
{
	return static_cast< UDPChannel * >( pChannel_.get() );
}


} // namespace Mercury

BW_END_NAMESPACE

// rescheduled_sender.cpp
