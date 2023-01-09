#include "pch.hpp"

#include "cstdmf/bw_map.hpp"

#include "network_app.hpp"
#include "test_channel_interfaces.hpp"

#include "network/channel_owner.hpp"
#include "network/endpoint.hpp"
#include "network/network_interface.hpp"
#include "network/udp_channel.hpp"


BW_BEGIN_NAMESPACE


namespace
{
uint g_tickRate = 1000; // 1ms

// The number of messages the client should send to the server.  Also the
// maximum number of messages the server will send to the client.
unsigned NUM_ITERATIONS = 100;
}

// -----------------------------------------------------------------------------
// Section: Peer
// -----------------------------------------------------------------------------

/**
 *  Someone the app is talking to.  Servers can have more than one of these,
 *  clients should only have one.
 */
class Peer : public Mercury::ChannelOwner, public SafeReferenceCount
{
public:
	Peer( Mercury::NetworkInterface & networkInterface,
			const Mercury::Address & addr,
			Mercury::UDPChannel::Traits traits ) :
		Mercury::ChannelOwner( networkInterface, addr, traits ),
		timerHandle_(),
		inSeq_( 0 ),
		outSeq_( 0 )
	{}

	~Peer()
	{
		timerHandle_.cancel();
	}

	Mercury::EventDispatcher & dispatcher()
	{
		return this->channel().networkInterface().mainDispatcher();
	}

	void startTimer( Mercury::EventDispatcher & mainDispatcher, uint tickRate,
			TimerHandler * pHandler )
	{
		timerHandle_ = this->dispatcher().addTimer( tickRate, pHandler, this );
	}

	void sendNextMessage()
	{
		ClientInterface::msg1Args & args =
			ClientInterface::msg1Args::start( this->bundle() );

		args.seq = outSeq_++;
		args.data = 0;

		if (outSeq_ == NUM_ITERATIONS)
		{
			timerHandle_.cancel();
			this->channel().isLocalRegular( false );
			this->channel().isRemoteRegular( false );
		}

		this->send();
	}

	void receiveMessage( uint32 seq, uint32 data );
	void disconnect( uint32 seq );

private:
	TimerHandle timerHandle_;
	uint32 inSeq_;
	uint32 outSeq_;
};

typedef SmartPointer< Peer > PeerPtr;

// -----------------------------------------------------------------------------
// Section: ChannelServerApp
// -----------------------------------------------------------------------------

class ChannelServerApp : public NetworkApp
{
public:
	ChannelServerApp( Mercury::EventDispatcher & mainDispatcher, TEST_ARGS_PROTO ) :
		NetworkApp( mainDispatcher, Mercury::NETWORK_INTERFACE_INTERNAL, 
			TEST_ARGS )
	{
		// Dodgy singleton code
		MF_ASSERT( s_pInstance == NULL );
		s_pInstance = this;

		ServerInterface::registerWithInterface( this->networkInterface() );
	}

	~ChannelServerApp()
	{
		MF_ASSERT( s_pInstance == this );
		s_pInstance = NULL;
	}

	void disconnect( const Mercury::Address & srcAddr,
			const ServerInterface::disconnectArgs & args );

	void msg1( const Mercury::Address & srcAddr,
			const ServerInterface::msg1Args & args );

	static ChannelServerApp & instance()
	{
		MF_ASSERT( s_pInstance != NULL );
		return *s_pInstance;
	}

	typedef BW::map< Mercury::Address, PeerPtr > Peers;

protected:
	void handleTimeout( TimerHandle handle, void * arg )
	{
		PeerPtr pPeer = (Peer*)arg;
		pPeer->sendNextMessage();
	}

private:
	PeerPtr startChannel( const Mercury::Address & addr,
		Mercury::UDPChannel::Traits traits );

	Peers peers_;

	static ChannelServerApp * s_pInstance;
};

ChannelServerApp * ChannelServerApp::s_pInstance = NULL;


/**
 *	Class for struct-style Mercury message handler objects.
 */
template <class ARGS> class ServerStructMessageHandler :
	public Mercury::InputMessageHandler
{
public:
	typedef void (ChannelServerApp::*Handler)(
			const Mercury::Address & srcAddr,
			const ARGS & args );

	ServerStructMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
	{
		ARGS * pArgs = (ARGS*)data.retrieve( sizeof(ARGS) );
		(ChannelServerApp::instance().*handler_)( srcAddr, *pArgs );
	}

	Handler handler_;
};


PeerPtr ChannelServerApp::startChannel( const Mercury::Address & addr,
	Mercury::UDPChannel::Traits traits )
{
	INFO_MSG( "Creating channel to %s\n", addr.c_str() );

	PeerPtr pPeer = new Peer( this->networkInterface(), addr, traits );
	peers_[ addr ] = pPeer;

	pPeer->startTimer( mainDispatcher_, g_tickRate, this );

	return pPeer;
}


// -----------------------------------------------------------------------------
// Section: ChannelServerApp Message Handlers
// -----------------------------------------------------------------------------

void ChannelServerApp::msg1( const Mercury::Address & srcAddr,
		const ServerInterface::msg1Args & args )
{
	PeerPtr pPeer = peers_[ srcAddr ];

	// If this is the first message from this client, connect him now.
	if (pPeer == NULL)
	{
		pPeer = this->startChannel( srcAddr, args.traits );
	}

	pPeer->receiveMessage( args.seq, args.data );
}


void ChannelServerApp::disconnect( const Mercury::Address & srcAddr,
		const ServerInterface::disconnectArgs & args )
{
	Peers::iterator peerIter = peers_.find( srcAddr );

	if (peerIter != peers_.end())
	{
		peerIter->second->disconnect( args.seq );
		peers_.erase( peerIter );

		if (peers_.empty())
		{
			this->dispatcher().breakProcessing();
		}
	}
	else
	{
		ERROR_MSG( "ChannelServerApp::disconnectArgs: "
				"Got message from unknown peer at %s\n",
			srcAddr.c_str() );
	}
}


// -----------------------------------------------------------------------------
// Section: ChannelClientApp
// -----------------------------------------------------------------------------

class ChannelClientApp : public NetworkApp
{
public:
	ChannelClientApp( Mercury::EventDispatcher & mainDispatcher,
			TEST_ARGS_PROTO,
			const Mercury::Address & dstAddr ) :
		NetworkApp( mainDispatcher, Mercury::NETWORK_INTERFACE_INTERNAL, 
			TEST_ARGS ),
		outSeq_( 0 ),
		numToSend_( NUM_ITERATIONS ),
		pChannel_( NULL )
	{
		pChannel_ =
			new Mercury::UDPChannel( this->networkInterface(), dstAddr, 
				Mercury::UDPChannel::INTERNAL );

		// Dodgy singleton code
		MF_ASSERT( s_pInstance == NULL );
		s_pInstance = this;

		// masterNub.add( interface_ );

		ClientInterface::registerWithInterface( this->networkInterface() );
	}

	~ChannelClientApp()
	{
		pChannel_->destroy();
		pChannel_ = NULL;
		MF_ASSERT( s_pInstance == this );
		s_pInstance = NULL;
	}

	void startTest();

	void handleTimeout( TimerHandle handle, void * arg );

	void msg1( const Mercury::Address & srcAddr,
			const ClientInterface::msg1Args & args );

	static ChannelClientApp & instance()
	{
		MF_ASSERT( s_pInstance != NULL );
		return *s_pInstance;
	}

private:
	uint32 outSeq_;
	uint32 numToSend_;
	Mercury::UDPChannel * pChannel_;

	static ChannelClientApp * s_pInstance;
};

ChannelClientApp * ChannelClientApp::s_pInstance = NULL;


/**
 *	Class for struct-style Mercury message handler objects.
 */
template <class ARGS> class ClientStructMessageHandler :
	public Mercury::InputMessageHandler
{
public:
	typedef void (ChannelClientApp::*Handler)(
			const Mercury::Address & srcAddr,
			const ARGS & args );

	ClientStructMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
	{
		ARGS * pArgs = (ARGS*)data.retrieve( sizeof(ARGS) );
		(ChannelClientApp::instance().*handler_)( srcAddr, *pArgs );
	}

	Handler handler_;
};


void ChannelClientApp::startTest()
{
	this->startTimer( g_tickRate );
}

void ChannelClientApp::handleTimeout( TimerHandle handle, void * arg )
{
	ServerInterface::msg1Args & args =
		ServerInterface::msg1Args::start( pChannel_->bundle() );

	args.traits = pChannel_->traits();
	args.seq = outSeq_++;
	args.data = 0;

	if (outSeq_ == numToSend_)
	{
		ServerInterface::disconnectArgs & args =
			ServerInterface::disconnectArgs::start( pChannel_->bundle() );

		args.seq = outSeq_;
		this->stopTimer();
		pChannel_->isLocalRegular( false );
		pChannel_->isRemoteRegular( false );
	}

	pChannel_->send();
}

void ChannelClientApp::msg1( const Mercury::Address & srcAddr,
			const ClientInterface::msg1Args & args )
{
}


// -----------------------------------------------------------------------------
// Section: Peer
// -----------------------------------------------------------------------------

void Peer::receiveMessage( uint32 seq, uint32 data )
{
	MF_ASSERT( inSeq_ == seq );
	inSeq_ = seq + 1;
}

void Peer::disconnect( uint32 seq )
{
	MF_ASSERT( inSeq_ == seq );
}


/**
 *	This method is a simple channel test.
 */
TEST( Channel_testSimpleChannel )
{
	Mercury::EventDispatcher mainDispatcher;
	ChannelServerApp serverApp( mainDispatcher, TEST_ARGS );
	ChannelClientApp clientApp( mainDispatcher, TEST_ARGS, serverApp.networkInterface().address() );

	clientApp.startTest();
	serverApp.run();
}

/**
 *	This method is a channel test with loss.
 */
// This tests fails under windows: http://bugzilla/show_bug.cgi?id=14203
#if 0
#ifdef MF_SERVER
TEST( Channel_testLoss )
{
	float LOSS_RATIO = 0.1f;

	ChannelServerApp serverApp;
	ChannelClientApp clientApp( serverApp.networkInterface().address(),
			serverApp.mainDispatcher() );

	serverApp.networkInterface().setLossRatio( LOSS_RATIO );
	clientApp.networkInterface().setLossRatio( LOSS_RATIO );

	clientApp.startTest();
	serverApp.run();
}
#endif
#endif

BW_END_NAMESPACE


#define DEFINE_SERVER_HERE
#include "test_channel_interfaces.hpp"

// test_channel.cpp
