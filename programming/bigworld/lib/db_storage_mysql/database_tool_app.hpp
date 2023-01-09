#ifndef LIB__DBAPP_MYSQL__DATABASE_TOOL_APP
#define LIB__DBAPP_MYSQL__DATABASE_TOOL_APP

#include "db_storage/db_entitydefs.hpp"

#include "db_storage_mysql/db_config.hpp"
#include "db_storage_mysql/locked_connection.hpp"

#include "db_storage_mysql/mappings/entity_type_mappings.hpp"

#include "network/event_dispatcher.hpp"

#include <memory>


BW_BEGIN_NAMESPACE

namespace Mercury
{
	class EventDispatcher;
}
class LoggerMessageForwarder;
class MySqlLockedConnection;
class SignalHandler;
class WatcherNub;

class DatabaseToolApp
{
public:
	DatabaseToolApp();

	virtual ~DatabaseToolApp();

	virtual void onSignalled( int sigNum )
	{}

protected:
	Mercury::EventDispatcher & dispatcher()
		{ return eventDispatcher_; }

	MySql & connection()
		{ return *(pLockedConn_->connection()); }

	const EntityDefs & entityDefs() const
		{ return entityDefs_; }

	bool init( const char * appName, const char * scriptName,
			bool isVerbose, bool shouldLock,
			const DBConfig::ConnectionInfo & connectionInfo = 
				DBConfig::connectionInfo() );

	bool connect( const DBConfig::ConnectionInfo & connectionInfo, 
		bool shouldLock );
	bool initLogger( const char * appName, const BW::string & loggerID, 
			bool isVerbose );
	bool initScript( const char * componentName );
	bool initEntityDefs();

	void enableSignalHandler( int sigNum, bool enable = true );

	WatcherNub & watcherNub()
		{ return *pWatcherNub_; }

protected:
	virtual MySqlLockedConnection * createMysqlConnection (
		const DBConfig::ConnectionInfo & connectionInfo ) const;
	
private:
	// Member data
	Mercury::EventDispatcher eventDispatcher_;

	std::auto_ptr< SignalHandler > pSignalHandler_;

	std::auto_ptr< WatcherNub >
							pWatcherNub_;
	std::auto_ptr< LoggerMessageForwarder > 
							pLoggerMessageForwarder_;

	std::auto_ptr< MySqlLockedConnection >
							pLockedConn_;
	EntityDefs 				entityDefs_;
};

BW_END_NAMESPACE

#endif // LIB__DBAPP_MYSQL__DATABASE_TOOL_APP
