#ifndef FLUSH_TASK_HPP
#define FLUSH_TASK_HPP

#include "mongodb_background_task.hpp"

#include "../connection_thread_data.hpp"
#include "../types.hpp"
#include "../user_log_buffer.hpp"

#include "../../types.hpp"

#include "cstdmf/bgtask_manager.hpp"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

class FlushTask : public MongoDBBackgroundTask
{
public:
	FlushTask( MessageLogger::UID uid,
			const UserLogBufferPtr & pUserLogBuffer );

protected:
	void performBackgroundTask( TaskManager & mgr,
		ConnectionThreadData & connectionData );

private:
	MessageLogger::UID uid_;
	BW::string username_;
	BSONObjBuffer logBuffer_;
};

} // namespace MongoDB

BW_END_NAMESPACE

#endif // FLUSH_TASK_HPP
