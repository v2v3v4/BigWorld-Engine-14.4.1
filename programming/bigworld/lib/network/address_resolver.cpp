#include "address_resolver.hpp"

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

BW_BEGIN_NAMESPACE

/**
 *	Resolve a host name to a dotted-decimal string representation of its IP
 *	address.
 *
 *	@param hostName		The host name to resolve.
 *	@param out 			If successful, this is filled with the dotted-decimal
 *						representation of the hostname's resolved address.
 *
 *	@return				true on success, false otherwise.
 */
bool Mercury::AddressResolver::resolve( const BW::string & hostName, 
		BW::string & out )
{
#if defined( EMSCRIPTEN )
	// No getaddrinfo(), fall back to gethostbyname() which is implemented.

	struct hostent * hostInfo = gethostbyname( hostName.c_str() );
	if (!hostInfo)
	{
		return false;
	}

	out.assign( hostInfo->h_name );
	return true;

#else // !defined( EMSCRIPTEN )

	struct addrinfo * addrInfo = NULL;
	int ret;
	if (0 != (ret = getaddrinfo( hostName.c_str(), NULL, NULL, &addrInfo )))
	{
		return false;
	}

	if (addrInfo->ai_family != AF_INET)
	{
		// We don't support anything other than IPv4 currently.
		return false;
	}

	char buf[64];
	struct sockaddr_in * addr = 
		reinterpret_cast< struct sockaddr_in * >( addrInfo->ai_addr );

	if (NULL == inet_ntop( addrInfo->ai_family, &(addr->sin_addr), 
			buf, sizeof( buf ) ))
	{
		return false;
	}
	out.assign( buf );

	freeaddrinfo( addrInfo );

	return true;

#endif // defined( EMSCRIPTEN )
}

BW_END_NAMESPACE

// address_resolver.cpp
