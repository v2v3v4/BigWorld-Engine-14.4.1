#include "pch.hpp"

#include "callstack.hpp"

#include "concurrency.hpp"
#include "stack_tracker.hpp"
#include "string_builder.hpp"
#include "stdmf.hpp" // for NEW_LINE definition

#if BW_ENABLE_STACKTRACE

namespace BW
{

//------------------------------------------------------------------------------------

bool stacktraceAccurate( char * stackMsg, size_t stackMsgSize,
	size_t skipFrames /*= 0*/, int threadID /*= 0*/ )
{
	if( ( stackMsg == NULL ) || (stackMsgSize <= 0 ) )
	{
		return( false );
	}

	// use an in place string builder to build the stack info
	BW::StringBuilder stackStringBuilder( stackMsg, stackMsgSize );

	if (threadID == 0 || OurThreadID() == (unsigned long)threadID)
	{
		stackStringBuilder.appendf( "Current thread #%d trace:",
			OurThreadID() );
	}
	else
	{
		stackStringBuilder.appendf( "Thread #%d trace:",
			threadID );
	}

	stackStringBuilder.append(BW_NEW_LINE);

	// if we have debug info loaded, use this
	if (stacktraceSymbolResolutionAvailable())
	{
		CallstackAddressType  callStack[256];
		int stackSize = stacktraceAccurate( callStack, 256, skipFrames, threadID );

		for( int i = 0; i < stackSize; i++ )
		{
			const int bufferSize = 256;
			char nameBuf[bufferSize] = { 0 };
			char fileBuf[bufferSize] = { 0 };
			int lineNo = 0;

			convertAddressToFunction( callStack[i], nameBuf, bufferSize, fileBuf,
				bufferSize, &lineNo );
			stackStringBuilder.appendf( "%s(%d) : %s", fileBuf, lineNo, nameBuf  );
			stackStringBuilder.append( BW_NEW_LINE );
		}

		if (stackSize > 0)
		{
			return( true );
		}
		else
		{
			return( false );
		}
	}
	else // no debug info loaded, try to use stacktracker
	{
#if ENABLE_STACK_TRACKER
		if (StackTracker::stackSize() > 0)
		{
			const int STACK_MAX = 1024;
			char stackBuffer[ STACK_MAX ];
			StackTracker::buildReport( stackBuffer, ARRAY_SIZE( stackBuffer ) );

			stackStringBuilder.append( BW_NEW_LINE );
			stackStringBuilder.append( BW_NEW_LINE );
			stackStringBuilder.append( "Stack trace: " );
			stackStringBuilder.append( stackBuffer );
			stackStringBuilder.append( BW_NEW_LINE );
			return( true );
		}
#endif // ENABLE_STACK_TRACKER
		return( false );
	}
}


} // namespace BW

#endif // BW_ENABLE_STACKTRACE

