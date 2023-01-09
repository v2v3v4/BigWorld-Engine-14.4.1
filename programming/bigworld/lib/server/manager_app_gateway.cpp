#include "manager_app_gateway.hpp"

#include "cstdmf/watcher.hpp"
#include "network/bundle.hpp"
#include "network/machined_utils.hpp"


BW_BEGIN_NAMESPACE

ManagerAppGateway::ManagerAppGateway( Mercury::NetworkInterface & networkInterface,
			const Mercury::InterfaceElement & retireAppIE ) :
		channel_( networkInterface, Mercury::Address::NONE ),
		retireAppIE_( retireAppIE )
{
	MF_ASSERT( retireAppIE_.lengthStyle() == Mercury::FIXED_LENGTH_MESSAGE );
	MF_ASSERT( retireAppIE_.lengthParam() == 0 );
}


ManagerAppGateway::~ManagerAppGateway()
{}

bool ManagerAppGateway::init( const char * interfaceName, int numRetries, float maxMgrRegisterStagger )
{
	if (!almostZero( maxMgrRegisterStagger ))
	{
		const float MICROSECONDS_IN_SECOND = 1000000.0f;

		// Spread starting time of processes within a tick to avoid possible network peaks during startup
		BWRandom rand( mf_getpid() );
		uint32 delay = static_cast<uint>( rand( 0.0f, maxMgrRegisterStagger ) * MICROSECONDS_IN_SECOND );

		if (delay > 0)
		{
			DEBUG_MSG( "ManagerAppGateway::init: "
					"Manager Registration Stagger mode is active: maxMgrRegisterStagger: %f s. "
					"Delaying process start for %f s.\n",
				maxMgrRegisterStagger, delay / MICROSECONDS_IN_SECOND );

			usleep( delay );
		}
	}

	Mercury::Address addr;

	Mercury::Reason reason =
		Mercury::MachineDaemon::findInterface( interfaceName,
			0, addr, numRetries );

	if (reason == Mercury::REASON_SUCCESS)
	{
		channel_.addr( addr );
	}

	// This channel is irregular until we start the game tick timer.
	this->isRegular( false );

	return reason == Mercury::REASON_SUCCESS;
}


void ManagerAppGateway::retireApp()
{
	Mercury::Bundle & bundle = channel_.bundle();
	bundle.startMessage( retireAppIE_ );
	channel_.send();
}


void ManagerAppGateway::addWatchers( const char * name, Watcher & watcher )
{
	watcher.addChild( name,	Mercury::ChannelOwner::pWatcher(), &channel_ );
}

BW_END_NAMESPACE

// manager_app_gateway.cpp
