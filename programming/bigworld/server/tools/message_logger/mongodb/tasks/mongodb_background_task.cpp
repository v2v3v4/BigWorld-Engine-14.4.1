#include "mongodb_background_task.hpp"

#include "../connection_thread_data.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/debug.hpp"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

void MongoDBBackgroundTask::doBackgroundTask( TaskManager & mgr,
	BackgroundTaskThread * pThread )
{
	MongoDB::ConnectionThreadData & connectionData =
		*static_cast< MongoDB::ConnectionThreadData * >(
			pThread->pData().get() );

	if (connectionData.shouldAbortFurtherProcessing())
	{
		// Do nothing as a previous task has signalled that an unrecoverable
		// error has occurred. All queued tasks will be aborted until the queue
		// is empty and the main thread will exit soon.
		return;
	}
	connectionData.reconnectIfNecessary();

	this->performBackgroundTask( mgr, connectionData );
}


} // namespace MongoDB

BW_END_NAMESPACE

// mongodb_background_task.cpp
