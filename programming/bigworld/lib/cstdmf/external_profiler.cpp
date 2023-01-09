#include "pch.hpp"

#include "external_profiler.hpp"
#include "debug.hpp"

namespace BW
{

ExternalProfiler::Hooks* ExternalProfiler::s_hooks = NULL;

// ITT hooks (GPA and VTune)
#if defined(EXTERNAL_PROFILER_AVAILABLE_ITT) && ENABLE_EXTERNAL_PROFILER

#undef INLINE
#include <ittnotify.h>
namespace ITT_ProfilerHooks
{
	__itt_domain* g_domain = __itt_domain_createA( "BigWorld.Profiler" );
	__itt_string_handle* g_pshTick = __itt_string_handle_createA( "Tick" );

	void tick()
	{
		return;
	}

	void beginCapture()
	{
		__itt_resume();
	}

	void endCapture()
	{
		__itt_pause();
	}
	
	ExternalProfiler::ProfilerStringHandle 
		createStringHandle(const char * string)
	{
		return reinterpret_cast<ExternalProfiler::ProfilerStringHandle>(
			__itt_string_handle_createA(string) );
	}

	
	ExternalProfiler::ProfilerFrameHandle enterFrame(
		ExternalProfiler::ProfilerFrameFlags flags, 
		ExternalProfiler::ProfilerStringHandle nameHandle)
	{
		__itt_task_begin( g_domain, 
			__itt_null, 
			__itt_null, 
			(__itt_string_handle*)nameHandle);
		return 0;
	}

	
	uint32 leaveFrame(uint32 id)
	{
		__itt_task_end(g_domain);

		return 0;
	}

	void initialize()
	{
		static ExternalProfiler::Hooks intelGPAHooks;
		bw_zero_memory( &intelGPAHooks, sizeof(intelGPAHooks) );

		intelGPAHooks.tick_ = &ITT_ProfilerHooks::tick;
		intelGPAHooks.beginCapture_ = &ITT_ProfilerHooks::beginCapture;
		intelGPAHooks.endCapture_ = &ITT_ProfilerHooks::endCapture;
		intelGPAHooks.createStringHandle_ = &ITT_ProfilerHooks::createStringHandle;
		intelGPAHooks.enterFrame_ = &ITT_ProfilerHooks::enterFrame;
		intelGPAHooks.leaveFrame_ = &ITT_ProfilerHooks::leaveFrame;
		ExternalProfiler::s_hooks = &intelGPAHooks;
	}
}

#endif

// NVidia tools hooks
#if defined(EXTERNAL_PROFILER_AVAILABLE_NVT) && ENABLE_EXTERNAL_PROFILER

#define NVTX_STDINT_TYPES_ALREADY_DEFINED
#include "stdint.h"
#include <nvToolsExt.h>
namespace NVTools_ProfilerHooks
{
	void tick()
	{
		nvtxMarkA("Tick");
		return;
	}


	ExternalProfiler::ProfilerStringHandle 
		createStringHandle(const char* string)
	{
		return reinterpret_cast<ExternalProfiler::ProfilerStringHandle>(
			string);
	}


	ExternalProfiler::ProfilerFrameHandle enterFrame(
		ExternalProfiler::ProfilerFrameFlags flags, 
		ExternalProfiler::ProfilerStringHandle nameHandle)
	{
		nvtxRangePushA( (const char *)nameHandle );
		return 0;
	}


	uint32 leaveFrame(uint32 id)
	{
		nvtxRangePop();

		return 0;
	}

	void initialize()
	{
		static ExternalProfiler::Hooks nvToolsHooks;
		bw_zero_memory( &nvToolsHooks, sizeof(nvToolsHooks) );

		nvToolsHooks.tick_ = &NVTools_ProfilerHooks::tick;
		nvToolsHooks.createStringHandle_ = 
			&NVTools_ProfilerHooks::createStringHandle;
		nvToolsHooks.enterFrame_ = &NVTools_ProfilerHooks::enterFrame;
		nvToolsHooks.leaveFrame_ = &NVTools_ProfilerHooks::leaveFrame;
		ExternalProfiler::s_hooks = &nvToolsHooks;
	}
}
#endif

void ExternalProfiler::initialize()
{
#if ENABLE_EXTERNAL_PROFILER
	// Use the command line to determine what external profiler to initialize
	LPSTR commandLine = ::GetCommandLineA();
	if (strstr( commandLine, "-profiler=ITT" ))
	{
#if defined(EXTERNAL_PROFILER_AVAILABLE_ITT)
		INFO_MSG("Initializing external profiler: ITT");
		ITT_ProfilerHooks::initialize();
#endif
	}
	else if (strstr( commandLine, "-profiler=NVT" ))
	{
#if defined(EXTERNAL_PROFILER_AVAILABLE_NVT)
		INFO_MSG("Initializing external profiler: NVT");
		NVTools_ProfilerHooks::initialize();
#endif
	}
#endif
}

} // namespace BW

// external_profiler.cpp