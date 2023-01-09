#ifndef WATCHER_CONNECTION_HPP
#define WATCHER_CONNECTION_HPP

#include "cstdmf/config.hpp"

#if ENABLE_WATCHERS

#include "interfaces.hpp"
#include "cstdmf/smartpointer.hpp"

#include <memory>

BW_BEGIN_NAMESPACE

class Endpoint;
class WatcherNub;
class BufferedTcpEndpoint;

namespace Mercury
{
class EventDispatcher;
}

class WatcherConnection;
typedef SmartPointer<WatcherConnection> WatcherConnectionPtr;


// -----------------------------------------------------------------------------
// Section: WatcherConnection
// -----------------------------------------------------------------------------

/**
 *
 */
class WatcherConnection : public Mercury::InputNotificationHandler,
	public ReferenceCount
{
public:
	WatcherConnection( WatcherNub & nub,
			Mercury::EventDispatcher & dispatcher,
			Endpoint * pEndpoint );

	~WatcherConnection();

	static WatcherConnection * handleAccept(
		Endpoint & listenEndpoint,
		Mercury::EventDispatcher & dispatcher,
		WatcherNub & watcherNub );

	Mercury::Address getRemoteAddress() const;

	int send( void * data, int32 size ) const;

	int handleInputNotification( int fd );

private:
	void handleDisconnect();

	bool recvSize();
	bool recvMsg();

	WatcherNub & nub_;
	Mercury::EventDispatcher & dispatcher_;
	std::auto_ptr<Endpoint> pEndpoint_;
	std::auto_ptr<BufferedTcpEndpoint> pBufferedTcpEndpoint_;

	uint32 receivedSize_;
	uint32 messageSize_;

	char * pBuffer_;

	// In order to keep the WatcherConnection active during a connection
	// it holds a reference to itself.
	WatcherConnectionPtr registeredFileDescriptorProxyRefHolder_;
};


BW_END_NAMESPACE

#endif /* ENABLE_WATCHERS */

#endif // WATCHER_CONNECTION_HPP
