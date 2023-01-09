#ifndef SHUTDOWN_HANDLER
#define SHUTDOWN_HANDLER

#include "server/common.hpp"


BW_BEGIN_NAMESPACE

class CellAppMgr;
class CellApp;

/**
 *	This class is used to manage the process of shutting down the CellAppMgr.
 */
class ShutDownHandler
{
public:
	virtual ~ShutDownHandler() {};
	virtual bool isPaused() const						{ return true; }
	virtual void checkStatus()							{};

	virtual void ackBaseApps( ShutDownStage stage ) = 0;
	virtual void ackCellApp( ShutDownStage stage, CellApp & app ) = 0;

	static void start( CellAppMgr & mgr );
};

BW_END_NAMESPACE

#endif // SHUTDOWN_HANDLER
