#include "pch.hpp"

#include "udp_bundle.hpp"


#include "bundle_piggyback.hpp"
#include "interface_element.hpp"
#include "interface_table.hpp"
#include "network_interface.hpp"
#include "nub_exception.hpp"
#include "process_socket_stats_helper.hpp"
#include "request_manager.hpp"
#include "udp_channel.hpp"

#include "cstdmf/concurrency.hpp"
#include "cstdmf/memory_stream.hpp"

DECLARE_DEBUG_COMPONENT2( "Network", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "udp_bundle.ipp"
#endif


namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: UDPBundle
// -----------------------------------------------------------------------------

/*
How requests and replies work (so I can get it straight):

When you make a request you put it on the bundle with a 'startRequest' message.
This means the bundle takes note of it and puts extra information (a reply id)
in the message's header.

When a request handler replies to a request, it puts it on the bundle with a
'startReply' message, passing in the replyID from the broken-out header info
struct passed to it. This means the bundle adds the special message of type
'REPLY_MESSAGE_IDENTIFIER', which is always handled by the system.
*/


/**
 * 	This constructor initialises an empty bundle for writing.
 */
UDPBundle::UDPBundle( uint8 spareSize, UDPChannel * pChannel ) :
	Bundle( pChannel ),
	pFirstPacket_( NULL ),
	pCurrentPacket_( NULL ),
	hasEndedMsgEarly_( false ),
	reliableDriver_( false ),
	extraSize_( spareSize ),
	reliableOrders_(),
	reliableOrdersExtracted_( 0 ),
	isCritical_( false ),
	piggybacks_(),
	ack_( SEQ_NULL ),
	curIE_( NULL ),
	msgLen_( 0 ),
	msgExtra_( 0 ),
	msgBeg_( NULL ),
	msgChunkOffset_( 0 ),
	msgIsReliable_( false ),
	msgIsRequest_( false ),
	numReliableMessages_( 0 )
{
	this->clear( /* newBundle: */ true );
}


/**
 * 	This constructor initialises an empty bundle for writing.
 */
UDPBundle::UDPBundle( Packet * p ) :
	Bundle( NULL ),
	pFirstPacket_( p ),
	pCurrentPacket_( p ),
	hasEndedMsgEarly_( false ),
	reliableDriver_( false ),
	extraSize_( 0 ),
	reliableOrders_(),
	reliableOrdersExtracted_( 0 ),
	isCritical_( false ),
	piggybacks_(),
	ack_( SEQ_NULL ),
	curIE_( NULL ),
	msgLen_( 0 ),
	msgExtra_( 0 ),
	msgBeg_( NULL ),
	msgChunkOffset_( 0 ),
	msgIsReliable_( false ),
	msgIsRequest_( false ),
	numReliableMessages_( 0 )
{
	this->clear( /* newBundle: */ true );
}


/**
 * 	This is the destructor. It releases the packets used by this bundle.
 */
UDPBundle::~UDPBundle()
{
	this->dispose();
}

/**
 * 	This method flushes the messages from this bundle making it empty.
 */
void UDPBundle::clear( bool newBundle )
{
	this->Bundle::clear( newBundle );

	// If this isn't the first time, then we need to flush everything.
	if (!newBundle)
	{
		this->dispose();
		hasEndedMsgEarly_ = false;
	}

	reliableDriver_ = false;
	// extraSize_ set in constructors
	reliableOrdersExtracted_ = 0;
	// pChannel_ set in constructors
	isCritical_ = false;
	curIE_ = NULL;
	msgLen_ = 0;
	msgBeg_ = NULL;
	msgChunkOffset_ = 0;
	msgIsReliable_ = false;
	msgIsRequest_ = false;
	numReliableMessages_ = 0;
	ack_ = SEQ_NULL;

	// If we have a packet, it means we're being called from 
	// UDPBundle( Packet * ) so we shouldn't touch it.
	if (pFirstPacket_ == NULL)
	{
		pFirstPacket_ = new Packet();
		this->startPacket( pFirstPacket_.get() );
	}
}




/**
 * 	This method releases all memory used by the UDPBundle.
 */
void UDPBundle::dispose()
{
	pFirstPacket_ = NULL;
	pCurrentPacket_ = NULL;

	replyOrders_.clear();

	reliableOrders_.clear();

	for (BundlePiggybacks::iterator iter = piggybacks_.begin();
		 iter != piggybacks_.end(); ++iter)
	{
		bw_safe_delete( *iter );
	}

	piggybacks_.clear();
}


/**
 * 	This method returns true if the bundle is empty of messages or any
 * 	data-carrying footers.
 */
bool UDPBundle::isEmpty() const
{
	// We check isReliable() because that indicates whether or not a sequence
	// number will be streamed onto this bundle during
	// NetworkInterface::send().
	bool hasData =
		numMessages_ > 0 ||
		this->hasMultipleDataUnits() ||
		this->isReliable() ||
		this->hasDataFooters() ||
		ack_ != SEQ_NULL;

	return !hasData;
}


/**
 * 	This method returns the accumulated size of the bundle (including
 * 	headers, and including footers if it's been sent).
 *
 * 	@return	Number of bytes in this bundle.
 */
int UDPBundle::size() const
{
	int	total = 0;

	for (const Packet * p = pFirstPacket_.get(); p; p = p->next())
	{
		total += p->totalSize();
	}

	return total;
}


/*
 * Override from Bundle.
 */
int UDPBundle::numDataUnits() const
{
	return pFirstPacket_->chainLength();
}


/**
 * 	This method starts a new message on the bundle.
 *
 * 	@param ie			The type of message to start.
 * 	@param reliable		True if the message should be reliable.
 */
void UDPBundle::startMessage( const InterfaceElement & ie, 
		ReliableType reliable )
{
	// Piggybacks should only be added immediately before sending.
	MF_ASSERT( !pCurrentPacket_->hasFlags( Packet::FLAG_HAS_PIGGYBACKS ) );
	MF_ASSERT( ie.name() );

	this->endMessage();
	curIE_ = ie;
	msgIsReliable_ = reliable.isReliable();
	msgIsRequest_ = false;
	isCritical_ = (reliable == RELIABLE_CRITICAL);
	this->newMessage();

	reliableDriver_ |= reliable.isDriver();
}


/**
 * 	This method starts a new request message on the bundle, and call
 * 	ReplyMessageHandler when the reply comes in or the
 * 	timeout (in microseconds) expires, whichever comes first.
 * 	A timeout of <= 0 means never time out (NOT recommended)
 *
 * 	@param ie			The type of request to start.
 * 	@param handler		This handler receives the reply.
 * 	@param arg			User argument that is sent to the handler.
 * 	@param timeout		Time before a timeout exception is generated.
 * 	@param reliable		True if the message should be reliable.
 */
void UDPBundle::startRequest( const InterfaceElement & ie,
	ReplyMessageHandler * handler,
	void * arg,
	int timeout,
	ReliableType reliable )
{
	MF_ASSERT( handler );

	if (pChannel_ && timeout != DEFAULT_REQUEST_TIMEOUT)
	{
		// Requests never timeout on channels.
		WARNING_MSG( "UDPBundle::startRequest(%s): "
				"Non-default timeout set on a channel bundle\n",
			pChannel_->c_str() );
	}

	this->endMessage();
	curIE_ = ie;
	msgIsReliable_ = reliable.isReliable();
	msgIsRequest_ = true;
	isCritical_ = (reliable == RELIABLE_CRITICAL);

	// Start a new message, and reserve extra space for the reply ID and the
	// next request offset.  The reply ID is actually written in
	// NetworkInterface::send().
	ReplyID * pReplyID = (ReplyID *)this->newMessage(
		sizeof( ReplyID ) + sizeof( Packet::Offset ) );

	Packet::Offset messageStart = 
		Packet::Offset( pCurrentPacket_->msgEndOffset() -
			(ie.headerSize() +
				sizeof( ReplyID ) + 
				sizeof( Packet::Offset )));
	Packet::Offset nextRequestLink = 
			Packet::Offset( pCurrentPacket_->msgEndOffset() -
				sizeof( Packet::Offset ) );

	// Update the request tracking stuff on the current packet.
	pCurrentPacket_->addRequest( messageStart, nextRequestLink );

	// now make and add a reply order
	ReplyOrder	ro = {handler, arg, timeout, pReplyID};
	replyOrders_.push_back(ro);
		// it'd be nice to eliminate this unnecessary copy...

	// this packet has requests
	pCurrentPacket_->enableFlags( Packet::FLAG_HAS_REQUESTS );

	reliableDriver_ |= reliable.isDriver();
}

/**
 * 	This method starts a reply to a request message. All replies are 4-byte
 * 	variable size. The first parameter, id, should be the replyID
 * 	from the message header of the request you're replying to.
 *
 * 	@param id			The id of the message being replied to
 * 	@param reliable		True if this reply should be reliable.
 */
void UDPBundle::startReply( ReplyID id, ReliableType reliable )
{
	this->endMessage();
	curIE_ = InterfaceElement::REPLY;
	msgIsReliable_ = reliable.isReliable();
	msgIsRequest_ = false;
	isCritical_ = (reliable == RELIABLE_CRITICAL);
	this->newMessage();

	reliableDriver_ |= reliable.isDriver();

	// stream on the id (counts as part of the length)
	(*this) << id;
}


/**
 *  This method returns true if this UDPBundle is owned by an external channel.
 */
bool UDPBundle::isOnExternalChannel() const
{
	return pChannel_ && this->udpChannel().isExternal();
}


/**
 *  This function returns a pointer to nBytes on a bundle.
 *  It assumes that the data will not fit in the current packet,
 *  so it adds a new one. This is a private function.
 *
 *  @param nBytes	Number of bytes to reserve.
 *
 *  @return	Pointer to the reserved data.
 */
void * UDPBundle::sreserve( int nBytes )
{
	this->endPacket( /* isExtending */ true );
	this->startPacket( new Packet() );

	void * writePosition = pCurrentPacket_->back();
	pCurrentPacket_->grow( nBytes );

	MF_ASSERT( pCurrentPacket_->freeSpace() >= 0 );
	return writePosition;
}


/*
 *	Override from Bundle.
 */
void UDPBundle::doFinalise()
{
	// make sure we're not sending a packet where the message
	// wasn't properly started...
	if (msgBeg_ == NULL && pCurrentPacket_->msgEndOffset() != msgChunkOffset_)
	{
		CRITICAL_MSG( "UDPBundle::finalise: "
			"data not part of message found at end of bundle!\n");
	}

	this->endMessage();
	this->endPacket( /* isExtending */ false );

	// if we don't have a reliable driver then any reliable orders present
	// are all passengers (hangers on), so get rid of them.
	if (!reliableDriver_ && this->isOnExternalChannel())
	{
		reliableOrders_.clear();
	}
}


/**
 *	This method prepares packets this bundle for sending.
 *
 *	@param pChannel			The channel, or NULL for off-channel sending.
 *	@param seqNumAllocator 	The network interface's sequence number allocator,
 *							used for off-channel sending.
 *	@param sendingStats 	The sending stats to update.
 */
Packet * UDPBundle::preparePackets( UDPChannel * pChannel,
		SeqNumAllocator & seqNumAllocator,
		SendingStats & sendingStats,
		bool shouldUseChecksums )
{
	// fill in all the footers that are left to us
	Packet * pFirstOverflowPacket = NULL;

	int	numPackets = this->numDataUnits();
	SeqNum firstSeq = 0;
	SeqNum lastSeq = 0;

	// Write footers for each packet.
	for (Packet * pPacket = this->pFirstPacket();
			pPacket;
			pPacket = pPacket->next())
	{
		MF_ASSERT( pPacket->msgEndOffset() >= Packet::HEADER_SIZE );

		if (shouldUseChecksums)
		{
			// Reserve space for the checksum footer

			MF_ASSERT( !pPacket->hasFlags( Packet::FLAG_HAS_CHECKSUM ) );
			pPacket->reserveFooter( sizeof( Packet::Checksum ) );
			pPacket->enableFlags( Packet::FLAG_HAS_CHECKSUM );
		}

		this->writeFlags( pPacket );

		if (pChannel)
		{
			pChannel->writeFlags( pPacket );
		}

		if ((pChannel && pChannel->isExternal()) ||  
			pPacket->hasFlags( Packet::FLAG_IS_RELIABLE ) || 
			pPacket->hasFlags( Packet::FLAG_IS_FRAGMENT )) 
		{ 
			pPacket->reserveFooter( sizeof( SeqNum ) ); 
			pPacket->enableFlags( Packet::FLAG_HAS_SEQUENCE_NUMBER ); 
		} 

		// At this point, pPacket->back() is positioned just after the message
		// data, so we advance it to the end of where the footers end, then
		// write backwards towards the message data. We check that we finish
		// up back at the message data as a sanity check.
		const int msgEndOffset = pPacket->msgEndOffset();
		pPacket->grow( pPacket->footerSize() );

		// Pack in a zero checksum.  We'll calculate it later.
		Packet::Checksum * pChecksum = NULL;

		if (pPacket->hasFlags( Packet::FLAG_HAS_CHECKSUM ))
		{
			pPacket->packFooter( Packet::Checksum( 0 ) );
			pChecksum = (Packet::Checksum*)pPacket->back();
		}

		// Write piggyback info.  Note that we should only ever do this to
		// the last packet in a bundle as it should be the only packet with
		// piggybacks on it.  Piggybacks go first so that they can be
		// stripped and the rest of the packet can be dealt with as normal.
		if (pPacket->hasFlags( Packet::FLAG_HAS_PIGGYBACKS ))
		{
			MF_ASSERT( pPacket->next() == NULL );

			int16 * lastLen = NULL;

			// Remember where the end of the piggybacks is for setting up
			// the piggyFooters Field after all the piggies have been
			// streamed on
			int backPiggyOffset = pPacket->msgEndOffset();

			for (BundlePiggybacks::const_iterator it = piggybacks_.begin();
					it != piggybacks_.end();
					++it)
			{
				const BundlePiggyback &pb = **it;

				// Stream on the length first
				pPacket->packFooter( pb.len_ );
				lastLen = (int16*)pPacket->back();

				// Reserve the area for the packet data
				pPacket->shrink( pb.len_ );
				char * pdata = pPacket->back();

				// Stream on the packet header
				*(Packet::Flags*)pdata = BW_HTONS( pb.flags_ );
				pdata += sizeof( Packet::Flags );

				// Stream on the reliable messages
				for (ReliableVector::const_iterator rvit = pb.rvec_.begin();
					 rvit != pb.rvec_.end(); ++rvit)
				{
					memcpy( pdata, rvit->segBegin, rvit->segLength );
					pdata += rvit->segLength;
				}

				// Stream on sequence number footer
				*(SeqNum*)pdata = BW_HTONL( pb.seq_ );
				pdata += sizeof( SeqNum );

				// Stream on any sub-piggybacks that were lost
				if (pb.flags_ & Packet::FLAG_HAS_PIGGYBACKS)
				{
					const Field & subPiggies = pb.pPacket_->piggyFooters();
					memcpy( pdata, subPiggies.beg_, subPiggies.len_ );
					pdata += subPiggies.len_;
				}

				// Sanity check
				MF_ASSERT( pdata == (char*)lastLen );

				++sendingStats.numPiggybacks_;
				sendingStats.numBytesPiggybacked_ += pb.len_;
				sendingStats.numBytesNotPiggybacked_ +=
					pb.pPacket_->totalSize() - pb.len_;
			}

			// One's complement the length of the last piggyback to indicate
			// that it's the last one.
			*lastLen = BW_HTONS( ~BW_NTOHS( *lastLen ) );

			// Finish setting up the piggyFooters field for this packet.
			// We'll need this if this packet is lost and we need to
			// piggyback it onto another packet later on.
			pPacket->piggyFooters().beg_ = pPacket->back();
			pPacket->piggyFooters().len_ =
				uint16( backPiggyOffset - pPacket->msgEndOffset() );
		}

		if (pChannel)
		{
			pChannel->writeFooter( pPacket );
		}
		else if (pPacket->hasFlags( Packet::FLAG_HAS_ACKS ))
		{
			// For once off reliable, we only ever have one ack.
			pPacket->packFooter( (Packet::AckCount) 1 );
			pPacket->packFooter( this->getAck() );
		}

		// Add the sequence number
		if (pPacket->hasFlags( Packet::FLAG_HAS_SEQUENCE_NUMBER ))
		{
			// If we're sending reliable traffic on a channel, use the
			// channel's sequence numbers.  Otherwise use the nub's.
			pPacket->seq() =
				(pChannel && pPacket->hasFlags( Packet::FLAG_IS_RELIABLE )) ?
					pChannel->useNextSequenceID() :
					seqNumAllocator.getNext();
			
			pPacket->packFooter( pPacket->seq() );

			if (pPacket == pFirstPacket_)
			{
				firstSeq = pPacket->seq();
				lastSeq = pPacket->seq() + numPackets - 1;
			}
		}

		// Add the first request offset.
		if (pPacket->hasFlags( Packet::FLAG_HAS_REQUESTS ))
		{
			pPacket->packFooter( pPacket->firstRequestOffset() );
		}

		// add the fragment info
		if (pPacket->hasFlags( Packet::FLAG_IS_FRAGMENT ))
		{
			pPacket->packFooter( lastSeq );
			pPacket->packFooter( firstSeq );
		}

		// Make sure writing all the footers brought us back to the end of
		// the message data.
		MF_ASSERT( pPacket->msgEndOffset() == msgEndOffset );

		pPacket->writeChecksum( pChecksum );

		// set up the reliable machinery
		if (pPacket->hasFlags( Packet::FLAG_IS_RELIABLE ))
		{
			if (pChannel)
			{
				const ReliableOrder *roBeg, *roEnd;

				if (pChannel->isInternal())
				{
					roBeg = roEnd = NULL;
				}
				else
				{
					this->reliableOrders( pPacket, roBeg, roEnd );
				}

				if (!pChannel->addResendTimer( pPacket->seq(), pPacket, 
						roBeg, roEnd ))
				{
					if (pFirstOverflowPacket == NULL)
					{
						pFirstOverflowPacket = pPacket;
					}
					// return REASON_WINDOW_OVERFLOW;
				}
				else
				{
					MF_ASSERT( pFirstOverflowPacket == NULL );
				}
			}
		}
	}

	return pFirstOverflowPacket;
}


/**
 *  Set up the flags on a packet in accordance with the bundle's information.
 */
void UDPBundle::writeFlags( Packet * p ) const
{
	// msgIsReliable_ is only set here if there are no msgs (only footers) on
	// the bundle, but the setter wants to indicate that it should still be
	// reliable

	if (reliableOrders_.size() || msgIsReliable_ || numReliableMessages_ > 0)
	{
		p->enableFlags( Packet::FLAG_IS_RELIABLE );
	}

	if (p->hasFlags( Packet::FLAG_HAS_REQUESTS ))
	{
		p->reserveFooter( sizeof( Packet::Offset ) );
	}

	if (this->hasMultipleDataUnits())
	{
		p->enableFlags( Packet::FLAG_IS_FRAGMENT );
		p->reserveFooter( sizeof( SeqNum ) * 2 );
	}

	if (ack_ != SEQ_NULL)
	{
		p->enableFlags( Packet::FLAG_HAS_ACKS );
		p->reserveFooter( sizeof( Packet::AckCount ) + 
						  sizeof( SeqNum ) );
	}
}


/**
 *  This method starts a new packet in this bundle.
 */
void UDPBundle::startPacket( Packet * p )
{
	Packet * prevPacket = pCurrentPacket_;

	// Link the new packet into the chain if necessary.
	if (prevPacket)
	{
		prevPacket->chain( p );
	}

	pCurrentPacket_ = p;
	pCurrentPacket_->reserveFilterSpace( extraSize_ );

	pCurrentPacket_->setFlags( 0 );

	pCurrentPacket_->msgEndOffset( Packet::HEADER_SIZE );

	// if we're in the middle of a message start the next chunk here
	msgChunkOffset_ = pCurrentPacket_->msgEndOffset();
}

/**
 *	This method end processing of the current packet, i.e. calculate its
 *	flags, and the correct size including footers.
 *
 *	@param isExtending	True if we are extending the bundle size, false
 *						otherwise (when we are finalising for send).
 */
void UDPBundle::endPacket( bool isExtending )
{
	// If this won't be the last packet, add a reliable order marker
	if (isExtending)
	{
		if (this->isOnExternalChannel())
		{
			// add a partial reliable order if in the middle of a message
			if (msgBeg_ != NULL && msgIsReliable_)
			{
				this->addReliableOrder();
			}

			// add a gap reliable order to mark the end of the packet
			ReliableOrder rgap = { NULL, 0, 0 };
			reliableOrders_.push_back( rgap );
		}
	}

	// if we're in the middle of a message add this chunk
	msgLen_ += pCurrentPacket_->msgEndOffset() - msgChunkOffset_;
	msgChunkOffset_ = uint16( pCurrentPacket_->msgEndOffset() );
}


/**
 * 	This method finalises a message. It is called from a number of places
 *	within Bundle when necessary.
 */
void UDPBundle::endMessage( bool isEarlyCall /* = false */ )
{
	// nothing to do if no message yet
	if (msgBeg_ == NULL)
	{
		MF_ASSERT( pCurrentPacket_->msgEndOffset() == Packet::HEADER_SIZE || 
			hasEndedMsgEarly_ );
		return;
	}

	// add the amt used in this packet to the length
	msgLen_ += pCurrentPacket_->msgEndOffset() - msgChunkOffset_;

	// fill in headers for this msg
	curIE_.compressLength( msgBeg_, msgLen_, this, msgIsRequest_ );

	// record its details if it was reliable
	if (msgIsReliable_)
	{
		if (this->isOnExternalChannel())
		{
			this->addReliableOrder();
		}

		msgIsReliable_ = false;	// for sanity
	}

	msgChunkOffset_ = Packet::Offset( pCurrentPacket_->msgEndOffset() );

	msgBeg_ = NULL;
	msgIsRequest_ = false;

	hasEndedMsgEarly_ = isEarlyCall;
}


/**
 * 	This message begins a new message, with the given number of extra bytes in
 * 	the header. These extra bytes are normally used for request information.
 *
 * 	@param extra	Number of extra bytes to reserve.
 * 	@return	Pointer to the body of the message.
 */
char * UDPBundle::newMessage( int extra )
{
	// figure the length of the header
	int headerLen = curIE_.headerSize();
	if (headerLen == -1)
	{
		CRITICAL_MSG( "Mercury::UDPBundle::newMessage: "
			"tried to add a message with an unknown length format %d\n",
			(int)curIE_.lengthStyle() );
	}

	++numMessages_;

	if (msgIsReliable_)
	{
		++numReliableMessages_;
	}

	// make space for the header
	MessageID * pHeader = (MessageID *)this->qreserve( headerLen + extra );

	// set the start of this msg
	msgBeg_ = (uint8*)pHeader;
	msgChunkOffset_ = Packet::Offset( pCurrentPacket_->msgEndOffset() );

	// write in the identifier
	*(MessageID*)pHeader = curIE_.id();

	// set the length to zero
	msgLen_ = 0;
	msgExtra_ = extra;

	// and return a pointer to the extra data
	return (char *)(pHeader + headerLen);
}

/**
 *	This internal method adds a reliable order for the current (reliable)
 *	message. Multiple orders are necessary if the message spans packets.
 */
void UDPBundle::addReliableOrder()
{
	MF_ASSERT( this->isOnExternalChannel() );

	uint8 * begInCur = (uint8*)pCurrentPacket_->data() + msgChunkOffset_;
	uint8 * begInCurWithHeader = begInCur - msgExtra_ - curIE_.headerSize();

	// If this message actually began on this packet, we can start from the
	// actual message header.  Otherwise, we have to settle for the part of the
	// message that's on this packet.
	if (msgBeg_ == begInCurWithHeader)
	{
		begInCur = begInCurWithHeader;
	}

	ReliableOrder reliableOrder = { begInCur,
							uint16((uint8*)pCurrentPacket_->back() - begInCur),
							msgIsRequest_ };

	reliableOrders_.push_back( reliableOrder );
}


/**
 * 	This method returns the vector of the reliable orders in this bundle
 *	that reference the given packet.
 */
void UDPBundle::reliableOrders( Packet * p,
	const ReliableOrder *& roBeg, const ReliableOrder *& roEnd )
{
	if (!reliableOrders_.empty())
	{
		const int roSize = int(reliableOrders_.size());
		if (pFirstPacket_ == pCurrentPacket_)
		{
			MF_ASSERT( p == pCurrentPacket_ );
			roBeg = &reliableOrders_.front();
			roEnd = roBeg + roSize;
		}
		else
		{
			if (p == pFirstPacket_) reliableOrdersExtracted_ = 0;

			roBeg = &reliableOrders_.front() + reliableOrdersExtracted_;
			const ReliableOrder * roFirst = &reliableOrders_.front();
			for (roEnd = roBeg;
				roEnd != roFirst + roSize && roEnd->segBegin != NULL;
				++roEnd) ; // scan

			size_t reliableOrdersExtracted = (roEnd + 1) - 
				&reliableOrders_.front();
			MF_ASSERT( reliableOrdersExtracted <= INT_MAX );
			reliableOrdersExtracted_ = ( int ) reliableOrdersExtracted;
		}
	}
	else
	{
		roBeg = 0;
		roEnd = 0;
	}
}

/**
 * 	This method grabs all the reliable data from the source packet, and
 * 	appends it to this bundle. It handles the case where the source packet
 *	contains partial messages from a multi-packet bundle. It does nothing
 *	and returns false if the reliable data cannot all fit into the current
 *	packet.
 *
 * 	@param seq	Sequence number of the packet being piggybacked.
 * 	@param reliableOrders	Vector of reliable messages.
 * 	@param p	The source packet to obtain the data from.
 *
 * 	@return True if there was room to piggyback this packet.
 */
bool UDPBundle::piggyback( SeqNum seq, const ReliableVector& reliableOrders,
	Packet *p )
{
	Packet::Flags flags =
		Packet::FLAG_HAS_SEQUENCE_NUMBER |
		Packet::FLAG_IS_RELIABLE |
		Packet::FLAG_ON_CHANNEL;

	// First figure out if we have enough space to piggyback these messages.
	// Allocate for packet header, sequence number footer, and 2-byte size
	// suffix.
	uint16 totalSize =
		sizeof( Packet::Flags ) + sizeof( SeqNum ) + sizeof( int16 );

	for (ReliableVector::const_iterator it = reliableOrders.begin();
		 it != reliableOrders.end(); ++it)
	{
		totalSize += it->segLength;

		// We don't support piggybacking requests at the moment.
		if (it->segPartOfRequest)
		{
			return false;
		}
	}

	// We also need to figure out if the dropped packet had piggybacks on it.
	// If so, we need to preserve these on the outgoing packet.  Yes this means
	// the piggyback has piggybacks on it.  Wheeeeee.
	if (p->hasFlags( Packet::FLAG_HAS_PIGGYBACKS ))
	{
		flags |= Packet::FLAG_HAS_PIGGYBACKS;
		totalSize += p->piggyFooters().len_;
	}

	// End the message before dealing with the current packet because
	// when we allow for endMessage() to be called later, it may change
	// the actual size of the message and even add a new packet to the chain
	// because of the special case in message length compression
	// (see InterfaceElement::specialCompressLength())
	if (msgBeg_ != NULL)
	{
		this->endMessage( /* isEarlyCall = */ true );
	}

	if (totalSize > pCurrentPacket_->freeSpace())
	{
		return false;
	}

	// It fits, so tag packet with piggyback and reliable flags, because we are
	// about to discard the original packet and therefore can't afford to lose
	// this packet too.
	pCurrentPacket_->enableFlags(
		Packet::FLAG_HAS_PIGGYBACKS |
		Packet::FLAG_IS_RELIABLE |
		Packet::FLAG_HAS_SEQUENCE_NUMBER );

	// Don't include the size suffix in the packet length
	BundlePiggyback *pPiggy = new BundlePiggyback( p, flags, seq,
		/* len: */ totalSize - sizeof( int16 ) );

	// Add each message to the Piggyback
	for (ReliableVector::const_iterator it = reliableOrders.begin();
		 it != reliableOrders.end(); ++it)
	{
		pPiggy->rvec_.push_back( *it );
	}

	piggybacks_.push_back( pPiggy );

	// Reserve enough footer space for the piggyback.  It's OK to do this late
	// since we've already worked out that this fits on the current packet.
	pCurrentPacket_->reserveFooter( totalSize );

	return true;
}


/**
 *	This method returns the UDP channel for this UDP bundle.
 */
UDPChannel & UDPBundle::udpChannel()
{
	return static_cast< UDPChannel & >( *pChannel_ );
}


} // namespace Mercury


BW_END_NAMESPACE


// udp_bundle.cpp
