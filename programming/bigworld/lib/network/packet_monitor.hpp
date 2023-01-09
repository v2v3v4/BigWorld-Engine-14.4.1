#ifndef PACKET_MONITOR_HPP
#define PACKET_MONITOR_HPP

BW_BEGIN_NAMESPACE

namespace Mercury
{

class Address;
class Packet;


/**
 *	This class defines an interface that can be used to receive
 *	a callback whenever an incoming or outgoing packet passes
 *	through Mercury.
 *
 *	@see NetworkInterface::setPacketMonitor
 *
 *	@ingroup mercury
 */
class PacketMonitor
{
public:
	virtual ~PacketMonitor() {};

	/**
	 *	This method is called when Mercury sends a packet.
	 *
	 *	@param addr 	The destination address of the packet.
	 *	@param packet	The actual packet.
	 */
	virtual void packetOut( const Address & addr, const Packet & packet ) = 0;

	/**
	 * 	This method is called when Mercury receives a packet, before
	 * 	it is processed.
	 *
	 * 	@param addr		The source address of the packet.
	 * 	@param packet	The actual packet.
	 */
	virtual void packetIn( const Address& addr, const Packet& packet ) = 0;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // PACKET_MONITOR_HPP
