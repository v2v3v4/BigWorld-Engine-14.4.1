#ifndef UDP_BUNDLE_HPP
#define UDP_BUNDLE_HPP

#include "basictypes.hpp"
#include "bundle.hpp"
#include "bundle_piggyback.hpp"
#include "misc.hpp"
#include "packet.hpp"
#include "reliable_order.hpp"
#include "reply_order.hpp"
#include "unpacked_message_header.hpp"
#include "interface_element.hpp"

#include "cstdmf/binary_stream.hpp"

#include <map>


BW_BEGIN_NAMESPACE


class Endpoint;

namespace Mercury
{

class UDPChannel;
class InterfaceElement;
class InterfaceTable;
class NetworkInterface;
class ReplyMessageHandler;
class RequestManager;
class ProcessSocketStatsHelper;
class SendingStats;


/**
 *	A UDP bundle is bundle sent over a UDP channel.
 *
 *	@ingroup mercury
 */
class UDPBundle : public Bundle
{
public:
	UDPBundle( uint8 spareSize = 0, UDPChannel * pChannel = NULL );
	UDPBundle( Packet * p );

	virtual ~UDPBundle();

	// Overrides from Bundle

	virtual void startMessage( const InterfaceElement & ie,
		ReliableType reliable = RELIABLE_DRIVER );

	virtual void startRequest( const InterfaceElement & ie,
		ReplyMessageHandler * handler,
		void * arg = NULL,
		int timeout = DEFAULT_REQUEST_TIMEOUT,
		ReliableType reliable = RELIABLE_DRIVER );

	virtual void startReply( ReplyID id, 
		ReliableType reliable = RELIABLE_DRIVER );
	
	virtual void clear( bool newBundle = false );

	virtual int freeBytesInLastDataUnit() const;

	virtual int numDataUnits() const;

	virtual void doFinalise();

	// Overrides from BinaryOStream
	virtual void * reserve( int nBytes );
	virtual void addBlob( const void * pBlob, int size );

	// Own methods
	
	bool isEmpty() const;

	int size() const;

	bool hasDataFooters() const;

	void reliable( ReliableType currentMsgReliabile );
	bool isReliable() const;

	bool isCritical() const { return isCritical_; }

	bool isOnExternalChannel() const;

	INLINE void * qreserve( int nBytes );

	void reliableOrders( Packet * p,
		const ReliableOrder *& roBeg, const ReliableOrder *& roEnd );
	uint numReliableMessages() const { return numReliableMessages_; }

	void writeFlags( Packet * p ) const;

	bool piggyback( SeqNum seq, const ReliableVector & reliableOrders,
		Packet * p );

	Packet * pFirstPacket() const		{ return pFirstPacket_.get(); }

	Packet * preparePackets( UDPChannel * pChannel,
		SeqNumAllocator & seqNumAllocator,
		SendingStats & sendingStats,
		bool shouldUseChecksums );

	int addAck( SeqNum seq ) 
	{ 
		MF_ASSERT( ack_ == SEQ_NULL );
		ack_ = seq;
		return true;
	}

	SeqNum getAck() const
	{
		MF_ASSERT( ack_ != SEQ_NULL );
		return ack_;
	}

private:
	void * sreserve( int nBytes );
	void dispose();
	void startPacket( Packet * p );
	void endPacket( bool isExtending );
	void endMessage( bool isEarlyCall = false );
	char * newMessage( int extra = 0 );
	void addReliableOrder();

	Packet::Flags packetFlags() const { return pCurrentPacket_->flags(); }

	UDPChannel & udpChannel();

	const UDPChannel & udpChannel() const 
	{ 
		return const_cast< UDPBundle * >( this )->udpChannel(); 
	}

	UDPBundle( const UDPBundle & );
	UDPBundle & operator=( const UDPBundle & );

	// per bundle stuff
	PacketPtr pFirstPacket_;		///< The first packet in the bundle
	Packet * pCurrentPacket_;	///< The current packet in the bundle
	bool	hasEndedMsgEarly_;	///< True if the message has been ended
								/// early in piggyback()
	bool	reliableDriver_;	///< True if any driving reliable messages added
	uint8	extraSize_;			///< Size of extra bytes needed for e.g. filter

	/// This vector stores all the reliable messages for this bundle.
	ReliableVector	reliableOrders_;
	int				reliableOrdersExtracted_;

	/// If true, this UDPBundle's packets will be considered to be 'critical'
	/// by the Channel.
	bool			isCritical_;

	BundlePiggybacks piggybacks_;

	// Off channel acking, we only store one but we only need one
	SeqNum	ack_;

	// per message stuff
	InterfaceElement 	curIE_;
	int					msgLen_;
	int					msgExtra_;
	uint8 * 			msgBeg_;
	uint16				msgChunkOffset_;
	bool				msgIsReliable_;
	bool				msgIsRequest_;

	// Statistics
	uint	numReliableMessages_;
};

} // namespace Mercury

#ifdef CODE_INLINE
#include "udp_bundle.ipp"
#endif


BW_END_NAMESPACE


#endif // UDP_BUNDLE_HPP
