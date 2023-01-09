#ifndef WEBSOCKET_HANDSHAKE_HANDLER_HPP
#define WEBSOCKET_HANDSHAKE_HANDLER_HPP


#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{

class WebSocketStreamFilter;
class HTTPRequest;

/**
 *	This interface class is used to validate WebSocket requests from the client.
 */
class WebSocketHandshakeHandler
{
public:
	typedef BW::set< BW::string > Subprotocols;

	/**
	 *	Destructor.
	 */
	virtual ~WebSocketHandshakeHandler() {}

	/**
	 *	This method is used to validate a WebSockets request.
	 *
	 *	Basic checks as per RFC 6455 have already been carried out, overriding
	 *	methods should validate the URI, Origin header value and/or the HTTP
	 *	method.
	 *
	 *	If a request fails validation, the overriding method should use the
	 *	WebSocketStreamFilter::rejectHandshake() method to send back an HTTP
	 *	response with an appropriate HTTP status code and reason phrase
	 *	describing the error before returning false.
	 *
	 *	If the handshake is accepted, returning true will allow for further
	 *	WebSockets traffic to be accepted and processed.
	 *
	 *	@param filter	The WebSockets stream filter receiving the handshake.
	 *	@param request	The WebSockets upgrade request.
	 *	@param subprotocols The subprotocols that will be supported. Initially
	 *					set to what was reported by the client (possibly
	 *					empty), the WebSocketHandshakeHandler should choose
	 *					which of these subprotocols are supported and remove
	 *					those that are unsupported. It should never add its own
	 *					subprotocols.
	 *
	 *	@return 		True if handshake is accepted, false if rejected.
	 */
	virtual bool shouldAcceptHandshake( const WebSocketStreamFilter & filter,
			const HTTPRequest & request,
			Subprotocols & subprotocols ) = 0;

protected:
	/**
	 *	This method is a helper for adjusting the subprotocols output parameter
	 *	in shouldAcceptHandshake() for Emscripten's expectations.
	 *
	 *	@param subprotocols		The subprotocols in the handshake.
	 */
	static void adjustForEmscriptenSubprotocols( Subprotocols & subprotocols )
	{
		// For connections from emscripten generated code, it will send both
		// "base64" and "binary" as subprotocols. Choose only "binary".

		if (subprotocols.count( "binary" ))
		{
			subprotocols.clear();
			subprotocols.insert( "binary" );
		}
	}

	/**
	 *	Constructor.
	 */
	WebSocketHandshakeHandler() {}
};


} // end namespace Mercury

BW_END_NAMESPACE 

#endif // WEBSOCKET_HANDSHAKE_HANDLER_HPP

