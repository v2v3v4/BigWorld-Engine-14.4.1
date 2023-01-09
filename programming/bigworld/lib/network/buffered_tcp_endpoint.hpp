#ifndef BUFFERED_TCP_ENDPOINT_HPP
#define BUFFERED_TCP_ENDPOINT_HPP

#include "cstdmf/config.hpp"

#include "network/endpoint.hpp"
#include "network/event_dispatcher.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{
class EventDispatcher;
}

class BufferedTcpEndpoint: public Mercury::InputNotificationHandler
{
public:
	BufferedTcpEndpoint( Mercury::EventDispatcher & dispatcher,
				Endpoint & endpoint );

	virtual ~BufferedTcpEndpoint();

	int send( void * data, int32 size ) const;

	virtual int handleInputNotification( int fd );

public:
	const Endpoint & getEndpoint() const { return endpoint_; }

private:
	void appendToBufferedStream( const char * data, int size ) const;
	int sendContent( const char *content, int size ) const;
	int flushBufferedStream() const;

	Mercury::EventDispatcher & dispatcher_;
	
	mutable MemoryOStream bufferedStream_;

	const Endpoint & endpoint_;
};


BW_END_NAMESPACE


#endif // BUFFERED_TCP_ENDPOINT_HPP
