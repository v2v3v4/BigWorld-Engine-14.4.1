#include "pch.hpp"

#include "udp_bundle.hpp"
#include "interface_element.hpp"
#include "interfaces.hpp"

#include "cstdmf/config.hpp"
#include "cstdmf/watcher.hpp"

DECLARE_DEBUG_COMPONENT2( "Network", 0 )

BW_BEGIN_NAMESPACE

namespace Mercury
{

const int IDENTIFIER_SIZE = sizeof( uint8 );

/**
 * 	@internal
 *	This structure contains the standard interface definition for a reply
 *	message. It is used internally by Mercury.
 */
const InterfaceElement InterfaceElement::REPLY( "Reply",
	REPLY_MESSAGE_IDENTIFIER, VARIABLE_LENGTH_MESSAGE, 4 );


const float InterfaceElementWithStats::AVERAGE_BIAS = 
	EMA::calculateBiasFromNumSamples( 5 );

// -----------------------------------------------------------------------------
// Section: InterfaceElement
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
InterfaceElement::InterfaceElement( const char * name, MessageID id,
		int8 lengthStyle, int lengthParam,
		InputMessageHandler * pHandler ) :
	id_( id ),
	lengthStyle_( lengthStyle ),
	lengthParam_( lengthParam ),
	name_( name ),
	pHandler_( pHandler ),
	shouldProcessEarly_( false )
{
}


/**
 *	This method will update the length details of this InterfaceElement, if
 *	required, the handler will be queried in the case of a
 *	CALLBACK_LENGTH_MESSAGE style message.
 *
 *	@param networkInterface 	The network interface the message was received
 *								from.
 *	@param srcAddr 				The source address of the message.
 *
 *	@return 					True if the message is valid and the length
 *								style and parameters were updated, false 
 *								otherwise if this could not be done.
 */
bool InterfaceElement::updateLengthDetails(
		NetworkInterface & networkInterface,
		const Address & srcAddr )
{
	if (lengthStyle_ != CALLBACK_LENGTH_MESSAGE)
	{
		return true;
	}

	int streamSize = pHandler_->getMessageStreamSize( networkInterface, srcAddr,
		id_ );

	if (streamSize == INVALID_STREAM_SIZE)
	{
		// Handler indicates that the bundle is invalid, stop processing.
		return false;
	}

	if (streamSize < 0)
	{
		lengthStyle_ = VARIABLE_LENGTH_MESSAGE;
		lengthParam_ = -streamSize;
	}
	else
	{
		lengthStyle_ = FIXED_LENGTH_MESSAGE;
		lengthParam_ = streamSize;
	}

	return true;
}


/**
 * 	This method returns the number of bytes occupied by a header
 * 	for this type of message.
 *
 * 	@return Number of bytes needed for this header.
 */
int InterfaceElement::headerSize() const
{
	int headerLen = sizeof( MessageID );
	switch (lengthStyle_)
	{
		case FIXED_LENGTH_MESSAGE:
			break;
		case VARIABLE_LENGTH_MESSAGE:
			headerLen += lengthParam_;
			break;
		default:
			headerLen = -1;
			break;
	}
	return headerLen;
}

/**
 * 	This method returns the number of bytes nominally occupied by the body
 * 	of this type of message.
 *
 * 	@return Number of bytes.
 */
int InterfaceElement::nominalBodySize() const
{
	// never guesses for variable-length messages
	return (lengthStyle_ == FIXED_LENGTH_MESSAGE) ? lengthParam_ : 0;
}


/**
 *	This class is used as a type of iterator on a bundle. It is used to keep a
 *	pointer to the body part of a bundle and move this pointer forward. This
 *	class will take care of moving through the packets in a bundle.
 */
class BundleDataPos
{
public:
	BundleDataPos( Packet * pPacket, void * pCurr ) :
		pPacket_( pPacket ),
		pCurr_( (uint8*)pCurr )
	{
		MF_ASSERT( pPacket->body() <= pCurr && pCurr < pPacket->back() );
	}

	Packet * pPacket() const { return pPacket_; }
	uint8 * pData() const	{ return pCurr_; }

	bool advance( int distance )
	{
		if ((pPacket_ == NULL) || (pCurr_ == NULL))
			return false;

		// prevents server crush when packet bundle length is corrupted (negative)
		if (distance < 0)
		{
			ERROR_MSG( "BundleDataPos::advance: Negative distance=%d\n", distance);
			return false;
		}

		int remainingLength = distance;
		uint8 * pEnd = (uint8*)pPacket_->back();

		while (remainingLength >= (pEnd - pCurr_))
		{
			remainingLength -= int(pEnd - pCurr_);
			pPacket_ = pPacket_->next();
			if (pPacket_ == NULL)
			{
				ERROR_MSG( "BundleDataPos::advance: "
								"Ran out of packets.\n" );
				return false;
			}
			pCurr_ = (uint8*)pPacket_->body();
			pEnd = (uint8*)pPacket_->back();
		}
		pCurr_ += remainingLength;

		return true;
	}

private:
	Packet * pPacket_;
	uint8 * pCurr_;
};


/**
 *	This method is called by InterfaceElement::compressLength when the amount
 *	of data added to the stream for the message is more than the message's size
 *	field can handle. For example, if lengthParam is 1 and there is at least
 *	255 bytes worth of data added for the message (or 65535 for 2 bytes etc).
 *
 *	To handle this, a 4-byte size is placed at the start of the message
 *	displacing the first four bytes of the message. These are appended to the
 *	end of the message. The original length field is filled with 0xff to
 *	indicate this special situation.
 */
void * InterfaceElement::specialCompressLength( void * data, int length,
		Packet * pPacket, bool isRequest ) const
{
	while (pPacket &&
			!(pPacket->body() <= data && data < pPacket->back()))
	{
		pPacket = pPacket->next();
	}

	IF_NOT_MF_ASSERT_DEV (pPacket != NULL)
	{
		ERROR_MSG( "InterfaceElement::specialCompressLength( %s ):"
				" data not in any packets\n",
			this->c_str() );

		return NULL;
	}

	// Move to the start of the message body.
	BundleDataPos head( pPacket, data );
	head.advance( IDENTIFIER_SIZE + lengthParam_ );

	// If we are a request, move it up past the reply ID and the next request
	// offset.
	if (isRequest)
	{
		head.advance( sizeof( ReplyID ) + sizeof( Packet::Offset ) );
	}

	BundleDataPos tail = head;
	MF_VERIFY( tail.advance( length ) );

	// Get four bytes to place the displaced start of the message
	uint32 localLength = length;
	uint8 * pTail = tail.pData();

	const int size = sizeof( int32 );
	BundleDataPos tailCheck = tail;
	IF_NOT_MF_ASSERT_DEV (tailCheck.advance( size-1 ))
	{
		ERROR_MSG( "InterfaceElement::specialCompressLength( %s ):"
				" tail not in packets.\n",
			this->c_str() );

		return NULL;
	}

	// Copy the length to the head of the message and the displaced start of
	// the message to the tail. This needs to be done a byte at a time as the
	// head may span packets.
	int i = 0;
	do {
		// Copy displaced head of the message to the tail of the message.
		*tail.pData() = *head.pData();

		// Copy the message size to the start of the message
		*head.pData() = (uint8)localLength;
		localLength >>= 8;
	} while ( ++i < size && tail.advance( 1 ) && head.advance( 1 ) );

	IF_NOT_MF_ASSERT_DEV( i == size )
	{
		ERROR_MSG( "InterfaceElement::specialCompressLength( %s ):"
				" length compressing failed.\n",
			this->c_str() );
		return NULL;
	}

	return pTail;
}


/**
 * 	This method compresses a length into the given header.
 *
 *	@param data		Pointer to the header
 *	@param length	Length to compress
 *	@param bundle	The bundle to use for oversize messages.
 *	@param isRequest	Is the message a request.
 *
 *	@return 0 if successful, -1 otherwise.
 */
int InterfaceElement::compressLength( void * data, int length,
		UDPBundle * pBundle, bool isRequest ) const
{
	switch (lengthStyle_)
	{
	case FIXED_LENGTH_MESSAGE:
		if (length != lengthParam_)
		{
			CRITICAL_MSG( "Mercury::InterfaceElement::compressLength( %s ): "
				"Fixed length message has wrong length (%d instead of %d)\n",
				this->c_str(), length, lengthParam_ );
		}
		break;

	case VARIABLE_LENGTH_MESSAGE:
	{
		// Beware of overflow in length, not sure if this actually happens in
		// practice, but it definitely happens on the expandLength() side of
		// things so it's probably worth having this check here
		if (length < 0)
		{
			ERROR_MSG( "Mercury::InterfaceElement::compressLength( %s ): "
				"Possible overflow in length (%d bytes) for "
				"variable length message\n",
				this->c_str(), length );

			return -1;
		}

		char *pLen = ((char*)data) + 1;
		int len = length;
		bool oversize = false;

		switch (lengthParam_)
		{
		case 1:
			if (len < 0xff)
			{
				*pLen = uint8( len );
			}
			else
			{
				oversize = true;
			}
			break;

		case 2:
			if (len < 0xffff)
			{
#if defined( BW_ENFORCE_ALIGNED_ACCESS )
				uint16 len16 = BW_HTONS( static_cast< uint16 >( len ) );
				memcpy( pLen, &len16, sizeof(len16) );
#else // !defined( BW_ENFORCE_ALIGNED_ACCESS )
				*(uint16 *)pLen = BW_HTONS( (uint16)len );
#endif // defined( BW_ENFORCE_ALIGNED_ACCESS )
			}
			else
			{
				oversize = true;
			}
			break;

		case 3:
			if (len < 0xffffff)
			{
				BW_PACK3( pLen, len );
			}
			else
			{
				oversize = true;
			}
			break;

		case 4:
			{
#if defined( BW_ENFORCE_ALIGNED_ACCESS )
				uint32 len32 = BW_HTONL( static_cast< uint32 >( len ) );
				memcpy( pLen, &len32, sizeof(len32) );
#else // !defined( BW_ENFORCE_ALIGNED_ACCESS )
				*(uint32 *)pLen = BW_HTONL( (uint32)len );
#endif // defined( BW_ENFORCE_ALIGNED_ACCESS )
			}
			break;

		default:
			CRITICAL_MSG( "InterfaceElement::compressLength( %s ): "
				"Unsupported variable length width: %d\n",
				this->c_str(), lengthParam_ );
		}

		// If the message length could not fit into a standard length field, we
		// need to handle this as a special case.
		if (oversize)
		{
			// Fill the original length field with ones to indicate the special
			// situation.
			static const int IDENTIFIER_SIZE = sizeof(uint8);
			for (int i = IDENTIFIER_SIZE; i <= lengthParam_; ++i)
			{
				((uint8*)data)[i] = 0xff;
			}

			if (pBundle)
			{
				void * tail = pBundle->reserve( sizeof( int32 ) );
				void * ret = this->specialCompressLength( data, length,
					pBundle->pFirstPacket(), isRequest );
				MF_ASSERT( !ret || tail == ret );
				return ret ? 0 : -1;
			}
			else
			{
				return -1;
			}
		}
		break;
	}
	default:
		ERROR_MSG( "Mercury::InterfaceElement::compressLength( %s ): "
			"Unrecognised length format %d\n",
			this->c_str(), (int)lengthStyle_ );

		return -1;
		break;
	}

	return 0;
}


/**
 * This method undoes expandLength()
 */
void InterfaceElement::unexpandLength( void * data, int length,
		Packet * pPacket, bool isRequest ) const
{
	if (lengthStyle_ != VARIABLE_LENGTH_MESSAGE)
	{
		return;
	}

	if ((lengthParam_ == 1 && length >= 0x0000ff) ||
		(lengthParam_ == 2 && length >= 0x00ffff) ||
		(lengthParam_ == 3 && length >= 0xffffff))
	{
		MF_VERIFY(
			this->specialCompressLength( data, length, pPacket, isRequest ) );
	}
}


/**
 *	This method handles the case where a message has had its length field added
 *	by InterfaceElement::specialCompressLength. See that method for more
 *	details.
 */
int InterfaceElement::specialExpandLength( void * data, Packet * pPacket, 
		bool isRequest ) const
{
	// Read the length out of the first four bytes of the message.
	int len = 0;

	BundleDataPos curr( pPacket, data );
	curr.advance( IDENTIFIER_SIZE + lengthParam_ );
	if (isRequest)
	{
		curr.advance( sizeof( ReplyID ) + sizeof( Packet::Offset ) );
	}

	BundleDataPos head = curr;

	const int size = sizeof( int32 );
	for (int i = 0; i < size; ++i)
	{
		len |= uint32(*curr.pData()) << 8*i;
		if (!curr.advance( 1 ))
		{
			ERROR_MSG( "InterfaceElement::specialExpandLength( %s ): "
					"Ran out of packets.\n",
				this->c_str() );

			return -1;
		}
	}

	// Now need to move the last four bytes of the message to the first.
	BundleDataPos tail = head;

	bool tailAdvanceSuccess = tail.advance( len );
	IF_NOT_MF_ASSERT_DEV (tailAdvanceSuccess)
	{
		ERROR_MSG( "InterfaceElement::specialExpandLength( %s ): "
				"Could not find tail.\n",
			this->c_str() );

		return -1;
	}

	BundleDataPos tailCheck = tail;
	// Do not advance on the last byte as it is likely to fail.
	IF_NOT_MF_ASSERT_DEV (tailCheck.advance( size-1 ))
	{
		ERROR_MSG( "InterfaceElement::specialExpandLength( %s ): "
				"Ran out of tail.\n",
			this->c_str() );
		return -1;
	}

	int i = 0;
	do {
		*head.pData() = *tail.pData();
	} while ( ++i < size && tail.advance( 1 ) && head.advance( 1 ) );

	IF_NOT_MF_ASSERT_DEV( i == size )
	{
		ERROR_MSG( "InterfaceElement::specialExpandLength( %s ): "
				" length expanding failed.\n",
			this->c_str() );
		return -1;
	}

	return len;
}


/**
 * 	This method expands a length from the given header.
 *
 * 	@param data	This is a pointer to a message header.
 *	@param pPacket
 *	@param isRequest
 *
 * 	@return Expanded length.
 */
int InterfaceElement::expandLength( void * data, Packet * pPacket, 
		bool isRequest ) const
{
	switch (lengthStyle_)
	{
	case FIXED_LENGTH_MESSAGE:
		return lengthParam_;
		break;
	case VARIABLE_LENGTH_MESSAGE:
	{
		uint8 *pLen = ((uint8*)data) + sizeof( MessageID );
		uint32 len = 0;

		switch (lengthParam_)
		{
			case 0: len = 0; break;
			case 1: len = *(uint8*)pLen; break;
			case 2:
			{
#if defined( BW_ENFORCE_ALIGNED_ACCESS )
				uint16 len16 = 0;
				memcpy( &len16, pLen, sizeof(uint16) );
				len = BW_NTOHS( len16 );
#else // !defined( BW_ENFORCE_ALIGNED_ACCESS )
				len = BW_NTOHS( *(uint16 *)pLen );
#endif // defined( BW_ENFORCE_ALIGNED_ACCESS )
				break;
			}
			case 3: len = BW_UNPACK3( (const char*)pLen ); break;
			case 4:
			{
#if defined( BW_ENFORCE_ALIGNED_ACCESS )
				uint32 len32;
				memcpy( &len32, pLen, sizeof(uint32) );
				len = BW_NTOHL( len32 );
#else // !defined( BW_ENFORCE_ALIGNED_ACCESS )
				len = BW_NTOHL( *(uint32*)pLen );
#endif // defined( BW_ENFORCE_ALIGNED_ACCESS )
				break;
			}
			default:
				CRITICAL_MSG( "InterfaceElement::expandLength( %s ): "
					"Unhandled variable message length: %d\n",
					this->c_str(), lengthParam_ );
		}

		// If lengthParam_ is 4, a length > 0x80000000 will cause an overflow
		// and a negative value will be returned from this method.
		if ((int)len < 0)
		{
			ERROR_MSG( "Mercury::InterfaceElement::expandLength( %s ): "
				"Overflow in calculating length of variable message!\n",
				this->c_str() );

			return -1;
		}

		// The special case is indicated with the length field set to maximum.
		// i.e. All bits set to 1.
		if (!this->canHandleLength( len ))
		{
			if (!pPacket)
			{
				return -1;
			}
			return this->specialExpandLength( data, pPacket, isRequest );
		}

		return len;
		break;
	}
	default:
		ERROR_MSG( "Mercury::InterfaceElement::expandLength( %s ): "
			"unrecognised length format %d\n",
			this->c_str(), (int)lengthStyle_ );

		break;
	}
	return -1;
}


/**
 *  Returns the string representation of this interface element, useful for
 *  debugging.
 */
const char * InterfaceElement::c_str() const
{
	static char buf[ 256 ];
	bw_snprintf( buf, sizeof( buf ), "%s/%d", name_, id_ );
	return buf;
}


// -----------------------------------------------------------------------------
// Section: InterfaceElementWithStats
// -----------------------------------------------------------------------------

#if ENABLE_WATCHERS
/**
 *	This static method returns a generic watcher for this class.
 */
WatcherPtr InterfaceElementWithStats::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		InterfaceElementWithStats * pNULL = NULL;

		pWatcher = new DirectoryWatcher();

		pWatcher->addChild( "name",
				makeWatcher( pNULL->name_ ) );

		pWatcher->addChild( "id",
				makeWatcher( *pNULL, &InterfaceElementWithStats::idAsInt ) );

		pWatcher->addChild( "maxBytesReceived",
				makeWatcher( pNULL->maxBytesReceived_ ) );

		pWatcher->addChild( "bytesReceived",
				makeWatcher( pNULL->numBytesReceived_ ) );

		pWatcher->addChild( "messagesReceived",
				makeWatcher( pNULL->numMessagesReceived_ ) );

		pWatcher->addChild( "avgMessageLength",
				makeWatcher( *pNULL,
					&InterfaceElementWithStats::avgMessageLength ) );

		pWatcher->addChild( "avgBytesPerSecond",
				makeWatcher( *pNULL,
					&InterfaceElementWithStats::avgBytesReceivedPerSecond ) );
		pWatcher->addChild( "avgMessagesPerSecond",
				makeWatcher( *pNULL,
					&InterfaceElementWithStats::
						avgMessagesReceivedPerSecond ) );

		pWatcher->addChild( "timing",
				ProfileVal::pSummaryWatcher(), &pNULL->profile_ );

		pWatcher->addChild( "timingInSeconds",
				ProfileVal::pWatcherSeconds(), &pNULL->profile_ );

		pWatcher->addChild( "timingInStamps",
				ProfileVal::pWatcherStamps(), &pNULL->profile_ );
	}

	return pWatcher;
}
#endif

} // namespace Mercury

BW_END_NAMESPACE

// interface_element.cpp
