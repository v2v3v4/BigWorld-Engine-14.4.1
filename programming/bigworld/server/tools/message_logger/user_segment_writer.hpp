#ifndef USER_SEGMENT_WRITER_HPP
#define USER_SEGMENT_WRITER_HPP

#include "user_segment.hpp"

#include "logging_component.hpp"
#include "log_entry.hpp"
#include "log_string_interpolator.hpp"
#include "user_log_writer.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class LogStorageMLDB;

class UserSegmentWriter : public UserSegment
{
public:
	UserSegmentWriter( const BW::string userLogPath, const char *suffix );
	//~UserSegmentWriter();

	bool init();

	bool addEntry( LoggingComponent * component, UserLogWriter * pUserLog,
		LogEntry & entry, LogStringInterpolator & handler,
		MemoryIStream & inputStream,
		MessageLogger::NetworkVersion version );

	bool isFull( const LogStorageMLDB * pLogStorage ) const;


};

BW_END_NAMESPACE

#endif // USER_SEGMENT_WRITER_HPP
