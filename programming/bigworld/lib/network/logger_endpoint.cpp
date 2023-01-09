// logger_endpoint.cpp

#include "pch.hpp"

#include "cstdmf/config.hpp"
#include "cstdmf/bw_memory.hpp"
#include "cstdmf/profiler.hpp"

#if ENABLE_WATCHERS

#include "network/endpoint.hpp"
#include "network/event_dispatcher.hpp"
#include "network/misc.hpp"
#include "network/network_utils.hpp"

#include "logger_endpoint.hpp"
#include "logger_message_forwarder.hpp"

#if defined( _MSC_VER )
// suppress no matching operator delete found warning
#pragma warning( push )
#pragma warning( disable : 4291 )
#endif

BW_BEGIN_NAMESPACE

namespace
{
const int registerWriteTimeout = 500000; // 500ms

const uint32 maxBufferedSize = 1024;

// For profiling the amount of message data waiting to be transmitted
// to loggers.
PROFILER_LEVEL_COUNTER( senderBufferLevel );

} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: LoggerEndpointMemberHandler
// -----------------------------------------------------------------------------

/**
 * Input Notification handler functor for LoggerEndpoint
 */
class LoggerEndpointMemberHandler :
	public Mercury::InputNotificationHandler
{
	typedef int (LoggerEndpoint::*NotifyCallback)( int fd );
public:
	LoggerEndpointMemberHandler( LoggerEndpoint & endpoint,
		NotifyCallback callback ) :
		endpoint_( endpoint ),
		callback_( callback )
	{}

	virtual int handleInputNotification( int fd )
	{
		return (endpoint_.*callback_)( fd );
	}

	// Only allow shutdown if no packets remaining
	virtual bool isReadyForShutdown() const
	{
		return endpoint_.loggerState_ == LoggerEndpoint::LOGGER_ENDPOINT_ERROR ||
					endpoint_.nextPacket_ == NULL;
	}

private:
	LoggerEndpoint & endpoint_;
	NotifyCallback callback_;
};


// -----------------------------------------------------------------------------
// Section: LoggerEndpointNeedWriteTimeout
// -----------------------------------------------------------------------------
class LoggerEndpointNeedWriteTimeout :
	public TimerHandler
{
public:
	LoggerEndpointNeedWriteTimeout( LoggerEndpoint & loggerEndpoint ) :
		loggerEndpoint_( loggerEndpoint )
		{}
	virtual void handleTimeout( TimerHandle handle, void * arg );
private:
	LoggerEndpoint & loggerEndpoint_;
};

/**
 *	This method handles timeouts from timers attached to the dispatcher
 *	the only timer which is registered is the timer for checking NEED_WRITE
 *	so the tcpEndpoint can be registered with the event dispatcher
 *	within the main thread.
 *
 *	@param handle	The timer handle, this will always be registerWriteTimer_
 *	@param arg		The timer argument, this will always be NULL
 */
void LoggerEndpointNeedWriteTimeout::handleTimeout(
	TimerHandle handle, void * arg )
{
	RecursiveMutexHolder smh( loggerEndpoint_.mutex_ );
	if (loggerEndpoint_.loggerState_ ==
		LoggerEndpoint::LOGGER_ENDPOINT_NEED_WRITE)
	{
		loggerEndpoint_.loggerState_ =
			LoggerEndpoint::LOGGER_ENDPOINT_REGISTERED_WRITE;
		loggerEndpoint_.dispatcher_.registerWriteFileDescriptor(
			loggerEndpoint_.tcpEndpoint_.fileno(),
			loggerEndpoint_.pTcpWriteHandler_, "LoggerEndpointTcpWrite" );
	}
}


// -----------------------------------------------------------------------------
// Section: BufferedLoggerPacket
// -----------------------------------------------------------------------------

/**
 *	This class is used for storing buffered packets within LoggerEndpoint
 *	be sure to use the overloaded new operator.
 */
class BufferedLoggerPacket
{
public:
	/**
	 *	This method is the default constructor, the main allocation happens
	 *	within the overloaded new operator
	 */
	BufferedLoggerPacket( ) :
		pNext_( NULL ),
		offset_( 0 )
	{
	}


	/**
	 *	This class works by storing the buffer at the end of its class in order
	 *	to achieve this we overload the new operator, to allocate extra space
	 *
	 *	@param size The size of the memory to allocate
	 *	@param buffer The memory ostream for this packet
	 *
	 *	@return The allocated memory, with the buffer appended to it
	 */
	void * operator new( size_t size, MemoryOStream & buffer )
	{
		char * buf = (char*)bw_malloc( size + buffer.size() + sizeof(int32) );
		char * data = buf + size;
		*((int32*)data) = buffer.size();
		memcpy( data + sizeof(int32), (char*)buffer.data(), buffer.size());
		return buf;
	}


	/**
	 *	The overloaded delete operator to free the memory using the same
	 *	malloc/free methods
	 */
	void operator delete( void * ptr )
		{ bw_free( ptr ); }


	/**
	 *	Get the pointer to the next buffered packet, NULL if no packet is next
	 *
	 *	@return The next buffered packet
	 */
	BufferedLoggerPacket * pNext()
		{ return pNext_; }


	/**
	 *	Set the pointer to the next buffered packet
	 *
	 *	@param pNext The next buffer packet
	 */
	void pNext( BufferedLoggerPacket * pNext )
	{
		MF_ASSERT( pNext_ == NULL );
		pNext_ = pNext;
	}


	/**
	 *	Get the packet data
	 *
	 *	@return The data within this buffered packet
	 */
	char * data() const
		{ return ((char*)this) + sizeof(*this); }


	/**
	 *	Get the current offset of the sending of the packet
	 *
	 *	@return The current offset
	 */
	int offset() const
		{ return offset_; }


	/**
	 *	Seek the offset of the current packet by offset
	 *
	 *	@param offset The offset to seek by
	 */
	void seek( int offset )
		{ offset_ += offset; }


	/**
	 *	Get the current length of the packet.
	 *
	 *	@return The length of the packet
	 */
	int len() const
		{ return sizeof(int32) + *(int*)(this->data()); }

	/**
	 * Create a new BufferedLoggerPacket
	 */
	static BufferedLoggerPacket * create( 
			MemoryOStream & ostream )
	{
		return new (ostream) BufferedLoggerPacket();
	}
private:
	BufferedLoggerPacket * pNext_;
	int offset_;
};

// -----------------------------------------------------------------------------
// Section: LoggerEndpoint
// -----------------------------------------------------------------------------

/**
 *	LoggerEndpoint constructor, this is used to create an Endpoint for
 *	communicating with a MessageLogger instance, no connection is established
 *	within the constructor, you must use the LoggerEndpoint::init method
 *
 *	@param dispatcher		The event dispatcher to register the sockets on
 *	@param logForwarder		The log message forwarder which this LoggerEndpoint 
 *							is added to
 *	@param remoteAddress	The remote address of the MessageLogger
 *
 *	@see init
 */
LoggerEndpoint::LoggerEndpoint(
		Mercury::EventDispatcher & dispatcher,
		LoggerMessageForwarder * logForwarder,
		const Mercury::Address & remoteAddress ) :
	loggerState_( LOGGER_ENDPOINT_UNINITALIZED ),
	dispatcher_( dispatcher ),
	logForwarder_( logForwarder ),
	consecutiveReconnects_( 0 ),
	registerWriteTimer_(),
	pWriteTimeoutHandler_( NULL ),
	tcpEndpoint_( ),
	pTcpConnectHandler_( NULL ),
	pTcpReadHandler_( NULL ),
	pTcpWriteHandler_( NULL ),
	udpEndpoint_( ),
	pUdpReadHandler_( NULL ),
	remoteAddress_( remoteAddress ),
	bufferCount_( 0 ),
	lastPacket_( NULL ),
	nextPacket_( NULL ),
	isTCP_( true ),
	receivedSize_( 0 ),
	messageSize_( 0 ),
	receivedBuffer_( NULL ),
	droppedMessageCount_( 0 ),
	mutex_(),
	majorVersion_( 0 ),
	minorVersion_( 0 ),
	isSendingExceededBufferedMessage_( false )
{
	pWriteTimeoutHandler_ = new LoggerEndpointNeedWriteTimeout( *this );

	pTcpConnectHandler_ = new LoggerEndpointMemberHandler( *this,
		&LoggerEndpoint::handleConnectTCP );
	pTcpReadHandler_ = new LoggerEndpointMemberHandler( *this,
		&LoggerEndpoint::handleReadTCP );
	pTcpWriteHandler_ = new LoggerEndpointMemberHandler( *this,
		&LoggerEndpoint::handleWriteTCP );

	pUdpReadHandler_ = new LoggerEndpointMemberHandler( *this,
		&LoggerEndpoint::handleReadUDP );
}


/**
 *	Endpoint destructor, this method will also send a watcher message
 *	to detach this component for MessageLoggers which do not support TCP
 */
LoggerEndpoint::~LoggerEndpoint()
{
	registerWriteTimer_.cancel();

	while (nextPacket_ != NULL)
	{
		BufferedLoggerPacket * next = nextPacket_->pNext();
		PROFILER_LEVEL_COUNTER_DEC( senderBufferLevel, 
			nextPacket_->len() - nextPacket_->offset() );
		bw_safe_delete( nextPacket_ );
		nextPacket_ = next;
	}

#if defined( MF_SERVER )
	if (udpEndpoint_.good())
	{
		this->sendDelete();
	}
#endif // defined( MF_SERVER )

	this->disconnectTCP();
	this->disconnectUDP();

	bw_safe_delete( pWriteTimeoutHandler_ );
	bw_safe_delete( pTcpConnectHandler_ );
	bw_safe_delete( pTcpReadHandler_ );
	bw_safe_delete( pTcpWriteHandler_ );
	bw_safe_delete( pUdpReadHandler_ );
}


/**
 *	This method establishes a connection to the remote endpoint, for older
 *	versions of MessageLogger the action of deleting connections did not work
 *	over TCP, in order to allow the component to be deleted from MessageLogger
 *	an additional UDP socket needs to be created, this is controlled with the
 *	supportsFullTCP parameter
 *
 *  @param majorVersion the bigworld major version number
 *  @param minorVersion the bigworld minor version number
 *
 *	@return true if connection is made successful, false other
 */
bool LoggerEndpoint::init( uint16 majorVersion, uint16 minorVersion )
{
	MF_ASSERT( loggerState_ == LOGGER_ENDPOINT_UNINITALIZED );

	if (!this->reconnect())
	{
		return this->enableUDP();
	}

	registerWriteTimer_ = dispatcher_.addTimer( registerWriteTimeout,
		pWriteTimeoutHandler_, NULL, "LoggerEndpointWrite" );

	majorVersion_ = majorVersion;
	minorVersion_ = minorVersion;

#if defined( MF_SERVER )
	if (MessageLogger::versionShouldOpenUDP( majorVersion, minorVersion ))
	{
		TRACE_MSG( "LoggerEndpoint::LoggerEndpoint: Due to MessageLogger at "
				"%s having an older version of MessageLogger. An additional "
				"UDP connection was required, to avoid this upgrade the "
				"MessageLogger service\n",
			remoteAddress_.c_str() );

		return this->enableUDP();
	}
#endif // defined( MF_SERVER )

	return true;
}


/**
 *	This method appends to the buffered packet list
 *
 *	@note This method is INLINE, in an attempt to get the compiler to
 *		ignore unnecessary branches.
 */
inline void LoggerEndpoint::appendPacket( BufferedLoggerPacket * pPacket )
{
	++bufferCount_;

	if (lastPacket_ != NULL)
	{
		lastPacket_->pNext( pPacket );
	}

	lastPacket_ = pPacket;

	if (nextPacket_ == NULL)
	{
		nextPacket_ = pPacket;
	}

	if (loggerState_ == LOGGER_ENDPOINT_WAITING)
	{
		loggerState_ = LOGGER_ENDPOINT_NEED_WRITE;
	}
}


/**
 *	This method forces the socket to use UDP rather then having a TCP connection
 *	this may be useful if a TCP connection has failed to connect
 *
 *	@return True if the UDP socket could be created, false otherwise
 */
bool LoggerEndpoint::enableUDP()
{
	udpEndpoint_.socket( SOCK_DGRAM );

	if (!udpEndpoint_.good())
	{
		ERROR_MSG( "LoggerEndpoint::enableUDP: "
			"Failed to UDP socket for %s\n", remoteAddress_.c_str() );
		return false;
	}

	udpEndpoint_.setnonblocking( true );

	int bindResult = -1;

#if defined( MF_SERVER )
	if (tcpEndpoint_.good())
	{
		u_int16_t localPort = 0;
		u_int32_t localIP = 0;
		tcpEndpoint_.getlocaladdress( &localPort, &localIP );

		bindResult = udpEndpoint_.bind( localPort, localIP );
	}
	else
	{
		bindResult = udpEndpoint_.bind();
		isTCP_ = false;
	}
#else // defined( MF_SERVER )
	bindResult = udpEndpoint_.bind();
	isTCP_ = false;
#endif // defined( MF_SERVER )

	if (bindResult == -1)
	{
		ERROR_MSG( "LoggerEndpoint::enableUDP: "
			"Failed to bind UDP socket\n" );
		return false;
	}
	dispatcher_.registerFileDescriptor( udpEndpoint_.fileno(),
		pUdpReadHandler_, "LoggerEndpointUdpRead" );

	return true;
}


/**
 *	To provide backwards compatibility we must add the ability to handle
 *	deregister the new TCP socket created, this is done using the
 *	watchers
 */
void LoggerEndpoint::sendDelete()
{
	if (!udpEndpoint_.good())
	{
		return;
	}

	char path[64];
	bw_snprintf( path, sizeof(path), "components/%s/attached",
		udpEndpoint_.getLocalAddress().c_str() );
	const int pathSize = static_cast<int>(strlen( path ) + 1);

	const char * value = "false";
	const int valueLen = static_cast<int>(strlen( value ) + 1);

	char buf[ sizeof(WatcherDataMsg) + sizeof(path) + sizeof(value) ];
	WatcherDataMsg & wdm = *(WatcherDataMsg *)buf;
	wdm.message = WATCHER_MSG_SET;
	wdm.count = 1;
	memcpy( wdm.string, path, pathSize );
	memcpy( wdm.string + pathSize, value, valueLen );

	udpEndpoint_.sendto( buf, sizeof(wdm) + pathSize + valueLen,
		remoteAddress_.port, remoteAddress_.ip );
}


/**
 *	This method sends a packet immediately, if not all the buffer can be
 *	sent then the remaining will be buffered
 *
 *	In order to call sendImmediate you must have the mutex locked
 *
 *	@param ostream The packet to send if the queue is empty
 */
void LoggerEndpoint::sendImmediate( MemoryOStream & buffer )
{
	MF_ASSERT( loggerState_ == LOGGER_ENDPOINT_WAITING );
	MF_ASSERT( nextPacket_ == NULL && lastPacket_ == NULL );

	int32 size = buffer.size();
	int sentVal = tcpEndpoint_.send( &size, sizeof(size) );

	if (sentVal < static_cast<int>(sizeof(size)))
	{
		// If we have no queue send
		this->appendPacket( BufferedLoggerPacket::create( buffer ) );

		if (sentVal > 0)
		{
			lastPacket_->seek( sentVal );
			PROFILER_LEVEL_COUNTER_DEC( senderBufferLevel, sentVal );
		}
		return;
	}

	sentVal = tcpEndpoint_.send( buffer.data(), size );

	if (sentVal == size)
	{
		// Successfully sent everything
		PROFILER_LEVEL_COUNTER_DEC( senderBufferLevel, 
			sentVal + sizeof(int32) );
		return;
	}

	// If we have no queue send
	this->appendPacket( BufferedLoggerPacket::create( buffer ) );

	if (sentVal > 0)
	{
		lastPacket_->seek( sentVal + sizeof(int32) );
		PROFILER_LEVEL_COUNTER_DEC( senderBufferLevel, 
			sentVal + sizeof(int32) );
	}
	else
	{
		lastPacket_->seek( sizeof(int32) );
		PROFILER_LEVEL_COUNTER_DEC( senderBufferLevel, sizeof(int32) );
	}
}


/**
 *	This method handles sending of buffered packets to the MessageLogger
 *	endpoint.
 *
 *	@return true if no errors occurred, false otherwise
 */
bool LoggerEndpoint::sendBuffered()
{
	MF_ASSERT( loggerState_ == LOGGER_ENDPOINT_REGISTERED_WRITE );

	while (nextPacket_ != NULL)
	{
		int remaining = (nextPacket_->len() - nextPacket_->offset());

		int sentVal = tcpEndpoint_.send(
			nextPacket_->data() + nextPacket_->offset(), remaining );

		if (sentVal < 0)
		{
			if (!isErrnoIgnorable())
			{
				// Remove packet
				BufferedLoggerPacket * next = nextPacket_->pNext();
				if (lastPacket_ == nextPacket_)
				{
					lastPacket_ = NULL;
				}

				// Account for dropped data in the buffer level profiling.
				PROFILER_LEVEL_COUNTER_DEC( senderBufferLevel, remaining );

				bw_safe_delete( nextPacket_ );
				--bufferCount_;
				nextPacket_ = next;

				++droppedMessageCount_;

				if (MainThreadTracker::isCurrentThreadMain())
				{
					// Reconnect
					if (!this->reconnect())
					{
						loggerState_ = LOGGER_ENDPOINT_ERROR;
						return false;
					}
				}

				return false;
			}

			return true;
		}


		PROFILER_LEVEL_COUNTER_DEC( senderBufferLevel, sentVal );
		if (sentVal < remaining)
		{
			nextPacket_->seek( sentVal );
			return true;
		}

		BufferedLoggerPacket * next = nextPacket_->pNext();
		bw_safe_delete( nextPacket_ );
		--bufferCount_;
		nextPacket_ = next;
	}

	lastPacket_ = NULL;
	dispatcher_.deregisterWriteFileDescriptor( tcpEndpoint_.fileno() );
	loggerState_ = LOGGER_ENDPOINT_WAITING;

	return true;
}


/**
 *	Disconnect the TCP connection if it exists
 */
void LoggerEndpoint::disconnectTCP()
{
	bw_safe_delete_array( receivedBuffer_ );
	receivedSize_ = 0;
	if (tcpEndpoint_.good())
	{
		if (loggerState_ == LOGGER_ENDPOINT_REGISTERED_WRITE ||
			loggerState_ == LOGGER_ENDPOINT_CONNECTING)
		{
			dispatcher_.deregisterWriteFileDescriptor( tcpEndpoint_.fileno() );
		}

		if (loggerState_ != LOGGER_ENDPOINT_CONNECTING)
		{
			dispatcher_.deregisterFileDescriptor( tcpEndpoint_.fileno() );
		}

		tcpEndpoint_.close();
		tcpEndpoint_.detach();
	}
}


/**
 *	Disconnect the UDP connection if it exists
 */
void LoggerEndpoint::disconnectUDP()
{
	if (udpEndpoint_.good())
	{
		dispatcher_.deregisterFileDescriptor( udpEndpoint_.fileno() );
		udpEndpoint_.close();
		udpEndpoint_.detach();
	}
}


/**
 *	Reconnect to the TCP socket
 *
 *	@return true on success, false on failure
 */
bool LoggerEndpoint::reconnect()
{
	this->disconnectTCP();

	{
		RecursiveMutexHolder rmh( mutex_ );
		loggerState_ = LOGGER_ENDPOINT_CONNECTING;
	}

	// Limit number of reconnects
	if (consecutiveReconnects_ >= 3)
	{
		return false;
	}

	++consecutiveReconnects_;

	tcpEndpoint_.socket( SOCK_STREAM );

	if (!tcpEndpoint_.good())
	{
		ERROR_MSG( "LoggerEndpoint::reconnect: "
			"Failed to create socket for %s\n", remoteAddress_.c_str() );
		return false;
	}

	tcpEndpoint_.setnonblocking( true );

	if ((tcpEndpoint_.connect( remoteAddress_.port, remoteAddress_.ip ) != 0) &&
		!isErrnoIgnorable())
	{
		ERROR_MSG( "LoggerEndpoint::reconnect: Failed to connect to %s\n",
			remoteAddress_.c_str() );
		tcpEndpoint_.close();
		tcpEndpoint_.detach();
		return false;
	}

	dispatcher_.registerWriteFileDescriptor( tcpEndpoint_.fileno(),
		pTcpConnectHandler_, "LoggerEndpointTcpConnect" );

	consecutiveReconnects_ = 0;
	return true;
}


/**
 *	This method will send the packet specified in buffer, to the MessageLogger
 *	if the TCP socket is currently waiting for its buffers to clear, then the
 *	message will be queued until this packet can be sent.
 *
 *	@param buffer The MemoryOStream with the packet data
 *
 *	@return true if the packet was sent or queued, or false if it could not be
 *				sent
 */
bool LoggerEndpoint::send( MemoryOStream & buffer )
{
	RecursiveMutexHolder rmh( mutex_ );

	if (!isTCP_)
	{
		PROFILER_LEVEL_COUNTER_INC( senderBufferLevel, buffer.size() );
		udpEndpoint_.sendto( buffer.data(), buffer.size(), remoteAddress_.port,
			remoteAddress_.ip );
		PROFILER_LEVEL_COUNTER_DEC( senderBufferLevel, buffer.size() );
		return true;
	}

	if (loggerState_ == LOGGER_ENDPOINT_ERROR)
	{
		return false;
	}

	// Simplify buffer tracking by including the int32 size field.
	PROFILER_LEVEL_COUNTER_INC( senderBufferLevel, buffer.size() + sizeof(int32) );

	// If we are not the owner of the mutex, then this is a recursive call
	// so just add to the queue.
	if (!rmh.isOwner() || loggerState_ != LOGGER_ENDPOINT_WAITING)
	{
		if (bufferCount_ > maxBufferedSize &&
			!isSendingExceededBufferedMessage_)
		{
			// The error message will only be logged once when the buffer size
			// exceeds the maximum size, isSendingExceededBufferedMessage_ flag
			// is used due to error message cause a recursive call
			if (rmh.isOwner() && bufferCount_ == (maxBufferedSize + 1))
			{
				isSendingExceededBufferedMessage_ = true;
				ERROR_MSG( "LoggerEndpoint::send: "
					"Buffer exceeds the maximum size %d\n", maxBufferedSize );
				isSendingExceededBufferedMessage_ = false;
			}
			PROFILER_LEVEL_COUNTER_DEC( senderBufferLevel, sizeof(int32) + buffer.size() );
			return false;
		}
		this->appendPacket( BufferedLoggerPacket::create( buffer ) );
		return true;
	}
	
	this->sendImmediate( buffer );
	return true;
}


/**
 *	This method handles received packets to the UDP or TCP endpoints, the only
 *	expected packets to this is a Watcher message for when the MessageLogger
 *	has disconnected
 *
 *	@param packet The raw packet data
 *	@param packetLength The length of packet
 *
 *	@return true if the packet was valid, false otherwise
 */
bool LoggerEndpoint::handlePacket(
	char * packet, int packetLength )
{
	if (packetLength < (int)sizeof(WatcherDataMsg))
	{
		ERROR_MSG( "LoggerEndpoint::handlePacket: "
			"Packet is too short\n" );
		return false;
	}

	WatcherDataMsg * wdm = (WatcherDataMsg*)packet;

	if (wdm->message == WATCHER_MSG_TELL)
	{
		// Tell messages can be ignored, should only come from
		// the destructor, sending the removal of the component.
		return false;
	}

	if (wdm->message != WATCHER_MSG_SET || wdm->count != 1)
	{
		ERROR_MSG( "LoggerEndpoint::handlePacket: "
			"Unexpected watcher message %d\n", wdm->message );
		return false;
	}

	const int pathOffset = sizeof(wdm->message) + sizeof(wdm->count);
	char * path = packet + pathOffset;
	int pathLen = 0;
	while (pathLen < (packetLength - pathOffset)
		&& path[pathLen] != 0)
	{
		++pathLen;
	}

	// Add null just in case we went past the packet end.
	path[pathLen] = 0;

	if (strcmp( path, "logger/del" ) == 0)
	{
		this->disconnectUDP();
		this->disconnectTCP();
		logForwarder_->delLogger( remoteAddress_ );
		return true;
	}

	ERROR_MSG( "LoggerEndpoint::handlePacket: "
		"Unexpected watcher set value for %s.\n", path );
	return false;
}


/**
 *	Returns whether this logger should send metadata.
 *
 *	@return whether it should send metadata.
 */
bool LoggerEndpoint::shouldSendMetaData( ) const
{
	return MessageLogger::versionSupportsMetaData( majorVersion_,
													minorVersion_ );
}


/**
 *	Returns whether this logger should stream metadata as a series of arguments.
 *
 *	@return whether it should stream metadata as arguments.
 */
bool LoggerEndpoint::shouldStreamMetaDataArgs( ) const
{
	return MessageLogger::versionSupportsMetaDataArgs( majorVersion_,
													minorVersion_ );
}


/**
 *	This method handles the read input notification from the UDP socket
 *
 *	@param fd The socket/file disruptor
 *
 *	@return The return value is ignored, should return 0.
 */
int LoggerEndpoint::handleReadUDP( int fd )
{
	sockaddr_in senderAddr;
	char packet[WN_PACKET_SIZE];
	int len = udpEndpoint_.recvfrom( packet, WN_PACKET_SIZE, senderAddr );

	if (len != -1)
	{
		this->handlePacket( packet, len );
	}
	return 0;
}


/**
 *	This method handles the connect input notification from the TCP socket
 *
 *	@param fd The socket/file disruptor
 *
 *	@return The return value is ignored, should return 0.
 */
int LoggerEndpoint::handleConnectTCP( int fd )
{
	/*
		We don't use the mutex here for a few reasons:
			- When sending the loggerState_ should already be connecting, so the
				queue will just be used
			- When a failure occurs the delLogger will do a write lock
				which will prevent the reads from triggering a send
			- Having a mutex holder would mean the mutex gets released
				after the memory is released
	*/
	MF_ASSERT( loggerState_ == LOGGER_ENDPOINT_CONNECTING );

	int32 errcode = -1;
	socklen_t errcodesize = sizeof(errcode);
	int err = getsockopt( tcpEndpoint_.fileno(), SOL_SOCKET, SO_ERROR,
		(char*)&errcode, &errcodesize );

	if (err != 0 || errcodesize != sizeof(errcode) || errcode != 0)
	{
		this->disconnectTCP();
		logForwarder_->delLogger( remoteAddress_ );
		return 0;
	}

	// Remove connect handler
	dispatcher_.deregisterWriteFileDescriptor(
			tcpEndpoint_.fileno() );

	dispatcher_.registerFileDescriptor(
			tcpEndpoint_.fileno(), pTcpReadHandler_, "LoggerEndpointTcpRead" );

	if (nextPacket_ == NULL)
	{
		loggerState_ = LOGGER_ENDPOINT_WAITING;
	}
	else
	{
		dispatcher_.registerWriteFileDescriptor(
			tcpEndpoint_.fileno(), pTcpWriteHandler_, "LoggerEndpointTcpWrite" );
		loggerState_ = LOGGER_ENDPOINT_REGISTERED_WRITE;
	}

	if (droppedMessageCount_)
	{
		ERROR_MSG( "LoggerEndpoint::handleConnectTCP(%s): "
			"%d messages were dropped during reconnect process\n",
			remoteAddress_.c_str(), droppedMessageCount_ );
		droppedMessageCount_ = 0;
	}

	return 0;
}


/**
 *	This method handles the read input notification from the TCP socket
 *
 *	@param fd The socket/file disruptor
 *
 *	@return The return value is ignored, should return 0.
 */
int LoggerEndpoint::handleReadTCP( int fd )
{
	if (receivedBuffer_ == NULL)
	{
		int size = tcpEndpoint_.recv( ((char *)&messageSize_) + receivedSize_,
			sizeof(messageSize_) - receivedSize_ );

		if (size == 0)
		{
			this->disconnectTCP();
			logForwarder_->delLogger( remoteAddress_ );
			return 0;
		}

		if (size < 0 && !isErrnoIgnorable())
		{
			if (!this->reconnect())
			{
				loggerState_ = LOGGER_ENDPOINT_ERROR;
			}
			return 0;
		}

		if (size > 0)
		{
			receivedSize_ += size;
		}

		if (receivedSize_ == sizeof(messageSize_))
		{
			if (messageSize_ > uint32(WN_PACKET_SIZE_TCP))
			{
				ERROR_MSG( "LoggerEndpoint::handleReadTCP: "
					"Invalid message size %d from %s\n",
					messageSize_, remoteAddress_.c_str() );
				messageSize_ = 0;
				receivedSize_ = 0;
			}
			else
			{
				receivedBuffer_ = new char[ messageSize_ ];
				receivedSize_ = 0;
			}
		}
	}

	if (receivedBuffer_ != NULL)
	{
		int size = tcpEndpoint_.recv( receivedBuffer_ + receivedSize_,
			messageSize_ - receivedSize_ );

		if (size == 0)
		{
			this->disconnectTCP();
			logForwarder_->delLogger( remoteAddress_ );
			return 0;
		}

		if (size < 0 && !isErrnoIgnorable())
		{
			if (!this->reconnect())
			{
				loggerState_ = LOGGER_ENDPOINT_ERROR;
			}
			return 0;
		}

		if (size > 0)
		{
			receivedSize_ += size;
		}

		if (receivedSize_ == messageSize_)
		{
			if (this->handlePacket( receivedBuffer_, messageSize_ ))
			{
				return 0;
			}

			bw_safe_delete_array( receivedBuffer_ );
			receivedBuffer_ = NULL;
			receivedSize_ = 0;
			messageSize_ = 0;
		}
	}

	return 0;
}


/**
 *	This method handles the write input notification from the TCP socket
 *
 *	@param fd The socket/file disruptor
 *
 *	@return The return value is ignored, should return 0.
 */
int LoggerEndpoint::handleWriteTCP( int fd )
{
	RecursiveMutexHolder smh( mutex_ );
	this->sendBuffered();
	return 0;
}


/**
 * Get the logger state as a const-char pointer
 */
const char * LoggerEndpoint::getLoggerState() const
{
	switch(loggerState_)
	{
	case LOGGER_ENDPOINT_UNINITALIZED:
		return "UNINITALIZED";
	case LOGGER_ENDPOINT_CONNECTING:
		return "CONNECTING";
	case LOGGER_ENDPOINT_WAITING:
		return "WAITING";
	case LOGGER_ENDPOINT_NEED_WRITE:
		return "NEED_WRITE";
	case LOGGER_ENDPOINT_REGISTERED_WRITE:
		return "REGISTERED_WRITE";
	case LOGGER_ENDPOINT_ERROR:
		return "ERROR";
	default:
		return "UNKNOWN";
	}
}

#if ENABLE_WATCHERS
/**
 *	Logger Endpoint watcher
 */
WatcherPtr LoggerEndpoint::pWatcher()
{
	WatcherPtr pWatcher( new DirectoryWatcher() );

	// NOTE: bufferCount_ is access without lock
	pWatcher->addChild( "bufferSize",
		makeWatcher( &LoggerEndpoint::bufferCount_ ) );
	pWatcher->addChild( "loggerState", 
		makeWatcher( &LoggerEndpoint::getLoggerState ) );

	LoggerEndpoint* pNull = NULL;
	pWatcher->addChild( "address", &Mercury::Address::watcher(),
		&pNull->remoteAddress_ );

	return pWatcher;
}
#endif


/**
 *	Overloaded comparison operator for handling LoggerEndpoint pointers compared
 *	with addresses, this helps with storing LoggerEndpoints within an BW::map
 *
 *	@param lhs The left-hand side operand
 *	@param rhs The right-hand side operand
 *
 *	@return true if the Endpoint has the same address, false otherwise
 */
bool operator==( LoggerEndpoint * lhs, const Mercury::Address & rhs )
{
	return lhs->addr().ip == rhs.ip &&
		lhs->addr().port == rhs.port;
}

BW_END_NAMESPACE

#if defined( _MSC_VER )
#pragma warning( pop )
#endif

#endif /* ENABLE_WATCHERS */

// logger_endpoint.cpp
