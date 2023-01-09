#ifndef PROCESS_DEFS_MSG_HANDLER_HPP
#define PROCESS_DEFS_MSG_HANDLER_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/debug_message_callbacks.hpp"
#include "cstdmf/debug_message_priority.hpp"

BW_BEGIN_NAMESPACE

//
// This class adds itself as a message handler to the Debug messaging
// system to redirect the output captured from a script back to std::cerr.
//

class ProcessDefsHelpMsgHandler: public DebugMessageCallback
{
public:

	ProcessDefsHelpMsgHandler();

	virtual ~ProcessDefsHelpMsgHandler();

	virtual bool handleMessage(
		DebugMessagePriority messagePriority,
		const char * pCategory, DebugMessageSource messageSource,
		const LogMetaData & metaData, const char * pFormat, va_list argPtr );
};

BW_END_NAMESPACE

#endif // #ifndef PROCESS_DEFS_MSG_HANDLER_HPP
