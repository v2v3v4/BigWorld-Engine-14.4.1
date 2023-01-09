#ifndef WRITE_APPID_TASK_HPP
#define WRITE_APPID_TASK_HPP

#include "mongodb_background_task.hpp"

#include "../connection_thread_data.hpp"
#include "../types.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/debug.hpp"

#include "mongo/bson/bsonobjbuilder.h"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

// -----------------------------------------------------------------------------
// Section: WriteAppIDTask
// -----------------------------------------------------------------------------

class WriteAppIDTask : public MongoDBBackgroundTask
{
public:
	WriteAppIDTask( MessageLogger::UID uid, const BW::string & userName,
		const mongo::BSONObj & obj ) :
	MongoDBBackgroundTask( "WriteAppIDTask" ),
	uid_( uid ),
	username_( userName ),
	obj_( obj )
	{
	}

protected:
	void performBackgroundTask( TaskManager & mgr,
		ConnectionThreadData & connectionData )
	{
		// Prepare user log writer. Create a new one if it's a new user.
		UserLogWriterPtr pUserLogWriter =
				connectionData.getUserLogWriter( uid_, username_ );

		if (pUserLogWriter == NULL)
		{
			ERROR_MSG( "WriteAppIDTask::performBackgroundTask: "
					"Unable to get a UserLog for UID %hu.\n", uid_ );

			return;
		}

		pUserLogWriter->writeComponent( obj_ );
	}

private:
	MessageLogger::UID uid_;
	BW::string username_;
	mongo::BSONObj obj_;
};

} // namespace MongoDB

BW_END_NAMESPACE

#endif // WRITE_APPID_TASK_HPP

