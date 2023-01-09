#ifndef ENDPOINT_HPP
#define ENDPOINT_HPP

#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "basictypes.hpp" // For Mercury::Address

#include "cstdmf/bw_map.hpp"

#include <sys/types.h>
#if defined( __unix__ ) || defined( PLAYSTATION3 ) || defined( __APPLE__ ) || \
		defined( __ANDROID__ ) || defined( EMSCRIPTEN )
	#include <sys/time.h>
	#include <sys/socket.h>
#ifndef PLAYSTATION3
#if ! defined( __ANDROID__ ) && ! defined( EMSCRIPTEN )
	#include <sys/fcntl.h>
	#include <sys/ioctl.h>
	#include <net/if.h>
#else
	#include <fcntl.h>
	#include <sys/ioctl.h>
	#include <net/if.h>
	#undef fileno
#endif
#endif // !PLAYSTATION3
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>
#elif defined(_XBOX)
	#include <xtl.h>
	#include <winsockx.h>
#else
	#include <Winsock2.h>
#endif
#include <errno.h>
#include <stdlib.h>

#ifdef PLAYSTATION3
#include <netex/libnetctl.h>
#endif

#include "cstdmf/memory_stream.hpp"

BW_BEGIN_NAMESPACE

#if !defined( __unix__ ) && !defined( __APPLE__ ) && !defined( __ANDROID__ ) && \
		!defined( EMSCRIPTEN )

#ifdef PLAYSTATION3

typedef uint8_t 	u_int8_t;
typedef uint16_t 	u_int16_t;
typedef uint32_t 	u_int32_t;
typedef int			socket_t;

#else //expect windows

	/// The length of a socket address. It is for Windows only.
	typedef int socklen_t;

	/// A 16 bit unsigned integer.
	typedef u_short u_int16_t;
	/// A 32 bit unsigned integer.
	typedef u_long u_int32_t;

	typedef SOCKET socket_t;
#endif

	/// The maximum length of a network interface name.
	#define IFNAMSIZ 16

#else //__unix__, apple or android
	typedef int	socket_t;
#endif

class BinaryIStream;

/**
 *	This class provides a wrapper around a socket.
 *
 *	@ingroup network
 */
class Endpoint
{
public:
	/// @name Construction/Destruction
	//@{
	Endpoint();
	~Endpoint();
	//@}

	static const socket_t NO_SOCKET = static_cast<socket_t>(-1);

	/// @name File descriptor access
	//@{
	int fileno() const;
	void setFileDescriptor(socket_t fd);
	bool good() const;
	//@}

	/// @name General Socket Methods
	//@{
	void socket( int type );

	int setnonblocking( bool nonblocking );
	int setbroadcast( bool broadcast );
	int setreuseaddr( bool reuseaddr );
#ifndef _XBOX
	int setkeepalive( bool keepalive );
#endif

	int bind( u_int16_t networkPort = 0, u_int32_t networkAddr = INADDR_ANY );

	INLINE int close();
	INLINE int detach();

	int getlocaladdress(
		u_int16_t * networkPort, u_int32_t * networkAddr ) const;
	int getremoteaddress(
		u_int16_t * networkPort, u_int32_t * networkAddr ) const;

	Mercury::Address getLocalAddress() const;
	Mercury::Address getRemoteAddress() const;

	const char * c_str() const;

#ifndef _XBOX
	int getremotehostname( BW::string * name ) const;
#endif

	void enableErrorQueue( bool shouldEnable );
	bool readFromErrorQueue( int & queuedErrNo, 
		Mercury::Address & offenderAddress, uint32 & info );
	//@}

	/// @name Connectionless Socket Methods
	//@{
	INLINE int sendto( void * gramData, int gramSize,
		u_int16_t networkPort, u_int32_t networkAddr = BROADCAST) const;
	INLINE int sendto( void * gramData, int gramSize,
		const struct sockaddr_in & sin ) const;
	INLINE int sendto( void * gramData, int gramSize,
		const Mercury::Address & addr ) const;
	INLINE int recvfrom( void * gramData, int gramSize,
		u_int16_t * networkPort, u_int32_t * networkAddr ) const;
	INLINE int recvfrom( void * gramData, int gramSize,
		struct sockaddr_in & sin ) const;
	INLINE int recvfrom( void * gramData, int gramSize,
		Mercury::Address & addr ) const;
	//@}

	/// @name Connecting Socket Methods
	//@{
	int listen( int backlog = 5 );
	int connect( u_int16_t networkPort, u_int32_t networkAddr = BROADCAST );
	Endpoint * accept(
		u_int16_t * networkPort = NULL, u_int32_t * networkAddr = NULL );

	INLINE int send( const void * gramData, int gramSize ) const;
	int recv( void * gramData, int gramSize ) const;
	bool recvAll( void * gramData, int gramSize ) const;
	//@}

	/// @name Network Interface Methods
	//@{
	typedef BW::map< u_int32_t, BW::string > InterfaceMap;
	int getInterfaceFlags( char * name, int & flags );
	int getInterfaceAddress( const char * name, u_int32_t & address );
	bool getInterfaces( InterfaceMap & interfaces, const char * netmaskStr = NULL );
	int findDefaultInterface( char * name );
	int findIndicatedInterface( const char * spec, char * name );
	static int convertAddress( const char * string, u_int32_t & address );
	//@}

	/// @name Queue Size Methods
	//@{
	int transmitQueueSize() const;
	int receiveQueueSize() const;
	int getQueueSizes( int & tx, int & rx ) const;
	int numBytesAvailableForReading() const;
	//@}

	/// @name Buffer Size Methods
	//@{
	int getBufferSize( int optname ) const;
	bool setBufferSize( int optname, int size );
	//@}

	/// @name Socket options
	//@{
	template< typename T >
	inline int getSocketOption( int level, int optname, T & optval );

	template< typename T >
	inline int setSocketOption( int level, int optname, const T & optval );
	//@}

private:

	/// This is internal socket representation of the Endpoint.
	socket_t	socket_;
};

extern void initNetwork();
extern void finiNetwork();




/**
 *	This method wraps the getsockopt() call.
 */
template< typename T >
inline int Endpoint::getSocketOption( int level, int optname, T & optval )
{
#if defined( _WIN32 )
	char * optvalCast = reinterpret_cast< char * >( &optval );
	int optlenCast = sizeof(T);
#else
	void * optvalCast = &optval;
	socklen_t optlenCast = sizeof(T);
#endif

	return ::getsockopt( socket_, level, optname, optvalCast, &optlenCast );
}


/**
 *	This method wraps the setsockopt() call.
 */
template< typename T >
inline int Endpoint::setSocketOption( int level, int optname, const T & optval )
{
#if defined( _WIN32 )
	const char * optvalCast = reinterpret_cast< const char * >( &optval );
#else
	const void * optvalCast = &optval;
#endif
	return ::setsockopt( socket_, level, optname, optvalCast, sizeof(T) );
}


/**
 *	Template specialisation for setSocketOption for boolean socket options.
 */
template<>
inline int Endpoint::setSocketOption( int level, int optname, const bool & optval )
{
	int intVal = optval ? 1 : 0;
	return this->setSocketOption( level, optname, intVal );
}


/**
 *	Template specialisation for getSocketOption for boolean socket options.
 */
template<>
inline int Endpoint::getSocketOption( int level, int optname, bool & optval )
{
	int intVal = 0;
	int result = this->getSocketOption( level, optname, intVal );

	if (result != 0)
	{
		return result;
	}

	optval = (intVal != 0);
	return result;
}


#ifdef CODE_INLINE
#include "endpoint.ipp"
#endif

BW_END_NAMESPACE

#endif // ENDPOINT_HPP
