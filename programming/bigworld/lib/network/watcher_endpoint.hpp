#ifndef WATCHER_ENDPOINT_HPP
#define WATCHER_ENDPOINT_HPP

#include "endpoint.hpp"
#include "watcher_connection.hpp"

BW_BEGIN_NAMESPACE


/**
 *	This class represents either a TCP or UDP Endpoint.
 */
class WatcherEndpoint
{
public:
	WatcherEndpoint( WatcherConnection & watcherConnection ) :
		destAddr_( watcherConnection.getRemoteAddress() ),
		udpEndpoint_( NULL ),
		tcpWatcherConnection_( &watcherConnection )
	{
	}


	WatcherEndpoint( Endpoint & endpoint, sockaddr_in destAddr ) :
		destAddr_( destAddr.sin_addr.s_addr, destAddr.sin_port ),
		udpEndpoint_( &endpoint ),
		tcpWatcherConnection_( NULL )
	{
	}

	int send( void * data, int32 size ) const
	{
		if (udpEndpoint_ == NULL)
		{
			return tcpWatcherConnection_->send( data, size );
		}
		else
		{
			return udpEndpoint_->sendto( data, size, destAddr_ );
		}
	}

	const Mercury::Address & remoteAddr() const
	{
		return destAddr_;
	}

	bool isTCP() const	{ return udpEndpoint_ == NULL; }

private:
	Mercury::Address destAddr_;

	const Endpoint * udpEndpoint_;
	WatcherConnectionPtr tcpWatcherConnection_;


	WatcherEndpoint & operator=( const WatcherEndpoint & other );
};

BW_END_NAMESPACE

#endif // WATCHER_ENDPOINT_HPP
