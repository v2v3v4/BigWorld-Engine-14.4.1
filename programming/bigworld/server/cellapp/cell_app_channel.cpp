#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "cell_app_channel.hpp"
#include "cellapp.hpp"
#include "cellapp_config.hpp"

#include "network/event_dispatcher.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: CellAppChannel
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
CellAppChannel::CellAppChannel( const Mercury::Address & addr ) :
	Mercury::ChannelOwner( CellApp::instance().interface(), addr ),
	mark_( 0 ),
	lastReceivedTime_( 0 )
{
	TRACE_MSG( "CellAppChannel::CellAppChannel: Created to %s\n",
		this->channel().c_str() );

	this->channel().isLocalRegular( true );
	this->channel().isRemoteRegular( false );
}

BW_END_NAMESPACE

// cell_app_channel.cpp
