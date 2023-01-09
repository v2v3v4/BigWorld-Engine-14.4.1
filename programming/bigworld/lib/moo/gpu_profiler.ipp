// gpu_profiler.ipp

#ifdef CODE_INLINE
    #define INLINE    inline
#else
    #define INLINE
#endif

#if ENABLE_GPU_PROFILER

namespace Moo {

INLINE
AutoGPUProfilerScope::AutoGPUProfilerScope( const char* name ) :
	scopeID_( GPUProfiler::instance().beginScope( name ) )
{
}

INLINE
AutoGPUProfilerScope::~AutoGPUProfilerScope()
{
	GPUProfiler::instance().endScope( scopeID_ );
}

INLINE
GPUProfiler& GPUProfiler::instance()
{
	return s_instance_;
}

INLINE
void GPUProfiler::GPUProfileEvent::set( const char* name, 
									   int64 time, Profiler::EventType event, int32 value )
{
	name_ = name;
	time_ = (int64)((time-startOfGPUFrameTimeStamp_)*stampsPerSecond_/(double)gpuFrequency_);
	event_ = event;
	value_ = value;
}

} // namespace Moo

#endif // ENABLE_GPU_PROFILER

// gpu_profiler.ipp
