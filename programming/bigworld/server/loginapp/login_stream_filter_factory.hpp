#ifndef LOGIN_STREAM_FILTER_FACTORY_HPP
#define LOGIN_STREAM_FILTER_FACTORY_HPP

#include "network/stream_filter_factory.hpp"
#include "network/tcp_channel_stream_adaptor.hpp"
#include "network/websocket_handshake_handler.hpp"
#include "network/websocket_stream_filter.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class is the filter factory for TCP channels created on the LoginApp,
 *	when WebSockets is enabled.
 */
class LoginStreamFilterFactory : public Mercury::StreamFilterFactory,
		public Mercury::WebSocketHandshakeHandler
{
public:

	/**
	 *	Constructor.
	 */
	LoginStreamFilterFactory() :
			Mercury::StreamFilterFactory(),
			Mercury::WebSocketHandshakeHandler()
	{}

	/**
	 *	Destructor.
	 */
	virtual ~LoginStreamFilterFactory()
	{}


	// Override from Mercury::StreamFilterFactory
	virtual Mercury::StreamFilterPtr createFor( Mercury::TCPChannel & channel )
	{
		Mercury::NetworkStreamPtr pChannelStream(
			new Mercury::TCPChannelStreamAdaptor( channel ) );

		return new Mercury::WebSocketStreamFilter( *pChannelStream, *this );
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


#endif // LOGIN_STREAM_FILTER_FACTORY_HPP
