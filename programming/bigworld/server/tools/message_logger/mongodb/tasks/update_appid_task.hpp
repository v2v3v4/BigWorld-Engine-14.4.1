#ifndef UPDATE_APPID_TASK_HPP
#define UPDATE_APPID_TASK_HPP

#include "mongodb_background_task.hpp"

#include "../constants.hpp"
#include "../connection_thread_data.hpp"
#include "../types.hpp"
#include "../user_log_writer.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/debug.hpp"

#include "mongo/bson/bsonobjbuilder.h"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

class UpdateAppIDTask : public MongoDBBackgroundTask
{
public:
	UpdateAppIDTask( const mongo::BSONObj & obj, ServerAppInstanceID appid ) :
	MongoDBBackgroundTask( "UpdateAppIDTask" ),
		uid_( obj.getIntField( COMPONENTS_UID ) ),
		username_( obj.getStringField( COMPONENTS_USERNAME ) ),
		obj_( obj ),
		appid_( appid )
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
			ERROR_MSG( "UpdateAppIDTask::performBackgroundTask: "
					"Unable to get a UserLog for UID %hu.\n", uid_ );

			return;
		}

		BW::string dbName_;
		BW::string collectionName_;
		dbName_ = pUserLogWriter->getUserDBName();
		collectionName_ = dbName_ + "." + COMPONENTS_COLL_NAME;
		// Form the structure of the existing record to be modified
		mongo::BSONObjBuilder originBuilder;
		originBuilder.append( COMPONENTS_KEY_HOST,
					obj_.getIntField( COMPONENTS_KEY_HOST ) );
		originBuilder.append( COMPONENTS_KEY_PID,
					obj_.getIntField( COMPONENTS_KEY_PID ) );
		originBuilder.append( COMPONENTS_KEY_COMPONENT,
					obj_.getIntField( COMPONENTS_KEY_COMPONENT ) );
		mongo::BSONObj existingRecord = originBuilder.obj();

		mongo::BSONObjBuilder objBuilder;
		objBuilder.append( COMPONENTS_KEY_HOST,
					obj_.getIntField( COMPONENTS_KEY_HOST ) );
		objBuilder.append( COMPONENTS_KEY_PID,
					obj_.getIntField( COMPONENTS_KEY_PID ) );
		objBuilder.append( COMPONENTS_KEY_COMPONENT,
					obj_.getIntField( COMPONENTS_KEY_COMPONENT ) );
		objBuilder.append( COMPONENTS_KEY_APPID, appid_ );

		DBTaskStatus status = connectionData.updateRecordInCollection(
			collectionName_,
			existingRecord, objBuilder.obj() );

		if (status == DB_TASK_GENERAL_ERROR )
		{
			ERROR_MSG( "UpdateAppIDTask::performBackgroundTask: "
				"Couldn't update record to value '%d' in collection '%s'\n",
				appid_, collectionName_.c_str() );

			// Unrecoverable. Stop processing immediately.
			connectionData.abortFurtherProcessing();
		}
		else if (status == DB_TASK_CONNECTION_ERROR)
		{
			ERROR_MSG( "UpdateAppIDTask::performBackgroundTask: "
				"Connection error when attempting to update record. "
				"Re-adding task as high priority.\n" );

			// If the DB reconnects, ensure this task is performed before any
			// logs are flushed.
			mgr.addBackgroundTask( this, TaskManager::HIGH );
		}
		else
		{
			DEBUG_MSG( "UpdateAppIDTask::performBackgroundTask: "
				"Collection '%s' Updated record: '%d'\n",
				collectionName_.c_str(), appid_ );
		}
	}

private:
	MessageLogger::UID uid_;
	BW::string username_;
	mongo::BSONObj obj_;
	ServerAppInstanceID appid_;
};

} // namespace MongoDB

BW_END_NAMESPACE

#endif // UPDATE_APPID_TASK_HPP

