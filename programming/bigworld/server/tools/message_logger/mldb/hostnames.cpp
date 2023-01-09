#include "hostnames.hpp"
#include "../constants.hpp"

#include "cstdmf/debug.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>


BW_BEGIN_NAMESPACE

static const char * HOSTNAME_FILE = "hostnames";


bool HostnamesMLDB::init( const char *logLocation, const char *mode )
{
	const char *hostnamesPath = this->join( logLocation, HOSTNAME_FILE );
	return TextFileHandler::init( hostnamesPath, mode );
}


void HostnamesMLDB::flush()
{
	this->clear();
}


bool HostnamesMLDB::handleLine( const char *line )
{
	char hostIp[64];
	char hostName[1024];
	struct in_addr addr;

	if (sscanf( line, "%63s %1023s", hostIp, hostName ) != 2)
	{
		ERROR_MSG( "HostnamesMLDB::handleLine: Unable to read hostnames file "
			"entry (%s)\n", line );
		return false;
	}

	if (inet_aton( hostIp, &addr ) == 0)
	{
		ERROR_MSG( "HostnamesMLDB::handleLine: Unable to convert hostname "
			"entry '%s' to a valid IPv4 address\n", hostIp );
		return false;
	}
	else
	{
		this->addHostnameToMap( addr.s_addr, hostName );
		return true;
	}
}


/**
 * This method write new Hostnames to DB.
 */
bool HostnamesMLDB::writeHostnameToDB( const MessageLogger::IPAddress & addr,
	const BW::string & hostname )
{
	// Write the mapping to disk
	const char *ipstr = inet_ntoa( (in_addr&)addr );
	char line[ 2048 ];
	bw_snprintf( line, sizeof( line ), "%s %s", ipstr, hostname.c_str() );
	if (!this->writeLine( line ))
	{
		CRITICAL_MSG( "HostnamesMLDB::getHostByAddr: "
			"Couldn't write hostname mapping for %s\n", line );
		return false;
	}
	else
	{
		return true;
	}
}


const char * HostnamesMLDB::getHostnamesFilename()
{
	return HOSTNAME_FILE;
}


BW_END_NAMESPACE

// hostnames.cpp
