#include "pch.hpp"

#include "CppUnitLite2/src/CppUnitLite2.h"
#include "common_interface.hpp"

#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"
#include "network/channel_finder.hpp"
#include "network/udp_channel.hpp"


BW_BEGIN_NAMESPACE

namespace
{

Mercury::UDPChannel * pFromChannel;
Mercury::UDPChannel * pToChannel;

bool g_hasTimedOut = false;
int g_numSent = 0;
int g_numReceived = 0;
const int NUM_SENDS = 1030;

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
		++g_numReceived;
		if (args.data != 0)
		{
			dispatcher_.breakProcessing();
		}
	}

	void handleTimeout( TimerHandle handle, void * arg )
	{
		g_hasTimedOut = true;
		dispatcher_.breakProcessing();
	}

public:
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

} // anonymous namespace



TEST( Channel_auto_switch )
{
	Mercury::EventDispatcher dispatcher;

	Mercury::NetworkInterface baseApp( &dispatcher,
		Mercury::NETWORK_INTERFACE_INTERNAL );
	Mercury::NetworkInterface cellApp1( &dispatcher,
		Mercury::NETWORK_INTERFACE_INTERNAL );
	Mercury::NetworkInterface cellApp2( &dispatcher,
		Mercury::NETWORK_INTERFACE_INTERNAL );

	LocalHandler handler( dispatcher );

	baseApp.pExtensionData( &handler );
	// cellApp1.pExtensionData( &handler );
	// cellApp2.pExtensionData( &handler );

	MyChannelFinder baseAppChannelFinder ( pToChannel );
	baseApp.registerChannelFinder( &baseAppChannelFinder );

	MyChannelFinder cellApp1ChannelFinder ( pFromChannel );
	cellApp1.registerChannelFinder( &cellApp1ChannelFinder );

	CommonInterface::registerWithInterface( cellApp1 );
	CommonInterface::registerWithInterface( cellApp2 );
	CommonInterface::registerWithInterface( baseApp );

	const Mercury::ChannelID CHANNEL_ID = 1;

	pFromChannel = 
		new Mercury::UDPChannel( cellApp1, baseApp.address(),
						  Mercury::UDPChannel::INTERNAL, 1.f, NULL, CHANNEL_ID );
	pFromChannel->isLocalRegular( false );
	pFromChannel->isRemoteRegular( false );

	pToChannel = 
		new Mercury::UDPChannel( baseApp, cellApp1.address(),
						  Mercury::UDPChannel::INTERNAL, 1.f, NULL, CHANNEL_ID );
	pToChannel->isLocalRegular( false );
	pToChannel->isRemoteRegular( false );
	pToChannel->shouldAutoSwitchToSrcAddr( true );

	TimerHandle h1 = dispatcher.addTimer( 5 * 1000000, &handler, &pToChannel );

	handler.sendMsg1();
	cellApp1.dropNextSend();

	for (int i = 0; i < NUM_SENDS - 10; ++i )
	{
		handler.sendMsg1();
	}

	// Move the from channel
	Mercury::UDPChannel * pNewChannel =
		new Mercury::UDPChannel( cellApp2,
						  pFromChannel->addr(),
						  Mercury::UDPChannel::INTERNAL, 1.f, NULL, CHANNEL_ID );
	MyChannelFinder cellApp2ChannelFinder( pNewChannel );
	cellApp2.registerChannelFinder( &cellApp2ChannelFinder );

	MemoryOStream mos;
	pFromChannel->addToStream( mos );
	pFromChannel->reset( Mercury::Address::NONE, false );
	pFromChannel->destroy();

	pNewChannel->initFromStream( mos, pNewChannel->addr() );
	pNewChannel->isLocalRegular( false );
	pNewChannel->isRemoteRegular( false );

	pFromChannel = pNewChannel;

	while (g_numSent < NUM_SENDS)
	{
		handler.sendMsg1();
	}

	dispatcher.processUntilBreak();

	h1.cancel();

	pFromChannel->destroy();
	pToChannel->destroy();

	CHECK_EQUAL( g_numSent, g_numReceived );
	CHECK_EQUAL( NUM_SENDS, g_numSent );
	CHECK( !g_hasTimedOut );
}

BW_END_NAMESPACE

// test_stream.cpp
