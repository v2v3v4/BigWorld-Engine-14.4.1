#ifndef ADD_SERVER_START_TASK_HPP
#define ADD_SERVER_START_TASK_HPP

#include "mongodb_background_task.hpp"

#include "../connection_thread_data.hpp"
#include "../types.hpp"

#include "../../types.hpp"

#include "cstdmf/bgtask_manager.hpp"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

class AddServerStartTask : public MongoDBBackgroundTask
{
public:
	AddServerStartTask( MessageLogger::UID uid, const BW::string & username,
		uint64 serverStartTime );

protected:
	void performBackgroundTask( TaskManager & mgr,
		ConnectionThreadData & connectionData );

private:
	MessageLogger::UID uid_;
	BW::string username_;
	uint64 serverStartTime_;
};

} // namespace MongoDB

BW_END_NAMESPACE

#endif // ADD_SERVER_START_TASK_HPP
