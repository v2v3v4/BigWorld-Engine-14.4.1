#ifndef MANAGER_APP_GATEWAY_HPP
#define MANAGER_APP_GATEWAY_HPP

#include "network/channel_owner.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
	class InterfaceElement;
}
class Watcher;

class ManagerAppGateway
{
public:

	ManagerAppGateway( Mercury::NetworkInterface & networkInterface,
		const Mercury::InterfaceElement & retireAppIE );
	virtual ~ManagerAppGateway();


	bool init( const char * interfaceName, int numRetries, float maxMgrRegisterStagger );

	const Mercury::Address & address() const { return channel_.addr(); }

	void addWatchers( const char * name, Watcher & watcher );

	void isRegular( bool localValue, bool remoteValue = false )
	{
		channel_.channel().isLocalRegular( localValue );
		channel_.channel().isRemoteRegular( remoteValue );
	}

	bool isInitialised() const
	{ return channel_.channel().addr() != Mercury::Address::NONE; }

	void retireApp();

protected:
	Mercury::ChannelOwner channel_;

private:
	const Mercury::InterfaceElement & retireAppIE_;
};

BW_END_NAMESPACE

#endif // MANAGER_APP_GATEWAY_HPP
