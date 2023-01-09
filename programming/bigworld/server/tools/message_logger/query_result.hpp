#ifndef QUERY_RESULT_HPP
#define QUERY_RESULT_HPP

#include "user_log.hpp"
#include "mldb/log_common.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 * This is the Python object returned from a Query.
 */
class QueryResult
{
public:
	QueryResult();
	QueryResult( const LogEntry &entry, LogCommonMLDB *pBWLog,
		const UserLog *pUserLog, const LoggingComponent *pComponent,
		const BW::string & message, const BW::string & metadata );

	// Static buffer used for generating result output.
	static char s_linebuf_[];

	const char *format( unsigned flags = SHOW_ALL, int *pLen = NULL );
	const BW::string & metadata() const;

	const BW::string & getMessage() const;
	MessageLogger::FormatStringOffsetId getStringOffset() const;
	double getTime() const;
	const char * getHost() const;
	int getPID() const;

	MessageLogger::AppInstanceID getAppInstanceID() const;

	const char * getUsername() const;
	const char * getComponent() const;
	int getSeverity() const;

private:
	double time_;
	BW::string host_;
	int pid_;
	MessageLogger::AppInstanceID appInstanceID_;
	const char * username_;
	const char * component_;

	const char * categoryName_;

	DebugMessageSource messageSource_;
	int severity_;

	// message_ and metadata_ need their own memory since an outer context may
	// well overwrite the buffer provided during construction
	BW::string message_;
	BW::string metadata_;

	// We also store the format string offset as this makes computing
	// histograms faster.
	MessageLogger::FormatStringOffsetId stringOffset_;

	int maxHostnameLen_;
};

BW_END_NAMESPACE

#endif // QUERY_RESULT_HPP
