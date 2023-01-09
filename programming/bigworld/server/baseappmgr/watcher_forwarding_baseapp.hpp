#ifndef BA_WATCHER_FORWARDING_HPP
#define BA_WATCHER_FORWARDING_HPP

#include "network/basictypes.hpp"
#include "server/watcher_forwarding.hpp"


BW_BEGIN_NAMESPACE

class BaseApp;
class WatcherPathRequestV2;
class ForwardingCollector;

/**
 * This class implments a BaseAppMgr specific Forwarding Watcher.
 */
class BAForwardingWatcher : public ForwardingWatcher
{
public:
	ForwardingCollector *newCollector(
		WatcherPathRequestV2 & pathRequest,
		const BW::string & destWatcher, const BW::string & targetInfo );


	AddressList * getComponentAddressList( const BW::string & targetInfo );

private:
	AddressPair addressPair( const BaseApp & baseApp ) const;

	AddressList * listLeastLoaded( bool useBaseApp, bool useServiceApp ) const;
	AddressList * listAll( bool useBaseApp, bool useServiceApp ) const;
};

BW_END_NAMESPACE

#endif
