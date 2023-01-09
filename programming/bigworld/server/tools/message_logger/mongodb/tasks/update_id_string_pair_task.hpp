#ifndef UPDATE_ID_STRING_PAIR_TASK_HPP
#define UPDATE_ID_STRING_PAIR_TASK_HPP

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

template< typename IDType >
class UpdateIDStringPairTask : public MongoDBBackgroundTask
{
public:
	UpdateIDStringPairTask( const BW::string & collectionName,
		const char * idKey, const IDType id,
		const char * stringKey, const BW::string & stringValue ) :
	MongoDBBackgroundTask( "UpdateIDStringPairTask" ),
		collectionName_( collectionName ),
		idKey_( idKey ),
		id_( id ),
		stringKey_( stringKey ),
		stringValue_( stringValue )
	{
	}

protected:
	void performBackgroundTask( TaskManager & mgr,
		ConnectionThreadData & connectionData )
	{
		// Form the structure of the existing record to be modified
		mongo::BSONObj existingRecord = mongo::BSONObjBuilder().append(
				idKey_, id_ ).obj();

		mongo::BSONObjBuilder objBuilder;
		objBuilder.append( idKey_, id_ );
		objBuilder.append( stringKey_, stringValue_.c_str() );

		DBTaskStatus status = connectionData.updateRecordInCollection(
			collectionName_.c_str(), existingRecord, objBuilder.obj() );

		if (status == DB_TASK_GENERAL_ERROR )
		{
			ERROR_MSG( "UpdateIDStringPairTask::performBackgroundTask: "
				"Couldn't update record to value '%s' in collection '%s'\n",
				stringValue_.c_str(), collectionName_.c_str() );

			// Unrecoverable. Stop processing immediately.
			connectionData.abortFurtherProcessing();
		}
		else if (status == DB_TASK_CONNECTION_ERROR)
		{
			ERROR_MSG( "UpdateIDStringPairTask::performBackgroundTask: "
				"Connection error when attempting to update record. "
				"Re-adding task as high priority.\n" );

			// If the DB reconnects, ensure this task is performed before any
			// logs are flushed.
			mgr.addBackgroundTask( this, TaskManager::HIGH );
		}
		else
		{
			DEBUG_MSG( "UpdateIDStringPairTask::performBackgroundTask: "
				"Collection '%s' Updated record: '%s'\n",
				collectionName_.c_str(), stringValue_.c_str() );
		}
	}

private:
	BW::string collectionName_;
	const char * idKey_;
	IDType id_;
	const char * stringKey_;
	BW::string stringValue_;
};

} // namespace MongoDB

BW_END_NAMESPACE

#endif // UPDATE_ID_STRING_PAIR_TASK_HPP

