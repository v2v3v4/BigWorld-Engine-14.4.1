#include "unit_test_lib/multi_proc_test_case.hpp"

#include "cstdmf/timestamp.hpp"
#include "cstdmf/smartpointer.hpp"

#include "network/channel_owner.hpp"
#include "network/interfaces.hpp"
#include "network/network_interface.hpp"
#include "network/nub_exception.hpp"
#include "network/packet_filter.hpp"
#include "network/unit_test/network_app.hpp"

#include "test_fragment_interfaces.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_set.hpp"

#include <string.h>


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Test constants
// -----------------------------------------------------------------------------

/**
 *	How many ticks/iterations clients will run for, (i.e. how many msg1
 *	messages from the client to the server to send)
 */
static const int NUM_ITERATIONS = 100;

/**
 *	The size of the message payloads between the client and server.
 */
static const uint PAYLOAD_SIZE = 8 * 1024;


/**
 *	The tick period in microseconds.
 */
static const long TICK_PERIOD = 100000L;


/**
 *  The loss ratio for sends on channels.
 */
static const float RELIABLE_LOSS_RATIO = 0.1f;



// -----------------------------------------------------------------------------
// Section: FragmentServerApp
// -----------------------------------------------------------------------------


/**
 *	The fragment server application.
 */
class FragmentServerApp : public NetworkApp
{

public:
	FragmentServerApp( Mercury::EventDispatcher & mainDispatcher,
		TestResult & result, const char * testName,
		unsigned payloadSizeBytes,
		unsigned long maxRunTimeMicros );

	~FragmentServerApp();

	int run();

	void connect( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void disconnect( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void channelMsg( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void onceOffMsg( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void handleTimeout( TimerHandle handle, void * /*arg*/ );

	unsigned channelMsgCount() const { return channelMsgCount_; }
	unsigned onceOffMsgCount() const { return onceOffMsgCount_; }

	/**
	 *	Singleton instance accessor.
	 */
	static FragmentServerApp & instance()
	{
		MF_ASSERT( s_pInstance != NULL );
		return *s_pInstance;
	}

private:
	class ConnectedClient :
		public Mercury::ChannelOwner,
		public SafeReferenceCount
	{
	public:
		ConnectedClient( Mercury::NetworkInterface & networkInterface,
				const Mercury::Address & addr,
				Mercury::UDPChannel::Traits traits ) :
			Mercury::ChannelOwner( networkInterface, addr, traits ),
			channelSeqAt_( 0 ),
			onceOffSeqAt_( 0 )
		{
			// We don't send anything to clients
			this->channel().isLocalRegular( false );
			this->channel().isRemoteRegular( false );
		}

		unsigned channelSeqAt_;
		unsigned onceOffSeqAt_;
	};

	typedef SmartPointer< ConnectedClient > ConnectedClientPtr;
	typedef BW::map< Mercury::Address, ConnectedClientPtr > ConnectedClients;
	ConnectedClients clients_;

	void handleMessage( ConnectedClientPtr pClient,
		const char * msgName,
		BinaryIStream & data,
		unsigned * pClientSeqAt,
		unsigned * pServerCount );

	unsigned			channelMsgCount_;
	unsigned			onceOffMsgCount_;
	unsigned 			payloadSize_;
	unsigned long 		maxRunTimeMicros_;

	TimerHandle 	watchTimerHandle_;

	static FragmentServerApp * s_pInstance;
};


/** Singleton instance pointer. */
FragmentServerApp * FragmentServerApp::s_pInstance = NULL;

/**
 *	Constructor.
 *
 *	@param payloadSizeBytes		the length of the payload to send to clients
 *	@param maxRunTimeMicros		the maximum time to run the test for before
 *								aborting and asserting test failure
 */
FragmentServerApp::FragmentServerApp( Mercury::EventDispatcher & mainDispatcher,
		TestResult & result,
		const char * testName,
		unsigned payloadSizeBytes,
		unsigned long maxRunTimeMicros ):
	NetworkApp( mainDispatcher, Mercury::NETWORK_INTERFACE_INTERNAL,
		result, testName ),
	channelMsgCount_( 0 ),
	onceOffMsgCount_( 0 ),
	payloadSize_( payloadSizeBytes ),
	maxRunTimeMicros_( maxRunTimeMicros )
{
	// Dodgy singleton code
	MF_ASSERT( s_pInstance == NULL );
	s_pInstance = this;

	FragmentServerInterface::registerWithInterface( this->networkInterface() );
}


/**
 *	Destructor.
 */
FragmentServerApp::~FragmentServerApp()
{
	watchTimerHandle_.cancel();

	MF_ASSERT( s_pInstance == this );
	s_pInstance = NULL;
}


/**
 *	App run function.
 */
int FragmentServerApp::run()
{
	INFO_MSG( "FragmentServerApp(%d)::run: started\n", getpid() );

	this->startTimer( TICK_PERIOD );

	watchTimerHandle_ = this->dispatcher().addTimer( maxRunTimeMicros_, this );

	this->dispatcher().processContinuously();

	TRACE_MSG( "FragmentServerApp(%d)::run: "
		"Processing until channels empty\n", getpid() );
	this->networkInterface().processUntilChannelsEmpty();

	INFO_MSG( "FragmentServerApp(%d)::run: finished\n",	getpid() );
	return 0;
}


/**
 *	Timeout handler. If the watch timer (configured with maxRunTimeMicros
 *	parameter at construction) goes off, then abort test and assert test
 *	failure. Otherwise, it is a tick timer and so send all connected clients a
 *	msg2 message.
 */
void FragmentServerApp::handleTimeout( TimerHandle timerHandle, void * /*arg*/ )
{
	if (timerHandle == watchTimerHandle_)
	{
		// our watchdog timer has expired
		ASSERT_WITH_MESSAGE( !clients_.empty(),
			"Timer expired but no clients remaining" );
		ConnectedClientPtr pClient = clients_.begin()->second;

		ERROR_MSG( "FragmentServerApp(%d)::handleTimeout: "
			"Max run time (%.1fs) is up (%d sent/%d recvd)\n",
			getpid(), maxRunTimeMicros_ / 1000000.f,
			pClient->channel().numDataUnitsSent(),
			pClient->channelSeqAt_ );

		this->dispatcher().breakProcessing();
	}
}


// -----------------------------------------------------------------------------
// Section: FragmentServerApp Message Handlers
// -----------------------------------------------------------------------------

/**
 *	Clients send this message to establish a channel.
 */
void FragmentServerApp::connect( const Mercury::Address & srcAddr,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data )
{
	TRACE_MSG( "FragmentServerApp(%d)::connect from %s\n",
		getpid(), srcAddr.c_str() );

	if (clients_.find( srcAddr ) != clients_.end())
	{
		// we may already have one - the client spams connect until it gets
		// connectAck
		TRACE_MSG( "FragmentServerApp(%d)::connect(%s): already have channel\n",
			getpid(), srcAddr.c_str() );

		return;
	}

	ConnectedClientPtr pClient = new ConnectedClient( this->networkInterface(),
				srcAddr, Mercury::UDPChannel::EXTERNAL );

	clients_[ srcAddr ] = pClient;
}


/**
 *	Clients disconnect after they have sent a certain number of msg1 messages
 *	to the server. The channel is expected to be destroyed.
 */
void FragmentServerApp::disconnect( const Mercury::Address & srcAddr,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data )
{
	TRACE_MSG( "FragmentServerApp(%d)::disconnect( %s )\n",
		getpid(), srcAddr.c_str() );

	ConnectedClients::iterator iter = clients_.find( srcAddr );

	if (iter != clients_.end())
	{
		clients_.erase( iter );
	}
	else
	{
		ERROR_MSG( "FragmentServerApp(%d)::disconnect( %s ): "
				"unknown address\n",
			getpid(), srcAddr.c_str() );

		return;
	}

	if (clients_.empty())
	{
		TRACE_MSG( "FragmentServerApp(%d)::disconnect: no more clients\n",
			getpid() );

		this->dispatcher().breakProcessing();
	}
}


/**
 *  Handler for channelMsg.
 */
void FragmentServerApp::channelMsg( const Mercury::Address & srcAddr,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data )
{
	ConnectedClientPtr pClient = clients_[ srcAddr ];

	ASSERT_WITH_MESSAGE( pClient != NULL,
		"Got message from unknown address" );

	this->handleMessage( pClient, "channelMsg", data,
		&pClient->channelSeqAt_, &channelMsgCount_ );
}


/**
 *  Handler for onceOffMsg.
 */
void FragmentServerApp::onceOffMsg( const Mercury::Address & srcAddr,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data )
{
	ConnectedClientPtr pClient = clients_[ srcAddr ];

	ASSERT_WITH_MESSAGE( pClient != NULL,
		"Got message from unknown address" );

	this->handleMessage( pClient, "onceOffMsg", data,
		&pClient->onceOffSeqAt_, &onceOffMsgCount_ );
}


/**
 *	Clients send NUM_ITERATIONS messages to the server.  Half of them are sent
 *	on-channel, and the other half off.  All messages are multi-packet, the idea
 *	being that the fragment reassembly code works for both kinds of traffic and
 *	can handle both types simultaneously.
 */
void FragmentServerApp::handleMessage( ConnectedClientPtr pClient,
	const char * msgName,
	BinaryIStream & data,
	unsigned * pClientSeqAt,
	unsigned * pServerCount )
{
	// Each message starts with a sequence number.
	unsigned seq = 0;
	data >> seq;

	TRACE_MSG( "FragmentServerApp(%d)::%s (%s): seq=%u\n",
		getpid(), msgName, pClient->channel().c_str(), seq );

	// Verify message length
	ASSERT_WITH_MESSAGE(
		(unsigned)data.remainingLength() == payloadSize_,
		"Incorrect message size" );

	ASSERT_WITH_MESSAGE( *pClientSeqAt == seq,
		"Got message out of sequence" );

	++(*pClientSeqAt);

	// The message payload should be increasing ints, starting from 1.
	int prev = 0, curr = 0;
	while (data.remainingLength() > 0)
	{
		data >> curr;
		ASSERT_WITH_MESSAGE( curr == prev + 1,
			"Payload incorrect" );
		prev = curr;
	}

	++(*pServerCount);
}


// -----------------------------------------------------------------------------
// Section: FragmentClientApp
// -----------------------------------------------------------------------------

class FragmentClientApp : public NetworkApp
{
public:
	FragmentClientApp( Mercury::EventDispatcher & mainDispatcher,
			TestResult & result, const char * testName,
			const Mercury::Address & dstAddr,
			unsigned payloadSizeBytes,
			unsigned numIterations );

	virtual ~FragmentClientApp();

	bool start();
	void stop();

	void startTest();

	void handleTimeout( TimerHandle handle, void * arg );

	static FragmentClientApp & instance()
	{
		MF_ASSERT( s_pInstance != NULL );
		return *s_pInstance;
	}

private:
	void connect();
	void disconnect();
	void sendMessage( const Mercury::InterfaceElement & ie );
	bool isGood() const { return status_ == Mercury::REASON_SUCCESS; }
	const char * errorMsg() const { return Mercury::reasonToString( status_ ); }

	Mercury::UDPChannel *	pChannel_;
	unsigned			payloadSize_;
	unsigned			channelSeqAt_;
	unsigned			onceOffSeqAt_;
	unsigned 			numIterations_;
	Mercury::Reason		status_;

	static FragmentClientApp * s_pInstance;
};

/** Singleton instance. */
FragmentClientApp * FragmentClientApp::s_pInstance = NULL;


/**
 *	Constructor.
 *
 *	@param dstAddr				the server address
 *	@param payloadSizeBytes		the size of the payload to send to the server
 *								(msg1)
 *	@param numIterations		how many msg1 messages to send to the server
 */
FragmentClientApp::FragmentClientApp( Mercury::EventDispatcher & mainDispatcher,
		TestResult & result, const char * testName,
		const Mercury::Address & dstAddr,
		unsigned payloadSize,
		unsigned numIterations ):
	NetworkApp( mainDispatcher, Mercury::NETWORK_INTERFACE_INTERNAL,
		result, testName ),
	pChannel_( new Mercury::UDPChannel(
			this->networkInterface(), dstAddr, Mercury::UDPChannel::EXTERNAL ) ),
	payloadSize_( payloadSize ),
	channelSeqAt_( 0 ),
	onceOffSeqAt_( 0 ),
	numIterations_( numIterations ),
	status_( Mercury::REASON_SUCCESS )
{
	// Dodgy singleton code
	MF_ASSERT( s_pInstance == NULL );
	s_pInstance = this;
}


/**
 *	Destructor.
 */
FragmentClientApp::~FragmentClientApp()
{
	INFO_MSG( "FragmentClientApp(%d)::~ClientApp: %s\n", getpid(),
			this->networkInterface().address().c_str() );
	MF_ASSERT( s_pInstance == this );
	s_pInstance = NULL;

	if (pChannel_ != NULL)
	{
		pChannel_->destroy();
	}
}


/**
 *	App start function.
 */
bool FragmentClientApp::start()
{
	this->startTimer( TICK_PERIOD );

	// Connect to the server immediately
	this->connect();

	return this->isGood();
}


void FragmentClientApp::stop()
{
	// Condemn our channel and wait till it runs dry
	INFO_MSG( "ClientApp(%d): Processing until channels empty\n",
		getpid() );

	pChannel_->condemn();
	pChannel_ = NULL;
	this->networkInterface().processUntilChannelsEmpty();

	delete this;
}


/**
 *	Attempt to connect to the server.
 */
void FragmentClientApp::connect()
{
	// Send connect unreliably as we don't have a channel yet
	Mercury::UDPBundle bundle;
	bundle.startMessage(
		FragmentServerInterface::connect, Mercury::RELIABLE_NO );

	this->networkInterface().send( pChannel_->addr(), bundle );

	if (this->isGood())
	{
		TRACE_MSG( "FragmentClientApp(%d)::connect: Sent connect\n", getpid() );
	}
	else
	{
		ERROR_MSG( "FragmentClientApp(%d)::connect: "
			"Couldn't connect to server (%s)\n",
			getpid(), this->errorMsg() );
	}
}


/**
 *  This method disconnects from the server.
 */
void FragmentClientApp::disconnect()
{
	pChannel_->bundle().startMessage( FragmentServerInterface::disconnect );
	pChannel_->send();

	if (this->isGood())
	{
		TRACE_MSG( "FragmentClientApp(%d): Disconnected\n", getpid() );
	}
	else
	{
		ERROR_MSG( "FragmentClientApp(%d)::disconnect: "
			"Couldn't disconnect from server (%s)\n",
			getpid(), this->errorMsg() );
	}
}


/**
 *	Timeout handler. This sends one of each message to the server each tick.
 */
void FragmentClientApp::handleTimeout( TimerHandle handle, void * arg )
{
	// Send a message on the channel
	if (this->isGood())
	{
		Mercury::Bundle & bundle = pChannel_->bundle();

		bundle.startMessage( FragmentServerInterface::channelMsg );

		// Stream on sequence number
		bundle << channelSeqAt_;
		++channelSeqAt_;

		// Stream on payload
		for (unsigned i=1; i <= payloadSize_ / sizeof( unsigned ); i++)
		{
			bundle << i;
		}

		// We need to temporarily toggle loss like this because we can't lose
		// any of the unreliable messages.
		this->networkInterface().setLossRatio( RELIABLE_LOSS_RATIO );

		pChannel_->send();

		if (!this->isGood())
		{
			ERROR_MSG( "FragmentClientApp(%d): "
				"Couldn't send channel msg to server (%s)\n",
				getpid(), this->errorMsg() );
		}

		this->networkInterface().setLossRatio( 0.f );
	}

	// Send a once-off message
	if (this->isGood())
	{
		Mercury::UDPBundle bundle;

		bundle.startMessage( FragmentServerInterface::onceOffMsg,
			Mercury::RELIABLE_NO );

		// Stream on sequence number
		bundle << onceOffSeqAt_;
		++onceOffSeqAt_;

		// Stream on payload
		for (unsigned i=1; i <= payloadSize_ / sizeof( unsigned ); i++)
		{
			bundle << i;
		}

		this->networkInterface().send( pChannel_->addr(), bundle );

		if (!this->isGood())
		{
			ERROR_MSG( "FragmentClientApp(%d): "
				"Couldn't send once off msg to server (%s)",
				getpid(), this->errorMsg() );
		}
	}

	if (onceOffSeqAt_ == numIterations_ || !this->isGood())
	{
		this->disconnect();
		this->stopTimer();

		// Note: This will delete this object.
		this->stop();
		// this->dispatcher().breakProcessing();
	}
}


/**
 *	Template class for handling variable length messages.
 */
template <class OBJECT_TYPE>
class VarLenMessageHandler : public Mercury::InputMessageHandler
{
	public:
		/**
		 *	This type is the function pointer type that handles the incoming
		 *	message.
		 */
		typedef void (OBJECT_TYPE::*Handler)( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & stream );

		/**
		 *	Constructor.
		 */
		VarLenMessageHandler( Handler handler ) : handler_( handler ) {}

	private:
		// Override
		virtual void handleMessage( const Mercury::Address & srcAddr,
				Mercury::UnpackedMessageHeader & header,
				BinaryIStream & data )
		{
			OBJECT_TYPE & object = OBJECT_TYPE::instance();
			(object.*handler_)( srcAddr, header, data );
		}

		Handler handler_;
};


TEST( Fragment_children )
{
	const unsigned numChildren = 1;
	const unsigned payloadSizeBytes = PAYLOAD_SIZE;
	const float maxRunTimeSeconds = NUM_ITERATIONS * (TICK_PERIOD / 1000000.) * 3;

	TRACE_MSG( "TestFragment::testChildren: "
			"numChildren = %d, payload = %d \n",
		numChildren, payloadSizeBytes );
	Mercury::EventDispatcher mainDispatcher;
	FragmentServerApp serverApp( mainDispatcher, result_, m_name,
			payloadSizeBytes,
			(unsigned long)(maxRunTimeSeconds * 1000000L) );
	// MultiProcTestCase mp( serverApp );

	for (unsigned i = 0; i < numChildren; ++i)
	{
		FragmentClientApp * pApp = new FragmentClientApp( mainDispatcher,
			result_, m_name,
			serverApp.networkInterface().address(),
			payloadSizeBytes, NUM_ITERATIONS );

		if (!pApp->start())
		{
			ASSERT_WITH_MESSAGE( false, "Failed to start client app." );
			delete pApp;
		}
	}

	// MULTI_PROC_TEST_CASE_WAIT_FOR_CHILDREN( mp );
	serverApp.run();

	INFO_MSG( "TestFragment::testChildren: "
		"Got %u channel msgs, %u once off msgs, expecting %u of each\n",
		serverApp.channelMsgCount(), serverApp.onceOffMsgCount(),
		numChildren * NUM_ITERATIONS );

	ASSERT_WITH_MESSAGE(
		(serverApp.channelMsgCount() >= numChildren * NUM_ITERATIONS) &&
		(serverApp.onceOffMsgCount() >= numChildren * NUM_ITERATIONS),
		"Failed to receive all messages that were expected" );

	ASSERT_WITH_MESSAGE(
		(serverApp.channelMsgCount() == numChildren * NUM_ITERATIONS) &&
		(serverApp.onceOffMsgCount() == numChildren * NUM_ITERATIONS),
		"Received more messages than were expected" );

}


BW_END_NAMESPACE

#define DEFINE_SERVER_HERE
#include "test_fragment_interfaces.hpp"


// test_fragment.cpp
