#ifndef MONGODB_CONNECTION_THREAD_DATA_HPP
#define MONGODB_CONNECTION_THREAD_DATA_HPP

#include "connection_info.hpp"

#include "user_log_writer.hpp"

#include "../types.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/smartpointer.hpp"

#include "mongo/client/dbclient.h"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

enum DBTaskStatus
{
	DB_TASK_SUCCESS,
	DB_TASK_CONNECTION_ERROR,
	DB_TASK_GENERAL_ERROR
};


class UserLogWriter;
typedef SmartPointer< UserLogWriter > UserLogWriterPtr;

/**
 *	This class is used to store data assoicated with background threads.
 */
class ConnectionThreadData : public BackgroundThreadData
{
public:
	ConnectionThreadData( const ConnectionInfo & connectionInfo,
		uint64 expireSeconds, const BW::string & loggerID );

	virtual bool onStart( BackgroundTaskThread & thread );
	virtual void onEnd( BackgroundTaskThread & thread ) {};

	UserLogWriterPtr getUserLogWriter( MessageLogger::UID uid,
		const BW::string & username );
	bool rollUserLogs();

	DBTaskStatus addRecordToCollection( const BW::string & collectionName,
		const mongo::BSONObj & obj );
	DBTaskStatus updateRecordInCollection( const BW::string & collectionName,
		const mongo::BSONObj & existingRecord,
		const mongo::BSONObj & newRecord );

	bool reconnectIfNecessary();
	bool getIsCluster() { return isCluster_; }

	// Thread-safe functions for dealing with disconnects.
	void setConnected( bool connected );
	bool isConnected();
	void stopReconnectAttempts();
	bool shouldAbortReconnect();
	void abortFurtherProcessing();
	bool shouldAbortFurtherProcessing();

	static bool getCollectionNames( mongo::DBClientConnection & conn,
			const BW::string & dbName, CollList & rCollList );

	mongo::DBClientConnection conn_;

private:
	bool initClusterStatus();
	bool initUserLogWriters();

	UserLogWriterPtr onNewUser(  MessageLogger::UID uid,
		const BW::string & username );
	bool getUidFromDB( const BW::string & dbName, MessageLogger::UID & uid );

	typedef BW::map< BW::string, BW::string > UserDbNameMap;
	bool getUserDatabases(
		ConnectionThreadData::UserDbNameMap & userDBNameMap );

	// map of uid and user log database name
	typedef BW::map< MessageLogger::UID, UserLogWriterPtr > UserLogWriterMap;
	UserLogWriterMap userLogWriters_;
	BW::string loggerID_;

	ConnectionInfo connectionInfo_;
	uint64 expireSeconds_;
	bool isCluster_;

	SimpleMutex statusFlagsMutex_;

	bool connectedFlag_;
	bool allowReconnectFlag_;
	bool abortFurtherProcessingFlag_;
};

} // namespace MongoDB

BW_END_NAMESPACE

#endif // MONGODB_CONNECTION_THREAD_DATA_HPP
