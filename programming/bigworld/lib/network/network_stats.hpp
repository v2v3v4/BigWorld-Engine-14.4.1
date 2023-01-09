#ifndef NETWORK_STATS_HPP
#define NETWORK_STATS_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class Endpoint;

namespace NetworkStats
{
	bool getQueueSizes( const Endpoint & ep, int & tx, int & rx );
};

BW_END_NAMESPACE


#endif //NETWORK_STATS_HPP

