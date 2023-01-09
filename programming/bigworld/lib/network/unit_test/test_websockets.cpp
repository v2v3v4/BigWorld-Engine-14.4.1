#include "pch.hpp"

#include "network/http_messages.hpp"
#include "network/websocket_handshake_handler.hpp"
#include "network/websocket_stream_filter.hpp"

namespace BWOpenSSL
{
#include "openssl/sha.h"
#include "openssl/rand.h"
} // end namespace BWOpenSSL

#include "cstdmf/base64.h"
#include "cstdmf/timestamp.hpp"

#if defined( __unix__ )
#include <arpa/inet.h>
#endif // defined( __unix__ )

BW_BEGIN_NAMESPACE

TEST( HTTPHeaders )
{
	Mercury::HTTPHeaders headers;

 	BW::string headerBytes =
"Host: server.example.com\r\n\
Upgrade: websocket\r\n\
Connection: Upgrade\r\n\
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\
Origin: http://example.com\r\n\
Sec-WebSocket-Protocol: chat, superchat\r\n\
Sec-WebSocket-Version: 13\r\n\r\n";

	// Check parsing completes successfully.
	const char * headerBytesData = headerBytes.data();
	CHECK( headers.parse( &headerBytesData, headerBytes.size() ) );

	// Check first header is included.
	CHECK( headers.contains( "Host" ) );
	CHECK_EQUAL( "server.example.com", headers.valueFor( "Host" ) );

	// Check last header is included.
	CHECK( headers.contains( "Sec-WebSocket-Version" ) );
	CHECK_EQUAL( "13", headers.valueFor( "Sec-WebSocket-Version" ) );

	// Check case-insensitive header names.
	CHECK( headers.contains( "sec-websocket-version" ) );
	CHECK_EQUAL( "13", headers.valueFor( "sec-websocket-version" ) );

	// Check non-existent headers
	CHECK( !headers.contains( "" ) );
	CHECK( !headers.contains( "random-string" ) );
	CHECK_EQUAL( "", headers.valueFor( "random-string" ) );

	CHECK_EQUAL( headerBytes.data() + headerBytes.size(), headerBytesData );
}


TEST( HTTPHeadersMultipleHeadersSameName )
{

	BW::string headerBytes =
"Host: server.example.com\r\n\
Upgrade: websocket\r\n\
Connection: Upgrade\r\n\
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\
Origin: http://example.com\r\n\
Sec-WebSocket-Protocol: chat\r\n\
Sec-WebSocket-Protocol: superchat\r\n\
Sec-WebSocket-Version: 13\r\n\r\n";

	Mercury::HTTPHeaders headers;

	const char * headerByteData = headerBytes.data();
	CHECK( headers.parse( &headerByteData, headerBytes.size() ) );

	CHECK_EQUAL( "chat,superchat",
		headers.valueFor( "sec-websocket-protocol" ) );

}


TEST( HTTPHeadersQuotedString )
{
	BW::string headerBytes =
"Host: server.example.com\r\n\
Upgrade: websocket\r\n\
Connection: Upgrade\r\n\
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\
Origin: http://example.com\r\n\
Sec-WebSocket-Protocol: chat, superchat\r\n\
Sec-WebSocket-Version: 13\r\n\
Test: \"quoted string\"\r\n\
Test-2: \"quoted \\\"string\\\\\"\r\n\r\n";

	Mercury::HTTPHeaders headers;

	const char * headerByteData = headerBytes.data();
	CHECK( headers.parse( &headerByteData, headerBytes.size() ) );

	CHECK_EQUAL( "quoted string", headers.valueFor( "test" ) );
	CHECK_EQUAL( "quoted \"string\\", headers.valueFor( "test-2" ) );
}


TEST( HTTPRequest )
{
	Mercury::HTTPRequest request;

 	BW::string requestBytes =
"GET /chat HTTP/1.1\r\n\
Host: server.example.com\r\n\
Upgrade: websocket\r\n\
Connection: Upgrade\r\n\
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\
Origin: http://example.com\r\n\
Sec-WebSocket-Protocol: chat, superchat\r\n\
Sec-WebSocket-Version: 13\r\n\r\n";

	const char * requestByteData = requestBytes.data();
	CHECK( request.parse( &requestByteData, requestBytes.size() ) );

	CHECK_EQUAL( "GET", request.method() );
	CHECK_EQUAL( "/chat", request.uri() );
	CHECK_EQUAL( "HTTP/1.1", request.httpVersionString() );

	CHECK_EQUAL( requestBytes.data() + requestBytes.size(), requestByteData  );
}


TEST( HTTPResponse )
{
	Mercury::HTTPResponse response;

	BW::string responseBytes =
"HTTP/1.1 101 Switching Protocols\r\n\
Upgrade: websocket\r\n\
Connection: Upgrade\r\n\
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n\r\n";

	const char * responseByteData = responseBytes.data();

	CHECK( response.parse( &responseByteData, responseBytes.size() ) );

	CHECK_EQUAL( "HTTP/1.1", response.httpVersion() );
	CHECK_EQUAL( "101", response.statusCode() );
	CHECK_EQUAL( "Switching Protocols", response.reasonPhrase() );

	CHECK_EQUAL( responseBytes.data() + responseBytes.size(),
		responseByteData );
}


class TestStream;
typedef SmartPointer< TestStream > TestStreamPtr;


class TestStream : public Mercury::NetworkStream,
		public Mercury::WebSocketHandshakeHandler
{
public:


	static TestStreamPtr create( BinaryIStream & readBuffer, 
		BinaryOStream & writeBuffer )
	{
		return new TestStream( readBuffer, writeBuffer );
	}


	// Overrides from NetworkStream
	virtual bool writeFrom( BinaryIStream & input, bool shouldCork )
	{
		int inputSize = input.remainingLength();
		writeBuffer_.transfer( input, inputSize );
		return true;
	}

	virtual int readInto( BinaryOStream & output )
	{
		int numBytesToRead = readBuffer_.remainingLength();
		output.transfer( readBuffer_, numBytesToRead );
		return numBytesToRead;
	}

	virtual BW::string streamToString() const
	{ return BW::string( "TestStream" ); }

	virtual bool isConnected() const
	{ return isConnected_; }

	virtual void shutDown()
	{
		isConnected_ = false;
	}


	// Override from WebSocketHandshakeHandler
	virtual bool shouldAcceptHandshake(
		const Mercury::WebSocketStreamFilter & filter,
		const Mercury::HTTPRequest & request,
		Subprotocols & protocols )
	{
		request_ = request;
		return shouldAcceptHandshake_;
	}


	// Own methods

	BinaryIStream & readBuffer()
	{
		return readBuffer_;
	}

	BinaryOStream & writeBuffer()
	{
		return writeBuffer_;
	}

	const Mercury::HTTPRequest & request() const { return request_; }

	void shouldAcceptHandshake( bool shouldAcceptHandshake )
	{ shouldAcceptHandshake_ = shouldAcceptHandshake; }


private:
	TestStream( BinaryIStream & readBuffer, BinaryOStream & writeBuffer ) :
			NetworkStream(),
			Mercury::WebSocketHandshakeHandler(),
			isConnected_( true ),
			readBuffer_( readBuffer ),
			writeBuffer_( writeBuffer ),
			shouldAcceptHandshake_( true ),
			request_()
	{}

	virtual ~TestStream() {}


	bool isConnected_;
	BinaryIStream & readBuffer_;
	BinaryOStream & writeBuffer_;
	bool shouldAcceptHandshake_;
	Mercury::HTTPRequest request_;
};



TEST( WebSocketStreamFilterServerHandshakeResponse )
{
	srand( uint( timestamp() ) );

	MemoryOStream readBuffer;
	MemoryOStream writeBuffer;
	TestStreamPtr pTestStream = TestStream::create( readBuffer, writeBuffer );

	uint32 nonce[4];
	BWOpenSSL::RAND_bytes( reinterpret_cast< unsigned char * >( &nonce ),
		sizeof(nonce) );
	BW::string nonceBase64;
	nonceBase64 = Base64::encode( reinterpret_cast< const char * >( &nonce ),
		sizeof( nonce ) );

	BW::string testHandshake = "GET /test/uri HTTP/1.1\r\n\
Host: server.example.com\r\n\
Connection: Upgrade\r\n\
Upgrade: websocket\r\n\
Sec-WebSocket-Key: " + nonceBase64 + "\r\n\
Sec-WebSocket-Version: 13\r\n\
Origin: www.example.com\r\n\r\n";

	readBuffer.addBlob( testHandshake.data(),
		static_cast<int>( testHandshake.size() ) );

	Mercury::StreamFilterPtr pStreamFilter( 
		new Mercury::WebSocketStreamFilter( *pTestStream, *pTestStream ) );

	MemoryOStream filteredRead;
	int numBytesRead = pStreamFilter->readInto( filteredRead );

	CHECK_EQUAL( 0, numBytesRead );
	CHECK_EQUAL( 0, filteredRead.remainingLength() );

	CHECK( pTestStream->isConnected() );

	CHECK_EQUAL( "GET", pTestStream->request().method() );
	CHECK_EQUAL( "/test/uri", pTestStream->request().uri() );
	CHECK_EQUAL( "www.example.com",
		pTestStream->request().headers().valueFor( "origin" ) );
	CHECK_EQUAL( "HTTP/1.1", pTestStream->request().httpVersionString() );

	Mercury::HTTPResponse response;
	int writeBufferLength = writeBuffer.remainingLength();
	const char * writeData =
		reinterpret_cast< const char * >(
			writeBuffer.retrieve( writeBufferLength ) );

	CHECK( response.parse( &writeData, writeBufferLength ) );

	CHECK_EQUAL( "HTTP/1.1", response.httpVersion() );
	CHECK_EQUAL( "101", response.statusCode() );
	CHECK_EQUAL( "Switching Protocols", response.reasonPhrase() );

	BW::string expectedAcceptField =
		nonceBase64 + Mercury::WebSocketStreamFilter::WEBSOCKETS_GUID;
	unsigned char expectedAcceptDigest[SHA_DIGEST_LENGTH];
	BWOpenSSL::SHA1(
		reinterpret_cast< const unsigned char * >( expectedAcceptField.data() ),
		expectedAcceptField.size(),
		expectedAcceptDigest );
	BW::string expectedAcceptDigestBase64 =
		Base64::encode(
			reinterpret_cast< const char * >( expectedAcceptDigest ),
			SHA_DIGEST_LENGTH );

	CHECK_EQUAL( expectedAcceptDigestBase64,
		response.headers().valueFor( "sec-websocket-accept" ) );
}


TEST( WebSocketStreamFilterClientHandshakeRequest )
{
	MemoryOStream readBuffer;
	MemoryOStream writeBuffer;

	TestStreamPtr pTestStream = TestStream::create( readBuffer, writeBuffer );
	Mercury::StreamFilterPtr pWebSocketStreamFilter( 
		new Mercury::WebSocketStreamFilter( *pTestStream, 
			"host.example.com", "/example/uri", "origin.example.com" ) );

	Mercury::HTTPRequest request;
	int requestSize = writeBuffer.remainingLength();
	const char * requestBytes = reinterpret_cast< const char * >(
		writeBuffer.retrieve( requestSize ) );

	CHECK( request.parse( &requestBytes, requestSize ) );

	CHECK_EQUAL( "GET", request.method() );
	CHECK_EQUAL( "/example/uri", request.uri() );

	CHECK( request.headers().contains( "host" ) );
	CHECK_EQUAL( "host.example.com", request.headers().valueFor( "host" ) );

	CHECK( request.headers().valueCaseInsensitivelyEquals(
		"upgrade", "websocket" ) );

	CHECK( request.headers().valueCaseInsensitivelyEquals(
		"connection", "upgrade" ) );

	CHECK( request.headers().contains( "sec-websocket-key" ) );
	const BW::string & websocketKeyBase64 =
		request.headers().valueFor( "sec-websocket-key" );
	BW::string websocketKey;
	CHECK( Base64::decode( websocketKeyBase64, websocketKey ) );
	CHECK_EQUAL( 16U, websocketKey.size() );

	CHECK_EQUAL( "13", request.headers().valueFor( "sec-websocket-version" ) );

	CHECK_EQUAL( "origin.example.com", request.headers().valueFor( "origin" ) );
}


TEST( WebSocketStreamFilterConnection )
{
	MemoryOStream clientToServerBuffer;
	MemoryOStream serverToClientBuffer;
	TestStreamPtr pTestStreamServer = TestStream::create( clientToServerBuffer, 
		serverToClientBuffer );
	TestStreamPtr pTestStreamClient = TestStream::create( serverToClientBuffer, 
		clientToServerBuffer );

	Mercury::StreamFilterPtr pServerFilter = 
		new Mercury::WebSocketStreamFilter( 
			*pTestStreamServer, *pTestStreamServer );
	Mercury::WebSocketStreamFilter & serverFilter = 
		static_cast< Mercury::WebSocketStreamFilter & >( *pServerFilter );

	Mercury::StreamFilterPtr pClientFilter = 
		new Mercury::WebSocketStreamFilter( 
			*pTestStreamClient, "host.example.com", "/example/uri",
			"origin.example.com" );
	Mercury::WebSocketStreamFilter & clientFilter = 
		static_cast< Mercury::WebSocketStreamFilter & >( *pClientFilter );

	MemoryOStream serverReadBuffer;
	CHECK_EQUAL( 0, pServerFilter->readInto( serverReadBuffer ) );
	CHECK_EQUAL( 0, serverReadBuffer.remainingLength() );

	MemoryOStream clientReadBuffer;
	CHECK_EQUAL( 0, pClientFilter->readInto( clientReadBuffer ) );
	CHECK_EQUAL( 0, clientReadBuffer.remainingLength() );

	CHECK( serverFilter.hasHandshakeCompleted() );
	CHECK( clientFilter.hasHandshakeCompleted() );

	CHECK_EQUAL( 0, clientToServerBuffer.remainingLength() );
	CHECK_EQUAL( 0, serverToClientBuffer.remainingLength() );

	MemoryOStream testData;
	BW::string testString( "Client-to-server" );
	testData << testString;
	size_t expectedPayloadLength = 
		static_cast< size_t >( testData.remainingLength() );

	pClientFilter->writeFrom( testData,
		Mercury::NetworkStream::WRITE_SHOULD_NOT_CORK );

	CHECK( clientToServerBuffer.remainingLength() > 0 );

	MemoryOStream testDataStream;
	testDataStream.addBlob( clientToServerBuffer.retrieve( 0 ),
		clientToServerBuffer.remainingLength() );

	uint8 flags;
	uint8 payloadLength;
	testDataStream >> flags >> payloadLength;
	const uint8 * maskingKeyBytes =
		reinterpret_cast< const uint8 * >(
			testDataStream.retrieve( sizeof( uint32 ) ) );

	// Is FINal fragment
	CHECK( flags & 0x80 );
	// Opcode:BINARY
	CHECK( flags & 0x02 );
	// Is MASKed
	CHECK( payloadLength & 0x80 );

	CHECK_EQUAL( expectedPayloadLength, (payloadLength & 0x7FU) );

	CHECK_EQUAL( expectedPayloadLength, 
		static_cast< size_t >( testDataStream.remainingLength() ) );

	MemoryOStream unmaskedData;

	int i = 0;
	while (testDataStream.remainingLength())
	{
		uint8 maskedByte =
			*reinterpret_cast< const uint8 * >( testDataStream.retrieve( 1 ) );
		unmaskedData << uint8( maskedByte ^ maskingKeyBytes[i % 4] );
		++i;
	}

	BW::string testStringOut;
	unmaskedData >> testStringOut;
	CHECK( !unmaskedData.error() );

	CHECK_EQUAL( testString, testStringOut );

	CHECK( pServerFilter->readInto( serverReadBuffer ) > 0 );
	CHECK( serverReadBuffer.remainingLength() > 0 );

	serverReadBuffer >> testStringOut;
	CHECK_EQUAL( testString, testStringOut );

	testString = "Server-to-client";
	testData.reset();
	testData << testString;

	pServerFilter->writeFrom( testData,
		Mercury::StreamFilter::WRITE_SHOULD_NOT_CORK );

	clientReadBuffer.reset();
	pClientFilter->readInto( clientReadBuffer );

	clientReadBuffer >> testStringOut;
	CHECK_EQUAL( testStringOut, testString );
}


BW_END_NAMESPACE

// test_websockets.cpp

