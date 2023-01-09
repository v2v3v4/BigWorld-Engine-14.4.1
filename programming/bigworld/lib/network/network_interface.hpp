#ifndef NETWORK_INTERFACE_HPP
#define NETWORK_INTERFACE_HPP

#include "basictypes.hpp"
#include "endpoint.hpp"
#include "interface_table.hpp"
#include "misc.hpp"
#include "sending_stats.hpp"
#include "udp_channel.hpp"


#include "math/ema.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{

class UDPBundle;
class UDPChannel;
class ChannelFinder;
class ChannelMap;
class CondemnedChannels;
class DelayedChannels;
class DispatcherCoupling;
class IrregularChannels;
class KeepAliveChannels;
class OnceOffSender;
class PacketMonitor;
class PacketReceiverStats;
class PacketSender;
class PacketLossParameters;
class RecentlyDeadChannels;
class RequestManager;

typedef SmartPointer< UDPChannel > UDPChannelPtr;

enum NetworkInterfaceType
{
	NETWORK_INTERFACE_INTERNAL,
	NETWORK_INTERFACE_EXTERNAL
};

class INoSuchPort;


/**
 *	This class manages sets of channels.
 */
class NetworkInterface : public TimerHandler
{
public:
	static const char * USE_BWMACHINED;

	enum VerbosityLevel
	{
		VERBOSITY_LEVEL_QUIET = 0,
		VERBOSITY_LEVEL_NORMAL,
		VERBOSITY_LEVEL_DEBUG,

		VERBOSITY_LEVEL_MAX = VERBOSITY_LEVEL_DEBUG
	};

	NetworkInterface( EventDispatcher * pMainDispatcher,
		NetworkInterfaceType interfaceType,
		uint16 listeningPort = 0, const char * listeningInterface = NULL );
	~NetworkInterface();

	void attach( EventDispatcher & mainDispatcher );
	void detach();

	void prepareForShutdown();
	void stopPingingAnonymous();

	void processUntilChannelsEmpty( float timeout = 10.f );

	bool recreateListeningSocket( uint16 listeningPort,
							const char * listeningInterface );

	Reason registerWithMachined( const BW::string & name, int id /*,
		bool isRegister*/ );

	bool registerChannel( UDPChannel & channel );
	bool deregisterChannel( UDPChannel & channel );

	void onAddressDead( const Address & addr );

	bool isDead( const Address & addr ) const;

	enum IndexedChannelFinderResult
	{
		INDEXED_CHANNEL_HANDLED,
		INDEXED_CHANNEL_NOT_HANDLED,
		INDEXED_CHANNEL_CORRUPTED
	};

	UDPChannel * findChannel( const Address & addr,
							bool createAnonymous  = false );

	INLINE UDPChannel & findOrCreateChannel( const Address & addr );

	IndexedChannelFinderResult findIndexedChannel( ChannelID channelID,
			const Mercury::Address & srcAddr,
			Packet * pPacket, UDPChannelPtr & pChannel ) const;

	UDPChannelPtr findCondemnedChannel( ChannelID channelID ) const;

	void registerChannelFinder( ChannelFinder * pFinder );

	void delAnonymousChannel( const Address & addr );

	void setIrregularChannelsResendPeriod( float seconds );

	bool hasUnackedPackets() const;
	bool deleteFinishedChannels();

	void onChannelGone( Channel * pChannel );

	void cancelRequestsFor( Channel * pChannel );
	void cancelRequestsFor( ReplyMessageHandler * pHandler, Reason reason );
	void cancelRequestsFor( Bundle & pBundle, Reason reason );

	void delayedSend( UDPChannel & channel );
	void sendIfDelayed( UDPChannel & channel );

	// -------------------------------------------------------------------------
	// Section: Processing
	// -------------------------------------------------------------------------

	Reason processPacketFromStream( const Address & addr,
		BinaryIStream & data );

	PacketFilterPtr pOffChannelFilter() const { return pOffChannelFilter_; }
	void pOffChannelFilter(PacketFilterPtr pFilter) { pOffChannelFilter_ = pFilter; }

	// -------------------------------------------------------------------------
	// Section: Accessors
	// -------------------------------------------------------------------------

	CondemnedChannels & condemnedChannels() { return *pCondemnedChannels_; }
	IrregularChannels & irregularChannels()	{ return *pIrregularChannels_; }
	KeepAliveChannels & keepAliveChannels()	{ return *pKeepAliveChannels_; }

	EventDispatcher & dispatcher()			{ return *pDispatcher_; }
	EventDispatcher & mainDispatcher()		{ return *pMainDispatcher_; }

	InterfaceTable & interfaceTable();
	const InterfaceTable & interfaceTable() const;

	const PacketReceiverStats & receivingStats() const;
	const SendingStats & sendingStats() const	{ return sendingStats_; }

	INLINE const Address & address() const;

	void setPacketMonitor( PacketMonitor * pPacketMonitor );

	bool isExternal() const				{ return isExternal_; }

	/** This method returns whether this network interface is verbose. */
	bool isVerbose() const { return verbosityLevel_ > VERBOSITY_LEVEL_QUIET; }

	/** This method returns whether this network interface is extra verbose. */
	bool isDebugVerbose() const 
	{ 
		return verbosityLevel_ >= VERBOSITY_LEVEL_DEBUG;
	}

	/**
	 *	This method sets the verbosity (backwards compatible).
	 *
	 *	@see NetworkInterface::verbosityLevel
	 */
	void isVerbose( bool value ) 
	{
		verbosityLevel_ = (value ? 
			VERBOSITY_LEVEL_DEBUG : 
			VERBOSITY_LEVEL_QUIET); 
	}

	/** This method returns the verbosity level. */
	VerbosityLevel verbosityLevel() const { return verbosityLevel_; } 

	/** This method sets the verbosity level. */
	void verbosityLevel( VerbosityLevel value ) { verbosityLevel_ = value; }

	Endpoint & socket()					{ return udpSocket_; }

	// Sending related
	OnceOffSender & onceOffSender()		{ return *pOnceOffSender_; }

	// PacketSender & packetSender()	{ return *pPacketSender_; }
	void addReplyOrdersTo( Bundle & bundle, Channel * pChannel );

	const char * c_str() const { return udpSocket_.c_str(); }

	void setLatency( float latencyMin, float latencyMax );
	void setLossRatio( float lossRatio );

	float maxSocketProcessingTime() const;
	void maxSocketProcessingTime( float maxTime );

	// This property is so that data can be associated with a interface. Message
	// handlers get access to the interface that received the message and can 
	// get access to this data. Bots use this so that they know which
	// ServerConnection should handle the message.
	void * pExtensionData() const			{ return pExtensionData_; }
	void pExtensionData( void * pData )		{ pExtensionData_ = pData; }

	unsigned int numBytesReceivedForMessage( uint8 msgID ) const;

	// -------------------------------------------------------------------------
	// Section: Sending
	// -------------------------------------------------------------------------
	Bundle * createOffChannelBundle();
	void send( const Address & address, UDPBundle & bundle,
		UDPChannel * pChannel = NULL );
	void sendOnExistingChannel( const Address & address, UDPBundle & bundle );
	void sendPacket( const Address & address, Packet * pPacket,
						UDPChannel * pChannel, bool isResend );

	void dropNextSend();

	bool shouldUseChecksums() const;
	void shouldUseChecksums( bool v );

	bool hasArtificialLossOrLatency() const;

	bool isGood() const
	{
		return (udpSocket_.fileno() != -1) && !address_.isNone();
	}

	void setNoSuchPortCallback( INoSuchPort * pCallback );

	// Rate limiting

	/**
	 *	This method returns the current rate-limiting period, or zero if
	 *	rate-limiting is not in effect.
	 */
	float rateLimitPeriod() const { return rateLimitPeriod_; }
	void rateLimitPeriod( float rateLimitPeriod );

	uint perIPAddressRateLimit() const { return rateLimitPerIPAddress_; }

	void perIPAddressRateLimit( uint limitPerPeriod )
	{
		rateLimitPerIPAddress_ = limitPerPeriod;
	}

	uint perIPAddressPortRateLimit() const 
	{
		return rateLimitPerIPAddressPort_;
	}

	void perIPAddressPortRateLimit( uint limitPerPeriod )
	{
		rateLimitPerIPAddressPort_ = limitPerPeriod;
	}

	bool incrementAndCheckRateLimit( const Address & addr );

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif

private:
	void finaliseRequestManager();

	virtual void handleTimeout( TimerHandle handle, void * arg );

	void clearRateLimitCounts();

	void closeSocket();

	// -------------------------------------------------------------------------
	// Section: Private data
	// -------------------------------------------------------------------------

	Endpoint		udpSocket_;

	// The address of this socket.
	Address	address_;

	PacketSender * pPacketSender_;
	PacketReceiver * pPacketReceiver_;
	PacketLossParameters * pPacketLossParameters_;

	// The desired buffer sizes on a socket.
	static int recvBufferSize_;
	static int sendBufferSize_;

	float rateLimitPeriod_;
	TimerHandle rateLimitTimer_;

	typedef BW::map< Mercury::Address, uint > RateLimitedAddresses;
	uint rateLimitPerIPAddress_;
	RateLimitedAddresses rateLimitedIPAddresses_;
	AccumulatingEMA<uint> ipAddressRateLimitAverage_;

	uint rateLimitPerIPAddressPort_;
	RateLimitedAddresses rateLimitedIPAddressPorts_;
	AccumulatingEMA<uint> ipAddressPortRateLimitAverage_;


	RecentlyDeadChannels * pRecentlyDeadChannels_;
	ChannelMap *				pChannelMap_;

	IrregularChannels * pIrregularChannels_;
	CondemnedChannels * pCondemnedChannels_;
	KeepAliveChannels * pKeepAliveChannels_;

	DelayedChannels * 			pDelayedChannels_;

	ChannelFinder *				pChannelFinder_;

	/// Indicates whether this is listening on an external interface.
	const bool isExternal_;

	VerbosityLevel verbosityLevel_;

	EventDispatcher * pDispatcher_;
	EventDispatcher * pMainDispatcher_;

	InterfaceTable * pInterfaceTable_;
	
	PacketFilterPtr	pOffChannelFilter_;

	// Must be below dispatcher_ and pInterfaceTable_
	RequestManager * pRequestManager_;

	void * pExtensionData_;

	// -------------------------------------------------------------------------
	// Section: Sending
	// -------------------------------------------------------------------------
	OnceOffSender *				pOnceOffSender_;

	SendingStats	sendingStats_;
};

} // namespace Mercury

#ifdef CODE_INLINE
#include "network_interface.ipp"
#endif

BW_END_NAMESPACE

#endif // NETWORK_INTERFACE_HPP
