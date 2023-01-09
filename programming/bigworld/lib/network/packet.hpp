#ifndef PACKET_HPP
#define PACKET_HPP

#include "misc.hpp"
#include "cstdmf/binary_stream.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/smartpointer.hpp"

#define PACKET_MAX_SIZE 1472

BW_BEGIN_NAMESPACE

class Endpoint;

namespace Mercury
{

/**
 *  This type is used to track regions of data on a packet.  The packet type is
 *  fairly opaque (i.e. just a chunk of bytes in the data array) and without
 *  these things it's kinda hard to extract stuff out of the packet data after
 *  it has been streamed on.
 */
struct Field
{
	char	*beg_;
	uint16	len_;
};

class Packet;
typedef SmartPointer< Packet > PacketPtr;


/**
 *
 */
class PacketVisitor
{
public:
	virtual bool onPacket( PacketPtr pPacket ) = 0;
};


/**
 *	All packets look like this. Only the data is actually sent;
 *	the rest is just housekeeping.
 *
 *	@ingroup mercury
 */
class Packet : public ReferenceCount
{
public:
	// -------------------------------------------------------------------------
	// Section: Types and Constants
	// -------------------------------------------------------------------------

	/// The type of per-packet header flags.
	typedef uint16 Flags;

	enum
	{
		FLAG_HAS_REQUESTS			= 0x0001,
		FLAG_HAS_PIGGYBACKS			= 0x0002,
		FLAG_HAS_ACKS				= 0x0004,
		FLAG_ON_CHANNEL				= 0x0008,
		FLAG_IS_RELIABLE			= 0x0010,
		FLAG_IS_FRAGMENT			= 0x0020,
		FLAG_HAS_SEQUENCE_NUMBER	= 0x0040,
		FLAG_INDEXED_CHANNEL		= 0x0080,
		FLAG_HAS_CHECKSUM			= 0x0100,
		FLAG_CREATE_CHANNEL			= 0x0200,
		FLAG_HAS_CUMULATIVE_ACK		= 0x0400,
		KNOWN_FLAGS					= 0x07FF
	};

	/// The type of the ACK counter in the packet footers.
	typedef uint8 AckCount;

	/// The type of offsets relative to the start of the packet data.
	typedef uint16 Offset;

	/// The type of the custom checksum we do
	typedef uint32 Checksum;

private:
	/// The maximum payload size for a packet.  This is actually defined in the
	/// cpp to allow changing packet size without needing to edit hpps.  This is
	/// private to force the use of freeSpace(), thereby respecting footerSize_
	/// and extraFilterSize_.
	static const int MAX_SIZE;

public:
	///	The size of the header on a packet.
	static const int HEADER_SIZE = sizeof( Flags );

	/// The maximum number of ACKs that can fit on a single packet.
	static const int MAX_ACKS = (1 << (8 * sizeof( AckCount ))) - 1;

	/// The amount of space that is reserved for fixed-length footers on a
	/// packet.  This is done so that the bundle logic can always assume that
	/// these footers will fit and not have to worry about pre-allocating them.
	/// This is currently 27 bytes, roughly 1.5% of the capacity of a packet, so
	/// there's not too much wastage.
	static const int RESERVED_FOOTER_SIZE =
		sizeof( Offset ) + // FLAG_HAS_REQUESTS
		sizeof( AckCount ) + // FLAG_HAS_ACKS
		sizeof( SeqNum ) + // FLAG_HAS_SEQUENCE_NUMBER
		sizeof( SeqNum ) * 2 + // FLAG_IS_FRAGMENT
		sizeof( ChannelID ) + sizeof( ChannelVersion ) + // FLAG_INDEXED_CHANNEL
		sizeof( Checksum ); // FLAG_HAS_CHECKSUM

	// -------------------------------------------------------------------------
	// Section: Methods
	// -------------------------------------------------------------------------

public:
	Packet();
	~Packet();

	Packet * next()				{ return next_.get(); }
	const Packet * next() const	{ return next_.get(); }

	void chain( Packet * pPacket ) { next_ = pPacket; }
	int chainLength() const;

	Flags flags() const { return BW_NTOHS( *(Flags*)data_ ); }
	bool hasFlags( Flags flags ) const { return (this->flags() & flags) == flags; }
	void setFlags( Flags flags ) { *(Flags*)data_ = BW_HTONS( flags ); }
	void enableFlags( Flags flags ) { *(Flags*)data_ |= BW_HTONS( flags ); }
	void disableFlags( Flags flags ) { *(Flags*)data_ &= ~BW_HTONS( flags ); }

	char * data() { return data_; }
	const char * data() const { return data_; }

	/// Returns a pointer to the start of the message data.
	const char * body() const { return data_ + HEADER_SIZE; }

	/// Returns a pointer to the end of the message data.
	char * back() { return data_ + msgEndOffset_; }

	int msgEndOffset() const	{ return msgEndOffset_; }
	int bodySize() const		{ return msgEndOffset_ - HEADER_SIZE; }
	int footerSize() const		{ return footerSize_; }
	int totalSize() const		{ return msgEndOffset_ + footerSize_; }

	void msgEndOffset( int offset )		{ msgEndOffset_ = offset; }
	void grow( int nBytes )				{ msgEndOffset_ += nBytes; }
	void shrink( int nBytes )			{ msgEndOffset_ -= nBytes; }

	int freeSpace() const
	{
		return MAX_SIZE -
			RESERVED_FOOTER_SIZE -
			msgEndOffset_ -
			footerSize_ -
			extraFilterSize_;
	}

	void reserveFooter( int nBytes ) { footerSize_ += nBytes; }
	void reserveFilterSpace( int nBytes ) { extraFilterSize_ = nBytes; }

	void updateChannelVersion( ChannelVersion version, ChannelID channelID );
	void writeChecksum( Checksum * pChecksum );

	bool isPiggyback() const		{ return isPiggyback_; }
	void isPiggyback( bool value )	{ isPiggyback_ = value; }

	AckCount nAcks() const { return nAcks_; }
	AckCount & nAcks() { return nAcks_; }

	Field & piggyFooters() { return piggyFooters_; }

	SeqNum seq() const { return seq_; }
	SeqNum & seq() { return seq_; }

	ChannelID channelID() const { return channelID_; }
	ChannelID & channelID() { return channelID_; }

	ChannelVersion channelVersion() const { return channelVersion_; }
	ChannelVersion & channelVersion() { return channelVersion_; }

	Offset firstRequestOffset() const { return firstRequestOffset_; }
	Offset & firstRequestOffset() { return firstRequestOffset_; }
	void addRequest( Offset messageStart, Offset nextRequestLink );

	SeqNum fragBegin() const { return fragBegin_; }
	SeqNum fragEnd() const { return fragEnd_; }

	bool stripFragInfo();

	bool validateChecksum();

	bool processPiggybackPackets( PacketVisitor & visitor );

	bool shouldCreateAnonymous() const
	{
		// Must be the first packet on a non-indexed channel.
		return this->hasFlags( FLAG_ON_CHANNEL ) &&
			this->hasFlags( FLAG_CREATE_CHANNEL ) &&
			!this->hasFlags( FLAG_INDEXED_CHANNEL );
	}

	/**
	 *  This method strips a footer off the back of this packet.  If the use of
	 *  a switch is a performance issue, this can be reimplemented as a macro.
	 *  It returns true if there was enough data on the packet for the footer.
	 */
	template <class TYPE>
	bool stripFooter( TYPE & value )
	{
		if (this->bodySize() < int( sizeof( TYPE ) ))
		{
			return false;
		}

		msgEndOffset_ -= sizeof( TYPE );
		footerSize_ += sizeof( TYPE );

		switch( sizeof( TYPE ) )
		{
			case sizeof( uint8 ):
				value = TYPE( *(TYPE*)this->back() ); break;

			case sizeof( uint16 ):
				value = TYPE( BW_NTOHS( *(TYPE*)this->back() ) ); break;

			case sizeof( uint32 ):
				value = TYPE( BW_NTOHL( *(TYPE*)this->back() ) ); break;

			default:
				CRITICAL_MSG( "Footers of size %" PRIzu " aren't supported",
					sizeof( TYPE ) );
		}

		return true;
	}


	/**
	 *  This method writes a footer to the back of this packet.  It should only
	 *  be called from NetworkInterface::send() and assumes that size_ has been
	 *  artificially increased so that it points to the end of the footers, the
	 *  idea being that we work back towards the real body end.
	 */
	template <class TYPE>
	void packFooter( TYPE value )
	{
		msgEndOffset_ -= sizeof( TYPE );

		switch( sizeof( TYPE ) )
		{
			case sizeof( uint8 ):
				*(TYPE*)this->back() = value; break;

			case sizeof( uint16 ):
				*(TYPE*)this->back() = BW_HTONS( value ); break;

			case sizeof( uint32 ):
				*(TYPE*)this->back() = BW_HTONL( value ); break;

			default:
				CRITICAL_MSG( "Footers of size %" PRIzu " aren't supported",
					sizeof( TYPE ) );
		}
	}


	int recvFromEndpoint( Endpoint & ep, Address & addr );

	// -------------------------------------------------------------------------
	// Section: Static methods
	// -------------------------------------------------------------------------

	/// Packets can be streamed in one of three states
	enum
	{
		/// Packets in a Channel's unackedPackets_
		UNACKED_SEND,

		/// Packets in a Channel's bufferedReceives_
		BUFFERED_RECEIVE,

		/// Packets in a Channel's pFragments_
		CHAINED_FRAGMENT
	};

	static void addToStream( BinaryOStream & data, const Packet * pPacket,
		int state );

	static PacketPtr createFromStream( BinaryIStream & data, int state );

	/**
	 *  This method returns the maximum possible body size for a single packet.
	 *  It does not take into account packetFilter reservations or footers, so
	 *  should only be used as a rough guide for payload size.
	 */
	static int maxCapacity()
	{
		return MAX_SIZE - HEADER_SIZE - RESERVED_FOOTER_SIZE;
	}

	void debugDump() const;

	// -------------------------------------------------------------------------
	// Section: Fields
	// -------------------------------------------------------------------------

private:
	/// Packets are linked together in a simple linked list fashion.
	PacketPtr	next_;

	/// This the offset of the end of the headers and message data. It is
	/// temporarily incorrect in two situations: when sending, it is incorrect
	/// in NetworkInterface::send() whilst footers are being written, and when
	/// receiving, it  is incorrect until processOrderedPacket() strips the
	/// fragment footers.
	int			msgEndOffset_;

	/// The number of bytes that have been reserved for footers.
	int			footerSize_;

	/// The number of bytes reserved for the PacketFilter.  This is only
	/// maintained for packets we are writing to.
	int			extraFilterSize_;

	/// The offset of the first request.
	Offset		firstRequestOffset_;

	/// The address of the 'next request offset' field of the last request
	/// written to this packet.
	Offset*		pLastRequestOffset_;

	/// The number of ACKs on this packet.
	AckCount	nAcks_;

	/// Piggyback footers, if FLAG_HAS_PIGGYBACKS is set
	Field		piggyFooters_;

	/// Sequence number, or Channel::SEQ_NULL if not set
	SeqNum		seq_;

	/// Channel ID, or CHANNEL_ID_NULL if not set
	ChannelID	channelID_;

	/// Channel version, or 0 if not set
	ChannelVersion channelVersion_;

	/// Fragment begin and end sequence numbers, or Channel::SEQ_NULL if this
	/// packet isn't a fragment.
	SeqNum		fragBegin_;
	SeqNum		fragEnd_;

	bool	isPiggyback_;

	/// Checksum
	Checksum	checksum_;

	/// The variable-length data follows the packet header in memory.
	char			data_[PACKET_MAX_SIZE];
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // PACKET_HPP
