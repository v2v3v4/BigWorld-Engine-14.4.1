#ifndef CA_WATCHER_FORWARDING_HPP
#define CA_WATCHER_FORWARDING_HPP

#include "network/basictypes.hpp"
#include "server/watcher_forwarding.hpp"


BW_BEGIN_NAMESPACE

class WatcherPathRequestV2;
class ForwardingCollector;

/**
 * This class implments a CellAppMgr specific Forwarding Watcher.
 */
class CAForwardingWatcher : public ForwardingWatcher
{
public:
	ForwardingCollector *newCollector(
		WatcherPathRequestV2 & pathRequest,
		const BW::string & destWatcher, const BW::string & targetInfo );


	AddressList * getComponentAddressList( const BW::string & targetInfo );
};

BW_END_NAMESPACE

#endif
