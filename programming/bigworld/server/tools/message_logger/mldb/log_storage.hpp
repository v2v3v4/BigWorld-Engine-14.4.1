#ifndef LOG_STORAGE_MLDB_HPP
#define LOG_STORAGE_MLDB_HPP

#include "log_common.hpp"
#include "hostnames_validator.hpp"
#include "../active_files.hpp"
#include "../logger.hpp"
#include "../log_storage.hpp"
#include "../unary_integer_file.hpp"
#include "../user_log_writer.hpp"

#include "server/config_reader.hpp"
#include "network/basictypes.hpp"
#include "network/logger_message_forwarder.hpp"


BW_BEGIN_NAMESPACE

class LogStorageMLDB : public LogCommonMLDB, public LogStorage
{
public:
	LogStorageMLDB( Logger & logger );
	~LogStorageMLDB();

	bool onUserLogInit( uint16 uid, const BW::string &username );

	bool init( const ConfigReader &config, const char *root );
	void tick() {}

	bool roll();
	bool setAppInstanceID( const Mercury::Address &addr, int id );
	bool stopLoggingFromComponent( const Mercury::Address &addr );

	AddLogMessageResult writeLogToDB(
		const LoggerComponentMessage & componentMessage,
		const Mercury::Address & address,
		MemoryIStream & inputStream, const LoggerMessageHeader & header,
		LogStringInterpolator *pHandler, MessageLogger::CategoryID categoryID );

	FormatStringsMLDB * getFormatStrings() { return &formatStrings_; }
	HostnamesMLDB * getHostnames() { return &hostnames_; }
	HostnamesValidatorMLDB * getHostnamesValidator() {
		return pHostnamesValidator_; }
	CategoriesMLDB * getCategories() { return &categories_; }

	bool updateActiveFiles();
	void deleteActiveFiles();

	int getMaxSegmentSize() const;

	HostnamesValidatorProcessStatus validateNextHostname();

private:
	bool initFromConfig( const ConfigReader &config );

	UserLogWriterPtr createUserLog( uint16 uid, const BW::string &username );
	UserLogWriterPtr getUserLog( uint16 uid );

	LoggingComponent* findLoggingComponent( const Mercury::Address &addr );

	bool initValidatedHostnames();
	bool cleanupTempHostnames( const char *tempFilename ) const;
	bool restoreBackupHostnames( const char *backupFilename,
		const char *restoreToFilename ) const;
	BW::string getBackupHostnamesFile() const;
	bool relinkValidatedHostnames();
	void cleanupValidatedHostnames();

	// The maximum size allowed for UserLog segment files (in bytes)
	int maxSegmentSize_;

	BW::string logDir_;

	UnaryIntegerFile lock_;

	// Both LogStorageMLDB and LogReaderMLDB need a UserLog list, however they both
	// handle the descruction of these lists differently so are kept in their
	// respective subclasses.
	UserLogs userLogs_;

	// The ActiveFiles contains a pointer to the UserLogs so it
	// is able to update itself whenever it's necessary.
	ActiveFiles activeFiles_;

	HostnamesValidatorMLDB *pHostnamesValidator_;
};

BW_END_NAMESPACE

#endif // LOG_STORAGE_MLDB_HPP
