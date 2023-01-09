#include "pch.hpp"

#include "packet_filter.hpp"

#ifndef CODE_INLINE

BW_BEGIN_NAMESPACE
#include "packet_filter.ipp"
BW_END_NAMESPACE

#endif

#include "packet_receiver.hpp"
#include "packet_sender.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{

// ----------------------------------------------------------------
// Section: PacketFilter
// ----------------------------------------------------------------

/*
 *	Documented in header file.
 */
Reason PacketFilter::send( PacketSender & packetSender,
		const Address & addr, Packet * pPacket )
{
	return packetSender.basicSendWithRetries( addr, pPacket );
}


/**
 *	Documented in header file.
 */
Reason PacketFilter::recv( PacketReceiver & receiver,
							const Address & addr, Packet * pPacket,
	   						ProcessSocketStatsHelper * pStatsHelper )
{
	return receiver.processFilteredPacket( addr, pPacket, pStatsHelper );
}

} // namespace Mercury

BW_END_NAMESPACE

// packet_filter.cpp
