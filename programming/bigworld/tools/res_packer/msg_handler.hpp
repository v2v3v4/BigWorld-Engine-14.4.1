#ifndef __MSG_HANDLER_HPP__
#define __MSG_HANDLER_HPP__

#include "cstdmf/debug_message_callbacks.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class filters normal and critical messages, printing only warning, 
 *	errors and critical messages. If an error or critical message is received
 *	the internal errorsOccurred_ flag is set.
 */
class MsgHandler : public DebugMessageCallback, public CriticalMessageCallback
{
public:
	MsgHandler();
	virtual ~MsgHandler();

	// normal DebugMessageCallback message handler
	virtual bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormat, va_list argPtr );

	// critical CriticalMessageCallback handler
	virtual void handleCritical( const char * msg );

	bool errorsOccurred() const;

private:
	bool errorsOccurred_;
};

BW_END_NAMESPACE

#endif // __MSG_HANDLER_HPP__
