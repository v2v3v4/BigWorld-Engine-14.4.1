// This test simulates an entity channel during baseapp death and restoring a
// base entity channel.
// It sends a message backwards and forwards between a "CellApp" and "BaseApp1"
// and then switches to BaseApp2 and does the same thing.

#include "pch.hpp"

#include "CppUnitLite2/src/CppUnitLite2.h"
#include "common_interface.hpp"

#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"
#include "network/channel_finder.hpp"


BW_BEGIN_NAMESPACE

namespace
{
const Mercury::ChannelID CHANNEL_ID = 1;
const uint32 MAX_SEQ = 5;

class MyChannelFinder : public Mercury::ChannelFinder
{
public:
	MyChannelFinder ( Mercury::UDPChannel *& rpChannel ):
		rpChannel_( rpChannel )
	{
	}

	virtual ~MyChannelFinder() {}

	virtual Mercury::UDPChannel * find( Mercury::ChannelID id,
							const Mercury::Address & srcAddr,
							const Mercury::Packet * pPacket,
							bool & rHasBeenHandled )
	{
		if (id == CHANNEL_ID)
		{
			rHasBeenHandled = false;
			return rpChannel_;
		}
		return NULL;
	}

private:
	Mercury::UDPChannel *& rpChannel_;
};

class App;

class MetaHandler
{
public:
	virtual void onAppMsg( App * pApp, uint32 seq, uint32 data ) = 0;
};

class App : public CommonHandler
{
public:
	App( Mercury::EventDispatcher & dispatcher, MetaHandler & metaHandler ) :
		interface_( &dispatcher, Mercury::NETWORK_INTERFACE_INTERNAL ),
		channelFinder_( pChannel_ ),
		pChannel_( NULL ),
		metaHandler_( metaHandler )
	{
		CommonInterface::registerWithInterface( interface_ );
		interface_.pExtensionData( this );
		interface_.registerChannelFinder( &channelFinder_ );
	}

	~App()
	{
		if (pChannel_)
		{
			pChannel_->destroy();
			pChannel_ = NULL;
		}
	}

	void initChannel( const Mercury::Address & addr,
			bool shouldAutoSwitchToSrcAddr = false )
	{
		MF_ASSERT( !pChannel_ );
		pChannel_ = new Mercury::UDPChannel( interface_, addr,
						  Mercury::UDPChannel::INTERNAL, 1.f, NULL, CHANNEL_ID );
		pChannel_->isLocalRegular( false );
		pChannel_->isRemoteRegular( false );

		if (shouldAutoSwitchToSrcAddr)
		{
			pChannel_->shouldAutoSwitchToSrcAddr( true );
		}
	}

	void resetChannel( const Mercury::Address & addr )
	{
		pChannel_->reset( addr );
	}

	const Mercury::Address & address() const
	{
		return interface_.address();
	}

	void sendMsg1( uint32 seq, uint32 data ) const
	{
		Mercury::Bundle & bundle = pChannel_->bundle();
		CommonInterface::msg1Args & args =
			CommonInterface::msg1Args::start( bundle );
		args.seq = seq;
		args.data = data;
		pChannel_->send();
	}

	virtual void on_msg1( const Mercury::Address & srcAddr,
			const CommonInterface::msg1Args & args )
	{
		metaHandler_.onAppMsg( this, args.seq, args.data );
	}

private:
	Mercury::NetworkInterface interface_;
	MyChannelFinder channelFinder_;
	Mercury::UDPChannel * pChannel_;
	MetaHandler & metaHandler_;
};


class LocalHandler : public MetaHandler, public TimerHandler
{
public:
	LocalHandler( Mercury::EventDispatcher & dispatcher ) :
		dispatcher_( dispatcher ),
		cellApp_( dispatcher, *this ),
		baseApp1_( dispatcher, *this ),
		baseApp2_( dispatcher, *this ),
		isOkay_( true )
	{
		cellApp_.initChannel( baseApp1_.address() );
		baseApp1_.initChannel( cellApp_.address(), true );
	}

	bool run()
	{
		// Set max running time
		TimerHandle timerHandle =
			dispatcher_.addTimer( 5000000, this, NULL );

		cellApp_.sendMsg1( 0, 0 );

		dispatcher_.processUntilBreak();

		timerHandle.cancel();

		return isOkay_;
	}

protected:
	virtual void onAppMsg( App * pApp, uint32 seq, uint32 data )
	{
		const bool isFromCellApp = (data == 0);

		if (isFromCellApp)
		{
			// Send reply to CellApp.
			pApp->sendMsg1( seq, 1 );

			if ((pApp == &baseApp1_) && (seq == MAX_SEQ))
			{
				baseApp2_.initChannel( cellApp_.address(), true );
				cellApp_.resetChannel( baseApp2_.address() );
				cellApp_.sendMsg1( 0, 0 );
			}
		}
		else
		{
			if (seq == MAX_SEQ)
			{
				dispatcher_.breakProcessing();
			}
			else
			{
				pApp->sendMsg1( seq + 1, 0 );
			}
		}
	}

	void handleTimeout( TimerHandle handle, void * arg )
	{
		ERROR_MSG( "LocalHandler::handleTimeout: Test has timed out\n" );
		isOkay_ = false;
		dispatcher_.breakProcessing();
	}

private:
	Mercury::EventDispatcher & dispatcher_;
	App cellApp_;
	App baseApp1_;
	App baseApp2_;

	bool isOkay_;
};

} // anonymous namespace

TEST( Channel_baseapp_death )
{
	Mercury::EventDispatcher dispatcher;
	LocalHandler handler( dispatcher );

	bool isOkay = handler.run();

	CHECK( isOkay );
}

BW_END_NAMESPACE

// test_stream.cpp
