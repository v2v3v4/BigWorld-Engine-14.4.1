#include "pch.hpp"

#include "tcp_bundle.hpp"

#include "interface_element.hpp"
#include "tcp_channel.hpp"

#include "cstdmf/memory_stream.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{

union ReplyIDOrOffset
{
	TCPBundle::Offset offset;
	void * pReplyID;
};


/**
 *	Constructor.
 */
TCPBundle::TCPBundle( TCPChannel & channel ) :
		Bundle( &channel ),
		frameStartOffset_( sizeof( Offset ) ),
		pFrameData_( new MemoryOStream() ),
		pMessageData_( new MemoryOStream() ),
		pCurrentMessage_( NULL ),
		isCurrentMessageRequest_( false )
{
	pFrameData_->reserve( sizeof( Flags ) );

	// First request offset. If needed, the frame will be extended down by
	// setting frameStartOffset_ to 0.
	pFrameData_->reserve( sizeof( Offset ) );

	this->flags() = 0;
}


/**
 *	Destructor.
 */
TCPBundle::~TCPBundle()
{
	bw_safe_delete( pFrameData_ );

	if (pMessageData_)
	{
		bw_safe_delete( pMessageData_ );
	}
}


/*
 *	Override from Bundle.
 */
void TCPBundle::startMessage( const InterfaceElement & ie,
		ReliableType reliable )
{
	this->newMessage( ie );
}


/*
 *	Override from Bundle.
 */
void TCPBundle::startRequest( const InterfaceElement & ie,
		ReplyMessageHandler * pHandler,
		void * arg,
		int timeout,
		ReliableType reliable )
{
	// We're going to be needing that first request offset header.
	if (frameStartOffset_ > 0)
	{
		Flags flags = this->flags();
		frameStartOffset_ = 0;
		this->flags() = flags | FLAG_HAS_REQUESTS;
	}

	Offset offset = this->size();

	this->newMessage( ie );

	this->setNextRequestOffset( offset );

	Offset replyIDOffset = this->size();

	*pMessageData_ << ReplyID( 0x12345678 ) << Offset( -1 );

	// Setting offset as the pReplyID member is a hack to work around if the
	// MemoryOStream reallocates memory when growing, and the pointer to the
	// replyID is no longer valid.  Instead of storing the pointer, we store
	// the numerical offset and resolve this to the actual pointer during
	// TCPBundle::finalise().

	ReplyIDOrOffset replyIDOrOffset;
	replyIDOrOffset.offset = replyIDOffset;

	ReplyOrder replyOrder = 
	{
		pHandler,  					/* handler */
		arg, 						/* arg */
		timeout, 					/* microsends */
		replyIDOrOffset.pReplyID 	/* pReplyID */
	};

	replyOrders_.push_back( replyOrder );

	isCurrentMessageRequest_ = true;
}


/**
 *	This method writes the next request offset into the bundle.
 *
 *	@param offset	The offset to write.
 */
void TCPBundle::setNextRequestOffset( Offset offset )
{
	uint8 * offsetPos = NULL;
	uint8 * frameData = reinterpret_cast< uint8 * >( pFrameData_->data() );

	if (replyOrders_.empty())
	{
		// First request
		offsetPos = frameData + sizeof(Flags);
	}
	else
	{
		// Not the first request, find the message's header.
		ReplyOrder & lastRequest = replyOrders_.back();

		ReplyIDOrOffset replyIDOrOffset;
		replyIDOrOffset.pReplyID = lastRequest.pReplyID;

		Offset messageOffset = replyIDOrOffset.offset;

		offsetPos = frameData + messageOffset + sizeof(ReplyID);
	}

#if defined( BW_ENFORCE_ALIGNED_ACCESS )
	Offset offsetNBO = BW_HTONL( offset );
	memcpy( offsetPos, &offsetNBO, sizeof(Offset) );
#else // !defined( BW_ENFORCE_ALIGNED_ACCESS )
	*(reinterpret_cast< Offset * >( offsetPos )) = BW_HTONL( offset );
#endif
}


/*
 *	Override from Bundle.
 */
void TCPBundle::startReply( ReplyID id, 
		ReliableType reliable )
{
	this->newMessage( InterfaceElement::REPLY );
	*pMessageData_ << id;
}


/*
 *	Override from Bundle.
 */
void TCPBundle::clear( bool newBundle )
{
	this->Bundle::clear( newBundle );

	frameStartOffset_ = sizeof( Offset );
	delete pFrameData_;
	pFrameData_ = new MemoryOStream();
	pFrameData_->reserve( sizeof( Flags ) );
	pFrameData_->reserve( sizeof( Offset ) );

	delete pMessageData_;
	pMessageData_ = new MemoryOStream();
	pCurrentMessage_.reset();
	isCurrentMessageRequest_ = false;

	this->flags() = 0;
}


/*
 *	Override from Bundle.
 */
int TCPBundle::freeBytesInLastDataUnit() const
{
	return this->tcpChannel().maxSegmentSize() -
		(this->size() % this->tcpChannel().maxSegmentSize());
}


/*
 *	Override from Bundle.
 */
int TCPBundle::numDataUnits() const
{
	const int mss = this->tcpChannel().maxSegmentSize();
	return (this->size() + mss - 1) / mss;
}


/*
 *	Override from BinaryOStream.
 */
void * TCPBundle::reserve( int nBytes )
{
	return pMessageData_->reserve( nBytes );
}


/*
 *	Override from BinaryOStream.
 */
int TCPBundle::size() const
{
	return pFrameData_->size() + 
		(pMessageData_ != NULL ? pMessageData_->size() : 0) - 
		frameStartOffset_;
}


/**
 *	This method starts a new message.
 */
void TCPBundle::newMessage( const InterfaceElement & ie )
{
	if (pCurrentMessage_.get())
	{
		this->finaliseCurrentMessage();
	}

	++numMessages_;
	pCurrentMessage_.reset( new InterfaceElement( ie ) );

	pMessageData_->reset();

	*pMessageData_ << ie.id();

	if (ie.lengthStyle() == VARIABLE_LENGTH_MESSAGE)
	{
		pMessageData_->reserve( ie.lengthParam() );
	}
}


/**
 *	This method finalises the current message.
 */
void TCPBundle::finaliseCurrentMessage()
{
	int length = this->currentMessagePayloadLength();
	if (-1 == pCurrentMessage_->compressLength( 
			pMessageData_->data(), length,
			NULL, isCurrentMessageRequest_ ))
	{
		int payloadStart = sizeof( MessageID ) + 
				pCurrentMessage_->lengthParam();

		pFrameData_->transfer( *pMessageData_, payloadStart );
		*pFrameData_ << uint32( length );
	}

	pFrameData_->transfer( *pMessageData_, pMessageData_->remainingLength() );

	pMessageData_->reset();

	pCurrentMessage_.reset();
	isCurrentMessageRequest_ = false;
}


/*
 *	Override from Bundle.
 */
void TCPBundle::doFinalise()
{
	if (pCurrentMessage_.get())
	{
		this->finaliseCurrentMessage();
	}

	// Convert the offset hacked into the ReplyID * member into an actual
	// ReplyID pointer.
	ReplyOrders::iterator iReplyOrder = replyOrders_.begin();
	while (iReplyOrder != replyOrders_.end())
	{
		ReplyIDOrOffset replyIDOrOffset;
		replyIDOrOffset.pReplyID = iReplyOrder->pReplyID;
		Offset replyIDOffset = replyIDOrOffset.offset;

		iReplyOrder->pReplyID = reinterpret_cast< void * >(
				reinterpret_cast< uint8 *>( pFrameData_->data() ) +
				replyIDOffset );
		++iReplyOrder;
	}
}


/**
 *	This method returns the current message's payload length.
 */
int TCPBundle::currentMessagePayloadLength()
{
	if (!pCurrentMessage_.get())
	{
		return 0;
	}

	MF_ASSERT( pMessageData_->size() >= int( sizeof( MessageID ) ) );

	int length = pMessageData_->size() - sizeof( MessageID );

	if (pCurrentMessage_->lengthStyle() == VARIABLE_LENGTH_MESSAGE)
	{
		MF_ASSERT( pMessageData_->size() >= 
			int( (sizeof( MessageID ) + pCurrentMessage_->lengthParam()) ) );

		length -= pCurrentMessage_->lengthParam();
	}

	if (isCurrentMessageRequest_)
	{
		length -= (sizeof(ReplyID) + sizeof(Offset));
	}

	return length;
}


/**
 *	This method returns the frame data.
 */
void * TCPBundle::data()
{
	return (uint8*) pFrameData_->data() + frameStartOffset_;
}


/**
 *	This method returns a reference to the flags in the frame header.
 */
TCPBundle::Flags & TCPBundle::flags()
{
	return *reinterpret_cast< Flags * >( this->data() );
}


/**
 *	This method returns the channel cast as a TCPChannel pointer.
 */
TCPChannel & TCPBundle::tcpChannel()
{
	return *(static_cast< TCPChannel *>( pChannel_ ));
}


} // end namespace Mercury

BW_END_NAMESPACE


// tcp_bundle.cpp

