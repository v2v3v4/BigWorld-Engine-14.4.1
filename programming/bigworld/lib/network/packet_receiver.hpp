#ifndef PACKET_RECEIVER_HPP
#define PACKET_RECEIVER_HPP

#include "interfaces.hpp"

#include "fragmented_bundle.hpp"
#include "once_off_packet.hpp"
#include "network_interface.hpp"
#include "packet.hpp"
#include "packet_receiver_stats.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class UDPChannel;
class EventDispatcher;
class PacketMonitor;
class ProcessSocketStatsHelper;


/**
 *	This interface is used to received notification when an ICMP packet is
 *	received indicating that a port was closed.
 */
class INoSuchPort
{
public:
	virtual void onNoSuchPort( const Mercury::Address & addr ) = 0;
};


/**
 *	This class is responsible for receiving Mercury packets from a network
 *	socket.
 */
class PacketReceiver : public InputNotificationHandler, private TimerHandler
{
public:
	PacketReceiver( Endpoint & socket, NetworkInterface & networkInterface );
	virtual ~PacketReceiver();

	Reason processPacket( const Address & addr, Packet * p,
			ProcessSocketStatsHelper * pStatsHelper );
	Reason processFilteredPacket( const Address & addr, Packet * p,
			ProcessSocketStatsHelper * pStatsHelper );

	PacketReceiverStats & stats()		{ return stats_; }

	/**
	 *	This method returns the maximum socket processing time.
	 *
	 *	@return		The maximum time (in seconds) allowed for socket processing
	 *				when notified of available input before relinquishing
	 *				control.
	 */
	float maxSocketProcessingTime() const
	{
	 	return float( maxSocketProcessingTimeStamps_ / 
			BW_NAMESPACE stampsPerSecondD() );
	}

	void maxSocketProcessingTime( float value );

	bool isVerbose() const;
	bool isDebugVerbose() const;

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif // ENABLE_WATCHERS

	void setPacketMonitor( PacketMonitor * pPacketMonitor )
	{
		pPacketMonitor_ = pPacketMonitor;
	}

	void setNoSuchPortCallback( INoSuchPort * pCallback )
	{
		pNoSuchPortCallback_ = pCallback;
	}

private:
	// Overrides from InputNotificationHandler
	virtual int handleInputNotification( int fd );
	virtual bool handleErrorNotification( int fd );

	// Overrides from TimerHandler
	virtual void handleTimeout( TimerHandle handle, void * pUser );

	bool processSocket( Address & srcAddr, bool expectingPacket );
	bool checkSocketErrors( int len, bool expectingPacket );
	void onConnectionRefusedTo( const Address & addr );
	void onMTUExceeded( const Address & offender, const uint32 & info );
	Reason processOrderedPacket( const Address & addr, Packet * p,
		UDPChannel * pChannel, ProcessSocketStatsHelper * pStatsHelper );
	void processEarlyPacket( const Address & addr, Packet * p,
		UDPChannel * pChannel, ProcessSocketStatsHelper * pStatsHelper );

	bool processPiggybacks( const Address & addr,
			Packet * p, ProcessSocketStatsHelper * pStatsHelper );
	EventDispatcher & dispatcher();


	// Member data
	Endpoint & socket_;
	NetworkInterface & networkInterface_;

	// Stores the packet as an optimisation for processSocket.
	PacketPtr pNextPacket_;
	PacketReceiverStats stats_;
	TimerHandle statsUpdateTimer_;
	OnceOffReceiver onceOffReceiver_;

	INoSuchPort * pNoSuchPortCallback_;
	PacketMonitor * pPacketMonitor_;

	uint64 maxSocketProcessingTimeStamps_;
};

} // namespace Mercury


#ifdef CODE_INLINE
#include "packet_receiver.ipp"
#endif


BW_END_NAMESPACE

#endif //  PACKET_RECEIVER_HPP
