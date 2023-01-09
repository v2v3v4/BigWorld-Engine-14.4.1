#ifndef PACKET_FILTER_HPP
#define PACKET_FILTER_HPP

#include "misc.hpp"
#include "cstdmf/smartpointer.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class Channel;
class Packet;
class PacketFilter;
class PacketReceiver;
class PacketSender;
class ProcessSocketStatsHelper;

/**
 *  This class defines an interface that can be used to filter packets
 *  coming into or out of a channel.
 */
class PacketFilter : public SafeReferenceCount
{
public:
	virtual ~PacketFilter() {}

	/**
	 *  This method is called when Mercury wants to send a packet.
	 *	It should return a Mercury::Reason.
	 *	The base class implementation sends it over the nub's socket
	 *	with a few retries if error conditions are signalled.
	 *	Calling this base class method from your own send is recommended.
	 */
	virtual Reason send( PacketSender & packetSender,
							const Address & addr, Packet * pPacket );

	/**
	 *  This method is called when Mercury has received a packet.
	 *	You should return a Reason (if it is not REASON_SUCCESS then
	 *	an exception will be thrown with that Reason).
	 *	Calling this base class method from your own recv is necessary
	 *	to get Mercury to process the packet after you have finished
	 *	filtering it. You may call it multiple (or no) times if you wish.
	 */
	virtual Reason recv( PacketReceiver & receiver, const Address & addr,
					Packet * pPacket,
					ProcessSocketStatsHelper * pStatsHelper );

	/**
	 *	Return the max spare amount that you need, so the Bundle knows
	 *	to leave at least that much room.
	 */
	virtual int maxSpareSize() { return 0; }
};

typedef SmartPointer< PacketFilter > PacketFilterPtr;

} // namespace Mercury

#ifdef CODE_INLINE
#include "packet_filter.ipp"
#endif

BW_END_NAMESPACE

#endif // PACKET_FILTER_HPP
