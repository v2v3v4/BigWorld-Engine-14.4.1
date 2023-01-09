#include "pch.hpp"

#include "tcp_channel.hpp"

#include "channel_listener.hpp"
#include "encryption_stream_filter.hpp"
#include "endpoint.hpp"
#include "event_dispatcher.hpp"
#include "network_interface.hpp"
#include "network_utils.hpp"
#include "stream_filter.hpp"
#include "tcp_bundle.hpp"
#include "tcp_bundle_processor.hpp"
#include "tcp_channel_stream_adaptor.hpp"

#include "cstdmf/memory_stream.hpp"

#if defined( __unix__ ) || defined( __APPLE__ ) || defined( EMSCRIPTEN )
#include <netinet/tcp.h>
#endif // defined( __unix__ ) || defined( __APPLE__ ) || defined( EMSCRIPTEN )


BW_BEGIN_NAMESPACE

namespace // anonymous
{

/// Simple header sent by the client to the server.
static const char CLIENT_HEADER[] = "TCP\r\n\r\n";
static const size_t CLIENT_HEADER_SIZE = sizeof(CLIENT_HEADER) - 1;

/**
 *	This class is used to wait until the channel socket is available for
 *	writing, upon which the channel buffered data will be attempted to be sent.
 */
class SendWaiter : public Mercury::InputNotificationHandler
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param channel 	The channel to notify when the socket is available for
	 *					writing.
	 */
	SendWaiter( Mercury::TCPChannel & channel ) :
			InputNotificationHandler(),
			channel_( channel )
	{}


	/**
	 *	Destructor.
	 */
	virtual ~SendWaiter() {}


	/*
	 *	Override from InputNotificationHandler.
	 */
	virtual int handleInputNotification( int fd )
	{
		if (!channel_.sendBufferedData())
		{
			channel_.destroy();
		}
		return 0;
	}


private:
	Mercury::TCPChannel & channel_;
};

} // end namespace anonymous


namespace Mercury
{

#if defined( _MSC_VER )
#pragma warning( push )
// C4355: 'this' : used in base member initializer list
#pragma warning( disable: 4355 )
#endif // defined( _MSC_VER )

/**
 *	Constructor.
 *
 *	@param networkInterface 	The associated network interface.
 *	@param endpoint				The connected socket endpoint.
 *	@param isServer 			Whether we are the server.
 */
TCPChannel::TCPChannel( NetworkInterface & networkInterface,
			Endpoint & endpoint,
			bool isServer ):
		Channel( networkInterface, endpoint.getRemoteAddress() ),
		InputNotificationHandler(),
		pStreamFilter_( NULL ),
		pEndpoint_( &endpoint ),
		headerState_( isServer ? HEADER_STATE_SERVER_WAIT_FOR_HEADER :
			HEADER_STATE_CLIENT_SEND_HEADER ),
		isShuttingDown_( false ),
		pFrameData_( new MemoryOStream( this->maxSegmentSize() ) ),
		frameLength_( 0 ),
		frameHeaderLength_( 0 ),
		pSendBuffer_( NULL ),
		pSendWaiter_( new SendWaiter( *this ) )
{
	pEndpoint_->setnonblocking( true );

	// Disable Nagle's algorithm
	pEndpoint_->setSocketOption( IPPROTO_TCP, TCP_NODELAY, true );

	this->dispatcher().registerFileDescriptor( pEndpoint_->fileno(), this,
		"TCPChannel" );

	pBundle_ = this->newBundle();
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif // _MSC_VER

/**
 *	Destructor.
 */
TCPChannel::~TCPChannel()
{
	bw_safe_delete( pEndpoint_ );
	bw_safe_delete( pFrameData_ );
	bw_safe_delete( pSendBuffer_ );
	bw_safe_delete( pSendWaiter_ );
}


/*
 *	Override from InputNotificationHandler.
 */
int TCPChannel::handleInputNotification( int fd )
{
	this->handleInput();
	return 0;
}


/**
 *	This method handles input from the socket.
 */
void TCPChannel::handleInput()
{
	// Guard against premature deletion.
	ChannelPtr pThis( this );

	int numAvailable = this->numBytesAvailableForReading();

	if (numAvailable < 0)
	{
		ERROR_MSG( "TCPChannel( %s )::handleInput: "
				"Failed to query amount of data for reading: %s\n",
			this->c_str(), strerror( errno ) );
	}

	if (numAvailable == 0)
	{
		// This is the peer shutting down the connection.
		this->handlePeerDisconnect();
		return;
	}

	lastReceivedTime_ = timestamp();

	if (pStreamFilter_)
	{
		if (-1 == pStreamFilter_->readInto( *pFrameData_ ))
		{
			ERROR_MSG( "TCPChannel::handleInput( %s ): "
					"Filter reported error while reading from endpoint\n",
				this->c_str() );

			this->destroy();
			return;
		}
	}
	else
	{
		int receivedBytes = this->readInto( *pFrameData_ );
		MF_ASSERT( receivedBytes == numAvailable );
	}

	bool potentiallyMoreFrames = true;

	while (potentiallyMoreFrames)
	{
		if (headerState_ == HEADER_STATE_SERVER_WAIT_FOR_HEADER)
		{
			// For WebSockets, the header would look like the first line of the
			// HTTP handshake - which we can then reject here immediately.

			// GET / HTTP/1.1 <CRLF>

			// Check for WebSockets header line, which starts with this. We
			// will only read this if there is a protocol mismatch where the
			// remote side is sending WebSockets frames and the local side is
			// expecting raw TCP frames, because otherwise the
			// WebSocketsStreamFilter would take care of the handshake.
			static const char HTTP_HEADER_START[] = "GET ";
			static const size_t HTTP_HEADER_START_SIZE =
				(sizeof(HTTP_HEADER_START) - 1);

			MF_ASSERT( frameLength_ == 0 );

			if (pFrameData_->remainingLength() <
					static_cast<int>(HTTP_HEADER_START_SIZE))
			{
				// Wait for more data.
				return;
			}

			if (0 == memcmp( pFrameData_->retrieve( 0 ),
					HTTP_HEADER_START, HTTP_HEADER_START_SIZE))
			{
				// It definitely doesn't match the first header frame, so we
				// can assume that it was a WebSockets frame and drop the
				// connection here.
				// TODO: Notify the client via a WebSockets handshake reply.
				ERROR_MSG( "TCPChannel::handleInput( %s ): "
						"Invalid first frame, appears to be from WebSockets "
						"client, but we are not expecting WebSockets "
						"connections\n",
					this->c_str() );
				this->destroy();
				return;
			}
		}

		if ((frameLength_ == 0) && (frameHeaderLength_ == 0))
		{
			this->readSmallFrameLength();
		}

		if ((frameLength_ == 0) && (frameHeaderLength_ != 0))
		{
			this->readLargeFrameLength();
		}

		if ((frameLength_ != 0) &&
				(uint( pFrameData_->remainingLength() ) >= frameLength_))
		{
			if (headerState_ == HEADER_STATE_SERVER_WAIT_FOR_HEADER)
			{
				// First frame is the header frame.
				potentiallyMoreFrames = this->readHeader();
			}
			else
			{
				this->processReceivedFrame();
			}
		}
		else
		{
			potentiallyMoreFrames = false;
		}
	}
}


/**
 *	This method returns the number of bytes available for reading from the
 *	socket.
 *
 *	@return 	The number of bytes available for reading, or -1 if an error
 *				occurred.
 */
int TCPChannel::numBytesAvailableForReading() const
{
	return pEndpoint_->numBytesAvailableForReading();
}


/**
 *	This method reads the connection header. This header identifies whether we
 *	are looking at a raw TCP connection or a WebSockets connection.
 *
 *	@return true if the header was read successfully, false otherwise.
 */
bool TCPChannel::readHeader()
{
	if (pFrameData_->remainingLength() <
			static_cast< int >( CLIENT_HEADER_SIZE ))
	{
		// Wait for more data to come in.
		return true;
	}

	const char * receivedHeaderData = reinterpret_cast< const char * >(
		pFrameData_->retrieve( 0 ) );

	if (0 != memcmp( receivedHeaderData, CLIENT_HEADER, CLIENT_HEADER_SIZE ))
	{
		// Doesn't look like a WebSockets handshake or a header.
		ERROR_MSG( "TCPChannel::readHeader( %s ): "
				"Invalid header, disconnecting\n",
			this->c_str() );

		this->destroy();
		return false;
	}

	frameLength_ = CLIENT_HEADER_SIZE;
	frameHeaderLength_ = sizeof( uint16 );
	pFrameData_->retrieve( CLIENT_HEADER_SIZE );
	headerState_ = HEADER_STATE_NONE;
	this->cleanUpAfterFrameReceive();
	return true;
}


/**
 *	This method reads the small frame length.
 */
void TCPChannel::readSmallFrameLength()
{
	if (pFrameData_->remainingLength() >= int( sizeof( uint16 ) ))
	{
		uint16 smallFrameLength;
		*pFrameData_ >> smallFrameLength;

		if (smallFrameLength != 0xFFFF)
		{
			frameLength_ = smallFrameLength;
			frameHeaderLength_ = sizeof( uint16 );
		}
		else
		{
			frameLength_ = 0;
			frameHeaderLength_ = sizeof( uint16 ) + sizeof( uint32 );
		}
	}
}


/**
 *	This method reads the large frame length.
 */
void TCPChannel::readLargeFrameLength()
{
	if (pFrameData_->remainingLength() >= int( sizeof( uint32 ) ))
	{
		uint32 largeFrameLength;
		*pFrameData_ >> largeFrameLength;

		frameLength_ = largeFrameLength;
	}
}


/**
 *	This method processes a completely received frame.
 */
void TCPChannel::processReceivedFrame()
{
	// Restrict data seen by TCPBundleProcessor to only to this frame.
	MemoryIStream frameData( pFrameData_->retrieve( frameLength_ ),
		frameLength_ );

	TCPBundleProcessor processor( frameData );
	Reason reason = processor.dispatchMessages( *this,
		pNetworkInterface_->interfaceTable() );

	if (reason != REASON_SUCCESS)
	{
		ERROR_MSG( "TCPChannel( %s )::processReceivedFrame: "
				"Error while while dispatching messages: %s\n",
			this->c_str(), reasonToString( reason ) );
	}

	this->cleanUpAfterFrameReceive();
}


/**
 *	This method cleans up state after a frame receive.
 */
void TCPChannel::cleanUpAfterFrameReceive()
{
	++numDataUnitsReceived_;
	numBytesReceived_ += frameHeaderLength_ + frameLength_;

	frameLength_ = 0;
	frameHeaderLength_ = 0;

	// Copy any leftover data to be processed as the start of the next frame.
	MemoryOStream * pNewData = new MemoryOStream( this->maxSegmentSize() );

	pNewData->transfer( *pFrameData_, pFrameData_->remainingLength() );

	delete pFrameData_;
	pFrameData_ = pNewData;
}


/*
 *	Override from Channel.
 */
Bundle * TCPChannel::newBundle()
{
	return new TCPBundle( *this );
}


/*
 *	Override from Channel.
 */
void TCPChannel::doSend( Bundle & bundleUncast )
{
	TCPBundle & bundle = static_cast< TCPBundle & >( bundleUncast );

	MemoryOStream frameData;

	if (headerState_ == HEADER_STATE_CLIENT_SEND_HEADER)
	{
		// First frame from client - send TCP header.
		MF_ASSERT( CLIENT_HEADER_SIZE < UINT16_MAX );
		frameData << uint16( CLIENT_HEADER_SIZE );
		frameData.addBlob( CLIENT_HEADER, CLIENT_HEADER_SIZE );
		headerState_ = HEADER_STATE_NONE;
	}

	if (bundle.size() >= UINT16_MAX)
	{
		frameData << uint16( 0xFFFF ) << uint32( bundle.size() );
	}
	else
	{
		frameData << uint16( bundle.size() );
	}

	frameData.addBlob( bundle.data(), bundle.size() );

	numBytesSent_ += frameData.size();
	++numDataUnitsSent_;

	if (pStreamFilter_)
	{
		pStreamFilter_->writeFrom( frameData,
			Mercury::NetworkStream::WRITE_SHOULD_NOT_CORK );
	}
	else
	{
		this->writeFrom( frameData, /* shouldCork */ false );
	}
}


/*
 *	Override from Channel.
 */
bool TCPChannel::hasUnsentData() const
{
	if (pBundle_->numMessages() > 0)
	{
		return true;
	}

	return ((pStreamFilter_.get() != NULL) && pStreamFilter_->hasUnsentData());
}


/**
 *	This method returns the maximum segment size for the underlying TCP
 *	channel.
 *
 *	@return 	The maximum segment size of our TCP socket.
 */
int TCPChannel::maxSegmentSize() const
{
#if defined( __unix__ ) && !defined( EMSCRIPTEN )
	struct tcp_info info;
	socklen_t infoSize = sizeof( info );
	if (-1 == getsockopt( pEndpoint_->fileno(), IPPROTO_TCP, TCP_INFO,
			&info, &infoSize ))
	{
		return 1450; // If there was a problem, default to this.
	}
	return int( info.tcpi_snd_mss );
#else // !defined( __unix__ ) || defined( EMSCRIPTEN )
	return 1450;
#endif // defined( __unix__ ) && !defined( EMSCRIPTEN )
}


/*
 *	Override from Channel.
 */
const char * TCPChannel::c_str() const
{
	const static int TOTAL_BUFFERS = 2;
	const static int BUFFER_SIZE = 64;
	static char buffers[TOTAL_BUFFERS][BUFFER_SIZE];
	static int nextBuffer = 0;

	char * buffer = buffers[nextBuffer];
	nextBuffer = (nextBuffer + 1) % TOTAL_BUFFERS;

	if (pStreamFilter_.exists())
	{
		BW::string streamString = pStreamFilter_->streamToString();
		memcpy( buffer, streamString.c_str(), streamString.size() + 1 );
	}
	else
	{
		bw_snprintf( buffer, BUFFER_SIZE, "%s:%s",
			"tcp", addr_.c_str() );
	}

	return buffer;
}


/*
 *	Override from Channel.
 */
double TCPChannel::roundTripTimeInSeconds() const
{
#if defined( __unix__ ) && !defined( EMSCRIPTEN )
	struct tcp_info info;
	socklen_t infoSize = sizeof( info );
	if (-1 == getsockopt( pEndpoint_->fileno(), IPPROTO_TCP, TCP_INFO,
			&info, &infoSize ))
	{
		return 1.0; // If there was a problem, default to this.
	}
	return double( info.tcpi_rtt ) / 1e6;
#else // !defined( __unix__ ) || defined( EMSCRIPTEN )
	return 1.0;
#endif // defined( __unix__ ) && !defined( EMSCRIPTEN )
}


/*
 *	Override from Channel.
 */
void TCPChannel::doDestroy()
{
	this->dispatcher().deregisterFileDescriptor( pEndpoint_->fileno() );

	if (pSendBuffer_)
	{
		this->dispatcher().deregisterWriteFileDescriptor(
			pEndpoint_->fileno() );

		bw_safe_delete( pSendBuffer_ );
	}

	pEndpoint_->close();

	pStreamFilter_ = NULL;
}


/*
 *	Override from Channel.
 *
 *	This works by adding an encryption stream filter on top of any pre-existing
 *	filter. Make sure to set lower-level stream filters on the channel (e.g.
 *	WebSocketsStreamFilter) before you set encryption.
 *
 *	There is currently no provision for removing an encryption filter
 *	after one has been set. Calling this more than once will result multiple
 *	encryption layers being active. Calling it with NULL will not remove
 *	existing filters, and will likely cause a seg fault.
 *
 *	TODO: Fix this.

 *	@param pBlockCipher 	The block cipher to use for the new encryption
 *							filter.
 */
void TCPChannel::setEncryption( Mercury::BlockCipherPtr pBlockCipher )
{
	Mercury::NetworkStreamPtr pStream = pStreamFilter_; 
	Mercury::NetworkStreamPtr pChannelStream;

	if (!pStream)
	{
		pChannelStream = new TCPChannelStreamAdaptor( *this );
		pStream = pChannelStream;
	}

	pStreamFilter_ = EncryptionStreamFilter::create( *pStream, pBlockCipher );
}


/*
 *	Override from Channel.
 */
void TCPChannel::shutDown()
{
	if ((pStreamFilter_.get() != NULL) &&
			!pStreamFilter_->didFinishShuttingDown())
	{
		// A filter in the stream filter stack wants to do something to
		// shutdown gracefully. When that's done, they'll call shutDown() again
		// and eventually all filters in the stack will return false in
		// didFinishShuttingDown().
		return;
	}

	isShuttingDown_ = true;

	if (pSendBuffer_ == NULL) // Wait for send buffer to empty
	{
		this->doShutDown();
	}
}


/**
 *	This method initiates the shutdown procedure.
 */
void TCPChannel::doShutDown()
{
#if defined( EMSCRIPTEN )
	// TODO: Need to remove this and use shutdown() when it's supported. In its
	// current state, close() is actually a no-op anyway. :(
	pEndpoint_->close();
#elif defined( _WIN32 )
	shutdown( pEndpoint_->fileno(), SD_SEND );
#else // !defined( EMSCRIPTEN ) && !defined( _WIN32 )
	shutdown( pEndpoint_->fileno(), SHUT_WR );
#endif // defined( EMSCRIPTEN )

	// And in response, the peer should shutdown their side, and we'll get that
	// in the form of a 0-length read, and can close the endpoint and destroy
	// the channel.
}


/*
 *	Override from Channel.
 */
void TCPChannel::startInactivityDetection( float period,
		float checkPeriod /* = 1.f */ )
{
	pEndpoint_->setSocketOption( SOL_SOCKET, SO_KEEPALIVE, true );

#if defined( __unix__ )
	// We check every second up to checkPeriod seconds before dropping.
	pEndpoint_->setSocketOption( IPPROTO_TCP, TCP_KEEPIDLE, int( 1 ) );
	pEndpoint_->setSocketOption( IPPROTO_TCP, TCP_KEEPINTVL, int( 1 ) );
	pEndpoint_->setSocketOption( IPPROTO_TCP, TCP_KEEPCNT, int( checkPeriod ) );
#endif
}


/*
 *	Override from Channel.
 */
void TCPChannel::onChannelInactivityTimeout()
{
	// We are relying on the kernel to detect this and shutdown the socket,
	// which we will detect.

	CRITICAL_MSG( "TCPChannel::onChannelInactivityTimeout: "
		"We should not have been called\n" );
}


/**
 * 	This method reads any available data from the socket's read buffer into the
 * 	given output stream.
 *
 *	@param output 	The output stream.
 *	@return 		The number of bytes read from the socket and placed into
 *					the output stream.
 */
int TCPChannel::readInto( BinaryOStream & output )
{
	if (this->isDestroyed())
	{
		WARNING_MSG( "TCPChannel::readInto( %s ): "
				"Reading from a destroyed channel\n",
			this->c_str() );
		return -1;
	}

	int numBytesToRead = this->numBytesAvailableForReading();

	// We check for disconnections in handleInput(), so the socket should be
	// valid.
	MF_ASSERT( numBytesToRead > 0 );

	void * buffer = output.reserve( numBytesToRead );
	int numBytesActuallyRead = pEndpoint_->recv( buffer, numBytesToRead );
	MF_ASSERT( numBytesToRead == numBytesActuallyRead );

	return numBytesToRead;
}


/**
 *	This method writes the given input data to the channel.
 *
 *	@param input 		The input data stream.
 *	@param shouldCork 	Whether we should send any corked data and this data
 *						now, or cork the data for sending later.
 */
bool TCPChannel::writeFrom( BinaryIStream & input, bool shouldCork )
{
	int numToSend = input.remainingLength();

	if (pSendBuffer_)
	{
		// We're already waiting for the socket to become available for
		// writing, add it to the end.
		pSendBuffer_->transfer( input, numToSend );
		return true;
	}

	int sendResult = this->basicSend( input.retrieve( 0 ), numToSend,
		shouldCork );

	if ((sendResult == -1) && !isErrnoIgnorable())
	{
		NOTICE_MSG( "TCPChannel::writeFrom( %s ): "
				"Write error, destroying; error: %s\n",
			this->c_str(), lastNetworkError() );

		input.finish();

		this->destroy();
		return false;
	}
	else if (sendResult == -1)
	{
		sendResult = 0;
	}

	input.retrieve( sendResult );

	if (sendResult < numToSend)
	{
		// Got short count when sending. Add the rest to the send buffer,
		// creating it and registering the socket for write events if
		// necessary.

		const uint8 * leftOverData =
			reinterpret_cast< const uint8 * >( input.retrieve(
				numToSend - sendResult ) );

		if (!pSendBuffer_)
		{
			pSendBuffer_ = new MemoryOStream( this->maxSegmentSize() );

			this->dispatcher().registerWriteFileDescriptor(
				pEndpoint_->fileno(),
				pSendWaiter_,
				"TCPChannel" );
		}

		pSendBuffer_->addBlob( leftOverData, numToSend - sendResult );
	}

	return true;
}


/**
 *	This method handles the basic operation of sending data.
 *
 *	@param data 		The data to send.
 *	@param size 		The size of the data in bytes.
 *	@param shouldCork 	Whether to cork the send or not. Corked sends will
 *						accumulate until an uncorked send is given, at which
 *						point the actual send will take place with the
 *						accumulated corked data.
 *	@return 			-1 on error, or the number of bytes sent successfully
 *						(can be zero or less than the given size if there is
 *						insufficient send buffer space is available).
 */
int TCPChannel::basicSend( const void * data, int size, bool shouldCork )
{
	if (shouldCork)
	{
#if defined( __unix__ ) && !defined( EMSCRIPTEN )
		pEndpoint_->setSocketOption( IPPROTO_TCP, TCP_CORK, true );
#else // !defined( __unix__ )
		// No TCP_CORK option, emulate using the send buffer and deferring the
		// send.
		return 0;
#endif // defined( __unix__ )
	}

	int sendResult = pEndpoint_->send( data, size );

	if ((sendResult == -1) && !isErrnoIgnorable())
	{
		ERROR_MSG( "TCPChannel::basicSend( %s ): Error sending data: %s\n",
			this->c_str(), strerror( errno ) );
		return sendResult;
	}

#if defined( __unix__ ) && !defined( EMSCRIPTEN )
	if (!shouldCork)
	{
		pEndpoint_->setSocketOption( IPPROTO_TCP, TCP_CORK, false );
	}
#endif // defined( __unix__ )

	return sendResult;
}


/**
 *	This method sends as much buffered data as is possible.
 *
 *	@return 	True if the send occurred, or false if sending failed.
 */
bool TCPChannel::sendBufferedData()
{
	int sendResult = this->basicSend( pSendBuffer_->retrieve( 0 ),
		pSendBuffer_->remainingLength() );

	if ((sendResult == -1) && !isErrnoIgnorable())
	{
		ERROR_MSG( "TCPChannel::sendBufferedData( %s ): "
				"Send failed, disconnecting: %s\n",
			this->c_str(), lastNetworkError() );

		return false;
	}
	else if (sendResult <= 0)
	{
		return true;
	}

	if (sendResult < pSendBuffer_->remainingLength())
	{
		// Still data left, advance the cursor and wait until we can send
		// again.
		pSendBuffer_->retrieve( sendResult );
	}
	else
	{
		// All remaining data sent, no need to keep waiting on send.
		this->dispatcher().deregisterWriteFileDescriptor(
			pEndpoint_->fileno() );

		bw_safe_delete( pSendBuffer_ );

		if (isShuttingDown_)
		{
			// Shut down was waiting on our sends to complete, do it now.
			this->doShutDown();
		}
	}

	return true;
}


/**
 *	This method is called to handle when the peer has disconnected us.
 */
void TCPChannel::handlePeerDisconnect()
{
	if (isShuttingDown_)
	{
		TRACE_MSG( "TCPChannel::handlePeerDisconnect( %s ): Shutdown cleanly\n",
			this->c_str() );
	}
	else
	{
		TRACE_MSG( "TCPChannel::handlePeerDisconnect( %s ): "
				"Disconnected by peer\n",
			this->c_str() );
	}

	this->destroy();
}


#if ENABLE_WATCHERS
/*
 *	Override from WatcherProvider via Channel.
 */
WatcherPtr TCPChannel::getWatcher()
{
	static WatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = this->createBaseWatcher();

#define ADD_WATCHER( PATH, MEMBER )		\
		pWatcher->addChild( #PATH, makeWatcher( &TCPChannel::MEMBER ) );

		ADD_WATCHER( maxSegmentSize, 	maxSegmentSize );
#undef ADD_WATCHER
	}

	return pWatcher;
}

#endif // ENABLE_WATCHERS

} // end namespace Mercury


BW_END_NAMESPACE

// tcp_channel.cpp

