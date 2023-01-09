#include "pch.hpp"

#include "network_app.hpp"
#include "test_tcp_channels_interfaces.hpp"

#include "cstdmf/singleton.hpp"

#include "network/channel_listener.hpp"
#include "network/encryption_stream_filter.hpp"
#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"
#include "network/stream_filter.hpp"
#include "network/stream_filter_factory.hpp"
#include "network/symmetric_block_cipher.hpp"
#include "network/tcp_channel.hpp"
#include "network/tcp_channel_stream_adaptor.hpp"
#include "network/tcp_connection_opener.hpp"
#include "network/tcp_server.hpp"
#include "network/websocket_handshake_handler.hpp"
#include "network/websocket_stream_filter.hpp"

#if defined( USE_OPENSSL )
namespace BWOpenSSL
{
#include "openssl/rand.h"
} // end namespace BWOpenSSL
#endif


BW_BEGIN_NAMESPACE


namespace // (anonymous)
{

const static uint32 MAX_SENDS = 10;

} // end namespace (anonymous)


// ----------------------------------------------------------------------------
// Section: Helper classes
// ----------------------------------------------------------------------------

/**
 *	This class is a simple timeout class that simply sets a once-off timer
 *	which sets a flag and breaks dispatcher processing when triggered. It can
 *	be used to check that a test has run within a certain amount of time.
 */
class TestTimeout : public TimerHandler
{
public:

	/**
	 *	Constructor.
	 *
	 *	@param dispatcher		The event dispatcher to add timers to, and to
	 *							break processing when timing out.
	 *	@param timeoutSeconds	The number of seconds to wait until timing out.
	 */
	TestTimeout( Mercury::EventDispatcher & dispatcher, float timeoutSeconds ) :
			TimerHandler(),
			dispatcher_( dispatcher ),
			timerHandle_(),
			hasTimedOut_( false )
	{
		timerHandle_ = dispatcher.addOnceOffTimer(
			int64( timeoutSeconds * 1000000  ),
			this );
	}


	/**
	 *	Destructor.
	 */
	virtual ~TestTimeout()
	{
		this->cancel();
	}


	/* Override from TimerHandler */
	virtual void handleTimeout( TimerHandle timerHandle, void * arg )
	{
		hasTimedOut_ = true;
		dispatcher_.breakProcessing();
	}

	/* Override from TimerHandler */
	virtual void onRelease( TimerHandle timerHandle, void * arg )
	{
	}

	// Own methods

	/** This method cancels the timer. */
	void cancel()
	{ timerHandle_.cancel(); }

	/** This method returns whether the timeout was triggered. */
	bool hasTimedOut() const
	{ return hasTimedOut_; }


private:

	Mercury::EventDispatcher &	dispatcher_;
	TimerHandle					timerHandle_;
	bool						hasTimedOut_;
};


/**
 *	This class is the TCPChannel stream filter factory, where it creates a
 *	WebSocketStreamFilter, and handshake handler used in the WebSockets tests.
 */
class WebSocketStreamFilterFactory : public Mercury::StreamFilterFactory,
		public Mercury::WebSocketHandshakeHandler
{
public:
	/**
	 *	Constructor.
	 */
	WebSocketStreamFilterFactory() :
			Mercury::StreamFilterFactory(),
			Mercury::WebSocketHandshakeHandler()
	{}


	/**
	 *	Destructor.
	 */
	virtual ~WebSocketStreamFilterFactory() {}


	/*
	 *	Override from Mercury::StreamFilterFactory.
	 */
	virtual Mercury::StreamFilterPtr createFor( Mercury::TCPChannel & channel )
	{
		Mercury::NetworkStreamPtr pChannelStream(
			new Mercury::TCPChannelStreamAdaptor( channel ) );

		return new Mercury::WebSocketStreamFilter( *pChannelStream, *this );
	}


	/*
	 *	Override from Mercury::WebSocketHandshakeHandler.
	 */
	virtual bool shouldAcceptHandshake(
			const Mercury::WebSocketStreamFilter & filter,
			const Mercury::HTTPRequest & request,
			Subprotocols & subprotocols )
	{
		return true;
	}

private:
};


/**
 *	Helper class for creating a TCP connection synchronously.
 */
class ConnectionHelper : public Mercury::TCPConnectionOpenerListener
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param eventDispatcher	The event dispatcher.
	 *	@param useWebSockets	Whether to use WebSockets for connecting.
	 */
	ConnectionHelper( Mercury::EventDispatcher & eventDispatcher,
				bool useWebSockets ) :
			eventDispatcher_( eventDispatcher ),
			pChannel_( NULL ),
			reason_( Mercury::REASON_GENERAL_NETWORK ),
			useWebSockets_( useWebSockets )
	{
	}


	/**
	 *	Destructor.
	 */
	virtual ~ConnectionHelper()
	{
	}


	// Override from Mercury::TCPConnectionOpenerListener
	virtual void onTCPConnect( Mercury::TCPConnectionOpener & opener,
		Mercury::TCPChannel * pChannel )
	{
		pChannel_ = pChannel;

		if (useWebSockets_)
		{
			Mercury::NetworkStreamPtr pChannelStream(
				new Mercury::TCPChannelStreamAdaptor( *pChannel_ ) );
			Mercury::StreamFilterPtr pWebSockets(
				new Mercury::WebSocketStreamFilter( *pChannelStream,
					"host.example.com", "/" ) );
			pChannel_->pStreamFilter( pWebSockets.get() );
		}

		reason_ = Mercury::REASON_SUCCESS;
		eventDispatcher_.breakProcessing();
	}


	// Override from Mercury::TCPConnectionOpenerListener
	virtual void onTCPConnectFailure( Mercury::TCPConnectionOpener & opener,
			Mercury::Reason reason )
	{
		pChannel_ = NULL;
		reason_ = reason;
		eventDispatcher_.breakProcessing();
	}

	// Own methods

	/** 
	 *	This method returns the created channel, or NULL if no connection was
	 *	established.
	 */
	Mercury::TCPChannel * pChannel()
	{ return pChannel_.get(); }

	/**
	 *	This method returns the status of a completed request.
	 */
	Mercury::Reason reason() const
	{ return reason_; }


private:
	Mercury::EventDispatcher & 	eventDispatcher_;
	Mercury::TCPChannelPtr 		pChannel_;
	Mercury::Reason 			reason_;
	bool 						useWebSockets_;
};


// ----------------------------------------------------------------------------
// Section: TCPChannelsServerApp
// ----------------------------------------------------------------------------


/**
 *	The server-side TCPChannels test app.
 */
class TCPChannelsServerApp : public NetworkApp,
		public Mercury::ChannelListener,
		public Singleton< TCPChannelsServerApp >
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param dispatcher		The event dispatcher.
	 *	@param useWebSockets	Whether to listen for WebSockets connections.
	 */
	TCPChannelsServerApp( Mercury::EventDispatcher & dispatcher,
				bool useWebSockets, TEST_ARGS_PROTO ) :
			NetworkApp( dispatcher, Mercury::NETWORK_INTERFACE_EXTERNAL,
				TEST_ARGS ),
			webSocketStreamFilterFactory_(),
			tcpServer_( this->networkInterface() )
	{
		if (useWebSockets)
		{
			tcpServer_.pStreamFilterFactory( &webSocketStreamFilterFactory_ );
		}

		if (!tcpServer_.init())
		{
			static const uint MAX_ATTEMPTS = 5;
			uint numAttempts = 0;
			while (!tcpServer_.initWithPort( 0 ) && 
					(numAttempts < MAX_ATTEMPTS))
			{
				NOTICE_MSG( "TCPChannelsServerApp::TCPChannelsServerApp: "
						"Failed to initialise, re-attempting %u/%u\n",
					numAttempts + 1, MAX_ATTEMPTS );
				++numAttempts;
			}

			if (!tcpServer_.isGood())
			{
				FAIL( "Could not initialise TCP server" ); 
			}
		}
	}

	virtual ~TCPChannelsServerApp();

	// Overrides from NetworkApp

	virtual void handleTimeout( TimerHandle handle, void * arg );
	virtual int run();

	// Overrides from Mercury::ChannelListener
	virtual void onChannelGone( Mercury::Channel & channel );

	// Message handlers
	void msg1( const Mercury::UnpackedMessageHeader & header,
		const TCPChannelsServerInterface::msg1Args & args );

	void msg2( const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data );

	void disconnect( const Mercury::UnpackedMessageHeader & header,
		const TCPChannelsServerInterface::disconnectArgs & args );

	// Own methods
	Mercury::TCPServer & tcpServer() { return tcpServer_; }

	/**
	 *	This method sets the block cipher to user for encryption.
	 *
	 *	@param pCipher 	The block cipher to use.
	 */
	void setCipher( Mercury::BlockCipherPtr pCipher )
	{
		pCipher_ = pCipher;
	}

private:
	void addClient( Mercury::Channel * pChannel );
	void removeClient ( Mercury::Channel * pChannel );


	typedef std::map< Mercury::Channel *, uint32 > Clients;
	Clients 					clients_;

	WebSocketStreamFilterFactory
								webSocketStreamFilterFactory_;

	Mercury::TCPServer 			tcpServer_;
	Mercury::BlockCipherPtr 	pCipher_;
};


/**
 *	Destructor.
 */
TCPChannelsServerApp::~TCPChannelsServerApp()
{
	for (Clients::iterator iter = clients_.begin();
			iter != clients_.end();
			++iter)
	{
		if (iter->first->isConnected())
		{
			iter->first->destroy();
		}

		iter->first->decRef();
	}
}


/**
 *	This method handles the msg1 message.
 */
void TCPChannelsServerApp::msg1( const Mercury::UnpackedMessageHeader & header,
		const TCPChannelsServerInterface::msg1Args & args )
{
	Mercury::Channel * pChannel = header.pChannel.get();
	TRACE_MSG( "TCPChannelsServerApp::msg1: %u\n", args.seq );
	if (clients_.count( pChannel ) == 0)
	{
		this->addClient( pChannel );
	}

	if (header.replyID != Mercury::REPLY_ID_NONE)
	{
		header.pChannel->bundle().startReply( header.replyID );
		header.pChannel->send();
	}
}


/**
 *	This method handles the msg1 message.
 */
void TCPChannelsServerApp::msg2( const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	Mercury::Channel * pChannel = header.pChannel.get();
	BW::string s;
	data >> s;

	TRACE_MSG( "TCPChannelsServerApp::msg2: \"%s\"\n",
		s.c_str() );

	if (header.replyID != Mercury::REPLY_ID_NONE)
	{
		pChannel->bundle().startReply( header.replyID );
		pChannel->send();
	}
}


/**
 *	This method handles the disconnect message.
 */
void TCPChannelsServerApp::disconnect(
		const Mercury::UnpackedMessageHeader & header,
		const TCPChannelsServerInterface::disconnectArgs & args )
{
	TRACE_MSG( "TCPChannelsServerApp::disconnect\n" );
	header.pChannel->shutDown();
}


/**
 *	This method adds the given client connection to our collection.
 */
void TCPChannelsServerApp::addClient( Mercury::Channel * pChannel )
{
	if (pCipher_)
	{
		pChannel->setEncryption( pCipher_ );
	}

	pChannel->incRef();
	pChannel->pChannelListener( this );
	clients_[pChannel] = 0;
}


/**
 *	This method removes the given client connection to our collection.
 */
void TCPChannelsServerApp::removeClient( Mercury::Channel * pChannel )
{

	Clients::iterator iChannel = clients_.find( pChannel );
	if (iChannel == clients_.end())
	{
		return;
	}

	iChannel->first->decRef();
	clients_.erase( iChannel );
}


/*
 *	Override from NetworkApp / TimerHandler.
 */
void TCPChannelsServerApp::handleTimeout( TimerHandle handle, void * arg )
{
	for (Clients::iterator iClient = clients_.begin();
			iClient != clients_.end();
			++iClient)
	{
		if (iClient->second <= MAX_SENDS)
		{
			Mercury::Bundle & bundle = iClient->first->bundle();

			TCPChannelsClientInterface::msg1Args & args = args.start( bundle );
			args.seq = iClient->second;
			args.data = 0xdeadcafe;

			iClient->first->send();

			++(iClient->second);
		}
	}
}


/*
 *	Override from Mercury::ChannelListener.
 */
void TCPChannelsServerApp::onChannelGone( Mercury::Channel & channel )
{
	DEBUG_MSG( "TCPChannelsServerApp::onChannelGone: %s\n", channel.c_str() );
	Mercury::TCPChannel & tcpChannel =
		static_cast< Mercury::TCPChannel & >( channel );

	this->removeClient( &tcpChannel );
	this->stop();
}


/*
 *	Override from NetworkApp.
 */
int TCPChannelsServerApp::run()
{
	TCPChannelsServerInterface::registerWithInterface( 
		this->networkInterface() );

	this->startTimer( 5000 );

	return this->NetworkApp::run();
}


// ----------------------------------------------------------------------------
// Section: TCPChannelsClientApp
// ----------------------------------------------------------------------------


/**
 *	The client-side TCPChannels test app.
 */
class TCPChannelsClientApp : public NetworkApp,
		public Singleton< TCPChannelsClientApp >,
		public Mercury::ChannelListener,
		public Mercury::ReplyMessageHandler
{
public:
	/**
	 *	Constructor.
	 */
	TCPChannelsClientApp( Mercury::EventDispatcher & dispatcher,
			const Mercury::Address & serverAddress,
			bool useWebSockets,
			TEST_ARGS_PROTO ) :
		NetworkApp( dispatcher, Mercury::NETWORK_INTERFACE_EXTERNAL,
			TEST_ARGS ),
		serverAddress_( serverAddress ),
		pChannel_( NULL ),
		nextExpectedSequence_( 0 ),
		useWebSockets_( useWebSockets ),
		pCipher_( NULL ),
		numReplies_( 0 )
	{
	}


	/**
	 *	Destructor.
	 */
	virtual ~TCPChannelsClientApp()
	{
		if (pChannel_->isConnected())
		{
			pChannel_->destroy();
		}
	}


	// Overrides from Mercury::ReplyMessageHandler

	virtual void handleMessage( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		void * arg )
	{
		++numReplies_;
		data.finish();
	}

	/**
	 * 	This method is called by Mercury when the request fails. The
	 * 	normal reason for this happening is a timeout.
	 *
	 * 	@param exception	The reason for failure.
	 * 	@param arg			The user-defined data associated with the request.
	 */
	virtual void handleException( const Mercury::NubException & exception,
		void * arg )
	{}

	// Overrides from NetworkApp

	/* Override from NetworkApp / TimerHandler. */
	virtual void handleTimeout( TimerHandle handle, void * arg )
	{
	}

	// Overrides from Mercury::ChannelListener
	virtual void onChannelGone( Mercury::Channel & channel );

	// Own methods.

	bool init();

	void msg1( const Mercury::UnpackedMessageHeader & header,
		const TCPChannelsClientInterface::msg1Args & args );

	
	/**
	 *	This method sets the block cipher to user for encryption.
	 *
	 *	@param pCipher 	The block cipher to use.
	 */
	void setCipher( Mercury::BlockCipherPtr pCipher )
	{
		pCipher_ = pCipher;
	}

	/**
	 *	This method returns the number of replies received from our requests.
	 */
	uint numReplies() const { return numReplies_; }


private:
	Mercury::Address 			serverAddress_;
	Mercury::TCPChannelPtr 		pChannel_;
	uint32 						nextExpectedSequence_;
	bool 						useWebSockets_;
	Mercury::BlockCipherPtr 	pCipher_;
	uint						numReplies_;
};

BW_SINGLETON_STORAGE( TCPChannelsClientApp );
BW_SINGLETON_STORAGE( TCPChannelsServerApp );


/**
 *	This method starts the test.
 */
bool TCPChannelsClientApp::init()
{
	TCPChannelsClientInterface::registerWithInterface( 
		this->networkInterface() );

	ConnectionHelper helper( this->dispatcher(), useWebSockets_ );

	Mercury::TCPConnectionOpener opener( helper, this->networkInterface(),
		serverAddress_ );

	this->dispatcher().processContinuously();

	if (helper.reason() != Mercury::REASON_SUCCESS)
	{
		ERROR_MSG( "TCPChannelsClientApp::run: "
			"Could not connect to server.\n" );
		return false;
	}

	pChannel_ = helper.pChannel();
	pChannel_->pChannelListener( this );

	TCPChannelsServerInterface::msg1Args & args =
		args.startRequest( pChannel_->bundle(), this );
	args.seq = 0;
	args.data = 0xcafebeef;

	pChannel_->bundle().startRequest( TCPChannelsServerInterface::msg2,
		this );
	pChannel_->bundle() << "TCPChannels-msg2";
	pChannel_->send();

	if (pCipher_)
	{
		pChannel_->setEncryption( pCipher_ );
	}

	return true;
}


/*
 *	Override from Mercury::ChannelListener.
 */
void TCPChannelsClientApp::onChannelGone( Mercury::Channel & channel )
{
	CHECK_EQUAL( MAX_SENDS, nextExpectedSequence_ );
}


/**
 *	This method handles the client-side msg1 message.
 */
void TCPChannelsClientApp::msg1( const Mercury::UnpackedMessageHeader & header,
		const TCPChannelsClientInterface::msg1Args & args )
{
	TRACE_MSG( "TCPChannelsClientApp::msg1: %u\n", args.seq );

	CHECK_EQUAL( nextExpectedSequence_, args.seq );

	if (nextExpectedSequence_ == MAX_SENDS)
	{
		TCPChannelsServerInterface::disconnectArgs & args =
			args.start( pChannel_->bundle() );
		pChannel_->send();
	}
	else
	{
		++nextExpectedSequence_;
		TCPChannelsServerInterface::msg1Args & args = 
			args.start( pChannel_->bundle() );

		args.seq = nextExpectedSequence_;
		args.data = 0xdeadbeef;
		pChannel_->send();
	}
}


// ----------------------------------------------------------------------------
// Section: MessageHandlers
// ----------------------------------------------------------------------------

/**
 *	Class for struct-style Mercury message handler objects.
 */
template <class ARGS>
class TCPChannelsServerStructMessageHandler :
		public Mercury::InputMessageHandler
{
public:
	typedef void (TCPChannelsServerApp::*Handler)(
			const Mercury::UnpackedMessageHeader & header,
			const ARGS & args );

	TCPChannelsServerStructMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & /* srcAddress */,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		ARGS * pArgs = (ARGS*)data.retrieve( sizeof(ARGS) );

		(TCPChannelsServerApp::instance().*handler_)( header, *pArgs );
	}

	Handler handler_;
};


/**
 *	This class handles variable length messages on the server.
 */
class TCPChannelsServerVarLenMessageHandler :
		public Mercury::InputMessageHandler
{
public:
	typedef void (TCPChannelsServerApp::*Handler)(
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	TCPChannelsServerVarLenMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & /* srcAddress */,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		(TCPChannelsServerApp::instance().*handler_)( header, data );
	}

	Handler handler_;
};


// ----------------------------------------------------------------------------
// Section: MessageHandlers
// ----------------------------------------------------------------------------

/**
 *	Class for struct-style Mercury message handler objects.
 */
template <class ARGS>
class TCPChannelsClientStructMessageHandler :
		public Mercury::InputMessageHandler
{
public:
	typedef void (TCPChannelsClientApp::*Handler)(
			const Mercury::UnpackedMessageHeader & header,
			const ARGS & args );

	TCPChannelsClientStructMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & /* srcAddr */,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		ARGS * pArgs = (ARGS*)data.retrieve( sizeof(ARGS) );
		(TCPChannelsClientApp::instance().*handler_)( header, *pArgs );
	}

	Handler handler_;
};


// ----------------------------------------------------------------------------
// Section: Tests proper
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Section: Tests proper
// ----------------------------------------------------------------------------


TEST( TCPChannelsBasicTesting )
{
	Mercury::EventDispatcher eventDispatcher;
	TestTimeout timeout( eventDispatcher, 5.0 );

	TCPChannelsServerApp serverApp( eventDispatcher, false, TEST_ARGS );
	TCPChannelsClientApp clientApp( eventDispatcher,
		serverApp.tcpServer().address(),
		false,
		TEST_ARGS );

	CHECK( clientApp.init() );
	serverApp.run();

	CHECK_EQUAL( 2U, clientApp.numReplies() );
	CHECK( !timeout.hasTimedOut() );
}


TEST( BlockCipher )
{
	Mercury::BlockCipherPtr pClientCipher = 
		Mercury::SymmetricBlockCipher::create();

	Mercury::BlockCipherPtr pServerCipher = 
		Mercury::SymmetricBlockCipher::create( pClientCipher->key() );

	CHECK( pClientCipher->key() == pServerCipher->key() );
	CHECK_EQUAL( pClientCipher->readableKey(), pServerCipher->readableKey() );

	std::string test( pClientCipher->blockSize(), '\0' );

	const char TEST_STRING[] = "My data";
	strncpy( (char *) test.data(), "My data", sizeof( TEST_STRING ) );

	std::string encrypted( pClientCipher->blockSize(), '\0' );
	pClientCipher->encryptBlock( (const uint8 *)test.data(), 
		(uint8 *)encrypted.data() );

	std::string decrypted( pClientCipher->blockSize(), '\0' );
	pServerCipher->decryptBlock( (const uint8 *)encrypted.data(), 
		(uint8 *)decrypted.data() );

	CHECK_EQUAL( test, decrypted );
	MemoryIStream testStream( test.data(), static_cast<int>( test.size() ) );
}


TEST( TCPChannelsEncryption )
{
	Mercury::EventDispatcher eventDispatcher;
	TestTimeout timeout( eventDispatcher, 5.0 );

	Mercury::BlockCipherPtr pClientCipher = 
		Mercury::SymmetricBlockCipher::create();

	Mercury::BlockCipherPtr pServerCipher = 
		Mercury::SymmetricBlockCipher::create( pClientCipher->key() );

	TCPChannelsServerApp serverApp( eventDispatcher, false, TEST_ARGS );
	serverApp.setCipher( pServerCipher );

	TCPChannelsClientApp clientApp( eventDispatcher,
		serverApp.tcpServer().address(),
		false,
		TEST_ARGS );
	clientApp.setCipher( pClientCipher );


	CHECK( clientApp.init() );
	serverApp.run();

	CHECK_EQUAL( 2U, clientApp.numReplies() );
	CHECK( !timeout.hasTimedOut() );
}


TEST( CombineBlocks )
{
	const size_t BLOCK_SIZE = 16;
	uint8 dataA[BLOCK_SIZE];
	uint8 dataB[BLOCK_SIZE];
	uint8 expected[BLOCK_SIZE];
	uint8 combined[BLOCK_SIZE];

	BWOpenSSL::RAND_bytes( dataA, 16 );
	BWOpenSSL::RAND_bytes( dataB, 16 );

	MemoryIStream streamA( dataA, BLOCK_SIZE );
	MemoryIStream streamB( dataB, BLOCK_SIZE );
	
	for (size_t i = 0; i < BLOCK_SIZE; i += sizeof( uint64 ))
	{
		uint64 aWord = *((uint64 *)( dataA + i ));
		uint64 bWord = *((uint64 *)( dataB + i ));
		uint64 cWord = aWord ^ bWord;
		*((uint64*)(expected + i)) = cWord;
	}

	Mercury::BlockCipherPtr pCipher = 
		Mercury::NullCipher::create( BLOCK_SIZE );
	pCipher->combineBlocks( dataA, dataB, combined );

	MemoryIStream expectedStream( expected, BLOCK_SIZE );
	MemoryIStream actualStream( combined, BLOCK_SIZE );

	CHECK_EQUAL( expectedStream.remainingBytesAsDebugString( -1, true ),
		actualStream.remainingBytesAsDebugString( -1, true ) );
	
}


TEST( TCPChannelsWebSockets )
{
	Mercury::EventDispatcher eventDispatcher;

	TestTimeout timeout( eventDispatcher, 5.0 );

	TCPChannelsServerApp serverApp( eventDispatcher, true, TEST_ARGS );

	TCPChannelsClientApp clientApp( eventDispatcher,
		serverApp.tcpServer().address(),
		true,
		TEST_ARGS );

	CHECK( clientApp.init() );
	serverApp.run();

	CHECK_EQUAL( 2U, clientApp.numReplies() );
	CHECK( !timeout.hasTimedOut() );
}


TEST( TCPChannelsWebSocketsEncryption )
{
	Mercury::EventDispatcher eventDispatcher;
	TestTimeout timeout( eventDispatcher, 5.0 );

	Mercury::BlockCipherPtr pClientCipher = 
		Mercury::SymmetricBlockCipher::create();
	Mercury::BlockCipherPtr pServerCipher = 
		Mercury::SymmetricBlockCipher::create( pClientCipher->key() );

	CHECK_EQUAL( pClientCipher->readableKey(), pServerCipher->readableKey() );

	TRACE_MSG( "TCPChannelsWebSocketsEncryption key: %s\n", 
		pClientCipher->readableKey().c_str() );

	TCPChannelsServerApp serverApp( eventDispatcher, true, TEST_ARGS );
	serverApp.setCipher( pServerCipher );

	TCPChannelsClientApp clientApp( eventDispatcher,
		serverApp.tcpServer().address(),
		true,
		TEST_ARGS );
	clientApp.setCipher( pClientCipher );


	CHECK( clientApp.init() );
	serverApp.run();

	CHECK_EQUAL( 2U, clientApp.numReplies() );
	CHECK( !timeout.hasTimedOut() );
}


BW_END_NAMESPACE

#define DEFINE_SERVER_HERE
#include "test_tcp_channels_interfaces.hpp"


// test_tcp_channels.cpp

