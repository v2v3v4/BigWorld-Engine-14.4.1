#include "pch.hpp"

#include "CppUnitLite2/src/CppUnitLite2.h"
#include "common_interface.hpp"

#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"


BW_BEGIN_NAMESPACE

namespace
{

class LocalHandler : public CommonHandler
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

private:
	Mercury::EventDispatcher & dispatcher_;
};

} // anonymous namespace


TEST( Channel_overflow )
{
	Mercury::EventDispatcher dispatcher;

	Mercury::NetworkInterface fromInterface( &dispatcher,
		Mercury::NETWORK_INTERFACE_INTERNAL );
	Mercury::NetworkInterface toInterface( &dispatcher,
		Mercury::NETWORK_INTERFACE_INTERNAL );

	LocalHandler handler( dispatcher );

	fromInterface.pExtensionData( &handler );
	toInterface.pExtensionData( &handler );

	CommonInterface::registerWithInterface( fromInterface );
	CommonInterface::registerWithInterface( toInterface );

	Mercury::UDPChannelPtr pChannel =
		fromInterface.findChannel( toInterface.address(), true );

	pChannel->isLocalRegular( false );
	pChannel->isRemoteRegular( false );

	const int NUM_SENDS = 2 * pChannel->windowSize() + 1;

	for (int i = 0; i < NUM_SENDS; ++i)
	{
		Mercury::Bundle & bundle = pChannel->bundle();
		CommonInterface::msg1Args & args =
			CommonInterface::msg1Args::start( bundle );
		args.seq = i;
		// The last one indicates "disconnect".
		args.data = (i == NUM_SENDS - 1);
		pChannel->send();
	}

	dispatcher.processUntilBreak();
}

BW_END_NAMESPACE

// test_overflow.cpp
