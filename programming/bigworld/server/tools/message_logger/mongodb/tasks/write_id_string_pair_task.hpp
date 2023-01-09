#ifndef WRITE_ID_STRING_PAIR_TASK_HPP
#define WRITE_ID_STRING_PAIR_TASK_HPP

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
class WriteIDStringPairTask : public MongoDBBackgroundTask
{
public:
	WriteIDStringPairTask( const BW::string & collectionName,
		const char * idKey, const IDType id,
		const char * stringKey, const BW::string & stringValue ) :
	MongoDBBackgroundTask( "WriteIDStringPairTask" ),
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
		mongo::BSONObjBuilder objBuilder;
		objBuilder.append( idKey_, id_ );
		objBuilder.append( stringKey_, stringValue_.c_str() );

		DBTaskStatus status = connectionData.addRecordToCollection(
			collectionName_.c_str(), objBuilder.obj() );

		if (status == DB_TASK_GENERAL_ERROR )
		{
			ERROR_MSG( "WriteIDStringPairTask::performBackgroundTask: "
				"Couldn't insert record '%s' to collection '%s'\n", 
				stringValue_.c_str(), collectionName_.c_str() );

			// Unrecoverable. Stop processing immediately.
			connectionData.abortFurtherProcessing();
		}
		else if (status == DB_TASK_CONNECTION_ERROR)
		{
			ERROR_MSG( "WriteIDStringPairTask::performBackgroundTask: "
				"Connection error when attempting to insert record '%s' to "
				"collection '%s'. Re-adding task as high priority.\n", 
				stringValue_.c_str(), collectionName_.c_str() );

			// If the DB reconnects, ensure this task is performed before any
			// logs are flushed.
			mgr.addBackgroundTask( this, TaskManager::HIGH );
		}
		else
		{
			DEBUG_MSG( "WriteIDStringPairTask::performBackgroundTask: "
				"Collection '%s' New record: %s\n",
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

#endif // WRITE_ID_STRING_PAIR_TASK_HPP

