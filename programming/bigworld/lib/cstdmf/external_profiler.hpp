#ifndef EXTERNAL_PROFILER_HPP
#define EXTERNAL_PROFILER_HPP

#include "cstdmf/stdmf.hpp"

namespace BW
{

class CSTDMF_DLL ExternalProfiler
{
public:
	typedef uint32 ProfilerFrameHandle;
	typedef uintptr ProfilerStringHandle;
	typedef uint32 ProfilerFrameFlags;

	class Hooks
	{
	public:
		typedef void (*VoidHook)();
		typedef ProfilerStringHandle (*CreateStringHandleHook)( const char * string );
		typedef ProfilerFrameHandle (*EnterFrameHook)( ProfilerFrameFlags flags, 
			ProfilerStringHandle nameHandle );
		typedef uint32 (*LeaveFrameHook)( ProfilerFrameHandle id );

		VoidHook tick_;
		VoidHook beginCapture_;
		VoidHook endCapture_;
		CreateStringHandleHook createStringHandle_;
		EnterFrameHook enterFrame_;
		LeaveFrameHook leaveFrame_;
	};

	static void initialize();
	
	static inline void tick()
	{
		if (s_hooks)
		{
			s_hooks->tick_();
		}
	}

	static inline void beginCapture()
	{
		if (s_hooks)
		{
			s_hooks->beginCapture_();
		}
	}

	static inline void endCapture()
	{
		if (s_hooks)
		{
			s_hooks->endCapture_();
		}
	}

	static inline uintptr createStringHandle( const char * string )
	{
		if (s_hooks)
		{
			return s_hooks->createStringHandle_( string );
		}
		else
		{
			return 0;
		}
	}

	static inline uint32 enterFrame( ProfilerFrameFlags flags, 
		ProfilerStringHandle nameHandle  )
	{
		if (s_hooks)
		{
			return s_hooks->enterFrame_( flags, nameHandle );
		}
		else
		{
			return 0;
		}
	}

	static inline uint32 leaveFrame( ProfilerFrameFlags id )
	{
		if (s_hooks)
		{
			return s_hooks->leaveFrame_( id );
		}
		else
		{
			return 0;
		}
	}

	static Hooks* s_hooks;
};

class ScopedProfilerFrame
{
public:
	ExternalProfiler::ProfilerFrameHandle frameHandle_;

	ScopedProfilerFrame(ExternalProfiler::ProfilerFrameHandle frameHandle)
		: frameHandle_(frameHandle)
	{
	}
	~ScopedProfilerFrame()
	{
		ExternalProfiler::leaveFrame( frameHandle_ );
	}
};

// Enable the external profiler by setting this flag to '1' and 
// supplying a command line argument to the application to select
// which profiler to use
#define ENABLE_EXTERNAL_PROFILER 0

#if ENABLE_EXTERNAL_PROFILER

// When the external profiler is enabled, we hijack all the standard
// profiler's macros and forward their information to the external 
// profiler.
#undef PROFILER_SCOPED
#undef PROFILER_COUNTER
#undef PROFILE_FILE_SCOPED
#undef PROFILER_SCOPED_DYNAMIC_STRING
#undef PROFILER_IDLE_SCOPED

#define PROFILER_CONCAT_HELPER2( x, y ) x ## y 
#define PROFILER_CONCAT_HELPER( x, y ) PROFILER_CONCAT_HELPER2( x, y )
#define PROFILER_TOKEN_GEN __LINE__
#define PROFILER_UNIQUE_VAR_NAME( prefix, token ) PROFILER_CONCAT_HELPER( prefix, token )
#define PROFILER_SCOPE_NAME( token ) PROFILER_UNIQUE_VAR_NAME( prof_, token )
#define PROFILER_STRINGHANDLE_NAME( token ) PROFILER_UNIQUE_VAR_NAME( prof_stringHandle_, token )

#define PROFILER_SCOPED_INTERNAL( token, name )\
	static BW::ExternalProfiler::ProfilerStringHandle PROFILER_STRINGHANDLE_NAME( token ) = \
		BW::ExternalProfiler::createStringHandle( name ); \
	BW::ScopedProfilerFrame PROFILER_SCOPE_NAME( token ) ( \
		BW::ExternalProfiler::enterFrame( 0,  PROFILER_STRINGHANDLE_NAME( token ) ) )

#define PROFILER_SCOPED_DYNAMIC_STRING_INTERNAL( token, name ) \
	BW::ScopedProfilerFrame PROFILER_SCOPE_NAME( token ) ( \
		BW::ExternalProfiler::enterFrame( 0, BW::ExternalProfiler::createStringHandle( name ) ) )
	
#define PROFILER_SCOPED( name )\
	PROFILER_SCOPED_INTERNAL( PROFILER_TOKEN_GEN, #name )

#define PROFILER_COUNTER( name, value ) \
	// Do nothing

#define PROFILE_FILE_SCOPED( name ) \
	PROFILER_SCOPED_INTERNAL( PROFILER_TOKEN_GEN, #name )

// NOTE: Dynamic strings often slow down the profiler
#define PROFILER_SCOPED_DYNAMIC_STRING( name ) \
	//PROFILER_SCOPED_DYNAMIC_STRING_INTERNAL( PROFILER_TOKEN_GEN, name )

#define PROFILER_IDLE_SCOPED( name ) \
	PROFILER_SCOPED_INTERNAL( PROFILER_TOKEN_GEN, "IDLE:" #name )

#endif // ENABLE_PROFILER && ENABLE_EXTERNAL_PROFILER

} // namespace BW

#endif // EXTERNAL_PROFILER_HPP
