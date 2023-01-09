#ifndef LOG_ENTRY_HPP
#define LOG_ENTRY_HPP

#include "log_time.hpp"
#include "types.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug_message_source.hpp"


BW_BEGIN_NAMESPACE

/**
 * The fixed-length portion of a log entry (i.e. the bit that gets written
 * to the 'entries' file).
 */
class LogEntry
{
	friend class UserSegmentWriter;

public:
	LogEntry( const struct timeval & tv,
			MessageLogger::UserComponentID userComponentID,
			MessageLogger::NetworkMessagePriority messagePriority,
			MessageLogger::FormatStringOffsetId stringOffset,
			MessageLogger::CategoryID categoryID,
			DebugMessageSource messageSource) :
		componentID_( userComponentID ),
		categoryID_( categoryID ),
		messageSource_( messageSource ),
		messagePriority_( messagePriority ),
		stringOffset_( stringOffset )
	{
		time_.secs_ = tv.tv_sec;
		time_.msecs_ = tv.tv_usec / 1000;
	}

	LogEntry()
	{
	}

	const LogTime & time() const
	{
		return time_;
	}

	MessageLogger::CategoryID categoryID() const
	{
		return categoryID_;
	}

	DebugMessageSource messageSource() const
	{
		return messageSource_;
	}

	MessageLogger::NetworkMessagePriority messagePriority() const
	{
		return messagePriority_;
	}

	MessageLogger::FormatStringOffsetId stringOffset() const
	{
		return stringOffset_;
	}

	uint32 argsOffset() const
	{
		return argsOffset_;
	}

	MessageLogger::UserComponentID userComponentID() const
	{
		return componentID_;
	}

	MessageLogger::MetaDataOffset metadataOffset() const
	{
		return metadataOffset_;
	}

	uint16 metadataLength() const
	{
		return metadataLen_;
	}

private:
	LogTime time_;

	MessageLogger::UserComponentID componentID_;

	MessageLogger::CategoryID categoryID_;

	DebugMessageSource messageSource_;

	MessageLogger::NetworkMessagePriority messagePriority_;

	MessageLogger::FormatStringOffsetId stringOffset_;

	uint32 argsOffset_;
	uint16 argsLen_;

	uint32 metadataOffset_;
	uint16 metadataLen_;
};

BW_END_NAMESPACE

#endif // LOG_ENTRY_HPP
