#include "add_server_start_task.hpp"

#include "../connection_thread_data.hpp"
#include "../types.hpp"
#include "../user_log_writer.hpp"

#include "../../types.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/debug.hpp"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

// -----------------------------------------------------------------------------
// Section: AddServerStartTask
// -----------------------------------------------------------------------------

/**
 *  The creation of this task consumes the passed-in log buffer, making it so
 *  that the contained BSON objects should belong only to this task. 
 */
AddServerStartTask::AddServerStartTask( MessageLogger::UID uid,
		const BW::string & username, uint64 serverStartTime ) :
	MongoDBBackgroundTask( "AddServerStartTask" ),
	uid_( uid ),
	username_( username ),
	serverStartTime_( serverStartTime )
{
}


/**
 *
 */
void AddServerStartTask::performBackgroundTask( TaskManager & mgr,
	ConnectionThreadData & connectionData )
{
	// Prepare user log writer. Create a new one if it's a new user.
	UserLogWriterPtr pUserLogWriter =
			connectionData.getUserLogWriter( uid_, username_ );

	if (pUserLogWriter == NULL)
	{
		ERROR_MSG( "AddServerStartTask::performBackgroundTask: "
			"Unable to get a UserLog for UID %hu.\n", uid_ );

		return;
	}

	pUserLogWriter->onServerStart( serverStartTime_ );
}

} // namespace MongoDB

BW_END_NAMESPACE

// add_server_start_task.cpp
