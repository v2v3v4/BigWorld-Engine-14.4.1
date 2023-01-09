#ifndef UDP_CHANNEL_HPP
#define UDP_CHANNEL_HPP

#include "channel.hpp"
#include "circular_array.hpp"
#include "fragmented_bundle.hpp"
#include "irregular_channels.hpp"
#include "keepalive_channels.hpp"
#include "packet_filter.hpp"
#include "misc.hpp"
#include "message_filter.hpp"
#include "udp_bundle.hpp"

#include <list>
#include <set>

BW_BEGIN_NAMESPACE

class Watcher;
typedef SmartPointer< Watcher > WatcherPtr;

const float  DEFAULT_INACTIVITY_RESEND_DELAY = 1.f;

namespace Mercury
{
class NetworkInterface;
class PacketFilter;
class PacketReceiverStats;
class ReliableOrder;

/**
 *	Channels are used for regular communication channels between two address.
 *
 *	@note Any time you call 'bundle' you may get a different bundle to the one
 *	you got last time, because the Channel decided that the bundle was full
 *	enough to send. This does not occur on high latency channels (or else
 *	tracking numbers would get very confusing).
 *
 *	@note If you use more than one Channel on the same address, they share the
 *	same bundle. This means that:
 *
 *	@li Messages (and message sequences where used) must be complete between
 *		calls to 'bundle' (necessary due to note above anyway)
 *
 *	@li Each channel must say send before the bundle is actually sent.
 *
 *	@li Bundle tracking does not work with multiple channels; only the last
 *		Channel to call 'send' receives a non-zero tracking number (or possibly
 *		none if deleting a Channel causes it to be sent), and only the first
 *		Channel on that address receives the 'bundleLost' call.
 *
 * 	@ingroup mercury
 */
class UDPChannel : public Channel
{
public:
	/**
	 *	The traits of a channel are used to decide the reliablity method.
	 *	There are two types of channels that we handle. The first is a
	 *	channel from server to server. These channels are low latency,
	 *	high bandwidth, and low loss. The second is a channel from client
	 *	to server, which is high latency, low bandwidth, and high loss.
	 *	Since bandwidth is scarce on client/server channels, only reliable
	 *	data is resent on these channels. Unreliable data is stripped from
	 *	dropped packets and discarded.
	 */
	enum Traits
	{
		/// This describes the properties of channel from server to server.
		INTERNAL = 0,

		/// This describes the properties of a channel from client to server.
		EXTERNAL = 1,
	};

	typedef void (*SendWindowCallback)( const UDPChannel & );

	UDPChannel( NetworkInterface & networkInterface, const Address & addr,
		Traits traits,
		float minInactivityResendDelay = DEFAULT_INACTIVITY_RESEND_DELAY,
		PacketFilterPtr pFilter = NULL, ChannelID id = CHANNEL_ID_NULL );

	static UDPChannel * get( NetworkInterface & networkInterface,
			const Address & addr );

private:
	virtual ~UDPChannel();

public:
	static void staticInit();


	// Overrides from Channel.
#if ENABLE_WATCHERS
	virtual WatcherPtr getWatcher();
#endif // ENABLE_WATCHERS

	virtual Bundle * newBundle();

	virtual const char * c_str() const;

	virtual bool hasUnsentData() const;

	virtual bool isDead() const
	{
		return this->isCondemned() || this->isDestroyed();
	}

	virtual bool isTCP() const { return false; }

	virtual void setEncryption( Mercury::BlockCipherPtr pBlockCipher );

	virtual void shutDown();

	void setAddress( const Mercury::Address & addr );

	void delayedSend();

	void sendIfIdle();

	void reset( const Address & newAddr, bool warnOnDiscard = true );

	void condemn();
	bool isCondemned() const { return isCondemned_; }

	void initFromStream( BinaryIStream & data, const Address & addr );
	void addToStream( BinaryOStream & data );

	void configureFrom( const UDPChannel & other );

	void switchInterface( NetworkInterface * pDestInterface );

	int windowSize() const;
	// TODO: Remove this
	int earliestUnackedPacketAge() const	{ return this->sendWindowUsage(); }

	int numOutstandingPackets() const;

	PacketFilterPtr pFilter() const { return pFilter_; }
	void pFilter(PacketFilterPtr pFilter) { pFilter_ = pFilter; }

	bool isLocalRegular() const			{ return isLocalRegular_; }
	void isLocalRegular( bool isLocalRegular );

	bool isRemoteRegular() const		{ return isRemoteRegular_; }
	void isRemoteRegular( bool isRemoteRegular );

	bool hasRemoteFailed() const { return hasRemoteFailed_; }
	void setRemoteFailed();

	bool addResendTimer( SeqNum seq, Packet * p,
		const ReliableOrder * roBeg, const ReliableOrder * roEnd );
	bool handleCumulativeAck( SeqNum seq );
	bool handleAck( SeqNum seq );
	void checkResendTimers( UDPBundle & bundle );
	bool resend( SeqNum seq, UDPBundle & bundle );
	uint64 roundTripTime() const { return roundTripTime_; }
	virtual double roundTripTimeInSeconds() const
		{ return roundTripTime_/stampsPerSecondD(); }

	enum AddToReceiveWindowResult
	{
		PACKET_IS_NEXT_IN_WINDOW,
		PACKET_IS_BUFFERED_IN_WINDOW,
		PACKET_IS_DUPLICATE,
		PACKET_IS_OUT_OF_WINDOW,
		PACKET_IS_CORRUPT,
	};

	AddToReceiveWindowResult addToReceiveWindow(
		Packet * p, const Address & srcAddr, PacketReceiverStats & stats );

	typedef BW::set< SeqNum > Acks;
	Acks acksToSend_;

	bool hasAcks() const { return !acksToSend_.empty(); }

	bool isAnonymous() const { return isAnonymous_; }
	void isAnonymous( bool v );

	bool isOwnedByInterface() const
		{ return !isDestroyed_ && (isAnonymous_ || isCondemned_); }

	bool hasUnackedCriticals() const 
	{ 
		return unackedCriticalSeq_ != SEQ_NULL;
	}

	void resendCriticals();

	bool wantsFirstPacket() const { return wantsFirstPacket_; }
	void gotFirstPacket() { wantsFirstPacket_ = false; }

	void dropNextSend() { shouldDropNextSend_ = true; }

	Traits traits() const { return traits_; }
	bool isExternal() const { return traits_ == EXTERNAL; }
	bool isInternal() const { return traits_ == INTERNAL; }

	bool shouldAutoSwitchToSrcAddr() const 
	{ 
		return shouldAutoSwitchToSrcAddr_; 
	}

	void shouldAutoSwitchToSrcAddr( bool b );

	SeqNum useNextSequenceID();
	void onPacketReceived( int bytes );

	/// The id for indexed channels (or CHANNEL_ID_NULL if not indexed).
	ChannelID id() const	{ return id_; }

	/// The version of indexed channels (or 0 if not indexed).
	ChannelVersion version() const { return version_; }
	void version( ChannelVersion v ) { version_ = v; }
	ChannelVersion creationVersion() const { return creationVersion_; }
	void creationVersion( ChannelVersion v ) { creationVersion_ = v; }

	bool isIndexed() const	{ return id_ != CHANNEL_ID_NULL; }
	bool isEstablished() const { return addr_.ip != 0; }

	void writeFlags( Packet * p );
	void writeFooter( Packet * p );

	FragmentedBundlePtr pFragments() { return pFragments_; }
	void pFragments( FragmentedBundlePtr pFragments )
		{ pFragments_ = pFragments; }

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif

	bool hasUnackedPackets() const	{ return oldestUnackedSeq_ != SEQ_NULL; }

	/// Returns how much of the send window is currently being used. This
	/// includes the overflow packets and so can be larger than windowSize_.
	int sendWindowUsage() const
	{
		return this->hasUnackedPackets() ?
			seqMask( largeOutSeqAt_ - oldestUnackedSeq_ ) : 0;
	}

	static void setSendWindowCallback( SendWindowCallback callback );
	static float sendWindowCallbackThreshold();
	static void sendWindowCallbackThreshold( float threshold );

	unsigned int pushUnsentAcksThreshold() const 
	{ 
		return pushUnsentAcksThreshold_;
	}

	void pushUnsentAcksThreshold( int i ) { pushUnsentAcksThreshold_ = i; }

	/**
	 *	This method returns the number of packets resent by this channel.
	 */
	uint32	numPacketsResent() const	{ return numPacketsResent_; }

	/**
	 *	This method returns the number of reliable packets sent by this
	 *	channel.
	 */
	uint32	numReliablePacketsSent() const { return numReliablePacketsSent_; }

	/**
	 *  This method returns the last time a reliable packet was sent for the
	 *  first time.
	 */
	uint64	lastReliableSendTime() const { return lastReliableSendTime_; }

	/**
	 *  This method returns the last time a reliable packet was sent for the
	 *  first time or re-sent.
	 */
	uint64	lastReliableSendOrResendTime() const
	{
		return std::max( lastReliableSendTime_, lastReliableResendTime_ );
	}

	bool validateUnreliableSeqNum( const SeqNum seqNum );


	// External Channels
	static void setExternalMaxOverflowPackets( uint16 maxPackets )
	{
		UDPChannel::s_maxOverflowPackets_[ 0 ] = maxPackets;
	}

	static uint16 getExternalMaxOverflowPackets()
	{
		return static_cast<uint16>(UDPChannel::s_maxOverflowPackets_[ 0 ]);
	}

	// Internal Channels
	static void setInternalMaxOverflowPackets( uint16 maxPackets )
	{
		UDPChannel::s_maxOverflowPackets_[ 1 ] = maxPackets;
	}

	static uint16 getInternalMaxOverflowPackets()
	{
		return static_cast<uint16>(UDPChannel::s_maxOverflowPackets_[ 1 ]);
	}

	// Indexed Channels
	static void setIndexedMaxOverflowPackets( uint16 maxPackets )
	{
		UDPChannel::s_maxOverflowPackets_[ 2 ] = maxPackets;
	}

	static uint16 getIndexedMaxOverflowPackets()
	{
		return static_cast<uint16>(UDPChannel::s_maxOverflowPackets_[ 2 ]);
	}

	/// Should the process assert when the maximum number of overflow
	/// packets has been reached.
	static bool s_assertOnMaxOverflowPackets;

	static bool assertOnMaxOverflowPackets()
	{
		return UDPChannel::s_assertOnMaxOverflowPackets;
	}

	static void assertOnMaxOverflowPackets( bool shouldAssert )
	{
		UDPChannel::s_assertOnMaxOverflowPackets = shouldAssert;
	}

	static bool allowInteractiveDebugging()
	{
		return s_allowInteractiveDebugging;
	}

	static void allowInteractiveDebugging( bool shouldAllow )
	{
		s_allowInteractiveDebugging = shouldAllow;
	}

protected:

	// Overrides from Channel
	virtual void doPreFinaliseBundle( Bundle & bundle );
	virtual void doSend( Bundle & bundle );

private:
	enum TimeOutType
	{
		TIMEOUT_INACTIVITY_CHECK
	};

	bool isInSentWindow( SeqNum seq ) const;

	void clearState( bool warnOnDiscard = false );

	UDPBundle & udpBundle() { return static_cast< UDPBundle & >( *pBundle_ ); }
	const UDPBundle & udpBundle() const 
	{ 
		return static_cast< const UDPBundle & >( *pBundle_ );
	}

	Traits		traits_;

	/// An indexed channel is basically a way of multiplexing multiple
	/// channels between a pair of addresses.  Regular channels distinguish
	/// traffic solely on the basis of address, so in situations where you need
	/// multiple channels between a pair of addresses (i.e. channels between
	/// base and cell entities) you use indexed channels to keep the streams
	/// separate.
	ChannelID	id_;

	/// Indexed channels have a 'version' number which basically tracks how
	/// many times they have been offloaded.  This allows us to correctly
	/// determine which incoming packets are out-of-date and also helps
	/// identify the most up-to-date information about lost entities in a
	/// restore situation.
	ChannelVersion version_;

	ChannelVersion creationVersion_;

	PacketFilterPtr		pFilter_;

	uint32			windowSize_;

	/// Generally, the sequence number of the next packet to be sent.
	SeqNum			smallOutSeqAt_; // This does not include packets in 
									// overflowPackets_

	SeqNumAllocator	largeOutSeqAt_; // This does include packets in 
									// overflowPackets_

	/// The sequence number of the oldest unacked packet on this channel.
	SeqNum			oldestUnackedSeq_;

	/// The last time a reliable packet was sent (for the first time) on this
	/// channel, as a timestamp.
	uint64			lastReliableSendTime_;

	/// The last time a reliable packet was resent on this channel.
	uint64			lastReliableResendTime_;

	/// The average round trip time for this channel, in timestamp units.
	uint64			roundTripTime_;

	/// The minimum time for a resend due to inactivity, used to stop thrashing
	/// when roundTripTime_ is low with respect to tick time.
	uint64			minInactivityResendDelay_;

	/// The last valid sequence number that was seen on an unreliable
	/// channel.
	SeqNum			unreliableInSeqAt_;

	class UnackedPacket;

	CircularArray< UnackedPacket * > unackedPackets_;
	uint sendWindowSize() const	{ return unackedPackets_.size(); }

	bool hasSeenOverflowWarning_;
	void checkOverflowErrors();

	static uint s_maxOverflowPackets_[3];

	uint getMaxOverflowPackets() const
	{
		if (this->isExternal())
		{
			return s_maxOverflowPackets_[ 0 ];
		}

		return s_maxOverflowPackets_[ this->isIndexed() ? 2 : 1 ];
	}

	uint maxWindowSize() const
	{
		return windowSize_ + this->getMaxOverflowPackets();
	}

	void sendUnacked( UnackedPacket & unacked );

	/// The next packet that we expect to receive.
	SeqNum			inSeqAt_;

	/// Stores ordered packets that are received out-of-order.
	CircularArray< PacketPtr > bufferedReceives_;
	uint32 numBufferedReceives_;

	/// The fragment chain for the partially reconstructed incoming bundle on
	/// this channel, or NULL if incoming packets aren't fragments right now.
	FragmentedBundlePtr pFragments_;

	/// The ACK received with the highest sequence number.
	uint32			highestAck_;

	/// Stores the location in s_irregularChannels_;
	friend class IrregularChannels;
	IrregularChannels::iterator irregularIter_;

	/// Stores the location (if any) in the interface's keepAliveChannels_.
	friend class KeepAliveChannels;
	KeepAliveChannels::iterator keepAliveIter_;

	/// If false, this channel is checked periodically for resends. This also
	/// causes ACKs to be sent immediately instead of on the next outgoing
	/// bundle.
	bool			isLocalRegular_;

	/// If true, the remote app sends data regularly. This affects resending
	/// policy. It can then be based on "NACKS" (at least holes in ACKs).
	bool			isRemoteRegular_;

	/// If true, this channel has been condemned (i.e. detached from its
	/// previous owner and is awaiting death).
	bool			isCondemned_;

	/// Used by CellAppChannels to indicate that we should not process further
	/// packets.
	bool			hasRemoteFailed_;

	/// If true, this channel is to an address that we don't really know much
	/// about, at least, not enough to be bothered writing a helper class for
	/// it on this app.  That means that the interface is responsible for
	/// creating and deleting this channel.
	bool			isAnonymous_;

	/// The highest unacked sequence number that is considered to be
	/// 'critical'. What this actually means is up to the app code, and is
	/// controlled by using the RELIABLE_CRITICAL reliability flag when
	/// starting messages.
	SeqNum			unackedCriticalSeq_;

	/// If non-zero and the number of ACKs on this channel's bundle exceeds
	/// this number, the bundle will be sent automatically, regardless of 
	/// whether or not this channel is regular.
	unsigned int	pushUnsentAcksThreshold_;

	/// If true, this indexed channel will automatically switch its address to
	/// the source address of incoming packets.
	bool			shouldAutoSwitchToSrcAddr_;

	/// If true, this channel will drop all incoming packets unless they are
	/// flagged as FLAG_CREATE_CHANNEL.  This is only used by Channels that are
	/// reset() and want to ensure that they don't buffer any delayed incoming
	/// packets from the old connection.
	bool			wantsFirstPacket_;

	/// If true, this channel will artificially drop its next send().  This is
	/// used to help debug BigWorld in lossy network environments.
	bool			shouldDropNextSend_;

	/// The send window sizes where warnings are triggered.  This should be
	/// indexed with a bool indicating whether we're talking about indexed or
	/// plain internal channels.  This grows each time it is exceeded.  If it
	/// keeps growing to the point where window overflows happen, dev asserts
	/// will be triggered.
	static int 		s_sendWindowWarnThresholds_[2];

	int & sendWindowWarnThreshold()
	{
		return s_sendWindowWarnThresholds_[ this->isIndexed() ];
	}

	static int		s_sendWindowCallbackThreshold_;
	static SendWindowCallback s_pSendWindowCallback_;

	static bool s_allowInteractiveDebugging;

	// Statistics
	uint32	numPacketsResent_;
	uint32	numReliablePacketsSent_;

};

} // namespace Mercury

#ifdef CODE_INLINE
#include "udp_channel.ipp"
#endif

BW_END_NAMESPACE

#endif // UDP_CHANNEL_HPP
