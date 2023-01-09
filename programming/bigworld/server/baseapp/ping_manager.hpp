#ifndef PING_MANAGER_HPP
#define PING_MANAGER_HPP


BW_BEGIN_NAMESPACE

namespace Mercury
{
class EventDispatcher;
class NetworkInterface;
}

namespace PingManager
{ 
	bool init( Mercury::EventDispatcher & dispatcher,
			Mercury::NetworkInterface & networkInterface );
	void fini();
}

BW_END_NAMESPACE

#endif // PING_MANAGER_HPP
