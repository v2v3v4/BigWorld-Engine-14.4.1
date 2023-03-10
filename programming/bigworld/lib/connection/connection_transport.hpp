#ifndef CONNECTION_TRANSPORT_HPP
#define CONNECTION_TRANSPORT_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE


enum ConnectionTransport
{
	CONNECTION_TRANSPORT_UDP,
	CONNECTION_TRANSPORT_TCP,
	CONNECTION_TRANSPORT_WEBSOCKETS
};


BW_END_NAMESPACE

#endif // CONNECTION_TRANSPORT_HPP
