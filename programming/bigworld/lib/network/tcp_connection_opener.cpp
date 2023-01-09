#include "pch.hpp"

#include "tcp_connection_opener.hpp"

#include "endpoint.hpp"
#include "event_dispatcher.hpp"
#include "network_interface.hpp"
#include "network_utils.hpp"
#include "tcp_channel.hpp"

#if defined( __unix__ ) || defined( __APPLE__ ) || defined( EMSCRIPTEN )
#include <netinet/tcp.h>
#endif // defined( __unix__ ) || defined( __APPLE__ ) || defined( EMSCRIPTEN )

BW_BEGIN_NAMESPACE


namespace Mercury
{


/**
 *	Constructor.
 *
 *	@param listener 			The listener to call back on.
 *	@param networkInterface 	The network interface.
 *	@param connectAddress 		The TCP address and port to connect to.
 *	@param pUserData 			User data to associate to this request.
 *	@param timeoutSeconds		The timeout period for the connection.
 */
TCPConnectionOpener::TCPConnectionOpener( 
			TCPConnectionOpenerListener & listener,
			NetworkInterface & networkInterface,
			const Mercury::Address & connectAddress,
			void * pUserData,
			float timeoutSeconds ):
		listener_( listener ),
		networkInterface_( networkInterface ),
		pEndpoint_( new Endpoint() ),
		connectAddress_( connectAddress ),
		timerHandle_(),
		pUserData_( pUserData )
{
	TRACE_MSG( "TCPConnectionOpener::TCPConnectionOpener: "
			"Opening connection to %s\n",
		connectAddress_.c_str() );

	pEndpoint_->socket( SOCK_STREAM );
	pEndpoint_->setnonblocking( true );

	pEndpoint_->setSocketOption( IPPROTO_TCP, TCP_NODELAY, true );

	int connectResult = pEndpoint_->connect( connectAddress.port, 
		connectAddress.ip );

	// Different implementations of asynchronous connect() will do different
	// things to notify of success/failure.

	// Connection successful:
	// * Linux: socket marked writeable, check SO_ERROR = 0
	// * Win32: socket marked writeable
	// * Emscripten: socket marked writeable, but getsockopt not available

	// Connection unsuccessful:
	// * Linux: socket set POLLOUT and check SO_ERROR
	// * Win32: socket set POLLERR, check SO_ERROR.
	// * Emscripten: POLLHUP, no error available

	// We would expect it to fail with EAGAIN or equivalent.
	if ((-1 != connectResult) ||
#if defined( _WIN32 )
			(WSAGetLastError() != WSAEWOULDBLOCK)
#else
			((errno != EAGAIN) && (errno != EINPROGRESS))
#endif
		)
	{
		ERROR_MSG( "TCPConnectionOpener::TCPConnectionOpener: "
				"Call to connect( fd:%d, addr:%s ) failed with %s\n",
			pEndpoint_->fileno(), connectAddress_.c_str(),
			lastNetworkError() );
		listener_.onTCPConnectFailure( *this,
			Mercury::REASON_GENERAL_NETWORK );
		return;
	}

	this->dispatcher().registerWriteFileDescriptor( pEndpoint_->fileno(), 
		this, "TCPConnectionOpener" );

	int64 timeoutMicros = int64( timeoutSeconds * 1e6 );
	timerHandle_ = this->dispatcher().addOnceOffTimer( timeoutMicros, this );
}


/**
 *	Destructor.
 */
TCPConnectionOpener::~TCPConnectionOpener()
{
	this->cancel();

	if (pEndpoint_.get() != NULL)
	{
		pEndpoint_->close();
	}
}


/*
 *	Override from Mercury::InputNotificationHandler.
 */
int TCPConnectionOpener::handleInputNotification( int fd )
{
	MF_ASSERT( (pEndpoint_.get() != NULL) && (fd == pEndpoint_->fileno()) );

	this->dispatcher().deregisterWriteFileDescriptor( fd );
	timerHandle_.cancel();

	// Linux will inform of both a connection success and error with a POLLOUT
	// notification. Shouldn't hurt to check for SO_ERROR on Windows as well,
	// but Emscripten does it via a POLLHUP event, which is caught in
	// handleErrorNotification.

#if !defined( EMSCRIPTEN )
	int socketError = 0;
	if (-1 == pEndpoint_->getSocketOption( SOL_SOCKET, SO_ERROR, socketError ))
	{
		ERROR_MSG( "TCPConnectionOpener::handleInputNotification( %s ): "
				"Failed to get SO_ERROR from socket: %s\n",
			connectAddress_.c_str(), lastNetworkError() );
		listener_.onTCPConnectFailure( *this, Mercury::REASON_GENERAL_NETWORK );
		return 0;
	}

	if (socketError != 0)
	{
		this->handleSocketError( socketError );
		return 0;
	}
#endif

	Endpoint * pEndpoint = pEndpoint_.get();
	pEndpoint_.release();

	Mercury::TCPChannel * pChannel = new TCPChannel( networkInterface_,
		*pEndpoint, /* isServer */ false );

	INFO_MSG( "TCPConnectionOpener::handleInputNotification: "
			"Connected to %s\n",
		pChannel->c_str() );

	listener_.onTCPConnect( *this, pChannel );
	return 0;
}


/*
 *	Override from Mercury::InputNotificationHandler.
 */
bool TCPConnectionOpener::handleErrorNotification( int fd )
{
	MF_ASSERT( (pEndpoint_.get() != NULL) && (fd == pEndpoint_->fileno()) );

	this->dispatcher().deregisterWriteFileDescriptor( fd );
	timerHandle_.cancel();

#if defined( EMSCRIPTEN )
	ERROR_MSG( "TCPConnectionOpener::handleErrorNotification( %s ): "
			"Failed to connect\n",
		connectAddress_.c_str() );
	listener_.onTCPConnectFailure( *this, Mercury::REASON_GENERAL_NETWORK );
#else
	// Linux won't notify of connect error via POLLERR or POLLHUP, but Windows
	// will via POLLERR.
	int socketError = 0;
	if (-1 == pEndpoint_->getSocketOption( SOL_SOCKET, SO_ERROR, socketError ))
	{
		ERROR_MSG( "TCPConnectionOpener::handleErrorNotification( %s ): "
				"Failed to get SO_ERROR from socket: %s\n",
			connectAddress_.c_str(), lastNetworkError() );

		listener_.onTCPConnectFailure( *this, Mercury::REASON_GENERAL_NETWORK );
		return true;
	}

	this->handleSocketError( socketError );
#endif

	return true;
}


/**
 *	This method handles the given socket error.
 *
 *	@param error	The error code.
 */
void TCPConnectionOpener::handleSocketError( int error )
{
	if (error == 0)
	{
		ERROR_MSG( "TCPConnectionOpener::handleSocketError( %s ): "
				"Got error notification, but no socket error\n",
			connectAddress_.c_str() );
		listener_.onTCPConnectFailure( *this, Mercury::REASON_GENERAL_NETWORK );
		return;
	}

	ERROR_MSG( "TCPConnectionOpener::handleSocketError: "
			"Connection failed to %s: %s\n",
		connectAddress_.c_str(), networkErrorMessage( error ) );

#if defined( _WIN32 )
	switch (error)
	{
	case WSAENETDOWN:
	case WSAECONNREFUSED:
		this->cancel();
		listener_.onTCPConnectFailure( *this, Mercury::REASON_NO_SUCH_PORT );
		break;

	case WSAETIMEDOUT:
		this->cancel();
		listener_.onTCPConnectFailure( *this, Mercury::REASON_TIMER_EXPIRED );
		break;
	default:
		this->cancel();
		listener_.onTCPConnectFailure( *this, Mercury::REASON_GENERAL_NETWORK );
		break;
	}
#else // ! defined( _WIN32 )
	switch (error )
	{
	case ECONNREFUSED:
	case ENETUNREACH:
		this->cancel();
		listener_.onTCPConnectFailure( *this, Mercury::REASON_NO_SUCH_PORT );
		break;
	case ETIMEDOUT:
		this->cancel();
		listener_.onTCPConnectFailure( *this, Mercury::REASON_TIMER_EXPIRED );
		break;
	default:
		this->cancel();
		listener_.onTCPConnectFailure( *this, Mercury::REASON_GENERAL_NETWORK );
		break;
	}
#endif // defined( WIN32 )
}


/*
 *	Override from TimerHandler.
 */
void TCPConnectionOpener::handleTimeout( TimerHandle handle, 
		void * pUser )
{
	this->dispatcher().deregisterWriteFileDescriptor( pEndpoint_->fileno() );
	pEndpoint_.reset();
	listener_.onTCPConnectFailure( *this, Mercury::REASON_TIMER_EXPIRED );
}


/**
 *	This method cancels this connection attempt.
 */
void TCPConnectionOpener::cancel()
{
	if (pEndpoint_.get() != NULL)
	{
		this->dispatcher().deregisterWriteFileDescriptor( 
			pEndpoint_->fileno() );
		pEndpoint_.reset();
	}

	timerHandle_.cancel();
}


/**
 *	This method returns the dispatcher.
 */
EventDispatcher & TCPConnectionOpener::dispatcher()
{
	return networkInterface_.dispatcher();
}


} // end namespace Mercury


BW_END_NAMESPACE

// tcp_connection_opener.cpp

