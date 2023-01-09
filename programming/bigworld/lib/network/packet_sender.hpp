#ifndef PACKET_SENDER_HPP
#define PACKET_SENDER_HPP

#include "misc.hpp"

#if ENABLE_WATCHERS
#include "cstdmf/watcher.hpp"
#endif

BW_BEGIN_NAMESPACE


class Endpoint;

namespace Mercury
{

class Address;
class EventDispatcher;
class OnceOffSender;
class Packet;
class PacketMonitor;
class PacketLossParameters;
class RequestManager;
class SendingStats;
class UDPBundle;
class UDPChannel;


/**
 *	This class is responsible for sending packets for a network interface.
 */
class PacketSender
{
public:
	PacketSender( Endpoint & socket, RequestManager & requestManager,
			EventDispatcher & eventDispatcher,
			OnceOffSender & onceOffSender, SendingStats & sendingStats,
			PacketLossParameters & packetLossParameters );

	void send( const Address & address, UDPBundle & bundle, 
		UDPChannel * pChannel );

	void sendPacket( const Address & address, Packet * pPacket,
						UDPChannel * pChannel, bool isResend );

	bool rescheduleSend( const Address & addr,
			Packet * pPacket, UDPChannel * pChannel );
	Reason basicSendWithRetries( const Address & addr, Packet * pPacket );
	void sendRescheduledPacket( const Address & address,
						Packet * pPacket, UDPChannel * pChannel );

	/** This method causes the packet sender to drop the next send. */
	void dropNextSend()					{ isDroppingNextSend_ = true; }
	/**
	 *	This method returns whether the next send will be dropped.
	 */
	bool isDroppingNextSend() const		{ return isDroppingNextSend_; }

	/**
	 *	This method sets whether checksums will be sent.
	 */
	void shouldUseChecksums( bool v ) 	{ shouldUseChecksums_ = v; }
	
	/**
	 *	This method returns whether checksums will be sent.
	 */
	bool shouldUseChecksums() const 	{ return shouldUseChecksums_; }

	/**
	 *	This method sets the packet monitor.
	 *
	 *	@param pPacketMonitor.
	 */
	void setPacketMonitor( PacketMonitor * pPacketMonitor )
	{
		pPacketMonitor_ = pPacketMonitor;
	}


private:
	Reason basicSendSingleTry( const Address & addr, Packet * pPacket );

	Endpoint & socket_;
	RequestManager & requestManager_;
	EventDispatcher & eventDispatcher_;
	OnceOffSender & onceOffSender_;

	SendingStats & sendingStats_;

	SeqNumAllocator seqNumAllocator_;

	PacketMonitor * pPacketMonitor_;

	PacketLossParameters & packetLossParameters_;

	/// State flag used in debugging to indicate that the next outgoing packet
	/// should be dropped
	bool	isDroppingNextSend_;

	// Whether to add checksums to packets.
	bool 	shouldUseChecksums_;
};


} // namespace Mercury


BW_END_NAMESPACE


#endif //  PACKET_SENDER_HPP
