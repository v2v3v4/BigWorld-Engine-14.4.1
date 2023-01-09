#include "cstdmf/debug.hpp"
#include "cstdmf/stringmap.hpp"

#include "server_info.hpp"

#include <unistd.h>
#include <limits.h>
#include <sstream>

DECLARE_DEBUG_COMPONENT( 0 );


BW_BEGIN_NAMESPACE

ServerInfo::ServerInfo()
{
#ifdef _WIN32 // WIN32PORT

	serverName_ = "Not yet implemented";

	cpuInfo_ = "Not yet implemented";
	memUsed_ = 0;
	memTotal_ = 0;

	cpuSpeeds_.push_back( 1.0 );

#else // _WIN32

	char hostName[ HOST_NAME_MAX ];
	gethostname( hostName, HOST_NAME_MAX );
	hostName[ HOST_NAME_MAX - 1 ] = '\0';

	char* firstDot = strchr( hostName, '.' );
	if ( firstDot != NULL )
	{
		*firstDot = '\0';
	}
	serverName_ = hostName;

	fetchLinuxCpuInfo();
	fetchLinuxMemInfo();

#endif // _WIN32

	BW::stringstream memInfoBuf;
	memInfoBuf << ( memTotal_ >> 10 ) << "kB";
	memInfo_ = memInfoBuf.str();
}

void ServerInfo::updateMem()
{
#ifndef _WIN32 // WIN32PORT
	fetchLinuxMemInfo();
#endif // _WIN32
}

#ifndef _WIN32 // WIN32PORT
/**
 * Parse /proc/cpuinfo and fill in our cpuName_ and cpuSpeeds_
 */
void ServerInfo::fetchLinuxCpuInfo()
{
	// /proc/cpuinfo format assumption
	// Each line is either blank, or is 'label\t+: Value'

	FILE* pProcCpu;
	char procLine[BUFSIZ];
	char* pInterest;
	float mhz;
	typedef StringHashMap< unsigned int > modelNameMap;
	modelNameMap modelNames;

	pProcCpu = fopen( "/proc/cpuinfo", "r" );
	if ( pProcCpu == NULL )
	{
		return;
	}

	while ( fgets( procLine, BUFSIZ, pProcCpu ) != NULL )
	{
		pInterest = strchr( procLine, '\n' );
		if ( pInterest != NULL )
		{
			*pInterest = '\0';
		}
		pInterest = strchr( procLine, '\r' );
		if ( pInterest != NULL )
		{
			*pInterest = '\0';
		}

		if ( procLine[ 0 ] != '\0' )
		{
			pInterest = strchr( procLine, '\t' );
			if ( pInterest == NULL )
			{
				continue;
			}
			*pInterest = '\0';
			pInterest++;
			pInterest = strchr( pInterest, ':' );
			if ( pInterest == NULL )
			{
				continue;
			}
			pInterest += 2;

			if (!strcmp( procLine, "cpu MHz" ) )
			{
				sscanf( pInterest, "%f", &mhz );
				cpuSpeeds_.push_back( mhz );
			}
			else
			if ( !strcmp( procLine, "model name" ) )
			{
				modelNames[ pInterest ] += 1;
			}
		}
	}

	fclose( pProcCpu );

	BW::stringstream cpuInfoBuf;

	modelNameMap::const_iterator modelNamesIt;
	modelNameMap::const_iterator modelNamesEnd = modelNames.end();

	bool first = true;

	for ( modelNamesIt = modelNames.begin(); modelNamesIt != modelNamesEnd;
		++modelNamesIt )
	{
		if ( first )
		{
			first = false;
		}
		else
		{
			cpuInfoBuf << ", ";
		}

		if ( modelNamesIt->second == 1 )
		{
			cpuInfoBuf << modelNamesIt->first;
		}
		else
		{
			cpuInfoBuf << modelNamesIt->first << " x" << modelNamesIt->second;
		}
	}

	cpuInfo_ = cpuInfoBuf.str();

}


/**
 * Parse /proc/meminfo and fill in our memInfo_, memTotal_ and memUsed_
 * 
 */
void ServerInfo::fetchLinuxMemInfo()
{

	// /proc/meminfo format assumption
	// Each line is either blank, or is 'label: *Value kB'

	// MemTotal:  Total usable mem (i.e. physical mem minus a few reserved
	//            bits and the kernel binary code)
	// MemFree:   Is sum of LowFree+HighFree (overall stat)
	// Buffers:   Memory in buffer cache. mostly useless as metric nowadays
	// Cached:    Memory in the pagecache (diskcache) minus SwapCache
	// Slab:      The total amount of memory, in kilobytes, used by the kernel
	//            to cache data structures for its own use.

	// The unsigned long type is determined in Linux source (fs/proc/meminfo.c)
	unsigned long memTotal, memFree, buffers, cached, slab;
	FILE* pProcMem;
	char procLine[BUFSIZ];

	pProcMem = fopen( "/proc/meminfo", "r" );
	if ( pProcMem == NULL )
	{
		return;
	}

	// This is the number of values we're interested in, lets us early-out
	// of the parse loop
#define INTERESTING_VALUES 5
	int i = 0;
	while ( fgets( procLine, BUFSIZ, pProcMem ) != NULL )
	{
		if (sscanf( procLine, "MemTotal: %lu kB", &memTotal ) == 1 )
		{
			i++;
		}
		else if (sscanf( procLine, "MemFree: %lu kB", &memFree ) == 1 )
		{
			i++;
		}
		else if (sscanf( procLine, "Buffers: %lu kB", &buffers ) == 1 )
		{
			i++;
		}
		else if (sscanf( procLine, "Cached: %lu kB", &cached ) == 1 )
		{
			i++;
		}
		else if (sscanf( procLine, "Slab: %lu kB", &slab ) == 1 )
		{
			i++;
		}

		if ( i >= INTERESTING_VALUES )
		{
			break;
		}
	}

	fclose( pProcMem );

	// We store bytes, kernel info is in kilobytes
	memTotal_ = memTotal << 10;

	// We store bytes, kernel info is in kilobytes
	memUsed_ = ( memTotal - memFree - buffers - cached - slab ) << 10;
}
#endif // _WIN32

BW_END_NAMESPACE

// server_info.cpp
