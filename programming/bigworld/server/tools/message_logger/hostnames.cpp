#include "hostnames.hpp"
#include "constants.hpp"

#include "cstdmf/debug.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>


BW_BEGIN_NAMESPACE


void Hostnames::clear()
{
	hostnames_.clear();
}


/**
 * This method attempts to discover the hostname associated with the provided
 * IP address.
 *
 * If the hostname does not already exist in the hostname map a network
 * query will occur to try and resolve the host.
 *
 * @param addr			The IP address for the hostname lookup
 * @param hostname		A reference to a BW::string where the hostname is stored
 *
 * @returns GetHostResult status (Found in cache; Added to cache; or Error)
 */
GetHostResult Hostnames::getHostByAddr( MessageLogger::IPAddress addr,
	BW::string & hostname )
{
	HostnamesMap::iterator it = hostnames_.find( addr );
	if (it != hostnames_.end())
	{
		hostname = it->second;
		return BW_GET_HOST_FOUND;
	}

	struct hostent *ent = gethostbyaddr( &addr, sizeof( addr ), AF_INET );

	// Unable to resolve the hostname, store the IP address as a string
	if (ent == NULL)
	{
		const char *reason = NULL;
		hostname = inet_ntoa( (in_addr&)addr );

		switch (h_errno)
		{
			case HOST_NOT_FOUND:
				reason = "HOST_NOT_FOUND"; break;
			case NO_DATA:
				reason = "NO_DATA"; break;
			case NO_RECOVERY:
				reason = "NO_RECOVERY"; break;
			case TRY_AGAIN:
				reason = "TRY_AGAIN"; break;
			default:
				reason = "Unknown reason";
		}

		WARNING_MSG( "Hostnames::getHostByAddr: Unable to resolve hostname "
			"of %s (%s)\n", hostname.c_str(), reason );
	}
	else
	{
		char *firstdot = strstr( ent->h_name, "." );
		if (firstdot != NULL)
		{
			*firstdot = '\0';
		}
		hostname = ent->h_name;
	}

	if (!this->writeHostnameToDB( addr, hostname ))
	{
		hostname.clear();
		return BW_GET_HOST_ERROR;
	}

	hostnames_[ addr ] = hostname;
	return BW_GET_HOST_ADDED;
}


/**
 * This method retrieves the address associated with the specified hostname
 * from the hostname map.
 *
 * @returns IP address of the hostname on success, 0 if not known.
 */
MessageLogger::IPAddress Hostnames::getHostByName( const char *hostname ) const
{
	BW::string hostString = hostname;
	HostnamesMap::const_iterator it = hostnames_.begin();

	while (it != hostnames_.end())
	{
		if (it->second == hostString)
		{
			return it->first;
		}

		++it;
	}

	return 0;
}


/**
 * This method invokes onHost on the visitor for all the hostname entries
 * stored in the hostname map.
 *
 * @returns true on success, false on error.
 */
bool Hostnames::visitAllWith( HostnameVisitor & visitor ) const
{
	HostnamesMap::const_iterator iter = hostnames_.begin();
	bool status = true;

	while ((iter != hostnames_.end()) && (status == true))
	{
		status = visitor.onHost( iter->first, iter->second );
		++iter;
	}

	return status;
}


BW_END_NAMESPACE

// hostnames.cpp
