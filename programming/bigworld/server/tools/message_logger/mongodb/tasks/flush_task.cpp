#include "flush_task.hpp"

#include "../connection_thread_data.hpp"
#include "../constants.hpp"
#include "../log_storage.hpp"
#include "../types.hpp"
#include "../user_log_buffer.hpp"
#include "../user_log_writer.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/debug.hpp"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

// -----------------------------------------------------------------------------
// Section: FlushTask
// -----------------------------------------------------------------------------

/**
 *  This constructor consumes the dynamic buffer by taking ownership of its
 *  vector content, storing it in a local buffer which will be written to the
 *  database when the task is performed.
 */
FlushTask::FlushTask( MessageLogger::UID uid,
		const UserLogBufferPtr & pUserLogBuffer ) :
	MongoDBBackgroundTask( "FlushTask" ),
	uid_( uid ),
	username_( pUserLogBuffer->getUsername() ),
	logBuffer_()
{
	logBuffer_.reserve( RESERVE_LOG_BUFFER_SIZE );

	// "Consume" the passed-in log buffer by replacing it with an empty one.
	logBuffer_.swap( pUserLogBuffer->logBuffer_ );
}


/**
 *
 */
void FlushTask::performBackgroundTask( TaskManager & mgr,
	ConnectionThreadData & connectionData )
{
	// Prepare user log writer. Create a new one if it's a new user.
	UserLogWriterPtr pUserLogWriter =
			connectionData.getUserLogWriter( uid_, username_ );

	if (pUserLogWriter == NULL)
	{
		ERROR_MSG( "FlushTask::performBackgroundTask: "
				"Unable to get a UserLog for UID %hu.\n", uid_ );

		return;
	}

	pUserLogWriter->flush( logBuffer_ );
}

} // namespace MongoDB

BW_END_NAMESPACE

// flush_task.cpp
