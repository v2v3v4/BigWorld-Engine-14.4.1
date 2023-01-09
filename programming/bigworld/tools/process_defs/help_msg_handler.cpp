#include "help_msg_handler.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/debug_filter.hpp"
#include "cstdmf/debug_message_source.hpp"
#include "cstdmf/log_msg.hpp"

#include <iostream>

BW_BEGIN_NAMESPACE


/**
 *	Default Constructor.
 */
ProcessDefsHelpMsgHandler::ProcessDefsHelpMsgHandler()
{
	DebugFilter::instance().addMessageCallback( this );
}


/**
 *	Destructor.
 */
ProcessDefsHelpMsgHandler::~ProcessDefsHelpMsgHandler()
{
	DebugFilter::instance().deleteMessageCallback( this );
}


/*
 *	Override from DebugMessageCallback.
 */
bool ProcessDefsHelpMsgHandler::handleMessage(
	DebugMessagePriority messagePriority, const char * /*pCategory*/,
	DebugMessageSource messageSource, const LogMetaData & /*metaData*/,
	const char * pFormat, va_list argPtr )
{
	if (messageSource != MESSAGE_SOURCE_SCRIPT)
	{
		return false;
	}

	static const int MAX_MESSAGE_BUFFER = 8192;
	static char buffer[ MAX_MESSAGE_BUFFER ];

	LogMsg::formatMessage( buffer, MAX_MESSAGE_BUFFER, pFormat, argPtr );

	std::cerr << buffer << std::endl;

	return true;
}

BW_END_NAMESPACE
