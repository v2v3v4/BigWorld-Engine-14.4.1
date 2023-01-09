#include "pch.hpp"

#include "critical_error_handler.hpp"

#include "config.hpp"

#if defined(_WIN32) && !defined(_XBOX)

#include "critical_message_box.hpp"
#include "string_builder.hpp"

BW_BEGIN_NAMESPACE

static const size_t s_cehBufferSize = 65536;

/**
 *
 */
class DefaultCriticalErrorHandler : public CriticalErrorHandler
{
	virtual Result ask( const char* msg )
	{
#if BUILT_BY_BIGWORLD
		// ensure msg is not bigger than the criticalMsgBox can handle, as if it is too big
		//  CriticalMsgBox will display nothing.
		char errorMsgString[CriticalMsgBox::INFO_BUFFER_SIZE] = { 0 };
		BW::StringBuilder errorStringBuilder( errorMsgString, ARRAY_SIZE( errorMsgString ) );
		errorStringBuilder.append( msg );
		CriticalMsgBox mb( errorMsgString, true );
#else // BUILT_BY_BIGWORLD
		CriticalMsgBox mb( msg, false );
#endif // BUILT_BY_BIGWORLD
		if (mb.doModal())
		{
			return ENTERDEBUGGER;
		}

		return EXITDIRECTLY;
	}

	virtual void recordInfo( bool willExit )
	{}
}
DefaultCriticalErrorHandler;

CriticalErrorHandler * CriticalErrorHandler::handler_ =
	&DefaultCriticalErrorHandler;

static char exceptionMessage[s_cehBufferSize] = {0};
void RaiseEngineException(const char* msg)
{
	// TODO not MT safety
	strcpy_s(exceptionMessage, sizeof(exceptionMessage), msg);
	throw;
}

BW_END_NAMESPACE

#else//defined(_WIN32)

BW_BEGIN_NAMESPACE

CriticalErrorHandler * CriticalErrorHandler::handler_;

BW_END_NAMESPACE

#endif//defined(_WIN32)

BW_BEGIN_NAMESPACE

/**
 *
 */
CriticalErrorHandler * CriticalErrorHandler::get()
{
	return handler_;
}


/**
 *
 */
CriticalErrorHandler * CriticalErrorHandler::set(
	CriticalErrorHandler * newHandler )
{
	CriticalErrorHandler * oldHandler = handler_;
	handler_ = newHandler;

	return oldHandler;
}

BW_END_NAMESPACE

// critical_error_handler.cpp
