#ifndef LOG_STORAGE_MONGODB_HPP
#define LOG_STORAGE_MONGODB_HPP

#include "categories.hpp"
#include "connection_info.hpp"
#include "connection_thread_data.hpp"
#include "hostnames.hpp"
#include "format_strings.hpp"
#include "log_component_names.hpp"
#include "types.hpp"
#include "user_log_buffer.hpp"
#include "user_log_writer.hpp"

#include "../hostnames_validator.hpp"
#include "../log_storage.hpp"
#include "../types.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

#include "mongo/client/dbclient.h"


BW_BEGIN_NAMESPACE


namespace MongoDB
{

// Map of Per-user log buffers which will be appended to until they are consumed
// by a flush (ie. copied to a WriteTask).
typedef BW::map< MessageLogger::UID, UserLogBufferPtr > UserLogs;

class ConnectionThreadData;
typedef SmartPointer< ConnectionThreadData > ConnectionThreadDataPtr;

} // namespace MongoDB


typedef BW::map< Mercury::Address, ServerAppInstanceID > AppIDMap;

typedef BW::map< Mercury::Address, mongo::BSONObj > ComponentMap;

class LogStorageMongoDB : public LogStorage, public TimerHandler
{
public:
	LogStorageMongoDB( Logger & logger );
	~LogStorageMongoDB();

	static bool initMongoDBDriver();
	static void shutdownMongoDBDriver();

	bool init( const ConfigReader & config, const char * root );
	bool isConnected();
	void tick();
	bool roll();

	bool setAppInstanceID( const Mercury::Address & addr, 
		ServerAppInstanceID id );
	bool stopLoggingFromComponent( const Mercury::Address & addr );
	HostnamesValidatorProcessStatus validateNextHostname();

	LogStorage::AddLogMessageResult addLogMessage(
		const LoggerComponentMessage & componentMessage,
		const Mercury::Address & address, MemoryIStream & inputStream );

	AddLogMessageResult writeLogToDB(
	 	const LoggerComponentMessage & componentMessage,
	 	const Mercury::Address & address, MemoryIStream & inputStream,
	 	const LoggerMessageHeader & header,
		LogStringInterpolator *pHandler,
		MessageLogger::CategoryID categoryID );

	FormatStrings * getFormatStrings() { return &formatStrings_; }
	Hostnames * getHostnames() { return &hostnames_; }
	Categories * getCategories() { return &categories_; }
	HostnamesValidator * getHostnamesValidator() {	return &hostnames_; }

	static bool connectToDB( mongo::DBClientConnection & connection, 
		MongoDB::ConnectionInfo & connectionInfo );
	static int getShardKey( const LoggerComponentMessage & componentMessage );

private:
	bool initFromConfig( const ConfigReader & config );
	bool initCommonCollections();

	bool initSourcesColl( const MongoDB::CollList & collList );
	bool insertSourceToDB( DebugMessageSource type );
	bool initSeveritiesColl( const MongoDB::CollList & collList );
	bool insertSeverityToDB( DebugMessagePriority level );
	bool initVersionColl( const MongoDB::CollList & collList );
	bool insertVersion();

	mongo::BSONObj createBson(
	 	const LoggerComponentMessage & componentMessage,
	 	const Mercury::Address & address, MemoryIStream & inputStream,
	 	const LoggerMessageHeader & header,	LogStringInterpolator *pHandler,
		MessageLogger::CategoryID categoryID, uint64 timestamp );

	uint64 getCurrentTime();
	uint32 getCounter( uint64 milliseconds );
	ServerAppInstanceID getAppID( const Mercury::Address & addr );
	void parseMessage( LogStringInterpolator *pHandler,
		MemoryIStream & inputStream,
		const Mercury::Address & address,
		MessageLogger::NetworkVersion version,
		BW::string & msg,
		mongo::BSONObj & metadataObj );

	void startFlushTimer();
	uint64 getExpireLogsSeconds();
	void handleTimeout( TimerHandle handle, void * arg );
	LogStorage::AddLogMessageResult flushLogs();

	enum TimeOutType
	{
		TIMEOUT_FLUSH_LOG,
	};

	Logger &logger_;
	TimerHandle flushTimer_;
	uint32 currentBufSize_;

	uint32 maxBufSize_;
	uint32 flushInterval_;
	uint32 expireLogsDays_;

	MongoDB::UserLogs userLogs_;

	BW::string loggerID_;
	BW::string commonDBName_;

	mongo::DBClientConnection conn_;
	MongoDB::ConnectionInfo connectionInfo_;
	MongoDB::ConnectionThreadDataPtr pConnectionThreadData_;
	TaskManager mongoDBTaskMgr_;
	bool canAddLogs_;

	HostnamesMongoDB hostnames_;
	LogComponentNamesMongoDB componentNames_;
	FormatStringsMongoDB formatStrings_;
	CategoriesMongoDB categories_;
	AppIDMap appIDMap_;

	uint64 currentSecond_;
	uint32 currentCounter_;

	ComponentMap componentMap_;
};

BW_END_NAMESPACE


#endif // LOG_STORAGE_MONGODB_HPP
