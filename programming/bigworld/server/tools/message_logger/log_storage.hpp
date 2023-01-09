#ifndef LOG_STORAGE_HPP
#define LOG_STORAGE_HPP

#include "network/logger_message_forwarder.hpp"
#include "server/config_reader.hpp"

#include "categories.hpp"
#include "format_strings.hpp"
#include "hostnames.hpp"
#include "hostnames_validator.hpp"
#include "logger.hpp"
#include "types.hpp"


BW_BEGIN_NAMESPACE

/**
 * Subclasses of LogStorage are used for storing the log data into the device,
 * such as MLDB.
 */
class LogStorage
{
public:
	enum AddLogMessageResult
	{
		LOG_ADDITION_SUCCESS,
		LOG_ADDITION_IGNORED,
		LOG_ADDITION_FAILED
	};

	LogStorage( Logger & logger );

	virtual ~LogStorage();

	// TODO: replace the root parameter with more generic parameter for MongoDB
	virtual bool init( const ConfigReader &config, const char *root ) = 0;
	virtual void tick() = 0;

	virtual bool roll() = 0;
	virtual bool stopLoggingFromComponent( const Mercury::Address &addr ) = 0;
	virtual HostnamesValidatorProcessStatus validateNextHostname() = 0;
	virtual bool setAppInstanceID( const Mercury::Address &addr, int id ) = 0;
	virtual AddLogMessageResult writeLogToDB(
	 	const LoggerComponentMessage & componentMessage,
	 	const Mercury::Address & address, MemoryIStream & inputStream,
	 	const LoggerMessageHeader & header,
		LogStringInterpolator *pHandler,
		MessageLogger::CategoryID categoryID ) = 0;
	virtual FormatStrings * getFormatStrings() = 0;
	virtual Hostnames * getHostnames() = 0;
	virtual HostnamesValidator * getHostnamesValidator() = 0;
	virtual Categories * getCategories() = 0;

	void writeToStdout( bool status );

	virtual AddLogMessageResult addLogMessage(
	 	const LoggerComponentMessage & componentMessage,
	 	const Mercury::Address & address, MemoryIStream & inputStream );

	static Mercury::Reason resolveUID( uint16 uid,
		MessageLogger::IPAddress ipAddress, BW::string &result );

protected:
	bool writeToStdout_;

private:
	Logger &logger_;
};

BW_END_NAMESPACE

#endif // LOG_STORAGE_HPP
