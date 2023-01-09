#ifndef USER_LOG_WRITER_MONGODB_HPP
#define USER_LOG_WRITER_MONGODB_HPP

#include "connection_thread_data.hpp"
#include "types.hpp"

#include "../types.hpp"

#include "cstdmf/bgtask_manager.hpp"

#include "mongo/client/dbclient.h"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

class ConnectionThreadData;


class UserLogWriter : public SafeReferenceCount
{
public:
	UserLogWriter( ConnectionThreadData & connectionData,
		MessageLogger::UID uid, const BW::string & userName,
		const BW::string & loggerID_ );

	bool initDB();
	bool flush( BSONObjBuffer & logBuffer );
	bool writeComponent( mongo::BSONObj & obj );
	bool onServerStart( uint64 timestamp );

	bool rollAndPurge( uint64 seconds );
	bool purgeExpiredColls( uint64 seconds );

	const BW::string & getUserName() { return userName_; };
	const BW::string & getUserDBName() { return dbName_; };
private:
	bool initUIDCollection();
	bool initServerStartsColl();
	bool initComponentsColl();
	bool shardDatabase();

	bool initActiveColl();
	bool createLogIndex( const BW::string & logCollNamespace );
	BW::string getNewCollName();
	BW::string getTimeStampSuffix();

	bool getCollList( CollList & collList );
	bool isCollectionExpired( const BW::string & collNS,
			const BW::string & timeSuffix, uint64 seconds );
	bool dropCollection( const BW::string & collNS );

	ConnectionThreadData & connectionData_;

	MessageLogger::UID uid_;
	BW::string userName_;
	BW::string dbName_;
	BW::string activeCollName_;
};

} // namespace MongoDB

BW_END_NAMESPACE

#endif
