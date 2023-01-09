#ifndef CONSOLIDATE_DBS_SECONDARY_DB_INFO_HPP
#define CONSOLIDATE_DBS_SECONDARY_DB_INFO_HPP

#include "cstdmf/stdmf.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Stores information about the location of a secondary DB.
 */
class SecondaryDBInfo
{
public:
	SecondaryDBInfo() : 
			hostIP( 0 ), 
			location()
	{}

	SecondaryDBInfo( uint32 ip, const BW::string & path ) :
		hostIP( ip ),
		location( path )
	{}

	uint32		hostIP;
	BW::string location;
};

typedef BW::vector< SecondaryDBInfo > SecondaryDBInfos;

BW_END_NAMESPACE

#endif // CONSOLIDATE_DBS_SECONDARY_DB_INFO_HPP

