#ifndef LOGGER_ENDPOINT_HPP
#define LOGGER_ENDPOINT_HPP

#include "cstdmf/config.hpp"
#if ENABLE_WATCHERS

#include "cstdmf/smartpointer.hpp"

#include "network/endpoint.hpp"
#include "network/interfaces.hpp"

#include <memory>
#include <queue>

BW_BEGIN_NAMESPACE

namespace Mercury
{
class EventDispatcher;
class NetworkInterface;
}

class BufferedLoggerPacket;
class LoggerMessageForwarder;

// These classes are defined in the .cpp, but are given friend access here.
class LoggerEndpointMemberHandler;
class LoggerEndpointNeedWriteTimeout;


class LoggerEndpoint
{
	friend class LoggerEndpointMemberHandler;
	friend class LoggerEndpointNeedWriteTimeout;
public:
	LoggerEndpoint( Mercury::EventDispatcher & dispatcher,
		LoggerMessageForwarder * logForwarder,
		const Mercury::Address & remoteAddress );

	bool init( uint16 majorVersion, uint16 minorVersion );

	~LoggerEndpoint();

	bool send( MemoryOStream & buffer );

	bool handlePacket( char * packet, int packetLength );

	const Mercury::Address & addr() const
	{
		return remoteAddress_;
	}

	bool shouldSendMetaData() const;
	bool shouldStreamMetaDataArgs() const;

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif

private:
	void appendPacket( BufferedLoggerPacket * pPacket );
	bool enableUDP();
	void sendDelete();
	void sendImmediate( MemoryOStream & ostream );
	bool sendBuffered();
	void disconnectTCP();
	void disconnectUDP();
	bool reconnect();
	const char * getLoggerState() const;

	enum LoggerState
	{
		LOGGER_ENDPOINT_UNINITALIZED,
		LOGGER_ENDPOINT_CONNECTING,
		LOGGER_ENDPOINT_WAITING,
		LOGGER_ENDPOINT_NEED_WRITE,
		LOGGER_ENDPOINT_REGISTERED_WRITE,
		LOGGER_ENDPOINT_ERROR
	};

	LoggerState loggerState_;

	Mercury::EventDispatcher & dispatcher_;
	LoggerMessageForwarder * logForwarder_;
	int consecutiveReconnects_;

	TimerHandle registerWriteTimer_;
	LoggerEndpointNeedWriteTimeout * pWriteTimeoutHandler_;

	Endpoint tcpEndpoint_;
	LoggerEndpointMemberHandler * pTcpConnectHandler_;
	LoggerEndpointMemberHandler * pTcpReadHandler_;
	LoggerEndpointMemberHandler * pTcpWriteHandler_;
	int handleConnectTCP( int fd );
	int handleReadTCP( int fd );
	int handleWriteTCP( int fd );

	Endpoint udpEndpoint_;
	LoggerEndpointMemberHandler * pUdpReadHandler_;
	int handleReadUDP( int fd );

	Mercury::Address remoteAddress_;

	uint32 bufferCount_;
	BufferedLoggerPacket * lastPacket_;
	BufferedLoggerPacket * nextPacket_;

	bool isTCP_;

	uint32 receivedSize_;
	uint32 messageSize_;
	char * receivedBuffer_;

	volatile int droppedMessageCount_;

	/*
		This mutex should be used to lock:
			* last/nextPacket_
			* loggerState_
			* tcpEndpoint_
	*/
	RecursiveMutex mutex_;

	uint16 majorVersion_;
	uint16 minorVersion_;

	bool isSendingExceededBufferedMessage_;
};

bool operator==( LoggerEndpoint * lhs, const Mercury::Address & rhs );

BW_END_NAMESPACE

#endif /* ENABLE_WATCHERS */

#endif // LOGGER_ENDPOINT_HPP
