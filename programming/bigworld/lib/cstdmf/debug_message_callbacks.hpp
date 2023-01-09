#ifndef DEBUG_MESSAGE_CALLBACKS_HPP
#define DEBUG_MESSAGE_CALLBACKS_HPP

#include "debug_message_source.hpp"
#include "debug_message_priority.hpp"

#include <stdarg.h>

BW_BEGIN_NAMESPACE

class LogMetaData;

/**
 *	Definition for the critical message callback functor
 */
class CriticalMessageCallback
{
public:
	virtual void handleCritical( const char * msg ) = 0;

	virtual ~CriticalMessageCallback() {};
};


/**
 *	Definition for the message callback functor. If the
 *  function returns true, the default behaviour for
 *  displaying messages is ignored.
 */
class DebugMessageCallback
{
public:
	virtual bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormat, va_list argPtr ) = 0;

	virtual ~DebugMessageCallback() {};
};

BW_END_NAMESPACE

#endif // DEBUG_MESSAGE_CALLBACKS_HPP
