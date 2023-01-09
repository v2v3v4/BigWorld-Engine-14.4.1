#include "roll_task.hpp"

#include "../connection_thread_data.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/debug.hpp"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

RollTask::RollTask() :
	MongoDBBackgroundTask( "RollTask" )
{
}


void RollTask::performBackgroundTask( TaskManager & mgr,
	ConnectionThreadData & connectionData )
{
	INFO_MSG( "RollTask::performBackgroundTask: Rolling user logs.\n" );

	if (!connectionData.rollUserLogs())
	{
		ERROR_MSG(
			"RollTask::performBackgroundTask: Unable to roll user logs.\n" );
	}
}

} // namespace MongoDB

BW_END_NAMESPACE

// roll_task.cpp
