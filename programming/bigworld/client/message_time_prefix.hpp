#ifndef MESSAGE_TIME_PREFIX_HPP
#define MESSAGE_TIME_PREFIX_HPP

#include "cstdmf/config.hpp"

#if ENABLE_DPRINTF

#include "cstdmf/debug_message_callbacks.hpp"
#include "cstdmf/dprintf.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This functor is called when any message is printed
 */
struct MessageTimePrefix : public DebugMessageCallback
{
	MessageTimePrefix();
	~MessageTimePrefix();

	bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormat, va_list argPtr );
};

BW_END_NAMESPACE

#endif // ENABLE_DPRINTF

#endif // MESSAGE_TIME_PREFIX_HPP

// message_time_prefix.hpp
