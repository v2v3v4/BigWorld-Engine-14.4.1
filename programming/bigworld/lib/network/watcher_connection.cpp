#include "pch.hpp"

#include "watcher_connection.hpp"

#if ENABLE_WATCHERS

#include "event_dispatcher.hpp"
#include "watcher_endpoint.hpp"
#include "watcher_nub.hpp"
#include "buffered_tcp_endpoint.hpp"

#include "cstdmf/profile.hpp"

DECLARE_DEBUG_COMPONENT2( "Network", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: WatcherConnection
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
WatcherConnection::WatcherConnection( WatcherNub & nub,
		Mercury::EventDispatcher & dispatcher,
		Endpoint * pEndpoint ) :
	nub_( nub ),
	dispatcher_( dispatcher ),
	pEndpoint_( pEndpoint ),
	pBufferedTcpEndpoint_( new BufferedTcpEndpoint( dispatcher, *pEndpoint ) ),
	receivedSize_( 0 ),
	messageSize_( 0 ),
	pBuffer_( NULL )
{
	// We have an active connection, so keep the reference count
	registeredFileDescriptorProxyRefHolder_ = this;

	dispatcher_.registerFileDescriptor( pEndpoint->fileno(), this,
		"WatcherConnection" );

	pEndpoint->setnonblocking( true );
}


WatcherConnection::~WatcherConnection()
{
	MF_ASSERT( pBufferedTcpEndpoint_.get() == NULL );
	MF_ASSERT( pBuffer_ == NULL );
}

void WatcherConnection::handleDisconnect()
{
	MF_ASSERT( pBufferedTcpEndpoint_.get() != NULL );

	WatcherEndpoint watcherEndpoint( *this );
	nub_.processDisconnect( watcherEndpoint );

	pBufferedTcpEndpoint_.reset();

	dispatcher_.deregisterFileDescriptor( pEndpoint_->fileno() );

	bw_safe_delete_array( pBuffer_ ); // Generally already NULL
	
	registeredFileDescriptorProxyRefHolder_ = NULL;
}


/**
 *	Accepts connections on the TCP endpoint
 *
 *	@return New WatcherConnection on success or NULL on accept failure
 */
/* static */ WatcherConnection * WatcherConnection::handleAccept(
	Endpoint & listenEndpoint,
	Mercury::EventDispatcher & dispatcher,
	WatcherNub & watcherNub )
{
	Endpoint * pNewEndpoint = listenEndpoint.accept();

	if (pNewEndpoint == NULL)
	{
		return NULL;
	}

	return new WatcherConnection( watcherNub, dispatcher, pNewEndpoint );
}


Mercury::Address WatcherConnection::getRemoteAddress() const
{
	return pEndpoint_->getRemoteAddress();
}

int WatcherConnection::send( void * data, int32 size ) const
{
	if (pBufferedTcpEndpoint_.get() == NULL)
	{
		return -1;
	}

	if (pBufferedTcpEndpoint_->send( &size, sizeof( size ) ) == -1)
	{
		return -1;
	}

	return pBufferedTcpEndpoint_->send( data, size );
}


int WatcherConnection::handleInputNotification( int fd )
{
	AUTO_SCOPED_PROFILE( "watchersTCP" );

	MF_ASSERT( registeredFileDescriptorProxyRefHolder_ );

	if (pBuffer_ == NULL)
	{
		if (!this->recvSize())
		{
			this->handleDisconnect();
		}

		// TODO: BWT-26304 - Early return, due to TCP Watchers having a
		// blocking connection
		return 0;
	}

	if (pBuffer_)
	{
		if (!this->recvMsg())
		{
			this->handleDisconnect();
			return 0;
		}
	}

	return 0;
}


/**
 *	This method attempts to receive the size of the message form the current
 *	connection.
 *
 *	@return false if the connection has been lost.
 */
bool WatcherConnection::recvSize()
{
	MF_ASSERT( registeredFileDescriptorProxyRefHolder_ );
	MF_ASSERT( receivedSize_ < sizeof( messageSize_ ) );
	MF_ASSERT( pBuffer_ == NULL );

	int size = pEndpoint_->recv( ((char *)&messageSize_) + receivedSize_,
			sizeof( messageSize_ ) - receivedSize_ );

	if (size <= 0)
	{
		// Lost watcher connection
		return false;
	}

	receivedSize_ += size;

	if (receivedSize_ == sizeof( messageSize_ ))
	{
		if (messageSize_ > uint32(WN_PACKET_SIZE_TCP))
		{
			ERROR_MSG( "WatcherConnection::recvSize: "
							"Invalid message size %d from %s\n",
						messageSize_, pEndpoint_->getLocalAddress().c_str() );
			return false;
		}

		pBuffer_ = new char[ messageSize_ ];
		receivedSize_ = 0;
	}

	return true;
}


/**
 *	This method attempts to receive the message from the current connection.
 *
 *	@return false if the connection has been lost.
 */
bool WatcherConnection::recvMsg()
{
	MF_ASSERT( registeredFileDescriptorProxyRefHolder_ );
	MF_ASSERT( pBuffer_ );

	int size = pEndpoint_->recv( pBuffer_ + receivedSize_,
			messageSize_ - receivedSize_ );

	if (size <= 0)
	{
		// Lost watcher connection
		return false;
	}

	receivedSize_ += size;

	if (receivedSize_ == messageSize_)
	{
		WatcherEndpoint watcherEndpoint( *this );
		nub_.processRequest( pBuffer_, messageSize_, watcherEndpoint );

		bw_safe_delete_array(pBuffer_);
		receivedSize_ = 0;
		messageSize_ = 0;
	}

	return true;
}

BW_END_NAMESPACE

#endif /* ENABLE_WATCHERS */

// watcher_connection.cpp
