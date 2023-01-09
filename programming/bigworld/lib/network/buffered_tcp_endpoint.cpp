// buffered_tcp_endpoint.cpp

#include "pch.hpp"

#include "buffered_tcp_endpoint.hpp"

#include "network/event_dispatcher.hpp"
#include "network/network_utils.hpp"

BW_BEGIN_NAMESPACE



/**
 *	BufferedTcpEndpoint constructor, this is used to create an Endpoint for
 *	communicating with non-blocking tcp connection accepted from listener.
 *
 *	@param dispatcher	The event dispatcher to register the sockets on
 *	@param endpoint		The socket connection accepted.
 *
 *	@return none.
 */
BufferedTcpEndpoint::BufferedTcpEndpoint(
		Mercury::EventDispatcher & dispatcher, Endpoint & endpoint ) : 
	dispatcher_( dispatcher ),
	endpoint_( endpoint )
{
	endpoint.setnonblocking( true );
}


/**
 *	BufferedTcpEndpoint destructor.
 */
BufferedTcpEndpoint::~BufferedTcpEndpoint()
{
	if (this->getEndpoint().good() && bufferedStream_.size())
	{
		dispatcher_.deregisterWriteFileDescriptor(
					this->getEndpoint().fileno() );
	}
}


/**
 *	Append a data buffer to the buffered stream.
 *
 *	@param data		the data to be buffered.
 *	@param size		the size of the data.
 *
 *	@return none.
 */
void BufferedTcpEndpoint::appendToBufferedStream( 
			const char * data, int size ) const
{
	bufferedStream_.addBlob( data, size );
}

/**
 *	Attempt to send with specified buffer and size
 *
 *	@param content		the buffer to be sent.
 *	@param size		the size of the buffer.
 *
 *	@return the actual sent bytes upon success;
 *		-1 if non-ignorable errno detected.
 */
int BufferedTcpEndpoint::sendContent( const char *content, int size ) const
{
	int sentVal = this->getEndpoint().send( content, size );
	if (sentVal < size)
	{
		if (!isErrnoIgnorable())
		{
			sentVal = -1;
		}
		else if (sentVal < 0)
		{
			sentVal = 0;
		}
	}
	return sentVal;
}


/**
 *	This method flushes the data in the buffered stream.
 *	If the buffered data can not be completely sent it will
 *	be triggered by the next write notification.
 *
 *	@return bytes sent in last send if no errors occurred,
 *		-1 otherwise
 */
int BufferedTcpEndpoint::flushBufferedStream() const
{
	int sentVal = 0;

	sentVal = this->sendContent( 
			(const char *)bufferedStream_.retrieve( 0 ),
			bufferedStream_.remainingLength() );

	if (sentVal > 0)
	{
		bufferedStream_.retrieve( sentVal );
	}
	else if (sentVal == -1)
	{
		int err = errno;
		WARNING_MSG( "BufferedTcpEndpoint::send: "
			"Could not send : %s, remaining %d bytes.\n",
			strerror( err ),
			bufferedStream_.remainingLength() );

		bufferedStream_.retrieve( bufferedStream_.remainingLength() );
	}

	if (bufferedStream_.size() == 0)
	{
		dispatcher_.deregisterWriteFileDescriptor( 
					this->getEndpoint().fileno() );
	}

	return sentVal;
}


/**
 *	This method will send the data specified in buffer,
 *	if the TCP socket is currently waiting for its buffers to clear,
 *	then the message will be queued until this data can be sent.
 *
 *	@param data		the buffer with the data to be sent
 *	@param size		the size of the data to be sent
 *
 *	@return bytes sent if successful 
 *		or -1 if it could not be sent
 */
int BufferedTcpEndpoint::send( void * data, int32 size ) const
{
	int sentVal = 0;

	if (bufferedStream_.size() == 0)
	{
		sentVal = this->sendContent( (const char*)data, size );
		if ((sentVal >= 0) && (sentVal < size))
		{
			this->appendToBufferedStream( (char *)data + sentVal,
							size - sentVal );
			dispatcher_.registerWriteFileDescriptor(
					this->getEndpoint().fileno(),
					const_cast<BufferedTcpEndpoint*>( this ),
					"BufferedTcpEndpointWrite" );
		}
		else if (sentVal == -1)
		{
			int err = errno;
			WARNING_MSG( "BufferedTcpEndpoint::send: "
				"Could not send : %s, remaining %d bytes.\n",
				strerror( err ),
				bufferedStream_.remainingLength() );
		}

	}
	else
	{
		this->appendToBufferedStream( (char *)data, size );
	}

	return sentVal;
}

/**
 *	This method overrides the InputNotificationHandler. It is called 
 *	when there are events to process.
 *
 *	@param fd		The socket disriptor
 *
 *	@return The return value is ignored, should return 0.
 */
int BufferedTcpEndpoint::handleInputNotification( int fd )
{
	this->flushBufferedStream();
	return 0;
}


BW_END_NAMESPACE

// buffered_tcp_endpoint.cpp
