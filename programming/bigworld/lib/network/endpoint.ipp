#ifdef __unix__
#include <sys/ioctl.h>

#if defined( __linux__ ) && !defined( EMSCRIPTEN )
#include <linux/sockios.h>
#endif 

#include <netinet/tcp.h>
#endif


#ifdef CODE_INLINE
    #define INLINE    inline
#else
	/// INLINE macro.
    #define INLINE
#endif

/**
 * 	This is the default constructor. The socket is not created until the socket
 * 	method is called.
 *
 * 	@see socket(int type)
 */
INLINE Endpoint::Endpoint() : socket_( NO_SOCKET )
{
	BW_GUARD;
}

/**
 *	This is the destructor.
 *	Note: use 'detach' if you don't want your socket to disappear when the
 *	Endpoint is destructed.
 *
 *	@see detach
 */
INLINE Endpoint::~Endpoint()
{
	BW_GUARD;
	this->close();
}

/**
 * 	This operator returns the file descriptor for this endpoint.
 */
INLINE int Endpoint::fileno() const
{
	BW_GUARD;
	return int(socket_);
}

/**
 *	This method sets the file descriptor used by this endpoint.
 */
INLINE void Endpoint::setFileDescriptor( socket_t fd )
{
	BW_GUARD;
	socket_ = fd;
}

/**
 * 	This method returns true if this endpoint is a valid socket.
 */
INLINE bool Endpoint::good() const
{
	BW_GUARD;
	return (socket_ != NO_SOCKET);
}

/**
 * 	This method creates a socket of the requested type.
 * 	It initialises Winsock if necessary.
 *
 * 	@param type		Normally SOCK_STREAM or SOCK_DGRAM
 */
INLINE void Endpoint::socket(int type)
{
	BW_GUARD;
	this->setFileDescriptor( int(::socket( AF_INET, type, 0 )) );
#if defined( _WIN32 )
	if ((socket_ == INVALID_SOCKET) && (WSAGetLastError() == WSANOTINITIALISED))
	{
		// not initialised, so do so...
		initNetwork();

		// ... and try it again
		this->setFileDescriptor( int(::socket( AF_INET, type, 0 )) );
	}
#endif
}

/**
 *	This method controls the blocking mode of the socket.
 *	When a socket is set to non-blocking mode, socket calls
 *	will return immediately.
 *
 *	@param nonblocking	The desired blocking mode.
 */
INLINE int Endpoint::setnonblocking(bool nonblocking)
{
	BW_GUARD;
#if defined( __unix__ ) || defined( __APPLE__ ) || defined( __ANDROID__ ) || \
		defined( EMSCRIPTEN )
	int val = nonblocking ? O_NONBLOCK : 0;
	return ::fcntl(socket_,F_SETFL,val);
#elif defined( PLAYSTATION3 )
	int val = nonblocking ? 1 : 0;
	return setsockopt( socket_, SOL_SOCKET, SO_NBIO, &val, sizeof(int) );
#else
	u_long val = nonblocking ? 1 : 0;
	return ::ioctlsocket(socket_,FIONBIO,&val);
#endif
}


/**
 *	This method returns the number of bytes that are available for reading.
 *
 *	@return 	The number of bytes available for reading, or -1 if the socket
 *				is invalid or an error otherwise occurred.
 */
INLINE int Endpoint::numBytesAvailableForReading() const
{
#if (defined( __unix__ ) || defined( __APPLE__ )) || defined( EMSCRIPTEN )
	int numBytes = 0;
	int error = ioctl( socket_,	FIONREAD, &numBytes);

	if (error != 0)
	{
		return -1;
	}

	return numBytes;
#elif defined( _WIN32 )
	u_long numBytes = 0;
	int error = ::ioctlsocket( socket_, FIONREAD, &numBytes );
	if (error != 0)
	{
		return -1;
	}
	return numBytes;
#else
	return -1;
#endif
}


/**
 *	This method toggles the broadcast mode of the socket.
 *
 *	@param broadcast	The desired broadcast mode.
 */
INLINE int Endpoint::setbroadcast(bool broadcast)
{
	BW_GUARD;
	int val = broadcast ? 1 : 0;
	return ::setsockopt(socket_,SOL_SOCKET,SO_BROADCAST,
		(char*)&val,sizeof(val));
}

/**
 *	This method toggles the reuse address mode of the socket.
 *
 *	@param reuseaddr	The desired reuse address mode.
 */
INLINE int Endpoint::setreuseaddr( bool reuseaddr )
{
	BW_GUARD;
#if defined( __unix__ ) || defined( EMSCRIPTEN )
	int val;
#else
	bool val;
#endif
	val = reuseaddr ? 1 : 0;
	return ::setsockopt( socket_, SOL_SOCKET, SO_REUSEADDR,
		(char*)&val, sizeof(val) );
}

#ifndef _XBOX
INLINE int Endpoint::setkeepalive( bool keepalive )
{
	BW_GUARD;
#if defined( __unix__ ) || defined( EMSCRIPTEN )
	int val;
#else
	bool val;
#endif
	val = keepalive ? 1 : 0;
	return ::setsockopt( socket_, SOL_SOCKET, SO_KEEPALIVE,
		(char*)&val, sizeof(val) );
}
#endif

/**
 *	This method binds the socket to a given address and port.
 *
 *	@param networkPort	The port, in network byte order.
 *	@param networkAddr	The address, in network byte order.
 *
 *	@return	0 if successful, -1 otherwise.
 */
INLINE int Endpoint::bind( u_int16_t networkPort, u_int32_t networkAddr )
{
	BW_GUARD;
	sockaddr_in	sin;
	sin.sin_family = AF_INET;
	sin.sin_port = networkPort;
	sin.sin_addr.s_addr = networkAddr;
	return ::bind( socket_, (struct sockaddr*)&sin, sizeof(sin) );
}


/**
 *	This method closes the socket associated with this endpoint.
 */
INLINE int Endpoint::close()
{
	BW_GUARD;
	if (socket_ == NO_SOCKET)
	{
		return 0;
	}

#if defined( __unix__ ) || defined( __APPLE__ ) || defined( __ANDROID__ ) ||  \
		defined( EMSCRIPTEN )
	int ret = ::close(socket_);
#elif defined( PLAYSTATION3 )
	int ret = ::socketclose(socket_);
#else
	int ret = ::closesocket(socket_);
#endif
	if (ret == 0)
	{
		this->setFileDescriptor( NO_SOCKET );
	}
	return ret;
}

/**
 * 	This method detaches the socket from this endpoint.
 * 	When the destructor is called, the socket will not be automatically closed.
 */
INLINE int Endpoint::detach()
{
	BW_GUARD;
	int ret = int(socket_);
	this->setFileDescriptor( NO_SOCKET );
	return ret;
}

/**
 *	This method returns the local address and port to which this endpoint
 *	is bound.
 *
 *	@param networkPort	The port is returned here in network byte order.
 *	@param networkAddr	The address is returned here in network byte order.
 *
 *	@return 0 if successful, or -1 if an error occurred.
 */
INLINE int Endpoint::getlocaladdress(
	u_int16_t * networkPort, u_int32_t * networkAddr ) const
{
	BW_GUARD;
	sockaddr_in		sin;
	socklen_t		sinLen = sizeof(sin);

	if (networkPort != NULL)
	{
		*networkPort = 0;
	}

	if (networkAddr != NULL)
	{
		*networkAddr = 0;
	}

	int ret = ::getsockname( socket_, (struct sockaddr*)&sin, &sinLen );
	if (ret == 0)
	{
		if (networkPort != NULL)
		{
			*networkPort = sin.sin_port;
		}

		if (networkAddr != NULL)
		{
			*networkAddr = sin.sin_addr.s_addr;
		}
	}
	return ret;
}


/**
 *	This method returns the remote address and port to which this endpoint
 *	is connected.
 *
 *	@param networkPort	The port is returned here in network byte order.
 *	@param networkAddr	The address is returned here in network byte order.
 *
 *	@return 0 if successful, or -1 if an error occurred.
 */
INLINE int Endpoint::getremoteaddress(
	u_int16_t * networkPort, u_int32_t * networkAddr ) const
{
	sockaddr_in		sin;
	socklen_t		sinLen = sizeof(sin);
	int ret = ::getpeername( socket_, (struct sockaddr*)&sin, &sinLen );
	if (ret == 0)
	{
		if (networkPort != NULL) *networkPort = sin.sin_port;
		if (networkAddr != NULL) *networkAddr = sin.sin_addr.s_addr;
	}
	return ret;
}


/**
 *  This method returns the string representation of the address this endpoint
 *  is bound to.
 */
INLINE const char * Endpoint::c_str() const
{
	BW_GUARD;
	Mercury::Address addr;
	this->getlocaladdress( &(u_int16_t&)addr.port, &(u_int32_t&)addr.ip );
	return addr.c_str();
}


#ifndef _XBOX
/**
 * 	This method returns the hostname of the remote computer
 *
 * 	@param host			The string to return the hostname in
 *
 * 	@return 0 if successful, or -1 if an error occurred.
 */
INLINE int Endpoint::getremotehostname( BW::string * host ) const
{
	BW_GUARD;
	sockaddr_in		sin;
	socklen_t		sinLen = sizeof(sin);
	int ret = ::getpeername( socket_, (struct sockaddr*)&sin, &sinLen );
	if (ret == 0)
	{
		hostent* h = gethostbyaddr( (char*) &sin.sin_addr,
				sizeof( sin.sin_addr ), AF_INET);

		if (h)
		{
			*host = h->h_name;
		}
		else
		{
			ret = -1;
		}
	}

	return ret;
}
#endif


/**
 * 	This method sends a packet to the given address.
 *
 * 	@param gramData		Pointer to a data buffer containing the packet.
 * 	@param gramSize		Number of bytes in the data buffer.
 * 	@param networkPort	Destination port, in network byte order.
 * 	@param networkAddr	Destination address, in network byte order.
 */
INLINE int Endpoint::sendto( void * gramData, int gramSize,
	u_int16_t networkPort, u_int32_t networkAddr ) const
{
	BW_GUARD;
	sockaddr_in	sin;
	sin.sin_family = AF_INET;
	sin.sin_port = networkPort;
	sin.sin_addr.s_addr = networkAddr;

	return this->sendto( gramData, gramSize, sin );
}

/**
 * 	This method sends a packet to the given address.
 *
 * 	@param gramData		Pointer to a data buffer containing the packet.
 * 	@param gramSize		Number of bytes in the data buffer.
 * 	@param sin			Destination address.
 */
INLINE int Endpoint::sendto( void * gramData, int gramSize,
	const struct sockaddr_in & sin ) const
{
	BW_GUARD;

	int flags = 0;
#ifdef __linux__
	flags = MSG_NOSIGNAL;
#endif

	return ::sendto( socket_, (char*)gramData, gramSize,
		flags, (sockaddr*)&sin, sizeof(sin) );
}


/**
 * 	This method sends a packet to the given Mercury address.
 *
 * 	@param gramData		Pointer to a data buffer containing the packet.
 * 	@param gramSize		Number of bytes in the data buffer.
 * 	@param addr			Destination address.
 */
INLINE int Endpoint::sendto( void * gramData, int gramSize,
	const Mercury::Address & addr ) const
{
	BW_GUARD;
	return this->sendto( gramData, gramSize, addr.port, addr.ip );
}


/**
 *	This method attempts to receive a packet.
 *
 *	@param gramData		Pointer to a data buffer to receive the packet.
 *	@param gramSize		Number of bytes in the data buffer.
 *	@param networkPort	The port from which the packet originated is returned here,
 *	                    if this pointer is non-NULL.
 *	@param networkAddr	The address from which the packet originated is returned here,
 *						if this pointer is non-NULL.
 *
 *	@return The number of bytes received, or -1 if an error occurred.
 */
INLINE int Endpoint::recvfrom( void * gramData, int gramSize,
	u_int16_t * networkPort, u_int32_t * networkAddr ) const
{
	BW_GUARD;
	sockaddr_in sin;

	if (networkPort != NULL)
	{
		*networkPort = 0;
	}

	if (networkAddr != NULL)
	{
		*networkAddr = 0;
	}

	int result = this->recvfrom( gramData, gramSize, sin );
	if (result >= 0)
	{
		if (networkPort != NULL)
		{
			*networkPort = sin.sin_port;
		}

		if (networkAddr != NULL)
		{
			*networkAddr = sin.sin_addr.s_addr;
		}
	}

	return result;
}


/**
 *	This method attempts to receive a packet.
 *
 *	@param gramData		Pointer to a data buffer to receive the packet.
 *	@param gramSize		Number of bytes in the data buffer.
 *	@param sin			The address from which the packet originated is returned
 *						here.
 *
 *	@return The number of bytes received, or -1 if an error occurred.
 */
INLINE int Endpoint::recvfrom( void * gramData, int gramSize,
	struct sockaddr_in & sin ) const
{
	BW_GUARD;
	socklen_t		sinLen = sizeof(sin);
	int ret = ::recvfrom( socket_, (char*)gramData, gramSize,
		0, (sockaddr*)&sin, &sinLen );

	return ret;
}


/**
 *	This method attempts to receive a packet.
 *
 *	@param gramData		Pointer to a data buffer to receive the packet.
 *	@param gramSize		Number of bytes in the data buffer.
 *	@param addr			The address from which the packet originated is returned
 *						here.
 *
 *	@return The number of bytes received, or -1 if an error occurred.
 */
INLINE int Endpoint::recvfrom( void * gramData, int gramSize,
	Mercury::Address & addr ) const
{
	BW_GUARD;
	return this->recvfrom( gramData, gramSize, 
		reinterpret_cast< u_int16_t * >(&addr.port),
		reinterpret_cast< u_int32_t * >(&addr.ip) );
}


/**
 *	This method instructs this endpoint to listen for incoming connections.
 */
INLINE int Endpoint::listen( int backlog ) { BW_GUARD;
	return ::listen( socket_, backlog );
}

/**
 *	This method connections this endpoint to a destination address
 */
INLINE int Endpoint::connect( u_int16_t networkPort, u_int32_t networkAddr )
{
	BW_GUARD;

	sockaddr_in	sin;
	sin.sin_family = AF_INET;
	sin.sin_port = networkPort;
	sin.sin_addr.s_addr = networkAddr;

	return ::connect( socket_, (sockaddr*)&sin, sizeof(sin) );
}

/**
 *	This method accepts a connection on the socket listen queue, returning
 *	a new endpoint if successful, or NULL if not. The remote port and
 *	address are set into the pointers passed in if not NULL.
 */
INLINE Endpoint * Endpoint::accept(
	u_int16_t * networkPort, u_int32_t * networkAddr )
{
	BW_GUARD;
	sockaddr_in		sin;
	socklen_t		sinLen = sizeof(sin);
	int ret = int(::accept( socket_, (sockaddr*)&sin, &sinLen));
#if defined( __unix__ ) || defined( PLAYSTATION3 ) || \
		defined( __APPLE__ ) || defined( __ANDROID__ ) || \
		defined( EMSCRIPTEN )
	if (ret < 0)
	{
		ERROR_MSG( "Endpoint::accept: accept failed (%s) for socket %d\n",
			strerror( errno ), socket_ );
		return NULL;
	}
#else
	if (ret == INVALID_SOCKET) return NULL;
#endif

	Endpoint * pNew = new Endpoint();
	pNew->setFileDescriptor( ret );

	if (networkPort != NULL) *networkPort = sin.sin_port;
	if (networkAddr != NULL) *networkAddr = sin.sin_addr.s_addr;

	return pNew;
}


/**
 * 	This method sends some data to the given address.
 *
 * 	@param gramData		Pointer to a data buffer
 * 	@param gramSize		Number of bytes in the data buffer.
 */
INLINE int Endpoint::send( const void * gramData, int gramSize ) const
{
	BW_GUARD;
	return ::send( socket_, (char*)gramData, gramSize, 0 );
}


/**
 *	This method attempts to receive some data
 *
 *	@param gramData		Pointer to a data buffer
 *	@param gramSize		Number of bytes in the data buffer.
 *
 *	@return The number of bytes received, or -1 if an error occurred.
 */
INLINE int Endpoint::recv( void * gramData, int gramSize ) const
{
	BW_GUARD;
	return ::recv( socket_, (char*)gramData, gramSize, 0 );
}


#if defined( __unix__ ) || defined( __APPLE__ ) || defined( __ANDROID__ )
/**
 *	This method returns the flags associated with the given interface.
 *
 *	@param name		Interface name for which flags are needed.
 *	@param flags	The flags are returned here.
 *
 *	@return	0 if successful, 1 otherwise.
 */
INLINE int Endpoint::getInterfaceFlags( char * name, int & flags )
{
	BW_GUARD;
	struct ifreq	request;

	strncpy( request.ifr_name, name, IFNAMSIZ );
#if !defined( EMSCRIPTEN )
	if (ioctl( socket_, SIOCGIFFLAGS, &request ) != 0)
#endif
	{
		return -1;
	}

	flags = request.ifr_flags;
	return 0;
}


/**
 * 	This method returns the address to which an interface is bound.
 *
 * 	@param name		Name of the interface.
 * 	@param address	The address is returned here.
 *
 * 	@return 0 if successful, 1 otherwise.
 */
INLINE int Endpoint::getInterfaceAddress( const char * name, u_int32_t & address )
{
	BW_GUARD;
	struct ifreq	request;

	strncpy( request.ifr_name, name, IFNAMSIZ );
#if !defined( EMSCRIPTEN )
	if (ioctl( socket_, SIOCGIFADDR, &request ) != 0)
#endif
	{
		return -1;
	}

	if (request.ifr_addr.sa_family == AF_INET)
	{
		address = ((sockaddr_in*)&request.ifr_addr)->sin_addr.s_addr;
		return 0;
	}
	else
	{
		return -1;
	}
}

#else

/**
 *	This method returns the flags associated with the given interface.
 *
 *	@param name		Interface name for which flags are needed.
 *	@param flags	The flags are returned here.
 *
 *	@return	0 if successful, 1 otherwise.
 */
//INLINE int Endpoint::getInterfaceFlags( char * name, int & flags )
// Not implemented on windows.

/**
 * 	This method returns the address to which an interface is bound.
 *
 * 	@param name		Name of the interface.
 * 	@param address	The address is returned here.
 *
 * 	@return 0 if successful, 1 otherwise.
 */
INLINE int Endpoint::getInterfaceAddress( const char * name, u_int32_t & address )
{
	BW_GUARD;
	if (!strcmp(name,"eth0"))
	{
#ifdef _XBOX
		XNADDR	xnaddr;
		xnaddr.ina.S_un.S_addr = 0;
		XNetGetTitleXnAddr( &xnaddr );
		address = xnaddr.ina.S_un.S_addr;
#elif defined( PLAYSTATION3 )
		CellNetCtlInfo netInfo;
		int ret = cellNetCtlGetInfo( CELL_NET_CTL_INFO_IP_ADDRESS, &netInfo );
		MF_ASSERT( ret == 0 );
		int ip0, ip1, ip2, ip3;
		sscanf( netInfo.ip_address, "%d.%d.%d.%d", &ip0, &ip1, &ip2, &ip3 );
		address = ( ip0 << 24 ) | ( ip1 << 16 ) | ( ip2 << 8 ) | ( ip3 << 0 );
#else
		char	myname[256];
		::gethostname(myname,sizeof(myname));

		struct hostent * myhost = gethostbyname(myname);
		if (!myhost)
		{
			return -1;
		}

		address = ((struct in_addr*)(myhost->h_addr_list[0]))->s_addr;
#endif
		return 0;
	}
	else if (!strcmp(name,"lo"))
	{
		address = htonl(0x7F000001);
		return 0;
	}

	return -1;
}
#endif

/**
 *	This method returns the current size of the transmit queue for this socket.
 *	It is only implemented on Unix.
 *
 *	@return	Current transmit queue size in bytes.
 */
INLINE int Endpoint::transmitQueueSize() const
{
	BW_GUARD;
	int tx=0, rx=0;
	this->getQueueSizes( tx, rx );
	return tx;
}

/**
 *	This method returns the current size of the receive queue for this socket.
 *	It is only implemented on Unix.
 *
 *	@return	Current receive queue size in bytes.
 */
INLINE int Endpoint::receiveQueueSize() const
{
	BW_GUARD;
	int tx=0, rx=0;
	this->getQueueSizes( tx, rx );
	return rx;
}

// endpoint.ipp
