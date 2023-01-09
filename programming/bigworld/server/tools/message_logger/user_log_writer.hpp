#ifndef USER_LOG_WRITER_HPP
#define USER_LOG_WRITER_HPP

#include "cstdmf/smartpointer.hpp"

#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

class UserLogWriter;
typedef SmartPointer< UserLogWriter > UserLogWriterPtr;
typedef BW::map< uint16, UserLogWriterPtr > UserLogs;

BW_END_NAMESPACE


#include "user_log.hpp"
#include "log_string_interpolator.hpp"
#include "mldb/log_component_names.hpp"

#include "network/basictypes.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class LogStorageMLDB;
class LogCommonMLDB;
class UserSegmentWriter;

class UserLogWriter : public UserLog
{
public:
	UserLogWriter( uint16 uid, const BW::string &username );

	bool init( const BW::string rootPath );

	LoggingComponent * findLoggingComponent( const Mercury::Address & addr );
	LoggingComponent * findLoggingComponent(
		const LoggerComponentMessage & msg, const Mercury::Address & addr,
		LogComponentNamesMLDB & logComponents );

	bool addEntry( LoggingComponent * component, LogEntry & entry,
		LogStringInterpolator & handler, MemoryIStream & inputStream,
		LogStorageMLDB * pLogStorage, MessageLogger::NetworkVersion version );

	bool updateComponent( LoggingComponent *component );
	bool removeUserComponent( const Mercury::Address &addr );

	void rollActiveSegment();

	const char *logEntryToString( const LogEntry & entry,
		LogCommonMLDB * pBWLog, const LoggingComponent * component,
		LogStringInterpolator & handler, MemoryIStream & inputStream,
		MessageLogger::NetworkVersion version ) const;

	static bool validateUserComponents( const BW::string & rootPath );

private:
	UserSegmentWriter * getLastSegment();
};

BW_END_NAMESPACE

#endif // USER_LOG_WRITER_HPP
