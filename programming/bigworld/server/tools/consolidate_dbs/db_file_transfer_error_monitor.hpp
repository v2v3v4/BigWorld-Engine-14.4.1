#ifndef CONSOLIDATE_DBS_DB_FILE_TRANSFER_ERROR_MONITOR_HPP
#define CONSOLIDATE_DBS_DB_FILE_TRANSFER_ERROR_MONITOR_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/timer_handler.hpp"


BW_BEGIN_NAMESPACE

class FileReceiverMgr;

/**
 * 	This class is checks on FileReceiverMgr periodically to see whether there
 * 	are any file transfers that are hung or failed to start.
 */
class DBFileTransferErrorMonitor : public TimerHandler
{
	enum
	{
		POLL_INTERVAL_SECS = 5,
		CONNECT_TIMEOUT_SECS = 30,
		INACTIVITY_TIMEOUT_SECS = 20
	};

public:

	DBFileTransferErrorMonitor( FileReceiverMgr & fileReceiverMgr );

	virtual ~DBFileTransferErrorMonitor();

	// Mercury::TimerExpiryHandler override.
	virtual void handleTimeout( TimerHandle handle, void * arg );

private:
	FileReceiverMgr & 	fileReceiverMgr_;
	TimerHandle			timerHandle_;
	uint64				startTime_;
};

BW_END_NAMESPACE


#endif // CONSOLIDATE_DBS_DB_FILE_TRANSFER_ERROR_MONITOR_HPP

