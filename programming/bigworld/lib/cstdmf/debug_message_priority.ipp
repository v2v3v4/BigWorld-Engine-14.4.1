#include <stdio.h>

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: DebugMessagePriority
// -----------------------------------------------------------------------------

/*
 * DebugMessageOutput:
 *
 *	returns the iostream of where this message priority should go
 *
 *	@param p priority of the message io requestd
 *
 */
inline FILE * DebugMessageOutput( DebugMessagePriority p )
{
	static FILE * DebugMessageOutputIOStreams[] = 
	{
		stdout,		// 	MESSAGE_PRIORITY_TRACE,
		stdout,		//	MESSAGE_PRIORITY_DEBUG,
		stdout,		//	MESSAGE_PRIORITY_INFO,
		stdout,		//	MESSAGE_PRIORITY_NOTICE,
		stdout,		//	MESSAGE_PRIORITY_WARNING,
		stderr,		//	MESSAGE_PRIORITY_ERROR,
		stderr,		//	MESSAGE_PRIORITY_CRITICAL,
		stderr,		//	MESSAGE_PRIORITY_HACK,
		stderr,		//	DEPRECATED_PRIORITY_SCRIPT,
		stderr		//	MESSAGE_PRIORITY_ASSET,
	};

	BW_STATIC_ASSERT( MESSAGE_PRIORITY_TRACE == 0,
		EnumDebugMessagePriorityTrace_notZero );
	BW_STATIC_ASSERT( NUM_MESSAGE_PRIORITY == 10, 
		EnumDebugMessagePriority_WontMatchDebugMessageOutputIOStreams );

	return (p >= 0 && (size_t)p < ARRAY_SIZE(DebugMessageOutputIOStreams)) ? 
		DebugMessageOutputIOStreams[(int)p] : NULL;
}


inline const char * messagePrefix( DebugMessagePriority p )
{
	static const char * prefixes[] =
	{
		"TRACE",
		"DEBUG",
		"INFO",
		"NOTICE",
		"WARNING",
		"ERROR",
		"CRITICAL",
		"HACK",
		"SCRIPT",
		"ASSET"
	};

	return ((size_t)p < ARRAY_SIZE(prefixes)) ? prefixes[(int)p] : "";
}


/**
 *	This function is the output stream operator for a DebugMessagePriority.
 *
 *	@relates DebugMessagePriority
 */
inline std::ostream& operator<<( std::ostream &s,
	const DebugMessagePriority &v )
{
	return s << (const unsigned int &)v;
}


/**
 *	This function is the input stream operator for DebugMessagePriority.
 *
 *	@relates DebugMessagePriority
 */
inline std::istream& operator>>( std::istream &s, DebugMessagePriority &v )
{
	return s >> (unsigned int&)v;
}


BW_END_NAMESPACE
