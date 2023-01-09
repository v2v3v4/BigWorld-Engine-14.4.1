#include "dbapp.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/watcher.hpp"

#include "db_storage/db_status.hpp"

#include "network/basictypes.hpp"
#include "network/machine_guard.hpp"
#include "network/watcher_nub.hpp"

#include "cstdmf/bw_string.hpp"

DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )

BW_BEGIN_NAMESPACE

DBApp::DBApp( WatcherNub & watcherNub ) :
	watcherNub_( watcherNub )
{
}


/**
 *	Initialise this DBApp instance.
 */
bool DBApp::init()
{
	ProcessStatsMessage	psm;
	psm.param_ = ProcessMessage::PARAM_USE_CATEGORY |
				ProcessMessage::PARAM_USE_UID |
				ProcessMessage::PARAM_USE_NAME;
	psm.category_ = psm.WATCHER_NUB;
	psm.uid_ = getUserId();
	psm.name_ = "dbapp";

	// onProcessStatsMessage() will be called inside sendAndRecv().
	if (psm.sendAndRecv( 0, BROADCAST, this ) != Mercury::REASON_SUCCESS)
	{
		ERROR_MSG( "DBApp::DBApp: Unable to query BWMachined.\n" );
		return false;
	}
	else if (addr_.isNone())
	{
		INFO_MSG( "DBApp::DBApp: No DBApp process found.\n" );
		return false;
	}

	return true;
}


/**
 * 	Sets DBApp detailed status watcher.
 */
void DBApp::setStatus( const BW::string & status )
{
	if (addr_.isNone())
	{
		return;
	}

	MemoryOStream	strm( status.size() + 32 );
	// Stream on WatcherDataMsg
	strm << int( WATCHER_MSG_SET2 ) << int( 1 ); // message type and count
	strm << uint32( 0 );	// Sequence number. We don't care about it.
	// Add watcher path
	strm.addBlob( DBSTATUS_WATCHER_STATUS_DETAIL_PATH,
			strlen( DBSTATUS_WATCHER_STATUS_DETAIL_PATH ) + 1 );
	// Add data
	strm << uchar( WATCHER_TYPE_STRING );
	strm << status;

	watcherNub_.udpSocket().sendto( strm.data(), strm.size(),
			addr_.port, addr_.ip );
}


/**
 *	This method is called to provide us with information about the DBApp
 * 	running on our cluster.
 */
bool DBApp::onProcessStatsMessage( ProcessStatsMessage & psm, uint32 addr )
{
	// DBApp not found on the machine
	if (psm.pid_ == 0)
	{
		return true;
	}

	if (addr_.isNone())
	{
		addr_.ip = addr;
		addr_.port = psm.port_;

		TRACE_MSG( "DBApp::onProcessStatsMessage: "
				"Found DBApp at %s\n",
			addr_.c_str() );
	}
	else
	{
		Mercury::Address dbAppAddr( addr, psm.port_ );

		WARNING_MSG( "DBConsolidator::onProcessStatsMessage: "
				"Already found a DBApp. Ignoring DBApp at %s\n",
			dbAppAddr.c_str() );
	}

	return true;
}

BW_END_NAMESPACE

// dbapp.cpp
