#include "msg_handler.hpp"

#include "cstdmf/concurrency.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/debug_filter.hpp"
#include "cstdmf/log_msg.hpp"

#include <iostream>


BW_BEGIN_NAMESPACE

/**
 *	Default Constructor.
 */
MsgHandler::MsgHandler() :
	errorsOccurred_( false )
{
	DebugFilter::instance().addMessageCallback( this );
	DebugFilter::instance().addCriticalCallback( this );
}


/**
 *	Destructor.
 */
MsgHandler::~MsgHandler()
{
	DebugFilter::instance().deleteCriticalCallback( this );
	DebugFilter::instance().deleteMessageCallback( this );
}


/*
 *	Override from DebugMessageCallback.
 */
bool MsgHandler::handleMessage(
	DebugMessagePriority messagePriority, const char * /*pCategory*/,
	DebugMessageSource /*messageSource*/, const LogMetaData & /*metaData*/,
	const char * pFormat, va_list argPtr )
{
	// Anything that is not an error, warning or notice should be
	// filtered with info
	if ((messagePriority != MESSAGE_PRIORITY_CRITICAL) &&
		(messagePriority != MESSAGE_PRIORITY_ERROR) &&
		(messagePriority != MESSAGE_PRIORITY_WARNING) &&
		(messagePriority != MESSAGE_PRIORITY_NOTICE))
	{
		return true;
	}

	errorsOccurred_ = (messagePriority >= MESSAGE_PRIORITY_ERROR);

	const char * priorityStr = messagePrefix( messagePriority );


	static const int MAX_MESSAGE_BUFFER = 8192;
	static char buffer[ MAX_MESSAGE_BUFFER ];
	LogMsg::formatMessage( buffer, MAX_MESSAGE_BUFFER, pFormat, argPtr );

	std::cout << priorityStr << ": " << buffer << std::endl;

	return true;
}


/*
 *	Override from CriticalMessageCallback.
 */
void MsgHandler::handleCritical( const char * msg )
{
	// exit with failure. The message was already printed in handleMessage
	std::cout << "...failed" << std::endl;
	// turn off writing to console before exiting
	DebugFilter::shouldWriteToConsole( false );
	exit( EXIT_FAILURE );
}


/**
 *	This method returns whether the message handler has encountered errors.
 *
 *	@returns true if errors have occurred, false otherwise.
 */
bool MsgHandler::errorsOccurred() const
{
	return errorsOccurred_;
}

BW_END_NAMESPACE
