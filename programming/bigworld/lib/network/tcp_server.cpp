#include "pch.hpp"

#include "tcp_server.hpp"

#include "event_dispatcher.hpp"
#include "network_interface.hpp"
#include "stream_filter_factory.hpp"
#include "tcp_channel.hpp"

#include "cstdmf/debug_message_categories.hpp"

#include <errno.h>
#include <string.h>


BW_BEGIN_NAMESPACE

namespace Mercury
{


/**
 *	Constructor.
 *
 *	@param networkInterface 	The network interface.
 *	@param backlog 	The backlog hint for listen().
 */
TCPServer::TCPServer( NetworkInterface & networkInterface,
			uint backlog /* = DEFAULT_BACKLOG */ ) :
		InputNotificationHandler(),
		networkInterface_( networkInterface ),
		backlog_( backlog ),
		serverSocket_(),
		pStreamFilterFactory_( NULL )
{
}


/**
 *	This method initialises the server socket by binding it to a TCP port
 *	numerically equal to the network interface's UDP port.
 *
 *	@return 		True on success, false otherwise.
 */
bool TCPServer::init()
{
	return this->initWithPort( networkInterface_.address().port );
}


/**
 *	This method initialises this instance by binding to a specific port, at the
 *	same address as the network interface given at construction.
 *
 *	@param port 	The port to bind to, in network byte order.
 *
 *	@return 		True on success, false otherwise.
 */
bool TCPServer::initWithPort( uint16 port )
{
	serverSocket_.socket( SOCK_STREAM );
	serverSocket_.setnonblocking( true );
	serverSocket_.setreuseaddr( true );

	Address addr = networkInterface_.address();
	addr.port = port;

	if (-1 == serverSocket_.bind( addr.port, addr.ip ))
	{
		NETWORK_ERROR_MSG( "TCPServer::initWithPort: "
				"Could not bind to %s: %s\n",
			addr.c_str(), strerror( errno ) );
		serverSocket_.close();
		return false;
	}

	if (-1 == serverSocket_.listen( backlog_ ))
	{
		NETWORK_ERROR_MSG( "TCPServer::initWithPort: "
				"Could not listen on %s:%s\n",
			addr.c_str(), strerror( errno ) );
		serverSocket_.close();
		return false;
	}

	this->dispatcher().registerFileDescriptor( serverSocket_.fileno(), this,
		"TCPServer" );

	NETWORK_INFO_MSG( "TCPServer::initWithPort: Listening on %s\n",
		addr.c_str() );

	return true;
}


/**
 *	Destructor.
 */
TCPServer::~TCPServer()
{
	this->dispatcher().deregisterFileDescriptor( serverSocket_.fileno() );
}


/*
 *	Override from InputNotificationHandler.
 */
int TCPServer::handleInputNotification( int fd )
{
	Endpoint * pNewEndpoint = serverSocket_.accept();

	Mercury::TCPChannel * pChannel = new TCPChannel( networkInterface_,
		*pNewEndpoint, /* isServer */ true );

	if (pStreamFilterFactory_ != NULL)
	{
		StreamFilterPtr pStreamFilter =
			pStreamFilterFactory_->createFor( *pChannel );

		pChannel->pStreamFilter( pStreamFilter.get() );
	}

	NETWORK_DEBUG_MSG( "TCPServer::handleInputNotification: connection from %s\n",
		pChannel->c_str() );

	return 0;
}


/**
 *	This method returns an event dispatcher to register the server socket with.
 */
EventDispatcher & TCPServer::dispatcher()
{
	return networkInterface_.dispatcher();
}



} // end namespace Mercury


BW_END_NAMESPACE

// tcp_server.cpp

