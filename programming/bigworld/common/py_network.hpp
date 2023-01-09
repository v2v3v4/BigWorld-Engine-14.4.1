#ifndef PY_NETWORK_HPP
#define PY_NETWORK_HPP

#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"


BW_BEGIN_NAMESPACE

namespace PyNetwork
{
	bool init( Mercury::EventDispatcher & dispatcher,
			Mercury::NetworkInterface & interface );
}

BW_END_NAMESPACE

#endif // PY_NETWORK_HPP
