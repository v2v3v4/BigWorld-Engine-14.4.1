#ifndef ROLL_TASK_HPP
#define ROLL_TASK_HPP

#include "mongodb_background_task.hpp"

#include "../connection_thread_data.hpp"

#include "cstdmf/bgtask_manager.hpp"

BW_BEGIN_NAMESPACE

namespace MongoDB
{

class RollTask : public MongoDBBackgroundTask
{
public:
	RollTask();

protected:
	void performBackgroundTask( TaskManager & mgr,
		ConnectionThreadData & connectionData );
};

} // namespace MongoDB

BW_END_NAMESPACE

#endif // ROLL_TASK_HPP
