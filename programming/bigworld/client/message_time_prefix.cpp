#include "pch.hpp"
#include "message_time_prefix.hpp"

#if ENABLE_DPRINTF

#include "cstdmf/debug_filter.hpp"
#include "app.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
MessageTimePrefix::MessageTimePrefix()
{
	DebugFilter::instance().addMessageCallback( this );
}


/**
 *	Destructor
 */
MessageTimePrefix::~MessageTimePrefix()
{
	DebugFilter::instance().deleteMessageCallback( this );
}

/*
 *	Override from DebugMessageCallback.
 */
bool MessageTimePrefix::handleMessage(
	DebugMessagePriority messagePriority, const char * /*pCategory*/,
	DebugMessageSource /*messageSource*/, const LogMetaData & /*metaData*/,
	const char * pFormat, va_list /*argPtr*/ )
{
	static THREADLOCAL( bool ) newLine = true;

	if (DebugFilter::shouldAccept( messagePriority ))
	{
		if (newLine && (&App::instance()) != NULL)
		{
			PROFILER_SCOPED( OutputDebugString );
			wchar_t wbuf[ 256 ];
			bw_snwprintf( wbuf, ARRAY_SIZE( wbuf ),
				L"%.3f ", float( App::instance().getRenderTimeNow() ) );
			OutputDebugString( wbuf );
		}

		int len = static_cast< int >( strlen( pFormat ) );
		newLine = (len && pFormat[ len - 1 ] == '\n');
	}

	return false;
}

BW_END_NAMESPACE

#endif // ENABLE_DPRINTF

// message_time_prefix.cpp
