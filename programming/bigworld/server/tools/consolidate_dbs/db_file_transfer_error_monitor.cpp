#include "db_file_transfer_error_monitor.hpp"

#include "file_receiver.hpp"

#include "file_receiver_mgr.hpp"

#include "cstdmf/timestamp.hpp"

#include "network/event_dispatcher.hpp"

DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
DBFileTransferErrorMonitor::DBFileTransferErrorMonitor( 
			FileReceiverMgr & fileReceiverMgr ) :
		fileReceiverMgr_( fileReceiverMgr ),
		timerHandle_( fileReceiverMgr_.dispatcher().addTimer(
				POLL_INTERVAL_SECS * 1000000, this, NULL,
				"DBFileTransferErrorMonitor" ) ),
		startTime_( timestamp() )
{}


/**
 *	Destructor.
 */
DBFileTransferErrorMonitor::~DBFileTransferErrorMonitor()
{
	timerHandle_.cancel();
}


/**
 *	This method checks that the file transfer operation is going smoothly.
 *	Otherwise it flags it as an error.
 */
void DBFileTransferErrorMonitor::handleTimeout( TimerHandle handle, void * arg )
{
	uint64 now = timestamp();
	bool isTimedOut = false;

	// Check connection timeouts
	if (fileReceiverMgr_.hasUnstartedDBs() &&
			((now - startTime_) >= CONNECT_TIMEOUT_SECS * stampsPerSecond()))
	{
		FileReceiverMgr::SourceDBs unstartedDBs =
				fileReceiverMgr_.getUnstartedDBs();
		for (FileReceiverMgr::SourceDBs::const_iterator i =
				unstartedDBs.begin(); i != unstartedDBs.end(); ++i)
		{
			Mercury::Address addr( i->second, 0 );
			ERROR_MSG( "DBFileTransferErrorMonitor::handleTimeout: Timed out "
				"waiting for transfer of %s from %s to start.\n"
				"Please check transfer_db logs for any errors - they appear "
				"under the Tool process.\n",
				i->first.c_str(), addr.ipAsString() );
		}

		isTimedOut = true;
	}

	// Check inactivity timeouts
	const FileReceiverMgr::ReceiverSet& inProgReceivers =
			fileReceiverMgr_.startedReceivers();
	for ( FileReceiverMgr::ReceiverSet::const_iterator ppReceiver =
			inProgReceivers.begin(); ppReceiver != inProgReceivers.end();
			++ppReceiver )
	{
		if ((now - (*ppReceiver)->lastActivityTime()) >=
				INACTIVITY_TIMEOUT_SECS * stampsPerSecond())
		{
			if ((*ppReceiver)->srcPath().empty())
			{
				ERROR_MSG( "DBFileTransferErrorMonitor::handleTimeout: "
						"File transfer from %s is hung\n",
						(*ppReceiver)->srcAddr().ipAsString() );
			}
			else
			{
				ERROR_MSG( "DBFileTransferErrorMonitor::handleTimeout: "
						"Transfer of file %s from %s is hung\n",
						(*ppReceiver)->srcPath().c_str(),
						(*ppReceiver)->srcAddr().ipAsString() );
			}
			isTimedOut = true;
		}
	}

	if (isTimedOut)
	{
		fileReceiverMgr_.onFileReceiveError();
	}
}

BW_END_NAMESPACE

// db_file_transfer_error_monitor.cpp

