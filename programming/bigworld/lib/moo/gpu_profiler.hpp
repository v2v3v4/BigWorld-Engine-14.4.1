#ifndef GPU_PROFILER_HPP
#define GPU_PROFILER_HPP

#include <d3d9.h>

#include "device_callback.hpp"

#define MAX_GPU_SCOPE_COUNT 256
#define MAX_GPU_FRAME_COUNT 3
#define MAX_GPU_STACK_DEPTH 256


BW_BEGIN_NAMESPACE

namespace Moo {

#if ENABLE_GPU_PROFILER

// if you want counters associated with the GPU timeline then #define this.
// #define GPU_COUNTERS

class GPUProfiler : public DeviceCallback
{
public:
	GPUProfiler();
	~GPUProfiler();

	static GPUProfiler& instance();

	void init();
	void fini();

	void beginFrame();
	void endFrame();

	int  beginScope( const char* name );
	void endScope( int id );

	void createUnmanagedObjects();
	void deleteUnmanagedObjects();
	// how many frames do we wait before querying GPU profile data?
	// one means stall for it this frame (one frame of data)
	// 3 means check it in 2 frames (after this one) (3 frames of data)
	void setFrameDelay(uint32 delay) 
	{
		frameStore_=delay; 
		if(frameStore_<1) 
			frameStore_=1;
		if(frameStore_>3)
			frameStore_=3;
	}
private:
	void createUnmanagedObjectsInternal();

#if ENABLE_NVIDIA_PERFKIT

	// NVidia perfkit support
	struct PerfkitCounter;

	enum PerfkitCounterType
	{
		perfkitCounterValue,
		perfkitCounterPercentage
	};

	enum PerfkitProfileType
	{
		perfkitProfileTypeCounters,
		perfkitProfileTypeExperiment
	};


public:
    typedef std::pair<const char*, int32> PerfkitStat;
    typedef BW::vector<PerfkitStat> PerfkitStatsArray;

    void perfkitGetStats( PerfkitStatsArray& stats ) const;
    void perfkitPerformExperiment();

private:
	void perfkitInit();
	void perfkitFini();
	void perfkitSetupExperimentCounters();
	void perfkitSetupCounters( PerfkitProfileType type );
	void perfkitFrameUpdate();
	void perfkitAddProfileEntry( PerfkitCounter& counter );

	static PerfkitCounter g_perfkitCounters_[];
	bool perkitPerformanceAnalyse_;
	bool perfkitInitialised_;

#endif // ENABLE_NVIDIA_PERFKIT

	struct GPUProfileEvent
	{
		const char* name_;
		int64 time_;
		Profiler::EventType event_;
		int32 value_;

		static uint64 startOfGPUFrameTimeStamp_;
		static uint64 gpuFrequency_;
		static uint64 stampsPerSecond_;

		void set( const char* name, int64 time, Profiler::EventType event, int32 value=0);
	};

	struct GPUProfilerRecord
	{
		const char*			name_;
		LPDIRECT3DQUERY9 	startQuery_;
		LPDIRECT3DQUERY9 	endQuery_;
		size_t			 	startIndex_;
		size_t			 	endIndex_;
#ifdef GPU_COUNTERS
		int32			 	numDrawPrimCalls_[2];
		int32			 	numPrimitives_[2];
#endif

	};

	struct GPUProfileFrame
	{
		GPUProfilerRecord			profilerRecords_[MAX_GPU_SCOPE_COUNT];
		LPDIRECT3DQUERY9			freqQuery_;
		LPDIRECT3DQUERY9			disjointQuery_;
		LPDIRECT3DQUERY9			frameBeginQuery_;
		LPDIRECT3DQUERY9			frameEndQuery_;
		size_t						queryCount_;
		size_t						recordsCount_;
	};

	size_t					curFrame_;
	uint32					frameStore_;
	GPUProfileFrame			profilerFrames_[MAX_GPU_FRAME_COUNT];
	bool					initialized_;
	bool					resourcesCreated_;
	bool					inFrame_;

	static GPUProfiler s_instance_;
};

class AutoGPUProfilerScope
{
public:
	AutoGPUProfilerScope( const char* name );
	~AutoGPUProfilerScope();

private:
	int scopeID_;
};

#define GPU_PROFILER_SCOPE(name) \
	Moo::AutoGPUProfilerScope	GPUProfilerAutoScope_##__LINE__( #name );

#else // ENABLE_GPU_PROFILER

#define GPU_PROFILER_SCOPE(name)

#endif // ENABLE_GPU_PROFILER

} // namespace Moo

#ifdef CODE_INLINE
    #include "gpu_profiler.ipp"
#endif

BW_END_NAMESPACE

#endif // GPU_PROFILER_HPP
