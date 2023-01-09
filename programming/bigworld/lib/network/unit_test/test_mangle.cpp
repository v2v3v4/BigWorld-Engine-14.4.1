#include "network_app.hpp"

#include "cstdmf/debug_filter.hpp"
#include "cstdmf/slot_tracker.hpp"

#include "network/network_interface.hpp"
#include "network/packet_filter.hpp"
#include "network/packet_receiver_stats.hpp"

#include "test_mangle_interfaces.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

//extern bool g_shouldWritePid;

namespace
{

// -----------------------------------------------------------------------------
// Section:Configuration options
// -----------------------------------------------------------------------------

// How much to mangle a packet
const uint32 DEFAULT_MANGLE_PER_PACKET_PERCENT = 10;

// The number of packets to mangle
const uint32 DEFAULT_MANGLE_NUM_PACKETS_PERCENT = 50;

// The number of bundles that each client should send
const uint32 DEFAULT_NUM_BUNDLES_TO_SEND = 1000;

// Each bundle is a random size in the following range
const int DEFAULT_MIN_BUNDLE_SIZE = 1000;
const int DEFAULT_MAX_BUNDLE_SIZE = 5000;

// How often the server app checks whether all client apps are done.
const int SERVER_TICK_PERIOD = 100000; // 100 ms

// How often the client should check to send to the server
const int CLIENT_TICK_PERIOD = 1000; // 1ms

// Number of client processes that create INTERNAL channels
const int NUM_INTERNAL_CLIENTS = 0;

// Number of client processes that create EXTERNAL channels
// TODO:TN Put back to 10.
// const int NUM_EXTERNAL_CLIENTS = 10;
const int NUM_EXTERNAL_CLIENTS = 1;

// The number of seconds the test should run before it is considered a failure
// with a timeout.
const int MAX_RUNNING_TIME = 15; // 15 seconds

} // namespace anonymous

// -----------------------------------------------------------------------------
// Section: MangleFilter
// -----------------------------------------------------------------------------

/**
 *  Mangling packet filter.
 */
class MangleFilter : public Mercury::PacketFilter
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param perPacketPercent Indicates what percentage to mangle a packet.
	 *		This percentage (on average) of bytes will be set to a random byte.
	 *	@param numPacketsPercent Indicates what percentage of packets should be
	 *		mangled.
	 */
	MangleFilter( int perPacketPercent, int numPacketsPercent ) :
		Mercury::PacketFilter(),
		perPacketPercent_( perPacketPercent ),
		numPacketsPercent_( numPacketsPercent )
	{
	}

	virtual Mercury::Reason recv( Mercury::PacketReceiver & receiver,
			const Mercury::Address & addr, Mercury::Packet * pPacket,
			Mercury::ProcessSocketStatsHelper * pStatsHelper );

private:
	int perPacketPercent_;
	int numPacketsPercent_;
};

typedef SmartPointer< MangleFilter > MangleFilterPtr;


/**
 *	This method processes a received packet. It randomly decides whether each
 *	packet should be mangled and, if so, it mangles a certain percentage of
 *	the bytes in the packet.
 */
Mercury::Reason MangleFilter::recv( Mercury::PacketReceiver & receiver,
		const Mercury::Address & addr, Mercury::Packet * pPacket,
		Mercury::ProcessSocketStatsHelper * pStatsHelper )
{
	if ((rand() % 100) < numPacketsPercent_)
	{
		for (int i = 0; i < pPacket->bodySize(); ++i)
		{
			if (rand() % 100 < perPacketPercent_)
			{
				(const_cast< char * >( pPacket->body() ))[i] = char( rand() );
			}
		}
	}

	return this->PacketFilter::recv( receiver, addr, pPacket, pStatsHelper );
}


// -----------------------------------------------------------------------------
// Section: MangleServerApp
// -----------------------------------------------------------------------------

/**
 *	This class is used to receive bundles from the client applications. A
 *	certain amount of the received packets are mangled.
 */
class MangleServerApp : public NetworkApp
{
public:
	MangleServerApp( Mercury::EventDispatcher & mainDispatcher, 
			TEST_ARGS_PROTO ) :
		NetworkApp( mainDispatcher, Mercury::NETWORK_INTERFACE_EXTERNAL,
			TEST_ARGS ),
		hasTimedOut_( false ),
		latestEndTime_( 0 ),
		numClients_( 0 )
	{
		// Dodgy singleton code
		MF_ASSERT( s_pInstance == NULL );
		s_pInstance = this;

		MangleServerInterface::registerWithInterface( this->networkInterface() );
		this->networkInterface().setIrregularChannelsResendPeriod( 0.1f );
	}

	~MangleServerApp();

	bool hasRunningClients() const
	{
		return numClients_ > 0;
	}

	int & clientCount()	{ return numClients_; }

	virtual void handleTimeout( TimerHandle handle, void * arg )
	{
		// Run until all client application have finished
		if (!this->hasRunningClients())
		{
			this->dispatcher().breakProcessing();
		}
		else if (timestamp() > latestEndTime_)
		{
			this->dispatcher().breakProcessing();
			hasTimedOut_ = true;
		}
	}

	virtual int run()
	{
		INFO_MSG( "MangleServerApp(%d)::run: started\n", getpid() );

		latestEndTime_ = timestamp() + MAX_RUNNING_TIME * stampsPerSecond();

		this->startTimer( SERVER_TICK_PERIOD );
		this->NetworkApp::run();

		INFO_MSG( "MangleServerApp(%d)::run: finished\n", getpid() );
		return 0;
	}

	// ---- Properties ----
	bool hasTimedOut() const	{ return hasTimedOut_; }

	// ---- Message Handlers ----

	void connect( const Mercury::Address & srcAddr,
			const MangleServerInterface::connectArgs & args );

	void msg1( const Mercury::Address & srcAddr,
			const MangleServerInterface::msg1Args & args );

	void msg2( const Mercury::Address & srcAddr,
			const MangleServerInterface::msg2Args & args );

	// ---- Statics ----

	static MangleServerApp & instance()
	{
		MF_ASSERT( s_pInstance != NULL );
		return *s_pInstance;
	}

private:
	// Whether the run ended with a timeout
	bool hasTimedOut_;

	// The time after which we consider the test has timed out
	uint64 latestEndTime_;

	int numClients_;

	static MangleServerApp * s_pInstance;
};

MangleServerApp * MangleServerApp::s_pInstance = NULL;


/**
 *	Destructor.
 */
MangleServerApp::~MangleServerApp()
{
	MF_ASSERT( s_pInstance == this );
	s_pInstance = NULL;
}

/**
 *	Class for struct-style Mercury message handler objects.
 */
template <class ARGS> class ServerMangleStructMessageHandler :
	public Mercury::InputMessageHandler
{
public:
	typedef void (MangleServerApp::*Handler)( const Mercury::Address & srcAddr,
			const ARGS & args );

	ServerMangleStructMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
	{
		ARGS * pArgs = (ARGS*)data.retrieve( sizeof(ARGS) );
		(MangleServerApp::instance().*handler_)( srcAddr, *pArgs );
	}

	Handler handler_;
};


// -----------------------------------------------------------------------------
// Section: MangleServerApp Message Handlers
// -----------------------------------------------------------------------------

// Magic data sent in messages
const uint32 CONNECT_MAGIC = 0x0708090a;
const uint32 MSG1_MAGIC = 0x01020304;
const uint8 MSG2_MAGIC = 0x06;

/**
 *	This method handles a message from the client informing us to connect.
 */
void MangleServerApp::connect( const Mercury::Address & srcAddr,
		const MangleServerInterface::connectArgs & args )
{
	// If the magic number does not match, this method has probably been called
	// because of a corrupted packet.
	if (args.magic != CONNECT_MAGIC)
	{
		WARNING_MSG( "MangleServerApp::connect: "
				"Incorrect magic number %x != %x\n",
			args.magic, CONNECT_MAGIC );
		return;
	}

	INFO_MSG( "MangleServerApp::connect( %s ): traits = %d\n",
		srcAddr.c_str(), args.traits );

	if (this->networkInterface().findChannel( srcAddr ))
	{
		DEBUG_MSG( "MangleServerApp::connect: Already have channel\n" );
	}
	else
	{
		Mercury::UDPChannel * pChannel =
			new Mercury::UDPChannel( this->networkInterface(), srcAddr, 
				args.traits,
				1.0,
				new MangleFilter( DEFAULT_MANGLE_PER_PACKET_PERCENT,
					DEFAULT_MANGLE_NUM_PACKETS_PERCENT ) );

		// Since we don't store a reference to this channel anywhere, we need
		// to mark it as anonymous so that the EventDispatcher will take
		// ownership of it and destroy it for us.
		pChannel->isAnonymous( true );

		Mercury::Bundle & bundle = pChannel->bundle();
		MangleClientInterface::ackConnectArgs & args =
			MangleClientInterface::ackConnectArgs::start( bundle );
		args.magic = CONNECT_MAGIC;
		pChannel->send();
	}
}


/**
 *	This method handles msg1 messages.
 */
void MangleServerApp::msg1( const Mercury::Address & srcAddr,
		const MangleServerInterface::msg1Args & args )
{
#if 0
	static int total = 0;
	DEBUG_MSG( "MangleServerApp::msg1: Got message from %s/%d\n",
			srcAddr.c_str(), ++total );
#endif
}


/**
 *	This method handles msg2 messages.
 */
void MangleServerApp::msg2( const Mercury::Address & srcAddr,
		const MangleServerInterface::msg2Args & args )
{
#if 0
	static int total = 0;
	DEBUG_MSG( "MangleServerApp::msg2: Got message from %s/%d\n",
			srcAddr.c_str(), ++total );
#endif
}

// -----------------------------------------------------------------------------
// Section: MangleClientApp
// -----------------------------------------------------------------------------

/**
 *	This class is used to send information to server application. Instances of
 *	this class run inside spawned child processes.
 */
class MangleClientApp : public NetworkApp
{
public:

	MangleClientApp( Mercury::EventDispatcher & mainDispatcher, TEST_ARGS_PROTO,
			int & clientCount,
			const Mercury::Address & dstAddr,
			Mercury::UDPChannel::Traits channelTraits = 
				Mercury::UDPChannel::INTERNAL,
			uint32 numBundlesToSend = DEFAULT_NUM_BUNDLES_TO_SEND,
			int minPacketSize = DEFAULT_MIN_BUNDLE_SIZE,
			int maxPacketSize = DEFAULT_MAX_BUNDLE_SIZE ) :
		NetworkApp( mainDispatcher, Mercury::NETWORK_INTERFACE_INTERNAL,
			TEST_ARGS ),
		outSeq_( 0 ),
		numBundlesToSend_( numBundlesToSend ),
		pChannel_( new Mercury::UDPChannel( this->networkInterface(), 
			dstAddr, channelTraits ) ),
		minPacketSize_( minPacketSize ),
		maxPacketSize_( maxPacketSize ),
		isConnectionEstablished_( false ),
		clientCount_( clientCount )
	{
		MF_ASSERT( minPacketSize_ < maxPacketSize_ );

		pChannel_->isLocalRegular( false );
		pChannel_->isRemoteRegular( false );

		TRACE_MSG( "MangleClientApp::MangleClientApp: server is at %s\n",
				dstAddr.c_str() );
		// Dodgy singleton code
		MF_ASSERT( s_pInstance == NULL );
		s_pInstance = this;

		MangleClientInterface::registerWithInterface( 
			this->networkInterface() );

		++clientCount_;
	}

	~MangleClientApp()
	{
		INFO_MSG( "MangleClientApp(%d)::~MangleClientApp: %p\n",
				getpid(), this );
		MF_ASSERT( s_pInstance == this );
		s_pInstance = NULL;
	}


	void start();

	void stop()
	{
		if( this->timerHandle_.isSet() )
		{
			this->stopTimer();
			this->networkInterface().processUntilChannelsEmpty();
			INFO_MSG( "MangleClientApp(%d)::run: Finished\n", getpid() );

			--clientCount_;
			pChannel_->destroy();
		}
	}

	void sendConnect();

	void handleTimeout( TimerHandle handle, void * arg );

	// ---- Message Handlers ----
	void ackConnect( const Mercury::Address & srcAddr,
			const MangleClientInterface::ackConnectArgs & args );

	static MangleClientApp & instance()
	{
		MF_ASSERT( s_pInstance != NULL );
		return *s_pInstance;
	}

private:
	uint32 outSeq_;
	unsigned numBundlesToSend_;
	Mercury::UDPChannel * pChannel_;

	int minPacketSize_;
	int maxPacketSize_;

	bool isConnectionEstablished_;

	int & clientCount_;

	void addMsg1( Mercury::Bundle & bundle );
	void addMsg2( Mercury::Bundle & bundle );

	static MangleClientApp * s_pInstance;
};

MangleClientApp * MangleClientApp::s_pInstance = NULL;


/**
 *	Class for struct-style Mercury message handler objects.
 */
template <class ARGS> class ClientMangleStructMessageHandler :
	public Mercury::InputMessageHandler
{
public:
	typedef void (MangleClientApp::*Handler)( const Mercury::Address & srcAddr,
			const ARGS & args );

	ClientMangleStructMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
	{
		ARGS * pArgs = (ARGS*)data.retrieve( sizeof(ARGS) );
		(MangleClientApp::instance().*handler_)( srcAddr, *pArgs );
	}

	Handler handler_;
};


/**
 *	This method is called to start this application.
 */
void MangleClientApp::start()
{
	const uint tickRate = CLIENT_TICK_PERIOD; // 1 ms
	INFO_MSG( "MangleClientApp(%d)::start: starting timer\n", getpid() );
	this->startTimer( tickRate );
}


/**
 *	This method sends a connect message to the server.
 */
void MangleClientApp::sendConnect()
{
	Mercury::UDPBundle bundle;
	MangleServerInterface::connectArgs & args =
		MangleServerInterface::connectArgs::start( bundle,
				Mercury::RELIABLE_NO );

	args.traits =
		pChannel_->isExternal() ?
			Mercury::UDPChannel::EXTERNAL : Mercury::UDPChannel::INTERNAL;
	args.magic = CONNECT_MAGIC;

	this->networkInterface().send( pChannel_->addr(), bundle );
}


/**
 *	This method adds a msg1 message to the bundle.
 */
void MangleClientApp::addMsg1( Mercury::Bundle & bundle )
{
	MangleServerInterface::msg1Args & args =
		MangleServerInterface::msg1Args::start( pChannel_->bundle() );

	args.magic = MSG1_MAGIC;
}


/**
 *	This method adds a msg2 message to the bundle.
 */
void MangleClientApp::addMsg2( Mercury::Bundle & bundle )
{
	MangleServerInterface::msg2Args & args =
		MangleServerInterface::msg2Args::start( pChannel_->bundle() );

	args.magic = MSG2_MAGIC;
}


/**
 *	The client sends to the server based on a timer.
 */
void MangleClientApp::handleTimeout( TimerHandle handle, void * arg )
{
	if (!isConnectionEstablished_)
	{
		this->sendConnect();
		return;
	}

	// If the send window is fairly full, it is too early to send more data.
	if (pChannel_->sendWindowUsage() * 5 > pChannel_->windowSize())
	{
#if 0
		DEBUG_MSG( "MangleClientApp::handleTimeout: sendWindowUsage = %d\n",
				pChannel_->sendWindowUsage() );
#endif
		return;
	}

	int desiredPacketSize =
		minPacketSize_ + rand() % (maxPacketSize_ - minPacketSize_);

	uint numMessagesSent = 0;
	while (pChannel_->bundle().size() < desiredPacketSize)
	{
		const int NUM_CHOICES = 2;

		switch (rand() % NUM_CHOICES)
		{
		case 0:
			this->addMsg1( pChannel_->bundle() );
			break;

		case 1:
			this->addMsg2( pChannel_->bundle() );
			break;

		// TODO: Add a variable length message

		default:
			MF_ASSERT( 0 );
		}
		++numMessagesSent;
	}

	pChannel_->send();

	if (--numBundlesToSend_ == 0)
	{
		this->stop();
	}
}


/**
 *	This method handles a message from the server indicating that it has
 *	accepted the connection.
 */
void MangleClientApp::ackConnect( const Mercury::Address & srcAddr,
		const MangleClientInterface::ackConnectArgs & args )
{
	isConnectionEstablished_ = true;
}

struct TestMangleFixture
{
	TestMangleFixture()
	{
#ifdef MF_SERVER
		// Initialise stampsPerSecond and timing method
		setenv( "BW_TIMING_METHOD", "gettimeofday", 1 );
		stampsPerSecond();
#endif

		// g_shouldWritePid = true;
		DebugFilter::instance().hasDevelopmentAssertions( false );
		originalOutputFilter_ = DebugFilter::instance().filterThreshold();

		DebugFilter::instance().filterThreshold( MESSAGE_PRIORITY_CRITICAL );
	}

	~TestMangleFixture()
	{
		// TODO: Restore to original state, not assume as true.
		DebugFilter::instance().hasDevelopmentAssertions( true );
		DebugFilter::instance().filterThreshold( originalOutputFilter_ );
		// g_shouldWritePid = false;
	}

	DebugMessagePriority originalOutputFilter_;
};


TEST_F( TestMangleFixture, mangledChannel )
{
	Mercury::EventDispatcher mainDispatcher;
	MangleServerApp serverApp( mainDispatcher, TEST_ARGS );

	MangleClientApp clientApp( mainDispatcher, TEST_ARGS,
					serverApp.clientCount(),
					serverApp.networkInterface().address(),
					Mercury::UDPChannel::EXTERNAL );

	clientApp.start();

	clientApp.networkInterface().shouldUseChecksums( true );
	serverApp.networkInterface().shouldUseChecksums( true );

	serverApp.run();

	clientApp.stop();

	const Mercury::PacketReceiverStats & serverReceiveStats =
		serverApp.networkInterface().receivingStats();


	// We don't fail for timeout because this is expected. We are testing that
	// the server does not crash under bad input from external sources.

	CHECK( serverReceiveStats.numPacketsReceived() > 0 );

	float corruptedPercent = 100 *
		float( serverReceiveStats.numCorruptedPacketsReceived() ) /
			float( serverReceiveStats.numPacketsReceived() );

	HACK_MSG( "Mangle percentage: %u%%, actual corrupted packets: %.01f%%\n",
		DEFAULT_MANGLE_NUM_PACKETS_PERCENT,
		corruptedPercent );

	static const float CHECK_TOLERANCE_PERCENT = 15;
	// Check that we get about what we expect for corrupted received packets
	if (!almostEqual( float( DEFAULT_MANGLE_NUM_PACKETS_PERCENT ),
			corruptedPercent, CHECK_TOLERANCE_PERCENT ))
	{
		char failMessage[64];
		bw_snprintf( failMessage, sizeof(failMessage),
			"Expected %u+/-%.01f%%, got %.01f%%",
			DEFAULT_MANGLE_NUM_PACKETS_PERCENT,
			CHECK_TOLERANCE_PERCENT,
			corruptedPercent );
		FAIL( failMessage );
	}
}

BW_END_NAMESPACE

#define DEFINE_SERVER_HERE
#include "test_mangle_interfaces.hpp"


// test_mangle.cpp
