#ifndef CLIENT_STREAM_FILTER_FACTORY_HPP
#define CLIENT_STREAM_FILTER_FACTORY_HPP

#include "network/stream_filter_factory.hpp"
#include "network/tcp_channel_stream_adaptor.hpp"
#include "network/websocket_handshake_handler.hpp"
#include "network/websocket_stream_filter.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class represents factories for stream filters to be attached to
 *	incoming TCP connections.
 */
class ClientStreamFilterFactory : public Mercury::StreamFilterFactory,
		public Mercury::WebSocketHandshakeHandler
{
public:
	/**
	 *	Constructor.
	 */
	ClientStreamFilterFactory() :
			Mercury::StreamFilterFactory(),
			Mercury::WebSocketHandshakeHandler()
	{}

	/**
	 *	Destructor.
	 */
	virtual ~ClientStreamFilterFactory()
	{}


	// Overrides from Mercury::StreamFilterFactory

	virtual Mercury::StreamFilterPtr createFor( Mercury::TCPChannel & channel )
	{
		return new Mercury::WebSocketStreamFilter( 
			*(new Mercury::TCPChannelStreamAdaptor( channel ) ),
			*this );
	}


	// Override from Mercury::WebSocketHandshakeHandler
	virtual bool shouldAcceptHandshake( 
			const Mercury::WebSocketStreamFilter & filter,
			const Mercury::HTTPRequest & request,
			Subprotocols & subprotocols )
	{
		Mercury::WebSocketHandshakeHandler::adjustForEmscriptenSubprotocols(
			subprotocols );

		return true;
	}

};


BW_END_NAMESPACE


#endif // CLIENT_STREAM_FILTER_FACTORY_HPP
