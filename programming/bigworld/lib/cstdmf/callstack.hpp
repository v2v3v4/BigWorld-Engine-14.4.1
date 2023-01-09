#ifndef BW_CALLSTACK_HPP
#define BW_CALLSTACK_HPP

#include "config.hpp"

#define BW_ENABLE_STACKTRACE ENABLE_MEMORY_DEBUG &&	\
	!BWENTITY_DLL &&								\
	(!defined( CSTDMF_EXPORT ) || defined( MF_SERVER ))

namespace BW
{
// TODO: This is used in CallstackHeader even if
// BW_ENABLE_STACKTRACE == 0.
// It may be worth to remove it from the header when
// stacktracing is disabled.
typedef void * CallstackAddressType;
}

#if BW_ENABLE_STACKTRACE

#include "cstdmf/stdmf.hpp"

namespace BW 
{

/// intitialise callstack subsystem
CSTDMF_DLL void initStacktraces();

/// shutdown callstack subsystem
CSTDMF_DLL void finiStacktraces();

/// \returns true if symbol resolution is available
bool stacktraceSymbolResolutionAvailable();

/// generate a stack trace using a fast method that may not retrieve a full
/// callstack.
/// \param trace		pointer to array to store stack trace in.
/// \param maxDepth	maximum number of traces that can be retrieved
/// \param skipFrames skip frames from the bottom of the stack
/// \returns number of addresses in trace
int stacktrace( CallstackAddressType * trace, size_t maxDepth,
	size_t skipFrames = 0 );

/// generate a stack trace using a slower but more accurate method.
/// \param trace		pointer to array to store stack trace in.
/// \param maxDepth	maximum number of traces that can be retrieved
/// \param skipFrames skip frames from the bottom of the stack
/// \param threadID get stack for a specific thread ID
/// \returns number of addresses in trace
int stacktraceAccurate( CallstackAddressType * trace, size_t maxDepth,
	size_t skipFrames = 0, int threadID = 0 );

/// generate a stack trace
/// \warning For this function to work correctly with optimisations enabled you
///          need to disable the frame pointer omission optimisation using /Oy-
/// \warning nameBuf and fileBuf must be SymbolStringMaxLength
bool convertAddressToFunction( CallstackAddressType address, char * nameBuf,
	size_t nameBufSize, char * fileBuf, size_t fileBufSize, int * line );

/// use stacktraceAccurate to get a string containing the stack
bool stacktraceAccurate( char * stackMsg, size_t stackMsgSize,
	size_t skipFrames = 0, int threadId = 0 );

} // namespace BW

#endif // BW_ENABLE_CALLSTACK

#endif // BW_CALLSTACK_HPP

