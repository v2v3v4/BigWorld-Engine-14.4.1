#include "pch.hpp"

#include "endpoint.hpp"
#include "netmask.hpp"
#include "network_stats.hpp"
#include "cstdmf/binary_stream.hpp"
#include "cstdmf/bw_memory.hpp"

#if defined( __unix__ ) || defined( __APPLE__ ) || defined( __ANDROID__ ) || \
		defined( EMSCRIPTEN )

extern "C" {
#ifdef BW_ENABLE_NAMESPACE
	typedef BW::uint32 uint32;
	typedef BW::uint8 uint8;
#endif

	/// 32 bit unsigned integer.
	#define __u32 uint32
	/// 8 bit unsigned integer.
	#define __u8 uint8
#if defined( __linux__ ) && !defined( EMSCRIPTEN )
	#include <linux/errqueue.h>
#endif
}
#include <signal.h>
#include <sys/uio.h>

#if !defined( EMSCRIPTEN )
#include <netinet/ip.h>
#endif

#else	// not __unix__ or APPLE
	// Need to implement if_nameindex functions on Windows
	/** @internal */
	struct if_nameindex
	{

		unsigned int if_index;	/* 1, 2, ... */

		char *if_name;			/* null terminated name: "eth0", ... */

	};

	/** @internal */
	struct if_nameindex *if_nameindex(void)
	{
		static struct if_nameindex staticIfList[3] =
		{ { 1, "eth0" }, { 2, "lo" }, { 0, 0 } };

		return staticIfList;
	}

	/** @internal */
	inline void if_freenameindex(struct if_nameindex *)
	{}
#endif	// not __unix__

#if defined( __ANDROID__ )
	// Need to implement if_nameindex functions on Android
	/** @internal */
	struct if_nameindex
	{

		unsigned int if_index;	/* 1, 2, ... */

		char *if_name;			/* null terminated name: "eth0", ... */

	};

	/** @internal */
	struct if_nameindex *if_nameindex(void)
	{
		static struct if_nameindex staticIfList[3] =
		{ { 1, "eth0" }, { 2, "lo" }, { 0, 0 } };

		return staticIfList;
	}

	/** @internal */
	inline void if_freenameindex(struct if_nameindex *)
	{}
#endif	// __ANDROID__

#if defined(_WIN32)
#if defined( USE_OPENSSL )
// Namespace to wrap OpenSSL symbols
namespace BWOpenSSL
{
#include "openssl/crypto.h"
#include "openssl/engine.h"
#include "openssl/err.h"
#include "openssl/evp.h"
}
#endif // USE_OPENSSL
#endif // _WIN32

#ifndef CODE_INLINE

BW_BEGIN_NAMESPACE
#include "endpoint.ipp"
BW_END_NAMESPACE

#endif

 
DECLARE_DEBUG_COMPONENT2( "Network", 0 )


BW_BEGIN_NAMESPACE


/**
 *	This method enables the per-socket error queue for this socket.
 *
 *	@param shouldEnable 	If true, the error queue will be enabled, otherwise 
 *							it will be disabled. 
 */
void Endpoint::enableErrorQueue( bool shouldEnable )
{
#if defined( __unix__ )
	int enableValue = shouldEnable ? 1 : 0;

	::setsockopt( socket_, SOL_IP, IP_RECVERR, &enableValue, sizeof(int) );
#else
	// No-op for other OS's.
#endif
}


/**
 *	This method reads the error data from the error queue. This requires that 
 *
 *	@param queuedErrNo 		This is filled with the error's errno.
 *	@param offenderAddress 	This is filled with the offending peer address
 *							associated with the error.
 *	@param info		 		This is filled with the ee_info 
 * 							from sock_extended_err.
 *
 *	@return True if closedPort was set, otherwise false.
 */
bool Endpoint::readFromErrorQueue( int & queuedErrNo,
		Mercury::Address & offenderAddress, uint32 & info )
{
	bool isResultSet = false;

#if defined( __linux__ ) && !defined( EMSCRIPTEN )
	struct sockaddr_in	offender;
	offender.sin_family = 0;
	offender.sin_port = 0;
	offender.sin_addr.s_addr = 0;

	struct msghdr	errHeader;
	struct iovec	errPacket;

	char data[ 256 ];
	char control[ 256 ];

	errHeader.msg_name = &offender;
	errHeader.msg_namelen = sizeof( offender );
	errHeader.msg_iov = &errPacket;
	errHeader.msg_iovlen = 1;
	errHeader.msg_control = control;
	errHeader.msg_controllen = sizeof( control );
	errHeader.msg_flags = 0;	// result only

	errPacket.iov_base = data;
	errPacket.iov_len = sizeof( data );

	int errMsgErr = recvmsg( this->fileno(), &errHeader, MSG_ERRQUEUE );
	if (errMsgErr < 0)
	{
		return false;
	}

	struct cmsghdr * ctlHeader = NULL;

	for (ctlHeader = CMSG_FIRSTHDR( &errHeader );
			ctlHeader != NULL;
			ctlHeader = CMSG_NXTHDR( &errHeader, ctlHeader ))
	{
		if ((ctlHeader->cmsg_level == SOL_IP) &&
				(ctlHeader->cmsg_type == IP_RECVERR))
		{
			break;
		}
	}

	// Was there an IP_RECVERR error.

	if (ctlHeader != NULL)
	{
		struct sock_extended_err * extError =
			(struct sock_extended_err*)CMSG_DATA( ctlHeader );

		queuedErrNo = extError->ee_errno;

		// Only use this address if the kernel has the bug where it does not
		// report the packet details.

		if (errHeader.msg_namelen == 0)
		{
			// Finally we figure out whose fault it is except that this is the
			// generator of the error (possibly a machine on the path to the
			// destination), and we are interested in the actual destination.
			offender = *(sockaddr_in*)SO_EE_OFFENDER( extError );
			offender.sin_port = 0;

			ERROR_MSG( "Endpoint::readFromErrorQueue: "
				"Kernel has a bug: recv_msg did not set msg_name.\n" );
		}

		offenderAddress.ip = offender.sin_addr.s_addr;
		offenderAddress.port = offender.sin_port;
		info = extError->ee_info;

		isResultSet = true;
	}
#endif // defined( __linux__ ) && !defined( EMSCRIPTEN )

	return isResultSet;
}


/**
 * Generate a address/name map of all network interfaces.
 *
 * @param	interfaces    The map to populate with the interface list.
 * @param	netmaskStr    Optional netmask of form "a.b.c.d/#bits"
 *
 * @returns true on success, false on error.
 */
bool Endpoint::getInterfaces( InterfaceMap & interfaces,
	const char * netmaskStr )
{
	BW_GUARD;
#ifdef _WIN32
	CRITICAL_MSG( "Endpoint::getInterfaces: Not implemented for Windows.\n" );
	return false;

#else
	struct ifconf ifc;
	char buf[ 1024 ];

	ifc.ifc_len = sizeof( buf );
	ifc.ifc_buf = buf;

	if (ioctl( socket_, SIOCGIFCONF, &ifc ) < 0)
	{
		ERROR_MSG( "Endpoint::getInterfaces: ioctl(SIOCGIFCONF) failed.\n" );
		return false;
	}

	// Iterate through the list of interfaces & filter by netmask if given.
    NetMask netmask;
    if (netmaskStr)
    {
        if (!netmask.parse( netmaskStr ))
        {
            WARNING_MSG( 
                "Endpoint::getInterfaces: failed to parse netmask string '%s'\n", 
                netmaskStr );
        }
    }

	struct ifreq * ifr = ifc.ifc_req;
	int nInterfaces = ifc.ifc_len / sizeof( struct ifreq );

	for (int i = 0; i < nInterfaces; i++)
	{
		struct ifreq *item = &ifr[i];
		struct sockaddr_in * s = (struct sockaddr_in *) &(item->ifr_addr);

        if (!netmask.containsAddress( s->sin_addr.s_addr ))
        {
            continue;
        }

        interfaces[ s->sin_addr.s_addr ] = item->ifr_name;
	}

	return true;
#endif
}


/**
 *  This function finds the default interface, i.e. the struct one to use if
 *	an IP address is required for a socket that is bound to all interfaces.
 *
 *	Currently, the first valid (non-loopback) interface is used, but this
 *	should really be changed to be whatever interface is used by a local
 *	network broadcast - i.e. the interface that the default route goes over
 */
int Endpoint::findDefaultInterface( char * name )
{
	BW_GUARD;
#if !defined( __unix__ ) && !defined( __APPLE__ ) || defined( EMSCRIPTEN )
	strcpy( name, "eth0" );
	return 0;
#else
	int		ret = -1;

	struct if_nameindex* pIfInfo = if_nameindex();
	if (pIfInfo)
	{
		int		flags = 0;
		struct if_nameindex* pIfInfoCur = pIfInfo;
		while (pIfInfoCur->if_name)
		{
			flags = 0;
			this->getInterfaceFlags( pIfInfoCur->if_name, flags );

			if ((flags & IFF_UP) && (flags & IFF_RUNNING))
			{
				u_int32_t	addr;
				if (this->getInterfaceAddress( pIfInfoCur->if_name, addr ) == 0)
				{
					strcpy( name, pIfInfoCur->if_name );
					ret = 0;

					// we only stop if it's not a loopback address,
					// otherwise we continue, hoping to find a better one
					if (!(flags & IFF_LOOPBACK)) break;
				}
			}
			++pIfInfoCur;
		}
		if_freenameindex(pIfInfo);
	}
	else
	{
		ERROR_MSG( "Endpoint::findDefaultInterface: "
							"if_nameindex returned NULL (%s)\n",
						strerror( errno ) );
	}

	return ret;
#endif // __unix__
}

/**
 *	This function finds the interfaced specified by a string. The
 *	specification may take the form of a straight interface name,
 *	a IP address (name/dotted decimal), or a netmask (IP/bits).
 */
int Endpoint::findIndicatedInterface( const char * spec, char * name )
{
	BW_GUARD;
	// start with it cleared
	name[0] = 0;

	// make sure there's something there
	if (spec == NULL || spec[0] == 0) return -1;

	// set up some working vars
	char * slash;
	int netmaskbits = 32;
	char iftemp[IFNAMSIZ+16];
	strncpy( iftemp, spec, IFNAMSIZ ); iftemp[IFNAMSIZ] = 0;
	u_int32_t addr = 0;

	// see if it's a netmask
	if ((slash = const_cast< char * >( strchr( spec, '/' ) )) && slash-spec <= 16)
	{
		// specified a netmask
		MF_ASSERT( IFNAMSIZ >= 16 );
		iftemp[slash-spec] = 0;
		bool ok = Endpoint::convertAddress( iftemp, addr ) == 0;

		netmaskbits = atoi( slash+1 );
		ok &= netmaskbits > 0 && netmaskbits <= 32;

		if (!ok)
		{
			ERROR_MSG("Endpoint::findIndicatedInterface: "
				"netmask match %s length %s is not valid.\n", iftemp, slash+1 );
			return -1;
		}
	}
	else if (this->getInterfaceAddress( iftemp, addr ) == 0)
	{
		// specified name of interface
		strncpy( name, iftemp, IFNAMSIZ );
	}
	else if (Endpoint::convertAddress( spec, addr ) == 0)
	{
		// specified ip address
		netmaskbits = 32; // redundant but instructive
	}
	else
	{
		ERROR_MSG( "Endpoint::findIndicatedInterface: "
			"No interface matching interface spec '%s' found\n", spec );
		return -1;
	}

	// if we haven't set a name yet then we're supposed to
	// look up the ip address
	if (name[0] == 0)
	{
		int netmaskshift = 32-netmaskbits;
		u_int32_t netmaskmatch = ntohl(addr);

		BW::vector< BW::string > interfaceNames;

		struct if_nameindex* pIfInfo = if_nameindex();
		if (pIfInfo)
		{
			struct if_nameindex* pIfInfoCur = pIfInfo;
			while (pIfInfoCur->if_name)
			{
				interfaceNames.push_back( pIfInfoCur->if_name );
				++pIfInfoCur;
			}
			if_freenameindex(pIfInfo);
		}

		BW::vector< BW::string >::iterator iter = interfaceNames.begin();

		while (iter != interfaceNames.end())
		{
			u_int32_t tip = 0;
			char * currName = (char *)iter->c_str();

			if (this->getInterfaceAddress( currName, tip ) == 0)
			{
				u_int32_t htip = ntohl(tip);

				if ((htip >> netmaskshift) == (netmaskmatch >> netmaskshift))
				{
					//DEBUG_MSG("Endpoint::bind(): found a match\n");
					strncpy( name, currName, IFNAMSIZ );
					break;
				}
			}

			++iter;
		}

		if (name[0] == 0)
		{
			uint8 * qik = (uint8*)&addr;
			ERROR_MSG( "Endpoint::findIndicatedInterface: "
				"No interface matching netmask spec '%s' found "
				"(evals to %d.%d.%d.%d/%d)\n", spec,
				qik[0], qik[1], qik[2], qik[3], netmaskbits );

			return -2; // parsing ok, just didn't match
		}
	}

	return 0;
}


/**
 * 	This method converts a string containing an IP address into
 * 	a 32 integer in network byte order. It handles both numeric
 * 	and named addresses.
 *
 *	@param string	The address as a string
 *	@param address	The address is returned here as an integer.
 *
 *	@return 0 if successful, -1 otherwise.
 */
int Endpoint::convertAddress(const char * string, u_int32_t & address)
{
	BW_GUARD;
	u_int32_t	trial;

	// first try it as numbers and dots
	#ifdef __unix__
	if (inet_aton( string, (struct in_addr*)&trial ) != 0)
	#else
	if ( (trial = inet_addr( string )) != INADDR_NONE )
	#endif
		{
			address = trial;
			return 0;
		}

	// ok, try looking it up then
#ifndef _XBOX
	struct hostent * hosts = gethostbyname( string );
	if (hosts != NULL)
	{
		address = *(u_int32_t*)(hosts->h_addr_list[0]);
		return 0;
	}
#else
	XNDNS * pResult = NULL;
	bool good = false;
	if (XNetDnsLookup( string, NULL, &pResult ) == 0)
	{
		while (pResult->iStatus == WSAEINPROGRESS)
		{
			Sleep( 10 );
		}
		if (pResult->iStatus == 0 && pResult->cina > 0)
		{
			address = pResult->aina[0].S_un.S_addr;
			good = true;
		}
	}
	if (pResult != NULL) XNetDnsRelease( pResult );
	if (good) return 0;
#endif	// _XBOX

	// that didn't work either - I give up then
	return -1;
}


/**
 *	This method returns the current size of the transmit and receive queues
 *	for this socket. This method is only implemented on Unix.
 *
 *	@param tx	The current size of the transmit queue is returned here.
 *	@param rx 	The current size of the receive queue is returned here.
 *
 *	@return 0 if successful, -1 otherwise.
 */
#ifdef __unix__
int Endpoint::getQueueSizes( int & tx, int & rx ) const
{
	BW_GUARD;

	if (NetworkStats::getQueueSizes( *this, tx, rx ))
	{
		return 0;
	}

	return -1;
}
#else
int Endpoint::getQueueSizes( int &, int & ) const
{
	BW_GUARD;
	return -1;
}
#endif


/**
 *  This method returns either the send or receive buffer size for this socket,
 *  or -1 on error.  You should pass either SO_RCVBUF or SO_SNDBUF as the
 *  argument to this method.
 */
int Endpoint::getBufferSize( int optname ) const
{
	BW_GUARD;
#ifdef __unix__
	MF_ASSERT( optname == SO_SNDBUF || optname == SO_RCVBUF );

	int recvbuf = -1;
	socklen_t rbargsize = sizeof( int );
	int rberr = getsockopt( socket_, SOL_SOCKET, optname,
		(char*)&recvbuf, &rbargsize );

	if (rberr == 0 && rbargsize == sizeof( int ))
	{
		return recvbuf;
	}
	else
	{
		ERROR_MSG( "Endpoint::getBufferSize: "
			"Failed to read option %s: %s\n",
			optname == SO_SNDBUF ? "SO_SNDBUF" : "SO_RCVBUF",
			strerror( errno ) );

		return -1;
	}

#else
	return -1;
#endif
}


/**
 *  This method sets either the send or receive buffer size for this socket.
 *  You should pass either SO_RCVBUF or SO_SNDBUF as the optname argument to
 *  this method.
 */
bool Endpoint::setBufferSize( int optname, int size )
{
	BW_GUARD;
#ifdef __unix__
	setsockopt( socket_, SOL_SOCKET, optname, (const char*)&size,
		sizeof( size ) );
#endif

	return this->getBufferSize( optname ) >= size;
}


/**
 *	This helper method wait until exactly gramSize data has been read.
 *
 *	True if gramSize was read, otherwise false (usually indicating the
 *	connection was lost.
 */
bool Endpoint::recvAll( void * gramData, int gramSize ) const
{
	BW_GUARD;
	while (gramSize > 0)
	{
		int len = this->recv( gramData, gramSize );

		if (len <= 0)
		{
			if (len == 0)
			{
				WARNING_MSG( "Endpoint::recvAll: Connection lost\n" );
			}
			else
			{
				WARNING_MSG( "Endpoint::recvAll: Got error '%s'\n",
					strerror( errno ) );
			}

			return false;
		}
		gramSize -= len;
		gramData = ((char *)gramData) + len;
	}

	return true;
}


/**
 *	This method returns the address that this endpoint is bound to.
 */
Mercury::Address Endpoint::getLocalAddress() const
{
	BW_GUARD;
	Mercury::Address addr( 0, 0 );

	if (this->getlocaladdress( (u_int16_t*)&addr.port,
				(u_int32_t*)&addr.ip ) == -1)
	{
		ERROR_MSG( "Endpoint::getLocalAddress: Failed\n" );
	}

	return addr;
}


/**
 *	This method returns the address that this endpoint is bound to.
 */
Mercury::Address Endpoint::getRemoteAddress() const
{
	BW_GUARD;
	Mercury::Address addr( 0, 0 );

	if (this->getremoteaddress( (u_int16_t*)&addr.port,
				(u_int32_t*)&addr.ip ) == -1)
	{
		ERROR_MSG( "Endpoint::getRemoteAddress: Failed\n" );
	}

	return addr;
}

#if defined(_WIN32)
#if defined( USE_OPENSSL )
MEMTRACKER_DECLARE( ThirdParty_OpenSSL, "ThirdParty - OpenSSL", 0 );
namespace OpenSSLMemHooks
{
	void* malloc( size_t size )
	{
		MEMTRACKER_SCOPED( ThirdParty_OpenSSL );
		return bw_malloc( size );
	}

	void free( void* mem )
	{
		MEMTRACKER_SCOPED( ThirdParty_OpenSSL );
		bw_free( mem );
	}

	void* realloc( void* mem, size_t size )
	{
		MEMTRACKER_SCOPED( ThirdParty_OpenSSL );
		return bw_realloc( mem, size );
	}
}
#endif // USE_OPENSSL
#endif // _WIN32

/**
 *	Global function to initialise the network
 */
static bool s_networkInitted = false;

void initNetwork()
{
	BW_GUARD;
	if (s_networkInitted) return;
	s_networkInitted = true;
	
#if defined(_WIN32)
#if defined( USE_OPENSSL )
	if (!BWOpenSSL::CRYPTO_set_mem_functions( 
			OpenSSLMemHooks::malloc, 
			OpenSSLMemHooks::realloc, 
			OpenSSLMemHooks::free))
	{
		DEV_CRITICAL_MSG( "Endpoint::initNetwork: "
			"Could not set OpenSSL memory hooks. This can be caused by OpenSSL "
			"being used and allocating memory before this point.\n" );
	}

#if !defined( _RELEASE )
	{
		void *(*currentMalloc)(size_t);
		void *(*currentRealloc)(void *, size_t);
		void (*currentFree)(void *);

		BWOpenSSL::CRYPTO_get_mem_functions( 
			&currentMalloc, 
			&currentRealloc, 
			&currentFree );

		MF_ASSERT( currentMalloc == OpenSSLMemHooks::malloc &&
			currentRealloc == OpenSSLMemHooks::realloc &&
			currentFree == OpenSSLMemHooks::free );
	}
#endif // !_RELEASE
#endif // USE_OPENSSL
#endif // _WIN32
	
#ifdef _WIN32
#ifdef _XBOX
    XNetStartupParams xnsp;
	memset(&xnsp, 0, sizeof(xnsp));
	xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
	xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
	INT err = XNetStartup(&xnsp);
#endif

	WSAData wsdata;
	WSAStartup( 0x202, &wsdata );
#endif // !__unix__
}

void finiNetwork()
{
	BW_GUARD;
	if (!s_networkInitted)
	{
		return;
	}

#if defined(_WIN32)
#if defined( USE_OPENSSL )
	BWOpenSSL::EVP_cleanup();
	BWOpenSSL::ENGINE_cleanup();
	BWOpenSSL::ERR_free_strings();
	BWOpenSSL::ERR_remove_state( 0 );
	BWOpenSSL::CRYPTO_cleanup_all_ex_data();
#endif // USE_OPENSSL
#endif // _WIN32

#ifdef _WIN32
	// Kill winsock
	WSACleanup();
#endif
	return;
}


#ifdef MF_SERVER
namespace
{

class StaticIniter
{
public:
	StaticIniter()
	{
		BW_GUARD;
		struct sigaction ignore;
		memset(&ignore, 0, sizeof(ignore)); // silence valgrind
		ignore.sa_handler = SIG_IGN;
		sigaction( SIGPIPE, &ignore, NULL );
	}
};

StaticIniter g_staticIniter;

} // anonymous namespace
#endif

BW_END_NAMESPACE

// endpoint.cpp
