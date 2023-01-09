#ifndef HOSTNAMES_HPP
#define HOSTNAMES_HPP

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/stdmf.hpp"

#include "types.hpp"


BW_BEGIN_NAMESPACE

/**
 * This abstract class provides a template which can be used to retrieve
 * the hosts being stored in a Hostnames instance.
 */
class HostnameVisitor
{
public:
	virtual bool onHost( MessageLogger::IPAddress ipAddress,
		const BW::string &hostname ) = 0;
};


enum GetHostResult
{
	BW_GET_HOST_ERROR,
	BW_GET_HOST_FOUND,
	BW_GET_HOST_ADDED
};


/**
 * This class handles the mapping between IP addresses and hostnames
 */
class Hostnames
{
public:
	virtual bool writeHostnameToDB( const MessageLogger::IPAddress & addr,
		const BW::string & hostname ) = 0;

	GetHostResult getHostByAddr( MessageLogger::IPAddress addr,
		BW::string & hostname );

	uint32 getHostByName( const char *hostname ) const;

	bool visitAllWith( HostnameVisitor &visitor ) const;

	void updateHostname(  const MessageLogger::IPAddress & addr,
			const BW::string & hostname )
	{
		if (hostnames_.find( addr ) != hostnames_.end())
		{
			hostnames_[ addr ] = hostname;
		}
	}

protected:
	void clear();

	void addHostnameToMap( const MessageLogger::IPAddress & addr,
		const BW::string & hostname ) { hostnames_[ addr ] = hostname; }

private:
	typedef BW::map< MessageLogger::IPAddress, BW::string > HostnamesMap;
	HostnamesMap hostnames_;
};

BW_END_NAMESPACE

#endif // HOSTNAMES_HPP
