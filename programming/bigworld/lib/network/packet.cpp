#include "pch.hpp"
#include "packet.hpp"
#include "endpoint.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/watcher.hpp"

DECLARE_DEBUG_COMPONENT2( "Network", 0 );

BW_BEGIN_NAMESPACE

namespace Mercury
{

namespace
{

int g_currentPackets = 0;
int g_maxPackets = 0;

#if ENABLE_WATCHERS

class PacketWatcherIniter
{
public:
	PacketWatcherIniter()
	{
		MF_WATCH( "network/packetsCurrent", g_currentPackets );
		MF_WATCH( "network/packetsMax", g_maxPackets );
	}
};

PacketWatcherIniter s_packetWatcherIniter_;

#endif // ENABLE_WATCHERS

} // anonymous

// -----------------------------------------------------------------------------
// Section: Packet
// -----------------------------------------------------------------------------

// The default max size for a packet is the MTU of an ethernet frame, minus the
// overhead of IP and UDP headers.  If you have special requirements for packet
// sizes (e.g. your client/server connection is running over VPN) you can edit
// this to whatever you need.
const int Packet::MAX_SIZE = PACKET_MAX_SIZE;


/**
 *  Constructor.
 */
Packet::Packet() :
	next_( NULL ),
	msgEndOffset_( 0 ),
	footerSize_( 0 ),
	extraFilterSize_( 0 ),
	firstRequestOffset_( 0 ),
	pLastRequestOffset_( NULL ),
	nAcks_( 0 ),
	seq_( SEQ_NULL ),
	channelID_( CHANNEL_ID_NULL ),
	channelVersion_( 0 ),
	fragBegin_( SEQ_NULL ),
	fragEnd_( SEQ_NULL ),
	isPiggyback_( false ),
	checksum_( 0 )
{
	piggyFooters_.beg_ = NULL;

	++g_currentPackets;
	g_maxPackets = std::max( g_maxPackets, g_currentPackets );
}


/**
 *	Destructor.
 */
Packet::~Packet()
{
	--g_currentPackets;
}


/**
 *  This method returns the total length of this packet chain.
 */
int Packet::chainLength() const
{
	int count = 1;

	for (const Packet * p = this->next(); p != NULL; p = p->next())
	{
		++count;
	}

	return count;
}


/**
 *  This method is called to inform the Packet that a new request has been
 *  added.  It updates the 'next request offset' linkage as necessary.  The
 *  value passed in is the offset of the message header.
 */
void Packet::addRequest( Offset messageStart, Offset nextRequestLink )
{
	if (firstRequestOffset_ == 0)
	{
		firstRequestOffset_ = messageStart;
	}
	else
	{
		*pLastRequestOffset_ = BW_HTONS( messageStart );
	}

	// Remember the offset of this link for next time.
	pLastRequestOffset_ = (Offset*)(data_ + nextRequestLink);

	// Mark this request as the last one on this packet (for now).
	*pLastRequestOffset_ = 0;
}


/**
 *
 */
bool Packet::stripFragInfo()
{
	if (this->bodySize() < int( sizeof( SeqNum ) * 2 ))
	{
		WARNING_MSG( "Packet::stripFragInfo: Not enough footers for fragment "
				"spec (have %d bytes but need %" PRIzu ")\n",
			this->bodySize(), 2 * sizeof( SeqNum ) );

		return false;
	}

	// Take off the fragment sequence numbers.
	this->stripFooter( fragEnd_ );
	this->stripFooter( fragBegin_ );

	const int numFragmentsInBundle = fragEnd_ - fragBegin_ + 1;

	// TODO: Consider a maximum number of fragments per packet,
	// smaller than this (about 2^31 :)
	if (numFragmentsInBundle < 2)
	{
		WARNING_MSG( "Packet::stripFragInfo: Illegal fragment count (%d)\n",
			numFragmentsInBundle );

		return false;
	}

	if (seq_ < fragBegin_ || seq_ > fragEnd_)
	{
		WARNING_MSG( "Packet::stripFragInfo: "
				"Fragment range [#%u,#%u] does not include "
				"packet's sequence #%u\n",
			fragBegin_, fragEnd_, seq_ );

		return false;
	}

	return true;
}


/**
 *  This method does a recv on the endpoint into this packet's data array,
 *  setting the length correctly on a successful receive.  The return value is
 *  the return value from the low-level recv() call.
 */
int Packet::recvFromEndpoint( Endpoint & ep, Address & addr )
{
	int len = ep.recvfrom( data_, MAX_SIZE,
		(u_int16_t*)&addr.port, (u_int32_t*)&addr.ip );

	if (len >= 0)
	{
		this->msgEndOffset( len );
	}

	return len;
}


/**
 *  This method writes this packet to the provided stream.  This is used when
 *  offloading entity channels and the buffered and unacked packets need to be
 *  streamed too.  Packets are streamed slightly differently depending on
 *  whether they were buffered receives or unacked sends.
 */
void Packet::addToStream( BinaryOStream & data, const Packet * pPacket,
	int state )
{
	data << uint8( pPacket != NULL );

	if (pPacket)
	{
		// Unacked sends need to have the entirety of the packet data included.
		if (state == UNACKED_SEND)
		{
			data.appendString( pPacket->data(), pPacket->totalSize() );
			data << int32( pPacket->footerSize() );
		}

		// Buffered receives and chained fragments should only have the
		// unprocessed part of their data included.
		else
		{
			data.appendString( pPacket->data(), pPacket->msgEndOffset() );
			// footerSize
			data << int32( 0 );
		}

		data << pPacket->seq() << pPacket->channelID();

		// Chained fragments need to have the fragment IDs and first request
		// offset sent too
		if (state == CHAINED_FRAGMENT)
		{
			data << pPacket->fragBegin() << pPacket->fragEnd() <<
				pPacket->firstRequestOffset();
		}
	}
}


/**
 *  This method reconstructs a packet from a stream.
 */
PacketPtr Packet::createFromStream( BinaryIStream & data, int state )
{
	uint8 hasPacket;
	data >> hasPacket;

	if (!hasPacket)
		return NULL;

	PacketPtr pPacket = new Packet();
	int length = data.readStringLength();

	if ((length < 0) || (data.remainingLength() < length) ||
			(PACKET_MAX_SIZE < length))
	{
		CRITICAL_MSG( "Packet::createFromStream: Invalid length %d\n",
				length );
		return NULL;
	}

	memcpy( pPacket->data_, data.retrieve( length ), length );
	int32 footerSize;
	data >> footerSize;
	MF_ASSERT( length > footerSize );
	pPacket->msgEndOffset( length - footerSize );
	pPacket->footerSize_ = footerSize;
	data >> pPacket->seq() >> pPacket->channelID();

	// Chained fragments have more footers
	if (state == CHAINED_FRAGMENT)
	{
		data >> pPacket->fragBegin_ >> pPacket->fragEnd_ >>
			pPacket->firstRequestOffset();
	}

	return pPacket;
}


/**
 *	This method strips and validates the checksum on this packet.
 *
 *	@return true is successful.
 */
bool Packet::validateChecksum()
{
	if (this->hasFlags( Packet::FLAG_HAS_CHECKSUM ))
	{
		// Strip checksum and verify correctness
		if (!this->stripFooter( checksum_ ))
		{
			WARNING_MSG( "Packet::validateChecksum: "
					"Packet too short (%d bytes) for checksum!\n",
				this->totalSize() );

			return false;
		}

		// Zero data in checksum field on packet to avoid padding issues
		*(Packet::Checksum*)this->back() = 0;

		// Calculate correct checksum for packet
		Packet::Checksum sum = 0;

		for (const Packet::Checksum * pData = (Packet::Checksum*)this->data();
			 pData < (Packet::Checksum*)this->back(); pData++)
		{
			sum ^= BW_NTOHL( *pData );
		}

		// Put the checksum back on the stream in case this packet is forwarded
		// on.
		*(Packet::Checksum*)this->back() = BW_HTONL( checksum_ );

		if (sum != checksum_)
		{
			ERROR_MSG( "Packet::validateChecksum: "
					"Packet (flags %hx, size %d) failed checksum "
					"(wanted %08x, got %08x)\n",
				this->flags(), this->totalSize(), sum,
				checksum_ );

			return false;
		}
	}

	return true;
}


/**
 *	This method extracts and visits any piggybacked packets that are on this
 *	packet.
 */
bool Packet::processPiggybackPackets( PacketVisitor & visitor )
{
	if (!this->hasFlags( FLAG_HAS_PIGGYBACKS ))
	{
		return true;
	}

	bool done = false;

	while (!done)
	{
		int16 len;

		if (!this->stripFooter( len ))
		{
			WARNING_MSG( "Packet::processPiggybackPackets: "
					"Not enough data for piggyback length (%d bytes left)\n",
				this->bodySize() );

			return false;
		}

		// The last piggyback on a packet has a negative length.
		if (len < 0)
		{
			len = ~len;
			done = true;
		}

		// Check there's enough space on the packet for this
		if (this->bodySize() < len)
		{
			WARNING_MSG( "Packet::processPiggybackPackets: "
					"Packet too small to contain piggyback message of "
					"length %d (only %d bytes remaining)\n",
				len, this->bodySize() );

			return false;
		}

		// Create piggyback packet and handle it
		this->shrink( len );

		PacketPtr pPiggybackPacket = new Packet();
		memcpy( pPiggybackPacket->data(), this->back(), len );
		pPiggybackPacket->msgEndOffset( len );
		pPiggybackPacket->isPiggyback( true );

		if (!visitor.onPacket( pPiggybackPacket ))
		{
			return false;
		}
	}

	return true;
}


/**
 *	This method updates the channel version of a packet being resent.
 */
void Packet::updateChannelVersion( ChannelVersion version,
	 ChannelID channelID )
{
	if (channelVersion_ == version)
		return;

	if (!this->hasFlags( FLAG_INDEXED_CHANNEL ))
	{
		return;
	}

	char * pCurr = data_ + msgEndOffset_ + footerSize_;

	Checksum * pChecksum = NULL;

	if (this->hasFlags( FLAG_HAS_CHECKSUM ))
	{
		pCurr -= sizeof( Checksum );
		pChecksum = reinterpret_cast< Checksum * >( pCurr );
	}

	MF_ASSERT( !this->hasFlags( FLAG_HAS_PIGGYBACKS ) );

	pCurr -= sizeof( ChannelID );
	ChannelID id = *reinterpret_cast< ChannelID * >( pCurr );

	MF_ASSERT( id == channelID );

	pCurr -= sizeof( ChannelVersion );
	ChannelVersion * pVersion = reinterpret_cast< ChannelVersion * >( pCurr );

	channelVersion_ = version;
	*pVersion = version;

	this->writeChecksum( pChecksum );
}


/**
 *	This method writes the checksum into the packet.
 *
 *	@param pChecksum The pointer to the location of the checksum in the packet's
 *		data. Everything up to this location is included in the checksum.
 */
void Packet::writeChecksum( Checksum * pChecksum )
{
	if (pChecksum == NULL)
	{
		return;
	}

	MF_ASSERT( (char *)pChecksum - data_ < PACKET_MAX_SIZE );
	MF_ASSERT( this->hasFlags( Packet::FLAG_HAS_CHECKSUM ) );

	// Calculate the checksum and write it in.  We don't have to worry
	// about padding issues here because we have written a 0 into the
	// checksum field, and any overrun will be read from that field.
	*pChecksum = 0;

	Packet::Checksum sum = 0;

	for (Packet::Checksum * pData = (Packet::Checksum *)this->data();
		 pData < pChecksum; pData++)
	{
		sum ^= BW_NTOHL( *pData );
	}

	*pChecksum = BW_HTONL( sum );
}



/**
 *	This method dumps the packets contents to log output.
 */
void Packet::debugDump() const
{
	DEBUG_MSG( "Packet length is %d\n", this->totalSize() );

	int lineSize = 1024;
	char line[ 1024 ];
	char * s = line;

	for (long i=0; i < this->totalSize(); i++)
	{
		bw_snprintf( s, lineSize, "%02x ",
				(unsigned char)this->data()[i] ); s += 3;
		lineSize -= 3;
		if (i > 0 && i % 20 == 19)
		{
			DEBUG_MSG( "%s\n", line );
			s = line;
			lineSize = 1024;
		}
	}

	if (s != line)
	{
		DEBUG_MSG( "%s\n", line );
	}
}

} // namespace Mercury

BW_END_NAMESPACE

// packet.cpp
