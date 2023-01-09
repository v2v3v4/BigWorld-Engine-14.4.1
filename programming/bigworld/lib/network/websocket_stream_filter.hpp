#ifndef WEBSOCKET_STREAM_FILTER_HPP
#define WEBSOCKET_STREAM_FILTER_HPP

#include "stream_filter.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/memory_stream.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
class HTTPRequest;
class HTTPResponse;
class WebSocketHandshakeHandler;


/**
 *	This class implements a stream filter that implements the WebSockets.
 */
class WebSocketStreamFilter : public StreamFilter
{
public:
	static const BW::string WEBSOCKETS_GUID;

	WebSocketStreamFilter( NetworkStream & stream,
		const BW::string & host,
		const BW::string & uri,
		const BW::string & origin = BW::string( "" ) );

	WebSocketStreamFilter( NetworkStream & stream,
			WebSocketHandshakeHandler & handler );

	virtual ~WebSocketStreamFilter();

	// Overrides from StreamFilter

	virtual bool writeFrom( BinaryIStream & input, bool shouldCork );
	virtual int readInto( BinaryOStream & output );
	virtual BW::string streamToString() const;
	virtual bool isConnected() const;
	virtual bool didHandleChannelInactivityTimeout();
	virtual bool didFinishShuttingDown();
	virtual bool hasUnsentData() const;

	// Own methods
	void rejectHandshake( const BW::string & errorCode = BW::string( "400" ),
			const BW::string & reasonPhrase =
				BW::string( "Bad Request" ) );

	/**
	 *	This method returns whether the handshake has completed.
	 */
	bool hasHandshakeCompleted() const { return hasHandshakeCompleted_; }

private:
	bool sendHandshakeRequestToServer( const BW::string & host,
		const BW::string & uri,
		const BW::string & origin );
	bool sendHandshakeResponseToClient( const BW::string & headers );
	bool receiveHandshake();
	bool processHandshakeFromClient( const HTTPRequest & request );
	bool doesClientHandshakePassBasicChecks(
		const HTTPRequest & request ) const;
	bool processDigestChallenge( const HTTPRequest & request,
		BW::string & acceptKey ) const;
	bool processHandshakeFromServer( const HTTPResponse & response );

	bool sendBufferedData();

	int receiveFrame( BinaryOStream & output );
	const char * receivedFrameData();
	void calculateFrameSize();
	void mask( uint32 maskingKey, BinaryIStream & input,
		BinaryOStream & output );
	int processFrameFragment( BinaryOStream & output );
	int processFrame( BinaryOStream & output );
	void processCloseFrame();

	bool sendCloseFrame( uint16 statusCode = 1000 );
	bool sendCloseFrame( uint16 statusCode, 
		const BW::string & reasonPhrase );

	void failConnection();
	void failConnection( uint16 statusCode, const BW::string & reasonPhrase );

	bool sendFrame( uint8 opcode, BinaryIStream & data );


	bool 							hasHandshakeCompleted_;
	BW::string 						expectedNonceAccept_;
	WebSocketHandshakeHandler * 	pHandshakeHandler_;
	bool 							isClient_;
	MemoryOStream 					receiveBuffer_;
	uint							expectedLengthFieldSize_;
	uint64							expectedReceiveFrameLength_;
	uint8							receivedPayloadOpcode_;
	MemoryOStream					receivedPayloadBuffer_;
	MemoryOStream 					sendBuffer_;

	enum CloseState
	{
		NOT_CLOSING,
		CLOSE_ON_CONNECT,
		CLOSE_SENT,
		CLOSED
	};

	CloseState 						closeState_;
};


} // end namespace Mercury


BW_END_NAMESPACE

#endif // WEBSOCKET_STREAM_FILTER_HPP
