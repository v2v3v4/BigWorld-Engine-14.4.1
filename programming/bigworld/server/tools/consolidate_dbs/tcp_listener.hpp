#ifndef TCP_LISTENER_HPP
#define TCP_LISTENER_HPP

#include "network/interfaces.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: TcpListener
// -----------------------------------------------------------------------------
/**
 *	TCP listener socket that just accepts all connections to it.
 */
template <class CONNECTION_MGR>
class TcpListener : public Mercury::InputNotificationHandler
{
public:
	TcpListener( CONNECTION_MGR& connectionMgr ) : 
		connectionMgr_( connectionMgr ), endPoint_()
	{}
	~TcpListener();

	bool init( uint16 port = 0, uint32 ip = INADDR_ANY, int backLog = 5 );

	bool getBoundAddr( Mercury::Address& addr );

	// InputNotificationHandler override
	virtual int handleInputNotification( int fd );

private:
	CONNECTION_MGR& 	connectionMgr_;
	Endpoint 			endPoint_;
};

/**
 * 	Destructor
 */
template <class CONNECTION_MGR>
TcpListener< CONNECTION_MGR >::~TcpListener()
{
	connectionMgr_.dispatcher().deregisterFileDescriptor( endPoint_.fileno() );
}

/**
 * 	Binds this listener to a specific IP adress and port.
 */
template <class CONNECTION_MGR>
bool TcpListener< CONNECTION_MGR >::init( uint16 port, uint32 ip, 
		int backLog )
{
	endPoint_.socket( SOCK_STREAM );
	endPoint_.setnonblocking( true );
	if (endPoint_.bind( port, ip ) == -1)
	{
		connectionMgr_.onFailedBind( ip, port );
		return false;
	}
	else
	{
		// Make socket into listener socket.
		endPoint_.listen( std::min( SOMAXCONN, backLog ) );
		connectionMgr_.dispatcher().registerFileDescriptor( endPoint_.fileno(),
			this, "TcpListener" );
	}
	return true;
}

/**
 * 	Gets the bound address and port.
 *
 *  @return true if successfull, false otherwise
 */
template <class CONNECTION_MGR>
bool TcpListener< CONNECTION_MGR >::getBoundAddr( Mercury::Address& addr )
{
	return endPoint_.getlocaladdress( &addr.port, &addr.ip ) == 0;
}

/**
 *	Handles an incoming connection
 */
template <class CONNECTION_MGR>
int TcpListener< CONNECTION_MGR >::handleInputNotification( int fd )
{
	sockaddr_in addr;
	socklen_t size = sizeof(addr);

	int socket = accept( endPoint_.fileno(), (sockaddr *)&addr, &size );

	if (socket == -1)
	{
		connectionMgr_.onFailedAccept( addr.sin_addr.s_addr, addr.sin_port );
	}
	else
	{
		connectionMgr_.onAcceptedConnection( socket, addr.sin_addr.s_addr,
				addr.sin_port );
	}

	return 0;
}

BW_END_NAMESPACE

#endif /*TCP_LISTENER_HPP*/
