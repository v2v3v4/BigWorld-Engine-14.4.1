#include "pch.hpp"

#include "CppUnitLite2/src/CppUnitLite2.h"
#include "common_interface.hpp"

#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"
#include "network/channel_finder.hpp"
#include "network/udp_channel.hpp"


BW_BEGIN_NAMESPACE

Mercury::UDPChannel * pFromChannel;
Mercury::UDPChannel * pToChannel;

int g_numSent = 0;
const int NUM_SENDS = 1000;

namespace
{

class LocalHandler : public CommonHandler, public TimerHandler
{
public:
	LocalHandler( Mercury::EventDispatcher & dispatcher ) :
		dispatcher_( dispatcher )
	{
	}

protected:
	virtual void on_msg1( const Mercury::Address & srcAddr,
			const CommonInterface::msg1Args & args )
	{
		if (args.data != 0)
		{
			dispatcher_.breakProcessing();
		}
	}

	virtual void handleTimeout( TimerHandle handle, void * arg )
	{
		if (arg)
		{
			this->recreateChannel( handle, 
								   static_cast<Mercury::UDPChannel **>(arg) );
		}
		else
		{
			this->sendMsg1();
		}
	}

	void recreateChannel( TimerHandle handle, Mercury::UDPChannel ** ppChannel )
	{	
		if (g_numSent == NUM_SENDS)
		{
			// If we don't stop recreating the channels, resends don't get
			// processed because the last send time of unacked packets is reset
			// each time we initFromStream.
			return;
		}

		Mercury::UDPChannel * pOldChannel = *ppChannel;
		Mercury::UDPChannel * pNewChannel =
			new Mercury::UDPChannel( pOldChannel->networkInterface(), 
								  pOldChannel->addr(),
								  Mercury::UDPChannel::INTERNAL, 1.f, NULL, 1 );
		MemoryOStream mos;
		pOldChannel->addToStream( mos );
		pOldChannel->destroy();

		pNewChannel->initFromStream( mos, pNewChannel->addr() );
		pNewChannel->isLocalRegular( false );
		pNewChannel->isRemoteRegular( false );

		*ppChannel = pNewChannel;
	}

	void sendMsg1()
	{
		if (g_numSent < NUM_SENDS)
		{
			Mercury::Bundle & bundle = pFromChannel->bundle();
			CommonInterface::msg1Args & args =
				CommonInterface::msg1Args::start( bundle );
			args.seq = g_numSent;
			// The last one indicates "disconnect".
			args.data = (g_numSent == NUM_SENDS - 1);
			g_numSent++;
			pFromChannel->send();
		}
	}

private:
	Mercury::EventDispatcher & dispatcher_;
};


class MyChannelFinder : public Mercury::ChannelFinder
{
public:
	MyChannelFinder ( Mercury::UDPChannel*& myChannel ):
		myChannel_( myChannel )
	{
	}

	Mercury::UDPChannel* find( Mercury::ChannelID id, 
							const Mercury::Address & srcAddr,
							const Mercury::Packet * pPacket,
							bool & rHasBeenHandled )
	{
		if ( id == 1 )
		{
			rHasBeenHandled = false;
			return myChannel_;
		}
		return NULL;
	}

	Mercury::UDPChannel *& myChannel_;
};


class Alarm : public TimerHandler
{
public:
	Alarm( Mercury::EventDispatcher & dispatcher ) :
			TimerHandler(),
			dispatcher_( dispatcher ),
			didTimeout_( false )
	{}

	virtual ~Alarm()
	{}

	bool didTimeout() const { return didTimeout_; }

	virtual void handleTimeout( TimerHandle handle, void * arg )
	{
		didTimeout_ = true;
		dispatcher_.breakProcessing();
	}

private:
	Mercury::EventDispatcher & dispatcher_;
	bool didTimeout_;
};

} // anonymous namespace



TEST( Channel_stream )
{
	Mercury::EventDispatcher dispatcher;

	Mercury::NetworkInterface fromInterface( &dispatcher,
		Mercury::NETWORK_INTERFACE_INTERNAL );
	Mercury::NetworkInterface toInterface( &dispatcher,
		Mercury::NETWORK_INTERFACE_INTERNAL );

	fromInterface.setIrregularChannelsResendPeriod( 0.15 );
	toInterface.setIrregularChannelsResendPeriod( 0.15 );

	LocalHandler handler( dispatcher );

	fromInterface.pExtensionData( &handler );
	toInterface.pExtensionData( &handler );

	MyChannelFinder fromChannelFinder ( pFromChannel );
	MyChannelFinder toChannelFinder ( pToChannel );
	fromInterface.registerChannelFinder( &fromChannelFinder );
	toInterface.registerChannelFinder( &toChannelFinder );

	CommonInterface::registerWithInterface( fromInterface );
	CommonInterface::registerWithInterface( toInterface );

	pFromChannel = 
		new Mercury::UDPChannel( fromInterface, toInterface.address(),
							  Mercury::UDPChannel::INTERNAL, 1.f, NULL, 1 );
	pFromChannel->isLocalRegular( false );
	pFromChannel->isRemoteRegular( false );

	pToChannel = 
		new Mercury::UDPChannel( toInterface, fromInterface.address(),
							  Mercury::UDPChannel::INTERNAL, 1.f, NULL, 1 );
	pToChannel->isLocalRegular( false );
	pToChannel->isRemoteRegular( false );
	TimerHandle h1 = dispatcher.addTimer( 3000, &handler, &pToChannel );
	TimerHandle h2 = dispatcher.addTimer( 5000, &handler, &pFromChannel );
	TimerHandle h3 = dispatcher.addTimer( 7000, &handler, NULL );

	fromInterface.setLossRatio( 0.1 );

	Alarm alarm( dispatcher );
	TimerHandle h4 = dispatcher.addOnceOffTimer( 60000000, &alarm, NULL );

	dispatcher.processUntilBreak();

	CHECK( !alarm.didTimeout() );

	h1.cancel();
	h2.cancel();
	h3.cancel();
	h4.cancel();

	pFromChannel->destroy();
	pToChannel->destroy();

}

BW_END_NAMESPACE

// test_stream.cpp
