#include "pch.hpp"
#include "access_monitor.hpp"

DECLARE_DEBUG_COMPONENT2( "AccessMonitor", 0 );

BW_BEGIN_NAMESPACE

void AccessMonitor::record( const BW::string &fileName )
{
	if ( active_ )
	{
		DEBUG_MSG( "AccessMonitor::(%s)\n", fileName.c_str() );
	}
}

AccessMonitor &AccessMonitor::instance()
{
	static AccessMonitor singletonInstance_;

	return singletonInstance_;
}

#ifndef CODE_INLINE
#include "access_monitor.ipp"
#endif

BW_END_NAMESPACE

// access_monitor.cpp

