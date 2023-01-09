#ifndef ADDRESS_LOAD_PAIR_HPP
#define ADDRESS_LOAD_PAIR_HPP

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

#pragma pack( push, 1 )
/**
 * Info on BaseApps sent from BaseAppMgr to BaseApps
 */
struct BaseAppAddrLoadPair
{
	BaseAppAddrLoadPair() :
		load_( 0 )
	{ }

	BaseAppAddrLoadPair( const Mercury::Address & addr, float load ) :
		addr_( addr ),
		load_( load )
	{ }

	float load() const { return load_; }
	const Mercury::Address & addr() const { return addr_; }

	Mercury::Address addr_;
	float load_;
};
#pragma pack( pop )

BW_END_NAMESPACE

#endif // ADDRESS_LOAD_PAIR_HPP
