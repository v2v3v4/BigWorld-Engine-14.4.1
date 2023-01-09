#include "pch.hpp"

#include "udp_bundle_processor.hpp"

#include "interface_table.hpp"
#include "process_socket_stats_helper.hpp"
#include "udp_channel.hpp"

#include "cstdmf/memory_stream.hpp"
#include "cstdmf/profiler.hpp"


BW_BEGIN_NAMESPACE


namespace Mercury
{

/**
 * 	This constructor initialises a bundle, given a packet chain.
 *
 * 	@param p	Linked list of packets that form this bundle.
 * 	@param isEarly	If set to true, processes only messages
 *  for message types which support early (out of order) processing
 */
UDPBundleProcessor::UDPBundleProcessor( Packet * p, bool isEarly ) :
	pFirstPacket_( p ),
	isEarly_( isEarly )
{
}


/**
 *	This method is responsible for dispatching the messages on this bundle to
 *	the appropriate handlers.
 *
 *	@param interfaceTable 	The interface table.
 *	@param addr 			The source address of the bundle.
 *	@param pChannel 		The channel.
 *	@param networkInterface The network interface.
 *	@param pStatsHelper 	The socket receive statistics.
 *
 *	@return 				REASON_SUCCESS on success, otherwise an appropriate
 *							Mercury::Reason describing the error.
 */
Reason UDPBundleProcessor::dispatchMessages( InterfaceTable & interfaceTable,
		const Address & addr, UDPChannel * pChannel,
		NetworkInterface & networkInterface, 
		ProcessSocketStatsHelper * pStatsHelper ) const
{
#	define SOURCE_STR (pChannel ? pChannel->c_str() : addr.c_str())
	bool breakLoop = pChannel ? pChannel->isDead() : false;
	Reason ret = REASON_SUCCESS;

	// NOTE: The channel may be destroyed while processing the messages so we
	// need to hold a local reference to keep pChannel valid. 
	ChannelPtr pChannelHolder = pChannel;
	MessageFilterPtr pMessageFilter =
		pChannel ? pChannel->pMessageFilter() : NULL;

	// now we simply iterate over the messages in that bundle
	iterator iter	= this->begin();
	iterator end	= this->end();

	interfaceTable.onBundleStarted( pChannel );

	while (iter != end && !breakLoop)
	{
		// find out what this message looks like
		InterfaceElementWithStats & ie = interfaceTable[ iter.msgID() ];
		if (ie.pHandler() == NULL)
		{
			// If there aren't any interfaces served on this nub
			// then don't print the warning (slightly dodgy I know)
			ERROR_MSG( "UDPBundleProcessor::dispatchMessages( %s ): "
					"Discarding bundle after hitting unhandled message ID "
					"%u\n",
				SOURCE_STR, iter.msgID() );

			// Note: Early returns are OK because the bundle will
			// release the packets it owns for us!
			ret = REASON_NONEXISTENT_ENTRY;
			break;
		}

		ie.pHandler()->processingEarlyMessageNow( isEarly_ );

		InterfaceElement updatedIE = ie;
		if (!updatedIE.updateLengthDetails( networkInterface, addr ))
		{
			ERROR_MSG( "UDPBundleProcessor::dispatchMessages( %s ): "
					"Discarding bundle after failure to update length "
					"details for message ID %hu\n",
				SOURCE_STR, (unsigned short int)iter.msgID() );
			ret = REASON_CORRUPTED_PACKET;
			break;
		}

		// get the details out of it
		UnpackedMessageHeader & header = iter.unpack( updatedIE );
		header.pInterfaceElement = &ie;
		header.pBreakLoop = &breakLoop;
		header.pChannel = pChannel;
		header.pInterface = &networkInterface;
		if (!iter.isUnpacked())
		{
			ERROR_MSG( "UDPBundleProcessor::dispatchMessages( %s ): "
					"Discarding bundle due to corrupted header for message ID "
					"%hu\n",
				SOURCE_STR, (unsigned short int)iter.msgID() );

			// this->dumpMessages( interfaceTable );
			ret = REASON_CORRUPTED_PACKET;
			break;
		}

		if (isEarly_ && !ie.shouldProcessEarly())
		{
			iter++; // next! (note: can only call this after unpack)
			continue;
		}


		// get the data out of it
		const char * msgData = iter.data();
		if (msgData == NULL)
		{
			ERROR_MSG( "UDPBundleProcessor::dispatchMessages( %s ): "
					"Discarding rest of bundle since chain too short for data "
					"of message ID %hu length %d\n",
				SOURCE_STR, (unsigned short int)iter.msgID(), header.length );

			// this->dumpBundleMessages( interfaceTable );
			ret = REASON_CORRUPTED_PACKET;
			break;
		}

		// make a stream to belay it
		MemoryIStream mis( msgData, header.length );

		if (pStatsHelper)
		{
			pStatsHelper->startMessageHandling( header.length );
		}

		ie.startProfile();

		{
			PROFILER_SCOPED_DYNAMIC_STRING( ie.c_str() );
			if (!pMessageFilter)
			{
				// and call the handler
				ie.pHandler()->handleMessage( addr, header, mis );
			}
			else
			{
				// or pass to our channel's message filter if it has one
				pMessageFilter->filterMessage( addr, header, mis, ie.pHandler() );
			}
		}

		ie.stopProfile( header.length );

		if (pStatsHelper)
		{
			pStatsHelper->stopMessageHandling();
		}

		// next! (note: can only call this after unpack)
		iter++;

		if (mis.remainingLength() != 0)
		{
			if (header.identifier == REPLY_MESSAGE_IDENTIFIER)
			{
				WARNING_MSG( "UDPBundleProcessor::dispatchMessages( %s ): "
						"Handler for reply left %d bytes\n",
					SOURCE_STR, mis.remainingLength() );

			}
			else
			{
				WARNING_MSG( "UDPBundleProcessor::dispatchMessages( %s ): "
					"Handler for message %s (ID %hu) left %d bytes\n",
					SOURCE_STR, ie.name(),
					(unsigned short int)header.identifier, mis.remainingLength() );
				mis.finish();
			}
		}

		if (pChannel && pChannel->isDead())
		{
			breakLoop = true;
		}
	}

	if (pStatsHelper)
	{
		const bool wasOkay = (iter == end) || breakLoop;

		if (wasOkay)
		{
			pStatsHelper->onBundleFinished();
		}
		else
		{
			pStatsHelper->onCorruptedBundle();
		}
	}

	interfaceTable.onBundleFinished( pChannel );

	return ret;
#	undef SOURCE_STR
}


/**
 *	This method dumps the messages in a (received) bundle.
 *
 *	@param interfaceTable 	The interface table.
 */
void UDPBundleProcessor::dumpMessages( 
		const InterfaceTable & interfaceTable ) const
{
	iterator iter	= this->begin();
	iterator end	= this->end();
	int count = 0;

	while (iter != end && count < 1000)	// can get stuck here
	{
		const InterfaceElement & ie = interfaceTable[ iter.msgID() ];

		if (ie.lengthStyle() == CALLBACK_LENGTH_MESSAGE)
		{
			WARNING_MSG( "\tMessage %d: CALLBACK_LENGTH_MESSAGE. Aborting.\n",
					iter.msgID() );
			return;
		}

		if (ie.pHandler() == NULL)
		{
			WARNING_MSG( "\tMessage %d: No handler. Aborting.\n",
					iter.msgID() );
			return;
		}

		UnpackedMessageHeader & header = iter.unpack( ie );

		WARNING_MSG( "\tMessage %d: ID %u, len %d\n",
				count, header.identifier, header.length );

		iter++;
		count++;
	}
}


/**
 * 	This method returns an iterator pointing to the first message in a bundle.
 *
 *	@return 	An iterator to the first message in the bundle.
 */
UDPBundleProcessor::iterator UDPBundleProcessor::begin() const
{
	return UDPBundleProcessor::iterator( pFirstPacket_.get() );
}

/**
 * 	This method returns an iterator pointing after the last message in a
 * 	bundle.
 *
 *	@return 	The end iterator.
 */
UDPBundleProcessor::iterator UDPBundleProcessor::end() const
{
	return UDPBundleProcessor::iterator( NULL );
}


// ----------------------------------------------------------------------------
// Section: UDPBundleProcessor::iterator
// ----------------------------------------------------------------------------


/**
 * 	Constructor.
 *
 *	@param first 	The first packet of the bundle.
 */
UDPBundleProcessor::iterator::iterator( Packet * first ) :
	cursor_( NULL ),
	isUnpacked_( false ),
	bodyEndOffset_( 0 ),
	offset_( 0 ),
	dataOffset_( 0 ),
	dataLength_( 0 ),
	dataBuffer_( NULL ),
	updatedIE_()
{
	// find the first packet with body data
	// (can have no body if only footers in packet)
	for (cursor_ = first; cursor_ != NULL; cursor_ = cursor_->next())
	{
		this->nextPacket();
		if (offset_ < bodyEndOffset_) break;
	}
}


/**
 * 	Copy constructor.
 */
UDPBundleProcessor::iterator::iterator( 
		const UDPBundleProcessor::iterator & i ) :
	cursor_( i.cursor_ ),
	isUnpacked_( i.isUnpacked_ ),
	bodyEndOffset_( i.bodyEndOffset_ ),
	offset_( i.offset_ ),
	dataOffset_( i.dataOffset_ ),
	dataLength_( i.dataLength_ ),
	dataBuffer_( NULL ),
	updatedIE_( i.updatedIE_ )
{
	if (i.dataBuffer_ != NULL)
	{
		dataBuffer_ = new char[dataLength_];
		memcpy( dataBuffer_, i.dataBuffer_, dataLength_ );
	}
}

/**
 * 	Destructor.
 */
UDPBundleProcessor::iterator::~iterator()
{
	if (isUnpacked_)
	{
		bool isRequest = (nextRequestOffset_ == offset_);
		updatedIE_.unexpandLength( cursor_->data() + offset_, curHeader_.length,
			cursor_, isRequest );
		isUnpacked_ = false;
	}

	bw_safe_delete_array( dataBuffer_ );
}


/**
 * 	This is the assignment operator for the UDPBundleProcessor iterator.
 */
const UDPBundleProcessor::iterator & UDPBundleProcessor::iterator::operator=(
	UDPBundleProcessor::iterator i )
{
	swap( *this, i );
	return *this;
}


/**
 * 	This method sets up the iterator for the packet now at the cursor.
 */
void UDPBundleProcessor::iterator::nextPacket()
{
	nextRequestOffset_ = cursor_->firstRequestOffset();
	bodyEndOffset_ = cursor_->msgEndOffset();
	offset_ = uint16(cursor_->body() - cursor_->data());
}


/**
 * 	This method returns the identifier of the message that this iterator is
 *	currently pointing to.
 */
MessageID UDPBundleProcessor::iterator::msgID() const
{
	return *(MessageID*)(cursor_->data() + offset_);
}


/**
 *	This method unpacks the current message using the given
 *	interface element.
 *
 *	@param ie	InterfaceElement for the current message.
 *
 *	@return		Header describing the current message.
 */
UnpackedMessageHeader & UDPBundleProcessor::iterator::unpack( 
		const InterfaceElement & ie )
{
	uint16	msgBeg = offset_;

	MF_ASSERT( !isUnpacked_ );

	bool isRequest = (nextRequestOffset_ == offset_);

	updatedIE_ = ie;
	// read the standard header
	if (int(offset_) + updatedIE_.headerSize() > int(bodyEndOffset_))
	{
		ERROR_MSG( "UDPBundleProcessor::iterator::unpack( %s ): "
				"Not enough data on stream at %hu for header "
				"(%d bytes, needed %d)\n",
			updatedIE_.name(), offset_, int(bodyEndOffset_) - int(offset_),
			updatedIE_.headerSize() );

		goto errorNoRevert;
	}

	curHeader_.identifier = this->msgID();
	curHeader_.length =
		updatedIE_.expandLength( cursor_->data() + msgBeg, cursor_, isRequest );

	// If length is -1, then chances are we've had an overflow
	if (curHeader_.length == -1)
	{
		ERROR_MSG( "UDPBundleProcessor::iterator::unpack( %s ): "
				"Error unpacking header length at %hu\n",
			updatedIE_.name(), offset_ );

		goto errorNoRevert;
	}

	msgBeg += updatedIE_.headerSize();

	// let's figure out some flags
	if (!isRequest)
	{
		curHeader_.replyID = REPLY_ID_NONE;
	}
	else
	{
		if (msgBeg + sizeof(ReplyID) + sizeof(Packet::Offset) > bodyEndOffset_)
		{
			ERROR_MSG( "UDPBundleProcessor::iterator::unpack( %s ): "
					"Not enough data on stream at %hu for request ID and NRO "
					"(%d left, needed %" PRIzu ")\n",
				updatedIE_.name(), offset_, int(bodyEndOffset_) - int(msgBeg),
				sizeof(ReplyID) + sizeof(Packet::Offset) );

			goto error;
		}

		curHeader_.replyID = BW_NTOHL( *(ReplyID*)(cursor_->data() + msgBeg) );
		msgBeg += sizeof(ReplyID);

		nextRequestOffset_ = BW_NTOHS( *(Packet::Offset*)(cursor_->data() + 
			msgBeg) );

		if (nextRequestOffset_ > bodyEndOffset_)
		{
			ERROR_MSG( "UDPBundleProcessor::iterator::unpack( %s ): "
					"Next request offset out of bounds (got offset %hu, "
					"msg end offset %hu)\n",
				updatedIE_.name(), nextRequestOffset_, bodyEndOffset_);
			goto error;
		}

		msgBeg += sizeof(Packet::Offset);
	}

	// and set up the fields about the message data
	if ((msgBeg + size_t(curHeader_.length) > bodyEndOffset_) &&
			(cursor_->next() == NULL))
	{
		ERROR_MSG( "UDPBundleProcessor::iterator::unpack( %s ): "
				"Not enough data on stream at %hu for payload "
				"(%d left, needed %d)\n",
			updatedIE_.name(), offset_, int(bodyEndOffset_) - int(msgBeg),
			curHeader_.length );

		goto error;
	}

	dataOffset_ = msgBeg;
	dataLength_ = curHeader_.length; // (copied since curHeader mods allowed)

	// If this is a special case of data length (where we put a four byte size
	// on the end), we need to now disregard these bytes.
	if (!updatedIE_.canHandleLength( dataLength_ ))
	{
		dataLength_ += sizeof( int32 );
	}

	isUnpacked_ = true;
	return curHeader_;

error:
	updatedIE_.unexpandLength( cursor_->data() + offset_, curHeader_.length,
		cursor_, isRequest );

errorNoRevert:
	ERROR_MSG( "UDPBundleProcessor::iterator::unpack: "
		"Got corrupted message header\n" );
	return curHeader_;
}


/**
 * 	This method returns the data for the message that the iterator
 * 	is currently pointing to.
 *
 * 	@return 	Pointer to message data.
 */
const char * UDPBundleProcessor::iterator::data()
{
	// does this message go off the end of the packet?
	if (dataOffset_ + dataLength_ <= bodyEndOffset_)
	{
		// no, ok, we're safe
		return cursor_->data() + dataOffset_;
	}

	// is there another packet? assert that there is because 'unpack' would have
	// flagged an error if the next packet was required but missing
	MF_ASSERT( cursor_->next() != NULL );
	if (cursor_->next() == NULL) return NULL;
	// also assert that data does not start mid-way into the next packet
	MF_ASSERT( dataOffset_ <= bodyEndOffset_ );	// (also implied by 'unpack')

	// is the entirety of the message data on the next packet?
	if (dataOffset_ == bodyEndOffset_ &&
		Packet::HEADER_SIZE + dataLength_ <= cursor_->next()->msgEndOffset())
	{
		// yes, easy then
		return cursor_->next()->body();
	}

	// ok, it's half here and half there, time to make a temporary buffer.
	// note that a better idea might be to return a stream from this function.

	if (dataBuffer_ != NULL)
	{
		// Already created a buffer for it.  
		return dataBuffer_;
	}

	// Buffer is destroyed in operator++() and in ~iterator().
	dataBuffer_ = new char[dataLength_];
	Packet *thisPack = cursor_;
	uint16 thisOff = dataOffset_;
	uint16 thisLen;
	for (int len = 0; len < dataLength_; len += thisLen)
	{
		if (thisPack == NULL)
		{
			DEBUG_MSG( "UDPBundleProcessor::iterator::data: "
				"Run out of packets after %d of %d bytes put in temp\n",
				len, dataLength_ );
			return NULL;
		}
		thisLen = thisPack->msgEndOffset() - thisOff;
		if (thisLen > dataLength_ - len) thisLen = dataLength_ - len;
		memcpy( dataBuffer_ + len, thisPack->data() + thisOff, thisLen );
		thisPack = thisPack->next();
		thisOff = Packet::HEADER_SIZE;
	}
	return dataBuffer_;
}

/**
 * 	This operator advances the iterator to the next message.
 */
void UDPBundleProcessor::iterator::operator++(int)
{
	MF_ASSERT( isUnpacked_ ); // operator++() can be called after unpack() only
	isUnpacked_ = false;

	bool isRequest = (nextRequestOffset_ == offset_);
	updatedIE_.unexpandLength( cursor_->data() + offset_, curHeader_.length,
		cursor_, isRequest );
	
	bw_safe_delete_array( dataBuffer_ );

	int biggerOffset = int(dataOffset_) + dataLength_;
	while (biggerOffset >= int(bodyEndOffset_))
	{
		// use up the data in this packet
		biggerOffset -= bodyEndOffset_;

		// move onto the next packet
		cursor_ = cursor_->next();
		if (cursor_ == NULL) break;

		// set up for the next packet
		this->nextPacket();

		// data starts after the header of the next packet
		biggerOffset += offset_;
	}
	offset_ = biggerOffset;
}


/**
 * 	Equality operator.
 *
 *	@return 	True if the given iterator is pointing to the same message as
 *				this one, false otherwise.
 */
bool UDPBundleProcessor::iterator::operator==(const iterator & x) const
{
	return cursor_ == x.cursor_ && (cursor_ == NULL || offset_ == x.offset_);
}


/**
 * 	Inequality operator.
 *
 *	@return 	True if the given iterator is not pointing to the same message
 *				as this one, false otherwise.
 */
bool UDPBundleProcessor::iterator::operator!=(const iterator & x) const
{
	return !operator==( x );
}


/**
 * This function swaps the iterators.
 */
void swap( UDPBundleProcessor::iterator & a, UDPBundleProcessor::iterator & b )
{
	using std::swap;

	swap( a.cursor_, b.cursor_ );
	swap( a.isUnpacked_, b.isUnpacked_ );
	swap( a.bodyEndOffset_, b.bodyEndOffset_ );
	swap( a.offset_, b.offset_ );
	swap( a.dataOffset_, b.dataOffset_ );
	swap( a.dataLength_, b.dataLength_ );
	swap( a.dataBuffer_, b.dataBuffer_ );
	swap( a.nextRequestOffset_, b.nextRequestOffset_ );
	swap( a.curHeader_, b.curHeader_ );
	swap( a.updatedIE_, b.updatedIE_ );
}

} // namespace Mercury


BW_END_NAMESPACE


// udp_bundle_processor.cpp
