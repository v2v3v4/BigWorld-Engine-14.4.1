#ifndef QUERY_PARAMS_HPP
#define QUERY_PARAMS_HPP

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class QueryParams;
typedef SmartPointer< QueryParams > QueryParamsPtr;

BW_END_NAMESPACE


// TODO: get rid of this dependency. We should have a way around needing this
#include "Python.h"

#include "log_time.hpp"
#include "log_entry_address_reader.hpp"
#include "types.hpp"
#include "user_log_reader.hpp"
#include "mldb/log_reader.hpp"

#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug_message_source.hpp"

#include <regex.h> // regex_t


BW_BEGIN_NAMESPACE

class LogReaderMLDB;

/**
 * This class represents a set of parameters to be used when querying logs.
 */
class QueryParams : public SafeReferenceCount
{
public:
	QueryParams();

	bool init( PyObject *args, PyObject *kwargs,
				const LogReaderMLDB *pLogReader );

	bool isGood() const { return isGood_; }

	uint16 getUID() const;
	void setUID( uint16 uid );

	// These accessors are used to populate the QueryRange
	LogTime getStartTime() const;
	LogTime getEndTime() const;
	LogEntryAddressReader getStartAddress() const;
	LogEntryAddressReader getEndAddress() const;
	SearchDirection getDirection() const;

	int getNumLinesOfContext() const;

	bool isPreInterpolate() const;
	bool isPostInterpolate() const;

	// These methods are used to validate the parameters for a Query
	bool validateAddress( MessageLogger::IPAddress ipAddress ) const;
	bool validatePID( uint16 pid ) const;
	bool validateAppID( MessageLogger::AppInstanceID appid ) const;
	bool validateProcessType( MessageLogger::AppTypeID appTypeID ) const;
	bool validateMessagePriority( int priority ) const;
	bool validateIncludeRegex( const char *searchStr ) const;
	bool validateExcludeRegex( const char *searchStr ) const;
	bool validateLogMessageType( DebugMessageSource messageSource ) const;
	bool validateCategory( MessageLogger::CategoryID categoryID ) const;

private:
	// Private because we want to let the reference counting handle deletion
	~QueryParams();

	bool initHost( const char *host, const LogReaderMLDB *pLogReader  );
	bool initRegex( const char *include, const char *exclude );
	bool initCategories(  PyObject * pCategories,
		const LogReaderMLDB * pLogReader );
	bool initStartPeriod( PyObject *startAddr, double start,
		UserLogReaderPtr pUserLog, const BW::string &period );
	bool initEndPeriod( PyObject *endAddr, double endTime,
		UserLogReaderPtr pUserLog, const BW::string &period );

	uint16 uid_;

	LogTime start_;
	LogTime end_;

	LogEntryAddressReader startAddress_;
	LogEntryAddressReader endAddress_;

	MessageLogger::IPAddress ipAddress_;
	uint16 pid_;
	MessageLogger::AppInstanceID appid_;
	int procs_;
	int severities_;

	uint32 logSourceTypeFlags_;

	typedef BW::multiset< MessageLogger::CategoryID > CategoryIDSet;
	CategoryIDSet categories_;

	regex_t *pInclude_;
	regex_t *pExclude_;

	int interpolate_;
	bool casesens_;
	SearchDirection direction_;
	int numLinesOfContext_;

	bool negateHost_;
	bool negatePID_;
	bool negateAppID_;

	bool isGood_;
};

BW_END_NAMESPACE

#endif // QUERY_PARAMS_HPP
