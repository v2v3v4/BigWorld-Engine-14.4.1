#ifndef MONGODB_BACKGROUND_TASK_HPP
#define MONGODB_BACKGROUND_TASK_HPP

#include "../connection_thread_data.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/debug.hpp"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

class MongoDBBackgroundTask : public BackgroundTask
{
public:
	MongoDBBackgroundTask( const char * name ) : BackgroundTask( name ) {}

protected:
	void doBackgroundTask( TaskManager & mgr, BackgroundTaskThread * pThread );

	void doBackgroundTask( TaskManager & mgr )
	{
		ERROR_MSG( "BackgroundTask::doBackgroundTask: "
			"erroneous call to function without pThread argument\n" );
	}

	virtual void performBackgroundTask( TaskManager & mgr,
		ConnectionThreadData & connectionData ) = 0;
};

} // namespace MongoDB

BW_END_NAMESPACE

#endif // MONGODB_BACKGROUND_TASK_HPP
