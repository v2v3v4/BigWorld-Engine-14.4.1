#ifndef RECONNECT_TASK_HPP
#define RECONNECT_TASK_HPP

#include "mongodb_background_task.hpp"

#include "../connection_thread_data.hpp"

#include "cstdmf/bgtask_manager.hpp"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

class ReconnectTask : public MongoDBBackgroundTask
{
public:
	ReconnectTask() :
		MongoDBBackgroundTask( "ReconnectTask" ) {}

protected:
	void performBackgroundTask( TaskManager & mgr, 
		ConnectionThreadData & connectionData ) {}
};

} // namespace MongoDB

BW_END_NAMESPACE

#endif // RECONNECT_TASK_HPP
