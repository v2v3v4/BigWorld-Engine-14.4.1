#pragma once

#if ENABLE_BW_RENDER_PERF_MARKERS

#include "forward_declarations.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	//----------------------------------------------------------------------------------------------
	inline void beginRenderEvent(const BW::string& name)
	{
		//-- PIX events only take wide-character strings.
		wchar_t dest[256];
		size_t  chars;
		mbstowcs_s(&chars, dest, name.c_str(), 256);
		D3DPERF_BeginEvent(D3DCOLOR_XRGB(0,0,0), dest);
	}

	//----------------------------------------------------------------------------------------------
	inline void endRenderEvent()
	{
		D3DPERF_EndEvent();
	}

	//--
	//----------------------------------------------------------------------------------------------
	struct ScopedRenderEvent
	{
		ScopedRenderEvent(const BW::string& name)
		{
			beginRenderEvent(name);
		}

		~ScopedRenderEvent()
		{
			endRenderEvent();
		}
	};

} //-- Moo

BW_END_NAMESPACE

#endif //-- ENABLE_BW_RENDER_PERF_MARKERS

BW_BEGIN_NAMESPACE

#if ENABLE_BW_RENDER_PERF_MARKERS
	#define BW_BEGIN_RENDER_PERF_MARKER(name)	Moo::beginRenderEvent(name);
	#define BW_END_RENDER_PERF_MARKER()			Moo::endRenderEvent();
	#define BW_SCOPED_RENDER_PERF_MARKER(name)	Moo::ScopedRenderEvent _hidden_scoped_render_event(name);
#else
	#define BW_BEGIN_RENDER_PERF_MARKER(name)
	#define BW_END_RENDER_PERF_MARKER()
	#define BW_SCOPED_RENDER_PERF_MARKER(name)
#endif

BW_END_NAMESPACE