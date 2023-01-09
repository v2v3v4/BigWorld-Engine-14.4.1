#ifndef LOGIN_HANDLER_HPP
#define LOGIN_HANDLER_HPP

#include "cstdmf/stdmf.hpp"
#include "network/basictypes.hpp"
#include "connection/baseapp_ext_interface.hpp"


BW_BEGIN_NAMESPACE

class BaseApp;
class BinaryIStream;
class PendingLogins;
class Proxy;
class Watcher;

namespace Mercury
{
class NetworkInterface;
class UnpackedMessageHeader;
}

template <class TYPE> class SmartPointer;

typedef SmartPointer< Watcher > WatcherPtr;

class LoginHandler
{
public:
	LoginHandler();
	~LoginHandler();

	SessionKey add( Proxy * pProxy, const Mercury::Address & loginAppAddr );

	void tick();

	void login( Mercury::NetworkInterface & networkInterface,
			const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			const BaseAppExtInterface::baseAppLoginArgs & args );

	static WatcherPtr pWatcher();

private:
	void updateStatistics( const Mercury::Address & addr,
			const Mercury::Address & expectedAddr,
			uint32 attempt );

	PendingLogins * pPendingLogins_;

	// Statistics
	uint32	numLogins_;
	uint32	numLoginsAddrNAT_;
	uint32	numLoginsPortNAT_;
	uint32	numLoginsMultiAttempts_;
	uint32	maxLoginAttempts_;
	uint32	numLoginCollisions_;
};

BW_END_NAMESPACE

#endif // LOGIN_HANDLER_HPP
