#include "pch.hpp"

#include "unit_test_lib/multi_proc_test_case.hpp"

#include "cstdmf/timestamp.hpp"
#include "cstdmf/slot_tracker.hpp"

#include "network/interfaces.hpp"
#include "network/network_interface.hpp"
#include "network/nub_exception.hpp"
#include "network/packet_filter.hpp"
#include "network/unit_test/network_app.hpp"

#include "test_flood_interfaces.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_set.hpp"

#include <string.h>


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Test constants
// -----------------------------------------------------------------------------

namespace // anonymous
{

/**
 *	How many ticks/iterations clients will run for, (i.e. how many msg1
 *	messages from the client to the server to send)
 */
const int NUM_ITERATIONS = 100;

/**
 *	The size of the message payloads between the client and server (msg1 and
 *	msg2).
 */
const uint PAYLOAD_SIZE = 64 * 1024;


/**
 *	The tick period in microseconds.
 */
const long TICK_PERIOD = 100000L;

class EmptyReplyHandler : public Mercury::ReplyMessageHandler
{
public:
	virtual ~EmptyReplyHandler()
	{}

private:
	virtual void handleMessage( const Mercury::Address & source,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			void * arg )
	{
	}

	virtual void handleException( const Mercury::NubException & exception,
			void * arg )
	{
		ERROR_MSG( "NothingReplyHandler::handleException: %s\n", 
				Mercury::reasonToString( exception.reason() ) );
	}
};

EmptyReplyHandler s_emptyReplyHandler;

} // end namespace (anonymous)

// -----------------------------------------------------------------------------
// Section: FloodServerApp
// -----------------------------------------------------------------------------


/**
 *	The flood server application.
 */
class FloodServerApp : public NetworkApp
{

public:
	FloodServerApp( Mercury::EventDispatcher & mainDispatcher, TEST_ARGS_PROTO,
		unsigned payloadSizeBytes,
		unsigned long maxRunTimeMicros );
	~FloodServerApp();

	virtual int run();


	void connect( const Mercury::Address & srcAddr,
			const FloodServerInterface::connectArgs & args );

	void disconnect( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void msg1( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void handleTimeout( TimerHandle id, void * /*arg*/ );

	/**
	 *	Return the number of msg1 messages we have received from our clients.
	 */
	unsigned getMsg1Count() const { return msg1Count_; }

	/**
	 *	Singleton instance accessor.
	 */
	static FloodServerApp & instance()
	{
		MF_ASSERT( s_pInstance != NULL );
		return *s_pInstance;
	}

private:

	typedef BW::map<Mercury::Address, Mercury::UDPChannel *> Channels;
	Channels			channels_;

	typedef BW::map<Mercury::Address, unsigned> ChannelCount;
	ChannelCount		channelCounts_;

	uint 				msg1Count_;
	unsigned 			payloadSize_;
	unsigned long 		maxRunTimeMicros_;

	TimerHandle 	watchTimerHandle_;

	static FloodServerApp * s_pInstance;
};


/** Singleton instance pointer. */
FloodServerApp * FloodServerApp::s_pInstance = NULL;

/**
 *	Constructor.
 *
 *	@param payloadSizeBytes		the length of the payload to send to clients
 *	@param maxRunTimeMicros		the maximum time to run the test for before
 *								aborting and asserting test failure
 */
FloodServerApp::FloodServerApp( Mercury::EventDispatcher & mainDispatcher, 
			TEST_ARGS_PROTO,
			unsigned payloadSizeBytes,
			unsigned long maxRunTimeMicros ):
		NetworkApp( mainDispatcher, Mercury::NETWORK_INTERFACE_INTERNAL, 
			TEST_ARGS ),
		channels_(),
		channelCounts_(),
		msg1Count_( 0 ),
		payloadSize_( payloadSizeBytes ),
		maxRunTimeMicros_( maxRunTimeMicros )
{
	// Dodgy singleton code
	MF_ASSERT( s_pInstance == NULL );
	s_pInstance = this;

	FloodServerInterface::registerWithInterface( this->networkInterface() );
}


/**
 *	Destructor.
 */
FloodServerApp::~FloodServerApp()
{
	MF_ASSERT( s_pInstance == this );
	s_pInstance = NULL;
	watchTimerHandle_.cancel();
}


/**
 *	App run function.
 */
int FloodServerApp::run()
{
	INFO_MSG( "FloodServerApp(%d)::run: started\n", getpid() );

	this->startTimer( TICK_PERIOD );

	watchTimerHandle_ = this->dispatcher().addTimer( maxRunTimeMicros_, this );

	this->dispatcher().processContinuously();

	// If the test has already failed, bail now
	// if (this->hasFailed())
	if (result_.FailureCount())
	{
		return 1;
	}

	TRACE_MSG( "FloodServerApp(%d)::run: "
		"Processing until channels empty\n", getpid() );
	this->networkInterface().processUntilChannelsEmpty();

	// condemn all the channels (we pass ownership to the mainDispatcher)
	Channels::iterator iChannel = channels_.begin();
	while (iChannel != channels_.end())
	{
		if (!iChannel->second->isCondemned())
		{
			// we should already have condemned it in disconnect
			WARNING_MSG( "FloodServerApp(%d): Condemning channel to %s\n",
				getpid(), iChannel->first.c_str() );
			iChannel->second->condemn();
		}
		++iChannel;
	}

	INFO_MSG( "FloodServerApp(%d)::run: finished (#msg1 = %d)\n",
		getpid(), msg1Count_ );
	return 0;
}


/**
 *	Timeout handler. If the watch timer (configured with maxRunTimeMicros
 *	parameter at construction) goes off, then abort test and assert test
 *	failure. Otherwise, it is a tick timer and so send all connected clients a
 *	msg2 message.
 */
void FloodServerApp::handleTimeout( TimerHandle timerHandle, void * /*arg*/ )
{
	if (timerHandle == watchTimerHandle_)
	{
		// Our watchdog timer has expired
		ERROR_MSG( "FloodServerApp(%d)::handleTimeout: "
				"Max run time (%.1fs) is up\n",
			getpid(), maxRunTimeMicros_ / 1000000.f );

		this->dispatcher().breakProcessing();
		FAIL( "Timed out" );
	}

	// Send to clients that we know are still connected
	Channels::iterator iter = channels_.begin();

	while (iter != channels_.end())
	{
		Channels::iterator oldIter = iter++;
		Mercury::UDPChannel & channel = *oldIter->second;
		Mercury::Bundle & bundle = channel.bundle();

		bundle.startRequest( FloodClientInterface::msg2, & s_emptyReplyHandler, 
			NULL );

		// put the length-prefixed payload on to msg2 bundle
		char buf[1024];
		memset( buf, 'b', 1024 );
		bundle << payloadSize_;
		for (unsigned i = 0; i < payloadSize_; i += 1024)
		{
			bundle.addBlob( buf, MF_MIN( payloadSize_ - i, (unsigned)1024 ) );
		}

		// try and send
		Mercury::Reason reason = Mercury::REASON_SUCCESS;
		try
		{
			channel.send();
		}
		catch (Mercury::NubException & e)
		{
			ERROR_MSG( "FloodServerApp(%d)::handleTimeout: "
					"exception when sending msg2 to %s:%s\n",
				getpid(), channel.c_str(),
				Mercury::reasonToString( e.reason() ) );

			reason = e.reason();
		}

		// if we failed to send, condemn the channel
		if (reason != Mercury::REASON_SUCCESS)
		{
			ERROR_MSG( "FloodServerApp(%d)::handleTimeout: "
				"Disconnecting because could not send msg2 to %s: %s\n",
				getpid(), channel.c_str(),
				Mercury::reasonToString( reason ) );

			// condemn this channel, remove later after loop
			channel.condemn();
			channels_.erase( oldIter );
		}
	}
}


// -----------------------------------------------------------------------------
// Section: FloodServerApp Message Handlers
// -----------------------------------------------------------------------------

/**
 *	Clients send this message to establish a channel.
 */
void FloodServerApp::connect( const Mercury::Address & srcAddr,
		const FloodServerInterface::connectArgs & args )
{
	TRACE_MSG( "FloodServerApp(%d)::connect( %s ): traits = %d\n",
		getpid(), srcAddr.c_str(), args.traits );

	if (this->networkInterface().findChannel( srcAddr ))
	{
		// we may already have one - the client spams connect until it gets
		// connectAck
		TRACE_MSG( "FloodServerApp(%d)::connect(%s): already have channel\n",
			getpid(), srcAddr.c_str() );
		return;
	}

	Mercury::UDPChannel * pChannel =
		new Mercury::UDPChannel( this->networkInterface(), srcAddr, args.traits );

	channels_[ srcAddr ] = pChannel;
	channelCounts_[ srcAddr ] = 0;

	// send the connect ack
	pChannel->bundle().startMessage( FloodClientInterface::connectAck );
	pChannel->send();
}


/**
 *	Clients disconnect after they have sent a certain number of msg1 messages
 *	to the server. The channel is expected to be destroyed.
 */
void FloodServerApp::disconnect( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	TRACE_MSG( "FloodServerApp(%d)::disconnect( %s )\n",
		getpid(), srcAddr.c_str() );

	Channels::iterator iter = channels_.find( srcAddr );

	if (iter != channels_.end())
	{
		iter->second->condemn();
		channels_.erase( iter );
	}
	else
	{
		ERROR_MSG( "FloodServerApp(%d)::disconnect( %s ): "
			"unknown address\n",
			getpid(), srcAddr.c_str() );
	}

	if (!channels_.size())
	{
		TRACE_MSG( "FloodServerApp(%d)::disconnect: no more clients\n",
			getpid() );
		this->dispatcher().breakProcessing();
	}
}


/**
 *	Clients send a certain number of msg1 messages to the server. It contains a
 *	4-byte length prefix and the message payload proper.
 */
void FloodServerApp::msg1( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & /*header*/,
		BinaryIStream & data )
{
	uint seq;
	uint lenBytes;

	data >> seq >> lenBytes;

	TRACE_MSG( "FloodServerApp(%d)::msg1 (%s): seq=%u (len=%u)\n",
			getpid(), srcAddr.c_str(), seq, lenBytes );

	const char * buf =
		reinterpret_cast<const char*>( data.retrieve( lenBytes ) );

	Channels::iterator iChannel = channels_.find( srcAddr );
	if (iChannel == channels_.end())
	{
		ERROR_MSG( "FloodServerApp(%d)::msg1: no channel exists for %s\n",
			getpid(), srcAddr.c_str() );
	}

	// some checks
	ASSERT_WITH_MESSAGE( lenBytes == PAYLOAD_SIZE,
		"Payload length does not match" );

	ASSERT_WITH_MESSAGE( iChannel != channels_.end(),
		"Channel not found" );

	ASSERT_WITH_MESSAGE( channelCounts_[srcAddr] == seq,
		"Got bad sequence number" );


	// verify buf
	const char * pChar = buf;
	while ((unsigned)(pChar - buf) < lenBytes )
	{
		ASSERT_WITH_MESSAGE( *(pChar++) == 'a',
			"Client gave us invalid data" );
	}

	// all is OK

	++msg1Count_;
	++channelCounts_[srcAddr];

	// ack it back to the client
	Mercury::UDPChannel * pChannel = iChannel->second;
	pChannel->bundle().startMessage( FloodClientInterface::msg1Ack );
	pChannel->bundle() << seq;
	pChannel->send();
}

// -----------------------------------------------------------------------------
// Section: FloodServerApp struct message handler
// -----------------------------------------------------------------------------
/**
 *	Class for struct-style Mercury message handler objects for FloodServerApp.
 */
template <class ARGS> class ServerFloodStructMessageHandler :
	public Mercury::InputMessageHandler
{
public:
	typedef void (FloodServerApp::*Handler)( const Mercury::Address & srcAddr,
			const ARGS & args );

	ServerFloodStructMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
	{
		ARGS * pArgs = (ARGS*)data.retrieve( sizeof(ARGS) );
		(FloodServerApp::instance().*handler_)( srcAddr, *pArgs );
	}

	Handler handler_;
};

// -----------------------------------------------------------------------------
// Section: FloodClientApp
// -----------------------------------------------------------------------------

class FloodClientApp : public NetworkApp
{
public:
	FloodClientApp( Mercury::EventDispatcher & mainDispatcher, TEST_ARGS_PROTO,
			const Mercury::Address & dstAddr,
			unsigned payloadSizeBytes,
			unsigned numIterations );

	virtual ~FloodClientApp();

	void start();
	void stop();

	void connectAck( const Mercury::Address & addr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void msg1Ack( const Mercury::Address & addr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data );

	void msg2( const Mercury::Address & addr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data );

	void startTest();

	void handleTimeout( TimerHandle id, void * arg );

	static FloodClientApp & instance()
	{
		MF_ASSERT( s_pInstance != NULL );
		return *s_pInstance;
	}

private:
	void connect();

private:
	Mercury::UDPChannel *	pChannel_;
	Mercury::Address	serverAddr_;
	unsigned 			payloadSize_;
	uint32 				outSeq_;
	unsigned 			numIterations_;
	unsigned			nextExpectedAck_;
	int 				errorStatus_;
	bool				hasConnected_;
	int					connectTriesLeft_;

	static FloodClientApp * s_pInstance;
};

/** Singleton instance. */
FloodClientApp * FloodClientApp::s_pInstance = NULL;


/**
 *	Constructor.
 *
 *	@param dstAddr				the server address
 *	@param payloadSizeBytes		the size of the payload to send to the server
 *								(msg1)
 *	@param numIterations		how many msg1 messages to send to the server
 */
FloodClientApp::FloodClientApp( Mercury::EventDispatcher & mainDispatcher, 
			TEST_ARGS_PROTO,
			const Mercury::Address & dstAddr,
			unsigned payloadSizeBytes,
			unsigned numIterations ):
		NetworkApp( mainDispatcher, Mercury::NETWORK_INTERFACE_INTERNAL,  
			TEST_ARGS ),
		pChannel_( NULL ),
		serverAddr_( dstAddr ),
		payloadSize_( payloadSizeBytes ),
		outSeq_( 0 ),
		numIterations_( numIterations ),
		nextExpectedAck_( 0 ),
		errorStatus_( 0 ),
		hasConnected_( false ),
		connectTriesLeft_( 5 )
{
	INFO_MSG( "FloodClientApp(%p)::ClientApp: server is at %s\n",
			this, dstAddr.c_str() );

	// Dodgy singleton code
	MF_ASSERT( s_pInstance == NULL );
	s_pInstance = this;

	FloodClientInterface::registerWithInterface( this->networkInterface() );
}


/**
 *	Destructor.
 */
FloodClientApp::~FloodClientApp()
{
	INFO_MSG( "FloodClientApp(%d)::~ClientApp\n", getpid() );
	MF_ASSERT( s_pInstance == this );
	s_pInstance = NULL;

	if (pChannel_)
	{
		// condemning channels passes ownership to the mainDispatcher
		pChannel_->condemn();
	}
}


/**
 *	App run function.
 */
void FloodClientApp::start()
{
	INFO_MSG( "FloodClientApp(%d)::run: Starting\n", getpid() );

	this->startTimer( TICK_PERIOD );
}


void FloodClientApp::stop()
{
	this->networkInterface().processUntilChannelsEmpty();
	delete this;
}


/**
 *	Attempt to connect to the server.
 */
void FloodClientApp::connect()
{
	if (!--connectTriesLeft_)
	{
		ERROR_MSG( "FloodClientApp(%d)::connect: Out of connect retries\n",
			getpid() );
		errorStatus_ = 1;
		this->dispatcher().breakProcessing();
		return;
	}

	// send connect unreliably as we don't have a channel yet
	Mercury::UDPBundle bundle;
	FloodServerInterface::connectArgs & args =
		FloodServerInterface::connectArgs::start( bundle,
			Mercury::RELIABLE_NO );

	args.traits = Mercury::UDPChannel::EXTERNAL;

	this->networkInterface().send( serverAddr_, bundle );

	TRACE_MSG( "FloodClientApp(%d)::connect: Sent connect\n", getpid() );

	if (!pChannel_)
	{
		pChannel_ = new Mercury::UDPChannel( this->networkInterface(), serverAddr_,
			Mercury::UDPChannel::EXTERNAL );
	}

}


/**
 *	Timeout handler. This serves to send the msg1 message to the server every
 *	tick.
 */
void FloodClientApp::handleTimeout( TimerHandle id, void * arg )
{
	//TRACE_MSG( "FloodClientApp(%d)::handleTimeout: Sending %d\n",
	//		getpid(), outSeq_ );

	if (!hasConnected_)
	{
		this->connect();
		return;
	}

	Mercury::Bundle & bundle = pChannel_->bundle();

	int outSeq = outSeq_++;

	bundle.startMessage( FloodServerInterface::msg1 );

	// sequence number
	bundle << outSeq;
	// payload length
	bundle << payloadSize_;
	// payload data
	char buf[1024];
	memset( buf, 'a', 1024 );
	for (unsigned i = 0; i < payloadSize_; i += 1024)
	{
		bundle.addBlob( buf, MF_MIN( payloadSize_ - i, (unsigned)1024 ) );
	}

	if (outSeq_ == numIterations_)
	{
		// we've sent enough payloads, stop ticking
		this->stopTimer();
	}

	pChannel_->send();
}


// -----------------------------------------------------------------------------
// Section: Message handlers
// -----------------------------------------------------------------------------

/**
 *	Server sends this message to acknowledge receipt of a connect message.
 */
void FloodClientApp::connectAck( const Mercury::Address & addr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	TRACE_MSG( "FloodClientApp(%d)::connectAck: connect handshake done\n",
		getpid() );
	hasConnected_ = true;
}


/**
 *	Server sends this message to acknowledge receipt of msg1's that we send it.
 */
void FloodClientApp::msg1Ack( const Mercury::Address & addr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	uint seq;
	data >> seq;
	TRACE_MSG( "FloodClientApp(%d)::msg1Ack: seq=%u\n", getpid(), seq );

	if (seq != nextExpectedAck_)
	{
		ERROR_MSG( "FloodClientApp(%d)::msg1Ack: "
				"got ack for unexpected sequence number %u (expecting %u)\n",
			getpid(), seq, nextExpectedAck_ );
		errorStatus_ = 1;
		this->dispatcher().breakProcessing();
		return;
	}
	++nextExpectedAck_;

	if (seq == numIterations_ - 1)
	{
		// we have the last msg1 ack
		TRACE_MSG( "FloodClientApp(%d)::msg1Ack: we're done, disconnecting\n",
			getpid() );
		pChannel_->bundle().startMessage( FloodServerInterface::disconnect );
		pChannel_->send();

		// start shutting down
		this->dispatcher().breakProcessing();
	}
}


/**
 *	Receive a test message from the server containing a large payload. A 4-byte
 *	length prefix is sent, followed by the payload proper (the length is
 *	configured at FloodServerApp's construction time).
 */
void FloodClientApp::msg2( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	static int numMsg2 = 0;
	unsigned lenBytes;
	data >> lenBytes;

	TRACE_MSG( "FloodClientApp(%d)::msg2(%s): #%d (len=%d)\n",
			getpid(), srcAddr.c_str(), numMsg2++, lenBytes );

	const char * buf =
		reinterpret_cast<const char*>( data.retrieve( lenBytes ) );

	if (header.replyID == Mercury::REPLY_ID_NONE)
	{
		header.pChannel->bundle().startReply( header.replyID );
		header.pChannel->send();
	}

	if (lenBytes != PAYLOAD_SIZE)
	{
		ERROR_MSG( "FloodClientApp(%d)::msg2: invalid payload size=%u\n",
			getpid(), lenBytes );
		errorStatus_ = 1;
		this->dispatcher().breakProcessing();
		return;
	}

	// verify the data
	const char * pChar = buf;
	while ((unsigned)(pChar - buf) < lenBytes)
	{
		if (*pChar != 'b')
		{
			ERROR_MSG( "FloodClientApp(%d)::msg2: "
				"invalid message payload, index %td char=%d\n",
				getpid(), pChar - buf, *pChar );
			errorStatus_ = 1;
			this->dispatcher().breakProcessing();
			return;
		}
		++pChar;
	}

	if (data.remainingLength())
	{
		ERROR_MSG( "FloodClientApp(%d)::msg2: "
				"message data has remaining length of %d\n",
			getpid(), data.remainingLength() );
		errorStatus_ = 1;
		this->dispatcher().breakProcessing();
	}
}


/**
 *	Class for struct-style Mercury message handler objects for FloodClientApp.
 */
template <class ARGS> class ClientStructMessageHandler :
	public Mercury::InputMessageHandler
{
public:
	typedef void (FloodClientApp::*Handler)( const Mercury::Address & srcAddr,
			const ARGS & args );

	ClientStructMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
	{
		ARGS * pArgs = (ARGS*)data.retrieve( sizeof(ARGS) );
		(FloodClientApp::instance().*handler_)( srcAddr, *pArgs );
	}

	Handler handler_;
};

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
			OBJECT_TYPE * pObject = &OBJECT_TYPE::instance();

			if (pObject != NULL)
			{
				(pObject ->*handler_)( srcAddr, header, data );
			}
			else
			{
				ERROR_MSG( "VarLenMessageHandler::handleMessage: "
					"Could not find object\n" );
			}
		}

		Handler handler_;
};


// -----------------------------------------------------------------------------
// Section: Tests
// -----------------------------------------------------------------------------


MEMTRACKER_DECLARE( TestFlood_testFlood, "TestFlood_testFlood",
	0 );

TEST( TestFlood_testFlood )
{
	MEMTRACKER_SCOPED( TestFlood_testFlood );

	const unsigned numChildren = 1;

	const float maxRunTimeSeconds = NUM_ITERATIONS *
		(TICK_PERIOD / 1000000.) * 3.0;

	TRACE_MSG( "testFlood: numChildren = %d, payload = %d\n",
		1, PAYLOAD_SIZE );

	Mercury::EventDispatcher mainDispatcher;

	// Create server and child processes
	FloodServerApp serverApp( mainDispatcher, TEST_ARGS,
		PAYLOAD_SIZE,
		(unsigned long)(maxRunTimeSeconds * 1000000L) );

	BW::vector< FloodClientApp * > apps;

	for (unsigned i=0; i < numChildren; i++)
	{
		FloodClientApp * pApp =
			new FloodClientApp( mainDispatcher, TEST_ARGS,
				serverApp.networkInterface().address(),
				PAYLOAD_SIZE, NUM_ITERATIONS );
		apps.push_back( pApp );
		pApp->start();
	}

	// Run all processes and wait for them to terminate.
	// MULTI_PROC_TEST_CASE_WAIT_FOR_CHILDREN( multiProcTestCase );
	serverApp.run();

	INFO_MSG( "testFlood: got %u count, expecting %u\n",
		serverApp.getMsg1Count(),
		numChildren * NUM_ITERATIONS );

	BW::vector< FloodClientApp * >::iterator iter = apps.begin();

	while (iter != apps.end())
	{
		(*iter)->stop();

		++iter;
	}

	ASSERT_WITH_MESSAGE(
		serverApp.getMsg1Count() >= numChildren * NUM_ITERATIONS,
		"Failed to receive all messages that were expected" );

	ASSERT_WITH_MESSAGE(
		serverApp.getMsg1Count() == numChildren * NUM_ITERATIONS,
		"Received more messages than were expected" );
}

BW_END_NAMESPACE

#define DEFINE_SERVER_HERE
#include "test_flood_interfaces.hpp"

// test_flood.cpp
