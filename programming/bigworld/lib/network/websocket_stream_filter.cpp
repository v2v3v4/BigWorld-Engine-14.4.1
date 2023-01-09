#include "pch.hpp"

#include "http_messages.hpp"
#include "websocket_stream_filter.hpp"
#include "websocket_handshake_handler.hpp"

#include "cstdmf/base64.h"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/timestamp.hpp"

namespace BWOpenSSL
{
#include "openssl/rand.h"
#include "openssl/sha.h"
} // end namespace BWOpenSSL

#if defined( __unix__ ) || defined( EMSCRIPTEN )
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <sstream>

BW_BEGIN_NAMESPACE

// Not needed now that Emscripten supports receiving binary frames
// #define WEBSOCKETS_SHOULD_SEND_AS_TEXT_BASE64

namespace Mercury
{


/// The WebSockets GUID.
const BW::string WebSocketStreamFilter::WEBSOCKETS_GUID =
	"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";


namespace
{

// Various constants for reading a WebSockets frame

const uint8 FIN						= 0x80;
const uint8 MASK					= 0x80;
const uint8 OPCODE_BITMASK			= 0x0F;

const uint8 SMALL_LENGTH_BITMASK 	= 0x7F;
const uint8 MAX_SMALL_LENGTH		= 125;
const uint8 MEDIUM_LENGTH			= 126;
const uint8 LARGE_LENGTH			= 127;


typedef uint32 MaskingKey;
const size_t MASKING_KEY_LENGTH = sizeof(MaskingKey);

/**
 *	WebSockets frame opcode values.
 */
enum WebSocketsOpcode
{
	OPCODE_CONTINUATION				= 0x00,
	OPCODE_TEXT						= 0x01,
	OPCODE_BINARY					= 0x02,
	OPCODE_CLOSE					= 0x08,
	OPCODE_PING						= 0x09,
	OPCODE_PONG						= 0x0A
};

/**
 *	WebSockets version that we understand.
 */
const BW::string WEBSOCKETS_VERSION = "13";

/**
 *	Minimum HTTP version required for connecting WebSockets.
 */
const uint MIN_HTTP_VERSION[] = {1, 1};


/**
 *	WebSockets close status codes.
 */
enum CloseStatusCode
{
	CLOSE_STATUS_NORMAL						= 1000,
	CLOSE_STATUS_GOING_AWAY					= 1001,
	CLOSE_STATUS_PROTOCOL_ERROR				= 1002,
	CLOSE_STATUS_UNSUPPORTED_DATA			= 1003,
	CLOSE_STATUS_RESERVED					= 1004,
	CLOSE_STATUS_NO_STATUS_RECEIVED			= 1005,
	CLOSE_STATUS_ABNORMAL_CLOSURE			= 1006,
	CLOSE_STATUS_INVALID_FRAME_PAYLOAD_DATA = 1007,
	CLOSE_STATUS_POLICY_VIOLATION 			= 1008,
	CLOSE_STATUS_MESSAGE_TOO_BIG 			= 1009,
	CLOSE_STATUS_MANDATORY_EXTENSION 		= 1010,
	CLOSE_STATUS_INTERNAL_SERVER_ERROR 		= 1011,
	CLOSE_STATUS_TLS_HANDSHAKE 				= 1015
};


/**
 *	This function gives a human read-able version for a given close status
 *	code.
 *
 *	@param statusCode 	The close status code.
 */
const char * closeStatusCodeToCString( uint16 statusCode )
{
	switch (statusCode)
	{
	case CLOSE_STATUS_NORMAL: 					return "Normal closure";
	case CLOSE_STATUS_GOING_AWAY: 				return "Going away";
	case CLOSE_STATUS_PROTOCOL_ERROR: 			return "Protocol error";
	case CLOSE_STATUS_UNSUPPORTED_DATA:			return "Unsupported data";
	case CLOSE_STATUS_RESERVED:					return "Reserved";
	case CLOSE_STATUS_NO_STATUS_RECEIVED:		return "No status received";
	case CLOSE_STATUS_ABNORMAL_CLOSURE:			return "Abnormal closure";
	case CLOSE_STATUS_INVALID_FRAME_PAYLOAD_DATA:
												return "Invalid frame payload "
													"data";
	case CLOSE_STATUS_POLICY_VIOLATION:			return "Policy violation";
	case CLOSE_STATUS_MESSAGE_TOO_BIG:			return "Message too big";
	case CLOSE_STATUS_MANDATORY_EXTENSION:		return "Mandatory extension";
	case CLOSE_STATUS_INTERNAL_SERVER_ERROR:	return "Internal server error";
	case CLOSE_STATUS_TLS_HANDSHAKE:			return "TLS handshake failed";
	default: 									return "Unknown";
	}
}


/**
 *	This function gives a human read-able version for a given WebSockets frame
 *	opcode.
 *
 *	@param opcode 	The opcode.
 */
const char * opcodeToCString( uint8 opcode )
{
	switch (opcode)
	{
	case OPCODE_CONTINUATION: 	return "OPCODE_CONTINUATION";
	case OPCODE_TEXT: 			return "OPCODE_TEXT";
	case OPCODE_BINARY: 		return "OPCODE_BINARY";
	case OPCODE_CLOSE: 			return "OPCODE_CLOSE";
	case OPCODE_PING: 			return "OPCODE_PING";
	case OPCODE_PONG: 			return "OPCODE_PONG";
	default: 					return "<Unknown OPCODE>";
	}
}


/**
 *	This method parses out the individual sub-protocol strings (separated by
 *	commas) from the header line.
 *
 *	@param subprotocolHeader	The header line to parse.
 *	@param subprotocols			The subprotocol string set.
 */
void parseSubprotocols( const BW::string & subprotocolHeader,
		WebSocketHandshakeHandler::Subprotocols & subprotocols )
{
	bw_tokenise( subprotocolHeader, BW::string( "," ), subprotocols );
}


/**
 *	This function outputs a set of subprotocols concatenated by commas into a
 *	single string.
 *
 *	@param subprotocols 	The set of subprotocols to output.
 *
 *	@return 				A concatenated comma-separated string of the given
 *							subprotocols.
 */
BW::string protocolSetToString(
		const WebSocketHandshakeHandler::Subprotocols & subprotocols )
{
	BW::string out;
	bw_stringify( subprotocols, BW::string( "," ), out );
	return out;
}


} // end namespace (anonymous)


/**
 *	Constructor for client-side filters.
 *
 *	@param stream 	The underlying stream.
 *	@param host 	The hostname to request.
 *	@param uri 		The URI to request.
 *	@param origin 	The Origin of the request, or empty string if none is to be
 *					reported to the server.
 */
WebSocketStreamFilter::WebSocketStreamFilter( NetworkStream & stream,
			const BW::string & host, const BW::string & uri,
			const BW::string & origin ) :
		StreamFilter( stream ),
		hasHandshakeCompleted_( false ),
		expectedNonceAccept_(),
		pHandshakeHandler_( NULL ),
		isClient_( true ),
		receiveBuffer_(),
		expectedLengthFieldSize_( 0 ),
		expectedReceiveFrameLength_( 0 ),
		receivedPayloadOpcode_( 0 ),
		receivedPayloadBuffer_(),
		sendBuffer_(),
		closeState_( NOT_CLOSING )
{
	this->sendHandshakeRequestToServer( host, uri, origin );
}


/**
 *	Constructor for server-side filters.
 *
 *	@param stream 	The underlying stream.
 *	@param handler 	The handshake handler.
 */
WebSocketStreamFilter::WebSocketStreamFilter( NetworkStream & stream,
			WebSocketHandshakeHandler & handler ) :
		StreamFilter( stream ),
		hasHandshakeCompleted_( false ),
		expectedNonceAccept_(),
		pHandshakeHandler_( &handler ),
		isClient_( false ),
		receiveBuffer_(),
		expectedLengthFieldSize_( 0 ),
		expectedReceiveFrameLength_( 0 ),
		receivedPayloadOpcode_( 0 ),
		receivedPayloadBuffer_(),
		sendBuffer_(),
		closeState_( NOT_CLOSING )
{
}


/**
 *	Destructor.
 */
WebSocketStreamFilter::~WebSocketStreamFilter()
{}


/*
 *	Override from StreamFilter.
 */
bool WebSocketStreamFilter::writeFrom( BinaryIStream & input, bool shouldCork )
{
	if (closeState_ != NOT_CLOSING)
	{
		return false;
	}

	sendBuffer_.transfer( input, input.remainingLength() );

	if (shouldCork || !hasHandshakeCompleted_)
	{
		return true;
	}

	return this->sendBufferedData();
}


/**
 *	This method sends any buffered send data.
 *
 *	@return 	True if the send succeeded, false otherwise.
 */
bool WebSocketStreamFilter::sendBufferedData()
{
	int sendBufferSize = sendBuffer_.remainingLength();

	if (!hasHandshakeCompleted_ || sendBufferSize == 0)
	{
		return true;
	}

	bool success = this->sendFrame( OPCODE_BINARY, sendBuffer_ );

	sendBuffer_.reset();
	return success;
}


/**
 *	This method masks the data in the given input data stream with the given
 *	masking key into the given output stream.
 *
 *	@param maskingKey 	The masking key.
 *	@param input 		The input data stream.
 *	@param output 		The output data stream.
 */
void WebSocketStreamFilter::mask( uint32 maskingKey, BinaryIStream & input,
		BinaryOStream & output )
{
	MaskingKey maskingKeyNBO = htonl( maskingKey );

	const char * maskingKeyBytes =
		reinterpret_cast< const char * >( &maskingKeyNBO );

	// Do data masking

	for (uint i = 0; input.remainingLength() > 0; ++i)
	{
		const char * byte = reinterpret_cast< const char * >(
			input.retrieve( 1 ) );
		output << uint8( maskingKeyBytes[i % 4] ^ *byte );
	}
}


/*
 *	Override from StreamFilter.
 */
int WebSocketStreamFilter::readInto( BinaryOStream & output )
{
	StreamFilterPtr pThis( this );

	int numBytesReadFromStream = this->stream().readInto( receiveBuffer_ );

	if (numBytesReadFromStream == 0)
	{
		return 0;
	}

	if ((!hasHandshakeCompleted_ && !this->receiveHandshake()))
	{
		NOTICE_MSG( "WebSocketStreamFilter::readInto( %s ): Failed handshake\n",
			this->streamToString().c_str() );

		this->failConnection();
		return -1;
	}

	if (!hasHandshakeCompleted_)
	{
		return 0;
	}

	return this->receiveFrame( output );
}


/*
 *	Override from StreamFilter.
 */
BW::string WebSocketStreamFilter::streamToString() const
{
	return (isClient_ ? "ws-server+" : "ws-client+") + 
		this->stream().streamToString();
}


/*
 *	Override from StreamFilter.
 */
bool WebSocketStreamFilter::didHandleChannelInactivityTimeout()
{
	// Peer has not responded for a while, shutdown gracefully.
	if (!this->sendCloseFrame( CLOSE_STATUS_PROTOCOL_ERROR, 
		"Inactivity timeout" ))
	{
		closeState_ = CLOSED;
		this->stream().shutDown();
		return true;
	}

	closeState_ = CLOSE_SENT;
	return true;
}


/*
 *	Override from StreamFilter.
 */
bool WebSocketStreamFilter::didFinishShuttingDown()
{
	if (closeState_ == CLOSED)
	{
		// We've already done all we have needed to do to shutdown, let the
		// lower level stream do its own handling of shutting down now.
		return this->stream().didFinishShuttingDown();
	}

	if (closeState_ != NOT_CLOSING)
	{
		// The shutdown process has already started.
		return false;
	}

	// We need to perform the closing handshake and so won't be shutting down
	// our underlying stream just yet.

	if (hasHandshakeCompleted_)
	{
		this->sendCloseFrame();
	}
	else
	{
		closeState_ = CLOSE_ON_CONNECT;
	}

	return false;
}


/*
 *	Override from StreamFilter.
 */
bool WebSocketStreamFilter::hasUnsentData() const
{
	return (sendBuffer_.remainingLength() > 0) || 
		this->stream().hasUnsentData();
}


/*
 *	Override from StreamFilter.
 */
bool WebSocketStreamFilter::isConnected() const
{
	return (closeState_ == NOT_CLOSING) && this->stream().isConnected();
}


/**
 *	This method is used to receive the handshake request/response from the
 *	peer.
 *
 *	@return 		False if error occurred, true if the request/response is
 *					incomplete or was processed and accepted successfully.
 */
bool WebSocketStreamFilter::receiveHandshake()
{
	// Wait until we have a two consecutive CRLFs terminating the receive
	// buffer.

	const char * data = this->receivedFrameData();

	// See if we have at least enough for 2 x CRLFs that terminate a HTTP
	// header.
	static const int HEADER_TERMINATOR_SIZE = 4;
	if (receiveBuffer_.remainingLength() <= HEADER_TERMINATOR_SIZE)
	{
		return true;
	}

	const char * end = 	data + receiveBuffer_.remainingLength() -
		HEADER_TERMINATOR_SIZE;

	// Guard against large garbage handshake headers on the server.
	static const int MAX_HANDSHAKE_MESSAGE_LENGTH = 4096;
	if (!isClient_ &&
			(receiveBuffer_.remainingLength() > MAX_HANDSHAKE_MESSAGE_LENGTH))
	{
		NOTICE_MSG( "WebSocketStreamFilter::receiveHandshake: "
				"Length of handshake header from %s exceeds limit of %d bytes "
				"(so far received %d bytes)\n",
			this->streamToString().c_str(),
			MAX_HANDSHAKE_MESSAGE_LENGTH,
			receiveBuffer_.remainingLength() );
		return false;
	}

	if ((end[0] == '\r') && (end[1] == '\n') &&
			(end[2] == '\r') && (end[3] == '\n'))
	{
		if (isClient_)
		{
			HTTPResponse response;
			bool parseResult = response.parse( &data,
				receiveBuffer_.remainingLength() );

			receiveBuffer_.reset();

			if (!parseResult)
			{
				ERROR_MSG( "WebSocketStreamFilter::receiveHandshake: "
						"Could not parse handshake response from %s\n",
					this->streamToString().c_str() );
				return false;
			}

			return this->processHandshakeFromServer( response );
		}
		else
		{
			HTTPRequest request;

			bool parseResult = request.parse( &data,
				receiveBuffer_.remainingLength() );

			receiveBuffer_.reset();

			if (!parseResult)
			{
				NOTICE_MSG( "WebSocketStreamFilter::receiveHandshake: "
						"Could not parse handshake request from %s\n",
					this->streamToString().c_str() );
				return false;
			}

			return this->processHandshakeFromClient( request );
		}
	}

	return true;
}


/**
 *	This method checks basic headers for the handshake from the client.
 *
 *	@param request	The handshake request.
 *
 *	@return 		True if the basic checks have passed, otherwise false.
 */
bool WebSocketStreamFilter::doesClientHandshakePassBasicChecks(
		const HTTPRequest & request ) const
{
	if (request.method() != "GET" )
	{
		NOTICE_MSG( "WebSocketStreamFilter::doesClientHandshakePassBasicChecks"
				"( %s ): Invalid request method: \"%s\"\n",
			this->streamToString().c_str(),
			request.method().c_str() );
		return false;
	}

	uint httpMajorVersion;
	uint httpMinorVersion;

	request.getHTTPVersion( httpMajorVersion, httpMinorVersion );

	if ((httpMajorVersion < MIN_HTTP_VERSION[0]) ||
			((httpMajorVersion == MIN_HTTP_VERSION[0]) &&
				(httpMinorVersion < MIN_HTTP_VERSION[1])))
	{
		NOTICE_MSG( "WebSocketStreamFilter::doesClientHandshakePassBasicChecks"
				"( %s ): Bad HTTP version: %u.%u\n",
			this->streamToString().c_str(),
			httpMajorVersion, httpMinorVersion );
		return false;
	}

	if (!request.headers().contains( "host" ))
	{
		NOTICE_MSG( "WebSocketStreamFilter::doesClientHandshakePassBasicChecks"
				"( %s ): Missing Host header\n",
			this->streamToString().c_str() );
		return false;
	}

	if (!request.headers().contains( "upgrade" ) ||
			!request.headers().valueCaseInsensitivelyContains(
				"upgrade", "websocket"))
	{
		NOTICE_MSG( "WebSocketStreamFilter::doesClientHandshakePassBasicChecks"
				"( %s ): Missing or invalid Upgrade header \"%s\"\n",
			this->streamToString().c_str(),
			request.headers().valueFor( "upgrade" ).c_str() );
		return false;
	}

	if (!request.headers().contains( "connection" ) ||
			!request.headers().valueCaseInsensitivelyContains(
				"connection", "Upgrade"))
	{
		NOTICE_MSG( "WebSocketStreamFilter::doesClientHandshakePassBasicChecks"
				"( %s ): Missing or invalid Connection header \"%s\"\n",
			this->streamToString().c_str(),
			request.headers().valueFor( "connection" ).c_str() );
		return false;
	}

	if (!request.headers().contains( "sec-websocket-key" ))
	{
		NOTICE_MSG( "WebSocketStreamFilter::doesClientHandshakePassBasicChecks"
				"( %s ): Missing Sec-WebSocket-Key header\n",
			this->streamToString().c_str() );
		return false;
	}

	if (!request.headers().contains( "sec-websocket-version" ) ||
			(request.headers().valueFor( "sec-websocket-version" ) !=
				WEBSOCKETS_VERSION))
	{
		NOTICE_MSG( "WebSocketStreamFilter::doesClientHandshakePassBasicChecks"
				"( %s ): Header Sec-WebSocket-Version value invalid: \"%s\"\n",
			this->streamToString().c_str(),
			request.headers().valueFor( "sec-websocket-version" ).c_str() );

		return false;
	}

	return true;
}


/**
 *	This method processes the response to the digest challenge from the client.
 *
 *	@param request			The handshake request.
 *	@param acceptKeyBase64	If successful, this is filled with the
 *							corresponding response.
 *
 *	@return					true if successful, otherwise false.
 */
bool WebSocketStreamFilter::processDigestChallenge( const HTTPRequest & request,
		BW::string & acceptBase64 ) const
{
	const BW::string & webSocketKeyBase64 =
		request.headers().valueFor( "sec-websocket-key" );

	BW::string webSocketKey;
	if (!Base64::decode( webSocketKeyBase64, webSocketKey ))
	{
		NOTICE_MSG( "WebSocketStreamFilter::processDigestChallenge( %s ): "
				"Header Sec-WebSocket-Key not valid Base-64\n",
			this->streamToString().c_str() );
		return false;
	}

	size_t webSocketKeySize = webSocketKey.size();
	if (webSocketKeySize != 16)
	{
		NOTICE_MSG( "WebSocketStreamFilter::processDigestChallenge( %s ): "
				"Header Sec-WebSocket-Key has invalid size: %" PRIzu " bytes\n",
			this->streamToString().c_str(), webSocketKeySize );
		return false;
	}

	BW::string keyResponse = webSocketKeyBase64 + WEBSOCKETS_GUID;

	unsigned char sha1Buffer[SHA_DIGEST_LENGTH];
	BWOpenSSL::SHA1(
		reinterpret_cast< const unsigned char * >( keyResponse.data() ),
		keyResponse.size(),
		sha1Buffer );

	acceptBase64 = Base64::encode(
		reinterpret_cast< const char * >( sha1Buffer ),
		SHA_DIGEST_LENGTH );

	return true;
}


/**
 *	This method validates and processes the handshake received from a client.
 *
 *	@param request 	The parsed request from the client.
 *
 *	@return 		True if handshake was accepted, false otherwise.
 */
bool WebSocketStreamFilter::processHandshakeFromClient(
		const HTTPRequest & request )
{
	MF_ASSERT( !isClient_ );

	if (!this->doesClientHandshakePassBasicChecks( request ))
	{
		this->rejectHandshake();
		return false;
	}

	BW::string acceptBase64;
	if (!this->processDigestChallenge( request, acceptBase64 ))
	{
		this->rejectHandshake();
		return false;
	}

	WebSocketHandshakeHandler::Subprotocols subprotocols;
	BW::string subprotocolsHeader =
		request.headers().valueFor( "sec-websocket-protocol" );
	parseSubprotocols( subprotocolsHeader, subprotocols );
	WebSocketHandshakeHandler::Subprotocols newSubprotocols( subprotocols );

	if (pHandshakeHandler_ &&
			!pHandshakeHandler_->shouldAcceptHandshake( *this, request,
					newSubprotocols ))
	{
		return false;
	}

	WebSocketHandshakeHandler::Subprotocols discardedProtocols;
	std::set_difference( subprotocols.begin(), subprotocols.end(),
		newSubprotocols.begin(), newSubprotocols.end(),
		std::inserter( discardedProtocols, discardedProtocols.begin() ) );

	const BW::string & host = request.headers().valueFor( "host" );
	const BW::string & origin = request.headers().valueFor( "origin" );

	TRACE_MSG( "WebSocketStreamFilter::processHandshakeFromClient( %s ): "
			"Handshake received successfully ws://%s/%s "
			"[origin: %s, subprotocols: %s (discarded: %s)]\n",
		this->streamToString().c_str(),
		host.c_str(),
		request.uri().c_str(),
		origin.size() ? origin.c_str() : "none",
		protocolSetToString( newSubprotocols ).c_str(),
		(discardedProtocols.empty() ? "(none)" :
			protocolSetToString( discardedProtocols ).c_str()) );

	BW::ostringstream headers;

	if (request.headers().contains( "sec-websocket-protocol" ))
	{
		// Send a response back, based on what the handshake handler has
		// specified.
		headers << "Sec-WebSocket-Protocol: " <<
			protocolSetToString( subprotocols ) << "\r\n";
	}

	// Add response to the digest challenge.

	headers << "Sec-WebSocket-Accept: " + acceptBase64 + "\r\n";

	this->sendHandshakeResponseToClient( headers.str() );

	hasHandshakeCompleted_ = true;

	this->sendBufferedData();

	if (closeState_ == CLOSE_ON_CONNECT)
	{
		this->sendCloseFrame();
	}

	return true;
}


/**
 *	This method processes a handshake response from the server.
 *
 *	@param response 	The parsed response from the server.
 *
 *	@return 			True if handshake was accepted, false otherwise.
 */
bool WebSocketStreamFilter::processHandshakeFromServer(
		const HTTPResponse & response )
{
	uint httpMajorVersion;
	uint httpMinorVersion;

	response.getHTTPVersion( httpMajorVersion, httpMinorVersion );

	if ((httpMajorVersion < MIN_HTTP_VERSION[0]) ||
			((httpMajorVersion == MIN_HTTP_VERSION[0]) &&
				(httpMinorVersion < MIN_HTTP_VERSION[1])))
	{
		ERROR_MSG( "WebSocketStreamFilter::processHandshakeFromServer( %s ): "
				"Bad HTTP version: %u.%u\n",
			this->streamToString().c_str(),
			httpMajorVersion, httpMinorVersion );
		return false;
	}

	if (response.statusCode() != "101")
	{
		ERROR_MSG( "WebSocketStreamFilter::processHandshakeFromServer( %s ): "
				"Invalid status code: \"%s\"\n",
			this->streamToString().c_str(),
			response.statusCode().c_str() );
		return false;
	}

	if (!response.headers().valueCaseInsensitivelyEquals(
			"upgrade", "websocket" ))
	{
		ERROR_MSG( "WebSocketStreamFilter::processHandshakeFromServer( %s ): "
				"Invalid Upgrade header: \"%s\"\n",
			this->streamToString().c_str(),
			response.headers().valueFor( "upgrade" ).c_str() );
		return false;
	}

	if (!response.headers().valueCaseInsensitivelyEquals(
			"connection", "upgrade" ))
	{
		ERROR_MSG( "WebSocketStreamFilter::processHandshakeFromServer( %s ): "
				"Invalid Connection header: \"%s\"\n",
			this->streamToString().c_str(),
			response.headers().valueFor( "connection" ).c_str() );
		return false;
	}

	if (response.headers().valueFor( "sec-websocket-accept" ) !=
			expectedNonceAccept_)
	{
		ERROR_MSG( "WebSocketStreamFilter::processHandshakeFromServer( %s ): "
				"Invalid Sec-WebSocket-Accept header: \"%s\"\n",
			this->streamToString().c_str(),
			response.headers().valueFor( "sec-websocket-accept" ).c_str() );
		return false;
	}

	expectedNonceAccept_.clear();

	TRACE_MSG( "WebSocketStreamFilter::processHandshakeFromServer( %s ): "
			"Handshake received successfully\n",
		this->streamToString().c_str() );

	hasHandshakeCompleted_ = true;

	this->sendBufferedData();

	if (closeState_ == CLOSE_ON_CONNECT)
	{
		this->sendCloseFrame();
	}

	return true;
}


/**
 *	This method is used to receive a frame from the remote peer.
 *
 *	@param output 	The output stream to place data from a received frame.
 *
 *	@return 		The number of bytes added to the output stream, or -1 if
 *					there an error occured.
 */
int WebSocketStreamFilter::receiveFrame( BinaryOStream & output )
{
	bool gotFrameFragment = false;
	int numBytesRead = 0;

	do
	{
		gotFrameFragment = false;
		const char * frameData = this->receivedFrameData();
		int numBytesReceived = receiveBuffer_.remainingLength();

		if ((numBytesReceived >= 2) &&
				(expectedLengthFieldSize_ == 0))
		{
			uint8 lengthByte = uint8( frameData[1] & SMALL_LENGTH_BITMASK );

			if (lengthByte < MAX_SMALL_LENGTH)
			{
				expectedLengthFieldSize_ = 1;
				this->calculateFrameSize();
			}
			else if (lengthByte == MEDIUM_LENGTH)
			{
				expectedLengthFieldSize_ = 1 + sizeof(uint16);
			}
			else if (lengthByte == LARGE_LENGTH)
			{
				expectedLengthFieldSize_ = 1 + sizeof(uint64);
			}
		}

		if ((expectedReceiveFrameLength_ == 0) &&
				expectedLengthFieldSize_ &&
				(uint64( numBytesReceived ) >= expectedLengthFieldSize_))
		{
			this->calculateFrameSize();
		}

		if (expectedReceiveFrameLength_ &&
				(uint( numBytesReceived ) >= expectedReceiveFrameLength_))
		{
			gotFrameFragment = true;
			int numBytesReadFromFrame = this->processFrameFragment( output );
			if (numBytesReadFromFrame == -1)
			{
				return -1;	
			}
			numBytesRead += numBytesReadFromFrame;
		}
	}
	while (gotFrameFragment);

	return numBytesRead;
}


/**
 *	This method calculates the expected frame size, and stores it internally
 *	for use in receiving the frame.
 */
void WebSocketStreamFilter::calculateFrameSize()
{
	MF_ASSERT( expectedLengthFieldSize_ != 0);

	uint64 headerSize = 1ULL + expectedLengthFieldSize_;

	const char * frameData = this->receivedFrameData();

	if (frameData[1] & MASK)
	{
		headerSize += MASKING_KEY_LENGTH;
	}

	uint64 payloadSize = 0;
	switch (expectedLengthFieldSize_)
	{
	case 1:
		payloadSize = frameData[1] & SMALL_LENGTH_BITMASK;
		break;

	case (1 + sizeof(uint16)):
	{
		uint16 mediumLength = *reinterpret_cast< const uint16 * >(
			frameData + 2 );
		payloadSize = ntohs( mediumLength );
		break;
	}

	case (1 + sizeof(uint64)):
	{
		const uint32 * largeLength = reinterpret_cast< const uint32 * >(
			frameData + 2 );

#ifdef _BIG_ENDIAN
		payloadSize = (uint64( ntohl( largeLength[0] ) ) << 32) |
			uint64( ntohl( largeLength[1] ) );
#else
		payloadSize = (uint64( ntohl( largeLength[1] ) ) << 32) |
			uint64( ntohl( largeLength[0] ) );
#endif
		break;
	}

	default:
		CRITICAL_MSG( "Invalid expected length field size\n" );
		break;
	}

	expectedReceiveFrameLength_ = headerSize + payloadSize;
}


/**
 *	This method processes frames that are possibly continuation frame fragments
 *	of a larger payload.
 *
 *	@param output	The output stream to put payload data onto.
 *
 *	@return 		The number of bytes added to the output stream, or -1 if
 *					there an error occured.
 */
int WebSocketStreamFilter::processFrameFragment( BinaryOStream & output )
{
	const char * frameData = this->receivedFrameData();

	bool isFIN = ((frameData[0] & FIN) != 0);
	uint8 opcode = (frameData[0] & OPCODE_BITMASK);
	bool isMasked = ((frameData[1] & MASK) != 0);

	if (receivedPayloadOpcode_ == 0)
	{
		if (opcode == OPCODE_CONTINUATION)
		{
			NOTICE_MSG( "WebSocketStreamFilter::processFrameFragment: %s: "
					"Got continuation opcode on first fragment\n",
				this->streamToString().c_str() );

			this->failConnection( CLOSE_STATUS_PROTOCOL_ERROR, 
				"Got continuation opcode on first fragment" );
			return -1;
		}
		receivedPayloadOpcode_ = opcode;
	}

	if (!isFIN && (opcode != OPCODE_CONTINUATION))
	{
		NOTICE_MSG( "WebSocketStreamFilter::processFrameFragment: %s: "
				"Got non-zero opcode on continuation frame\n",
			this->streamToString().c_str() );

		this->failConnection( CLOSE_STATUS_PROTOCOL_ERROR, 
			"Non-zero opcode on continuation frame" );
		return -1;
	}

	MemoryOStream payload;
	int headerSize = 1 + expectedLengthFieldSize_ +
		(isMasked ? MASKING_KEY_LENGTH : 0);
	int payloadSize = static_cast<int>(expectedReceiveFrameLength_) - headerSize;

	if (isMasked)
	{
		receiveBuffer_.retrieve( headerSize - MASKING_KEY_LENGTH );
		MaskingKey maskingKey = ntohl( *reinterpret_cast< const MaskingKey * >(
			receiveBuffer_.retrieve( MASKING_KEY_LENGTH ) ) );

		MemoryIStream maskedPayload( receiveBuffer_.retrieve( payloadSize ),
			payloadSize );
		this->mask( maskingKey, maskedPayload, payload );
	}
	else
	{
		receiveBuffer_.retrieve( headerSize );

		payload.transfer( receiveBuffer_, payloadSize );
	}

	receivedPayloadBuffer_.transfer( payload, payload.remainingLength() );

	int numBytesRead = 0;
	if (isFIN)
	{
		numBytesRead = this->processFrame( output );
		receivedPayloadOpcode_ = 0;
		receivedPayloadBuffer_.reset();
	}

	MemoryOStream remainder;
	remainder.transfer( receiveBuffer_, receiveBuffer_.remainingLength() );
	receiveBuffer_.reset();
	receiveBuffer_.transfer( remainder, remainder.remainingLength() );

	expectedLengthFieldSize_ = 0;
	expectedReceiveFrameLength_ = 0;

	return numBytesRead;
}


/**
 *	This method processes the received frame, placing any output into the given
 *	output stream.
 *
 *	@param output 	The output stream to fill with processed frame data.
 *
 *	@return 		The size of the payload data added to the output stream, or
 *					-1 on error.
 */
int WebSocketStreamFilter::processFrame( BinaryOStream & output )
{
	if (receivedPayloadOpcode_ == OPCODE_CLOSE)
	{
		// Close opcode
		TRACE_MSG( "WebSocketStreamFilter::processFrame: %s: "
				"Got CLOSE frame\n",
			this->streamToString().c_str() );

		this->processCloseFrame();

		return 0;
	}
	else if (receivedPayloadOpcode_ == OPCODE_PING)
	{
		// Ping: send a pong frame in response.
		if (!this->sendFrame( OPCODE_PONG, receivedPayloadBuffer_ ))
		{
			return -1;
		}
		return 0;
	}
	else if (receivedPayloadOpcode_ == OPCODE_PONG)
	{
		// Pong; ignore.
		return 0;
	}
	else if ((receivedPayloadOpcode_ != OPCODE_TEXT) &&
			(receivedPayloadOpcode_ != OPCODE_BINARY))
	{
		// Reserved opcodes, ignore.
		return 0;
	}

	if (receivedPayloadOpcode_ == OPCODE_TEXT)
	{
		this->failConnection( CLOSE_STATUS_PROTOCOL_ERROR,
			"Text frames not accepted\n" );
		return -1;
	}

	// OPCODE_BINARY

	int numBytesRead = receivedPayloadBuffer_.remainingLength();
	MF_ASSERT( numBytesRead > 0 );
	output.transfer( receivedPayloadBuffer_, numBytesRead );

	return numBytesRead;
}


/**
 *	This method is called to process a received CLOSE frame.
 */
void WebSocketStreamFilter::processCloseFrame()
{
	if (closeState_ == CLOSE_SENT)
	{
		// We received a CLOSE frame response, we're good to go now.
		closeState_ = CLOSED;
		this->stream().shutDown();
	}
	else if (closeState_ == NOT_CLOSING)
	{
		uint16 statusCode = 0;

		if (receivedPayloadBuffer_.remainingLength() > 2)
		{
			// Unsolicited CLOSE frame from peer.
			uint16 statusCodeNBO = *reinterpret_cast< const uint16 * >( 
				receivedPayloadBuffer_.retrieve( sizeof(uint16) ) );
			statusCode = ntohs( statusCodeNBO );
		}

		int reasonPhraseLength = receivedPayloadBuffer_.remainingLength();
		BW::string reasonPhrase;

		if (reasonPhraseLength > 0)
		{
			reasonPhrase.assign(
				reinterpret_cast< const char * >(
					receivedPayloadBuffer_.retrieve( reasonPhraseLength ) ),
				reasonPhraseLength );
		}

		TRACE_MSG( "WebSocketStreamFilter::processCloseFrame: "
				"Got CLOSE: %04hu : \"%s\"\n",
			statusCode, reasonPhrase.c_str() );

		// Echo back the status code and reason.
		this->sendCloseFrame( statusCode, reasonPhrase );

		closeState_ = CLOSED;
		this->stream().shutDown();
	}
}


/**
 *	This method sends a close frame immediately.
 *
 *	@param statusCode 	The status code, as defined in RFC 6455 section 7.4.
 *
 *	@return 			True if the send succeeded, false otherwise.
 */
bool WebSocketStreamFilter::sendCloseFrame( uint16 statusCode )
{
	return this->sendCloseFrame( statusCode, 
		closeStatusCodeToCString( statusCode ) );
}


/**
 *	This method sends a close frame immediately.
 *
 *	@param statusCode 	The status code, as defined in RFC 6455 section 7.4.
 *	@param reasonPhrase The reason phrase, as a UTF-8 encoded string.
 *
 *	@return 			True if the send succeeded, false otherwise.
 */
bool WebSocketStreamFilter::sendCloseFrame( uint16 statusCode,
		const BW::string & reasonPhrase )
{
	// Make sure we send out any buffered unsent data first.
	this->sendBufferedData();

	MemoryOStream payloadStream;
	int statusCodeNBO = htonl( statusCode );
	payloadStream.addBlob( &statusCodeNBO, sizeof(statusCodeNBO) );

	payloadStream.addBlob( reasonPhrase.data(),
		static_cast<int>(reasonPhrase.size()) );

	DEBUG_MSG( "WebSocketStreamFilter::sendCloseFrame: %04hu : \"%s\"\n",
		statusCode, reasonPhrase.c_str() );

	if (!this->sendFrame( OPCODE_CLOSE, payloadStream ))
	{
		return false;
	}

	if (closeState_ != CLOSED)
	{
		closeState_ = CLOSE_SENT;
	}
	return true;
}


/**
 *	This method implements the "Fail the WebSocket Connection" as described in 
 *	RFC 6455, section 7.1.7, using the generic protocol error status code.
 */
void WebSocketStreamFilter::failConnection()
{
	this->failConnection( CLOSE_STATUS_PROTOCOL_ERROR, 
		closeStatusCodeToCString( CLOSE_STATUS_PROTOCOL_ERROR ) );
}


/**
 *	This method implements the "Fail the WebSocket Connection" as described in
 *	RFC 6455, section 7.1.7.
 *
 *	@param statusCode 	The status code, as defined in RFC 6455 section 7.4.
 *	@param reasonPhrase The reason phrase, as a UTF-8 encoded string.
 */
void WebSocketStreamFilter::failConnection( uint16 statusCode,
		const BW::string & reasonPhrase )
{
	if (hasHandshakeCompleted_)
	{
		this->sendCloseFrame( statusCode, reasonPhrase );
	}

	closeState_ = CLOSED;
	this->stream().shutDown();
}


/**
 *	This method sends a frame with the given opcode and data payload.
 *
 *	@param opcode	The frame opcode.
 *	@param data 	The payload data.
 *
 *	@return 		True if the send succeeded, false otherwise.
 */
bool WebSocketStreamFilter::sendFrame( uint8 opcode, BinaryIStream & data )
{
	MemoryOStream header;

	header << uint8( FIN | opcode );

	// Masking indicator | Small payload length
	uint8 payloadLengthByte = (isClient_ ? MASK : 0x00);

	int dataSize = data.remainingLength();
	if (dataSize < MAX_SMALL_LENGTH)
	{
		payloadLengthByte |= uint8( dataSize );
		header << payloadLengthByte;
	}
	else if (dataSize < (1 << 16))
	{
		payloadLengthByte |= uint8( MEDIUM_LENGTH );
		header << payloadLengthByte;

		uint16 mediumFrameSize = htons( uint16( dataSize ) );
		header.addBlob( &mediumFrameSize, sizeof(uint16) );
	}
	else
	{
		payloadLengthByte |= uint8( LARGE_LENGTH );
		header << payloadLengthByte;

		uint32 largeFrameSize[2] = { 0, htonl( uint32( dataSize ) ) };
		header.addBlob( largeFrameSize, sizeof(uint64) );
	}

	if (!this->stream().writeFrom( header, WRITE_SHOULD_CORK ))
	{
		return false;
	}

	if (isClient_)
	{
		MaskingKey maskingKey = 0;

		BWOpenSSL::RAND_bytes( reinterpret_cast< unsigned char * >(&maskingKey),
			sizeof(MaskingKey) );

		MaskingKey maskingKeyNBO = htonl( maskingKey );

		MemoryOStream maskedData( sizeof(maskingKeyNBO) +
			data.remainingLength() );

		maskedData.addBlob( &maskingKeyNBO,
			sizeof(maskingKeyNBO) );

		this->mask( maskingKey, data, maskedData );

		return this->stream().writeFrom( maskedData, WRITE_SHOULD_NOT_CORK );
	}
	else
	{
		// No masking, just pass data through.
		return this->stream().writeFrom( data, WRITE_SHOULD_NOT_CORK );
	}
}


/**
 *	This method returns the received frame data as a constant pointer to the
 *	byte storage.
 *
 *	@return 	The received frame data.
 */
const char * WebSocketStreamFilter::receivedFrameData()
{
	return reinterpret_cast< const char * >( receiveBuffer_.retrieve( 0 ) );
}


/**
 *	This method sends back a HTTP error response to the client in response to a
 *	rejected request.
 *
 *	@param errorCode 		The HTTP status code.
 *	@param reasonPhrase 	The HTTP reason phrase.
 */
void WebSocketStreamFilter::rejectHandshake( const BW::string & errorCode,
		const BW::string & reasonPhrase )
{
	BW::string response =
		"HTTP/1.1 " + errorCode + " " + reasonPhrase + "\r\n" +
		"Sec-WebSocket-Version: " + WEBSOCKETS_VERSION + "\r\n\r\n";

	MemoryIStream responseStream( response.data(),
		static_cast<int>(response.size()) );
	this->stream().writeFrom( responseStream, WRITE_SHOULD_NOT_CORK );
}


/**
 *	This method sends the handshake response to the client.
 *
 *	@param headers		Header lines to send in the response, each line
 *						terminated by CRLF.
 *
 *	@return 			True if the send succeeded, false otherwise.
 */
bool WebSocketStreamFilter::sendHandshakeResponseToClient(
		const BW::string & headers )
{
	MF_ASSERT( !isClient_ );

	BW::string response =
		"HTTP/1.1 101 Switching Protocols\r\n\
Upgrade: websocket\r\n\
Connection: Upgrade\r\n" + headers + "\r\n";

	MemoryIStream responseStream( response.data(),
		static_cast<int>(response.size()) );
	return this->stream().writeFrom( responseStream, WRITE_SHOULD_NOT_CORK );
}


/**
 *	This method sends the handshake request to the server.
 *
 *	@param host 	The host name.
 *	@param uri 		The request URI.
 *	@param origin 	The origin, or empty string if unspecified.
 *
 *	@return 		True if the send succeeded, false otherwise.
 */
bool WebSocketStreamFilter::sendHandshakeRequestToServer(
		const BW::string & host, const BW::string & uri,
		const BW::string & origin )
{
	MF_ASSERT( isClient_ );

	// Nonce is 16-byte as per RFC 6455 4.1 
	uint32 nonce[4];
	BWOpenSSL::RAND_bytes( reinterpret_cast< unsigned char * >(nonce),
		sizeof(nonce) );

	BW::string nonceBase64 = Base64::encode(
		reinterpret_cast< const char * >( &nonce ),
		sizeof(nonce) );

	BW::string expectedNonceAcceptRaw = nonceBase64 + WEBSOCKETS_GUID;
	unsigned char expectedNonceAcceptDigest[ SHA_DIGEST_LENGTH ];

	BWOpenSSL::SHA1( reinterpret_cast< const unsigned char * >(
			expectedNonceAcceptRaw.data() ),
		expectedNonceAcceptRaw.size(),
		expectedNonceAcceptDigest );

	expectedNonceAccept_ = Base64::encode(
		reinterpret_cast< const char * >( expectedNonceAcceptDigest ),
		SHA_DIGEST_LENGTH );

	BW::string request = "GET " + uri + " HTTP/1.1\r\n\
Host: " + host + "\r\n\
Connection: Upgrade\r\n\
Upgrade: websocket\r\n\
Sec-WebSocket-Key: " + nonceBase64 + "\r\n\
Sec-WebSocket-Version: 13\r\n";

	if (origin.size())
	{
		request += "Origin: " + origin + "\r\n";
	}

	request += "\r\n";

	MemoryIStream requestStream( request.data(),
		static_cast<int>(request.size()) );
	return this->stream().writeFrom( requestStream, WRITE_SHOULD_NOT_CORK );
}


} // end namespace Mercury

BW_END_NAMESPACE


// websocket_stream_filter.cpp
