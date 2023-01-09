#include "pch.hpp"
#include "gpu_profiler.hpp"
#include "moo/render_context.hpp"
#include "cstdmf/profiler.hpp"

#ifndef CODE_INLINE

BW_BEGIN_NAMESPACE
	#include "gpu_profiler.ipp"
BW_END_NAMESPACE

#endif

#define CREATE_QUERY( queryType, obj )\
{ HRESULT hr = pDevice->CreateQuery( queryType, &obj );\
if (!obj) \
{ \
	ERROR_MSG( "GPUProfiler: failed to create D3D query (hr=%s)\n",\
		DX::errorAsString(hr).c_str());\
	this->deleteUnmanagedObjects();\
	resourcesCreated_ = false;\
	return;\
}\
}
#define RELEASE_QUERY( obj ) { if (obj) { obj->Release(); } obj = NULL; }


#if ENABLE_NVIDIA_PERFKIT

// Set up NVPMAPI
#define NVPM_INITGUID
#include "perfkit/inc/NvPmApi.Manager.h"


// Simple singleton implementation for grabbing the NvPmApi
static NvPmApiManager S_NVPMManager;
extern NvPmApiManager *GetNvPmApiManager() {return &S_NVPMManager;}

const NvPmApi *GetNvPmApi() {return S_NVPMManager.Api();}

bool G_bNVPMInitialized = false;
NVPMContext g_hNVPMContext;

#endif // ENABLE_NVIDIA_PERFKIT


BW_BEGIN_NAMESPACE

namespace Moo {

#if ENABLE_GPU_PROFILER

GPUProfiler GPUProfiler::s_instance_;

uint64 GPUProfiler::GPUProfileEvent::startOfGPUFrameTimeStamp_ = 0;
uint64 GPUProfiler::GPUProfileEvent::gpuFrequency_ = 0;
uint64 GPUProfiler::GPUProfileEvent::stampsPerSecond_ = 0;

GPUProfiler::GPUProfiler() :
	inFrame_( false )
{
	BW_GUARD;
	memset( profilerFrames_, 0, sizeof(profilerFrames_) );
	frameStore_=3; // should be between 1 and MAX_GPU_FRAME_COUNT

	MF_WATCH( "profiler/gpuProfileDelay(1-3)", frameStore_, Watcher::WT_READ_WRITE, "ranges from 1 to 3" );

}

GPUProfiler::~GPUProfiler()
{
	BW_GUARD;
}

void GPUProfiler::init()
{
	BW_GUARD;
	if (initialized_)
	{
		return;
	}

	this->createUnmanagedObjectsInternal();
	if (!resourcesCreated_)
	{
		this->deleteUnmanagedObjects();
		ERROR_MSG( "GPUProfiler::init: failed to create Query resources. "
			"Profiler will be disabled.\n" );
		return;
	}

	curFrame_    =  0;
	initialized_ = true;
	g_profiler.addGPUThread();

#if ENABLE_NVIDIA_PERFKIT
	perfkitInit();
#endif
}

void GPUProfiler::fini()
{
	BW_GUARD;
	if (!initialized_)
	{
		return;
	}

#if ENABLE_NVIDIA_PERFKIT
	perfkitFini();
#endif

	this->deleteUnmanagedObjects();
	curFrame_    =  0;
	initialized_ = false;

	memset( profilerFrames_, 0, sizeof(profilerFrames_) );
}

void GPUProfiler::createUnmanagedObjects()
{
	BW_GUARD;
	if (!initialized_ || resourcesCreated_)
	{
		return;
	}
	this->createUnmanagedObjectsInternal();

}

void GPUProfiler::createUnmanagedObjectsInternal()
{
	BW_GUARD;
	if (resourcesCreated_)
	{
		return;
	}

	IDirect3DDevice9* pDevice = rc().device();
	MF_ASSERT( pDevice != NULL );

	for (size_t i = 0; i < frameStore_; ++i)
	{
		for (size_t j = 0; j < MAX_GPU_SCOPE_COUNT; ++j)
		{
			CREATE_QUERY( D3DQUERYTYPE_TIMESTAMP, 
				profilerFrames_[i].profilerRecords_[j].startQuery_ );
			CREATE_QUERY( D3DQUERYTYPE_TIMESTAMP, 
				profilerFrames_[i].profilerRecords_[j].endQuery_ );
		}
		CREATE_QUERY( D3DQUERYTYPE_TIMESTAMP, 
			profilerFrames_[i].frameBeginQuery_ );
		CREATE_QUERY( D3DQUERYTYPE_TIMESTAMP, 
			profilerFrames_[i].frameEndQuery_ );
		CREATE_QUERY( D3DQUERYTYPE_TIMESTAMPFREQ, 
			profilerFrames_[i].freqQuery_ );
		CREATE_QUERY( D3DQUERYTYPE_TIMESTAMPDISJOINT, 
			profilerFrames_[i].disjointQuery_ );
	}

	resourcesCreated_ = true;
}

void GPUProfiler::deleteUnmanagedObjects()
{
	BW_GUARD;
	for (size_t i = 0; i < frameStore_; ++i)
	{
		for (size_t j = 0; j < MAX_GPU_SCOPE_COUNT; ++j)
		{
			RELEASE_QUERY( profilerFrames_[i].profilerRecords_[j].startQuery_ );
			RELEASE_QUERY( profilerFrames_[i].profilerRecords_[j].endQuery_ );
		}
		RELEASE_QUERY( profilerFrames_[i].frameBeginQuery_ );		
		RELEASE_QUERY( profilerFrames_[i].frameEndQuery_ );
		RELEASE_QUERY( profilerFrames_[i].freqQuery_ );
		RELEASE_QUERY( profilerFrames_[i].disjointQuery_ );
	}

	resourcesCreated_ = false;
}

void GPUProfiler::beginFrame()
{
	BW_GUARD;
	PROFILER_SCOPED(GPUPRofiler_beginFrame);
	if (!initialized_ || !resourcesCreated_)
	{
		return;
	}

	MF_ASSERT( !inFrame_ );

	inFrame_ = true;

	curFrame_ = (curFrame_+1)%frameStore_;
	GPUProfileFrame& frame = profilerFrames_[curFrame_];

	frame.disjointQuery_->Issue(D3DISSUE_BEGIN);
	frame.frameBeginQuery_->Issue(D3DISSUE_END);

	frame.queryCount_ = 0;
	frame.recordsCount_ = 0;
}


static const char* s_primCountName = "Prim Count";
static const char* s_drawPrimCallsName = "draw Prim Calls";

void GPUProfiler::endFrame()
{
	BW_GUARD;
	PROFILER_SCOPED(gpuProfilerEndFrame);
	static const char* name = "GPUFrame";

	if (!initialized_ || !resourcesCreated_ || !inFrame_)
	{
		inFrame_ = false;
		return;
	}

	inFrame_ = false;

	GPUProfileFrame& frame = profilerFrames_[curFrame_];

	frame.frameEndQuery_->Issue(D3DISSUE_END);
	frame.freqQuery_->Issue(D3DISSUE_END);
	frame.disjointQuery_->Issue(D3DISSUE_END);

	GPUProfileFrame& frameToProcess = profilerFrames_[(curFrame_+1)%frameStore_];
	GPUProfileEvent::stampsPerSecond_ = stampsPerSecond();	
#ifdef GPU_COUNTERS
	GPUProfileEvent events[6*MAX_GPU_SCOPE_COUNT+2];
#else
	GPUProfileEvent events[2*MAX_GPU_SCOPE_COUNT+2];
#endif
//	UINT64	freq=0;
	BOOL	disjoint=FALSE;

	while (frameToProcess.disjointQuery_->GetData((void*)&disjoint, sizeof(BOOL), D3DGETDATA_FLUSH)==S_FALSE
		&& Moo::rc().device()->TestCooperativeLevel() != D3DERR_DEVICELOST);

	if (disjoint)
	{
		return;
	}

	while (frameToProcess.freqQuery_->GetData((void*)&GPUProfileEvent::gpuFrequency_, sizeof(UINT64), D3DGETDATA_FLUSH)==S_FALSE
		&& Moo::rc().device()->TestCooperativeLevel() != D3DERR_DEVICELOST);

	UINT64 frameStartTime = 0; 
	UINT64 frameEndTime = 0;

	while (frameToProcess.frameBeginQuery_->GetData((void*)&frameStartTime, sizeof(UINT64), D3DGETDATA_FLUSH)==S_FALSE
		&& Moo::rc().device()->TestCooperativeLevel() != D3DERR_DEVICELOST);
	while (frameToProcess.frameEndQuery_->GetData((void*)&frameEndTime, sizeof(UINT64), D3DGETDATA_FLUSH)==S_FALSE
		&& Moo::rc().device()->TestCooperativeLevel() != D3DERR_DEVICELOST);

	GPUProfileEvent::startOfGPUFrameTimeStamp_ = frameStartTime;

	GPUProfileEvent startEvent;
	startEvent.set( name, frameStartTime, Profiler::EVENT_START );
	GPUProfileEvent endEvent;
	endEvent.set( name, frameEndTime, Profiler::EVENT_END );

	size_t queryCount = frameToProcess.queryCount_;
	size_t recordsCount = frameToProcess.recordsCount_;

#ifdef GPU_COUNTERS
	const size_t QUERY_MULTIPLIER = 3;
#else
	const size_t QUERY_MULTIPLIER = 1;
#endif

	for (size_t i = 0; i < recordsCount; ++i)
	{
		PROFILER_SCOPED(scopeLoop);
		UINT64 startTime = 0;
		UINT64 endTime = 0;

		GPUProfilerRecord& gpuRecord = frameToProcess.profilerRecords_[i];

		while (gpuRecord.startQuery_->GetData( (void*)&startTime, sizeof(UINT64), D3DGETDATA_FLUSH)==S_FALSE
			&& Moo::rc().device()->TestCooperativeLevel() != D3DERR_DEVICELOST);
		while (gpuRecord.endQuery_->GetData( (void*)&endTime, sizeof(UINT64), D3DGETDATA_FLUSH)==S_FALSE
			&& Moo::rc().device()->TestCooperativeLevel() != D3DERR_DEVICELOST);

		size_t startQueryIndex = gpuRecord.startIndex_ * QUERY_MULTIPLIER;
		size_t endQueryIndex = gpuRecord.endIndex_ * QUERY_MULTIPLIER;


		events[startQueryIndex].set( gpuRecord.name_, startTime, Profiler::EVENT_START );
		events[endQueryIndex].set( gpuRecord.name_, endTime, Profiler::EVENT_END );
#ifdef GPU_COUNTERS
		events[startQueryIndex + 1].set( s_primCountName, startTime, Profiler::EVENT_COUNTER, gpuRecord.numPrimitives_[0]);
		events[endQueryIndex + 1].set( s_primCountName, endTime, Profiler::EVENT_COUNTER, gpuRecord.numPrimitives_[1]);

		events[startQueryIndex + 2].set( s_drawPrimCallsName, startTime, Profiler::EVENT_COUNTER, gpuRecord.numDrawPrimCalls_[0]);
		events[endQueryIndex + 2].set( s_drawPrimCallsName, endTime, Profiler::EVENT_COUNTER, gpuRecord.numDrawPrimCalls_[1]);
#endif
	}

	g_profiler.addGPUEntry( name, startEvent.time_ , Profiler::EVENT_START );

	size_t eventCount = queryCount * QUERY_MULTIPLIER;
	for (size_t e = 0; e < eventCount; ++e)
	{
		g_profiler.addGPUEntry(events[e].name_, events[e].time_, events[e].event_, events[e].value_);
	}

	g_profiler.addGPUEntry( name, endEvent.time_ , Profiler::EVENT_END );

#if ENABLE_NVIDIA_PERFKIT
	perfkitFrameUpdate();
#endif
}

int GPUProfiler::beginScope(const char* name)
{
	if (!initialized_ || !resourcesCreated_ || !inFrame_)
	{
		return -1;
	}

	GPUProfileFrame& frame = profilerFrames_[curFrame_];

	if (frame.recordsCount_==MAX_GPU_SCOPE_COUNT)
	{
		return -1;
	}

	int id = static_cast<int>(frame.recordsCount_++);

	GPUProfilerRecord& gpuRecord = frame.profilerRecords_[id];

	gpuRecord.name_ = name;
#ifdef GPU_COUNTERS
  	gpuRecord.numDrawPrimCalls_[0] = rc().liveProfilingData().nDrawcalls_;
  	gpuRecord.numPrimitives_[0]= rc().liveProfilingData().nPrimitives_;
#else
	g_profiler.addEntry(s_primCountName, Profiler::EVENT_COUNTER, rc().liveProfilingData().nPrimitives_);
	g_profiler.addEntry(s_drawPrimCallsName, Profiler::EVENT_COUNTER, rc().liveProfilingData().nDrawcalls_);
#endif
	gpuRecord.startQuery_->Issue(D3DISSUE_END);
	gpuRecord.startIndex_ = frame.queryCount_++;

	return id;
}

void GPUProfiler::endScope(int id)
{
	if (!initialized_ || !resourcesCreated_  || !inFrame_)
	{
		return;
	}

	if (id < 0)
	{
		return;
	}

	GPUProfileFrame& frame = profilerFrames_[curFrame_];
	frame.profilerRecords_[id].endQuery_->Issue(D3DISSUE_END);
	frame.profilerRecords_[id].endIndex_ = frame.queryCount_++;

#ifdef GPU_COUNTERS
	frame.profilerScopes_[id].numDrawPrimCalls_[1] = rc().liveProfilingData().nDrawcalls_;
	frame.profilerScopes_[id].numPrimitives_[1] = rc().liveProfilingData().nPrimitives_;
#else
	g_profiler.addEntry(s_primCountName, Profiler::EVENT_COUNTER, rc().liveProfilingData().nPrimitives_);
	g_profiler.addEntry(s_drawPrimCallsName, Profiler::EVENT_COUNTER, rc().liveProfilingData().nDrawcalls_ );
#endif
}


///////////////////////////////////////////////////////////////////////////////
// NVidia perfkit integration
///////////////////////////////////////////////////////////////////////////////

#if ENABLE_NVIDIA_PERFKIT
struct GPUProfiler::PerfkitCounter
{
	char* name;
	char* display_name;
	PerfkitCounterType type;
	PerfkitProfileType profile_type;
    int32 value;
    bool  valid;
};

typedef GPUProfiler gpu;

GPUProfiler::PerfkitCounter GPUProfiler::g_perfkitCounters_[] =
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Counters
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{ "D3D primitive count",	"NV: Primtive Count",	gpu::perfkitCounterValue,		gpu::perfkitProfileTypeCounters },
	{ "gpu_idle",				"NV: % GPU Idle",		gpu::perfkitCounterPercentage,	gpu::perfkitProfileTypeCounters },
    { "D3D FPS", "NV: Frame Time (1/FPS) (#) in mSec", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D frame time", "NV: Driver Time (#) in mSec", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D time in driver", "NV: Driver Sleep Time (all reasons) (#) in mSec", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D driver sleeping", "NV: Triangle Count (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D triangle count", "NV: Triangle Count Instanced (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D triangle count instanced", "NV: Batch Count (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D batch count", "NV: Locked Render Targets Count (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D Locked Render Targets", "NV: AGP/PCIE Memory Used in Integer MB (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D agpmem MB", "NV: AGP/PCIE Memory Used in Bytes (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D agpmem bytes", "NV: Video Memory Used in Integer MB (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D vidmem MB", "NV: Video Memory Used in Bytes (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D vidmem bytes", "NV: Total video memory available in bytes (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D vidmem total bytes", "NV: Total video memory available in integer MB (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D vidmem total MB", "NV: Total Number of GPU to GPU Transfers (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D SLI P2P transfers", "NV: Total Byte Count for GPU to GPU Transfers (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D SLI P2P Bytes", "NV: Number of IB/VB GPU to GPU Transfers (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D SLI Linear Buffer Syncs", "NV: Byte Count of IB/VB GPU to GPU Transfers (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D SLI Linear Buffer Sync Bytes", "NV: Number of Render Target Syncs (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D SLI Render Target Syncs", "NV: Byte Count of Render Target Syncs (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D SLI Render Target Sync Bytes", "NV: Number of Texture Syncs (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D SLI Texture Syncs", "NV: Byte Count of Texture Syncs (#)", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },
    { "D3D SLI Texture Sync Bytes", "NV: ", gpu::perfkitCounterValue, gpu::perfkitProfileTypeCounters },

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Experiment
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{ "TEX SOL",	"NV: TEX Utilization",	gpu::perfkitCounterPercentage,	gpu::perfkitProfileTypeExperiment },
	{ "ROP SOL",	"NV: ROP Utilization",	gpu::perfkitCounterPercentage,	gpu::perfkitProfileTypeExperiment },
	{ "FB SOL",		"NV: FB Utilization",	gpu::perfkitCounterPercentage,	gpu::perfkitProfileTypeExperiment },
	{ "SHD SOL",	"NV: SHD Utilization",	gpu::perfkitCounterPercentage,	gpu::perfkitProfileTypeExperiment },
	{ "TEX Bottleneck",		"NV: TEX Bottleneck",	gpu::perfkitCounterPercentage,	gpu::perfkitProfileTypeExperiment },
	{ "ROP Bottleneck",		"NV: ROP Bottleneck",	gpu::perfkitCounterPercentage,	gpu::perfkitProfileTypeExperiment },
	{ "FB Bottleneck",		"NV: FB Bottleneck",	gpu::perfkitCounterPercentage,	gpu::perfkitProfileTypeExperiment },
	{ "SHD Bottleneck",		"NV: SHD Bottleneck",	gpu::perfkitCounterPercentage,	gpu::perfkitProfileTypeExperiment },

	// terminator
	{ 0 }
};



void GPUProfiler::perfkitInit()
{
	perfkitInitialised_ = false;

    // Initialize NVPerfKit
    if(GetNvPmApiManager()->Construct(L".\\NvPmApi.Core.dll") != S_OK) 
    {
        DEBUG_MSG( "NVidia PerfKit : Failed load NvPmApi.Core.dll\n" );
		return;
    }

    NVPMRESULT nvResult;
    if((nvResult = GetNvPmApi()->Init()) != NVPM_OK) 
    {
        DEBUG_MSG( "NVidia PerfKit : Failed to initialise\n" );
		return;
	}

    if((nvResult = GetNvPmApi()->CreateContextFromD3D9Device( rc().device(), &g_hNVPMContext)) != NVPM_OK) 
    {
        DEBUG_MSG( "NVidia PerfKit : Failed create context from d3d device\n" );
        return;
    }

	perkitPerformanceAnalyse_ = false;
	perfkitInitialised_ = true;

	perfkitSetupCounters( perfkitProfileTypeCounters );
}

void GPUProfiler::perfkitFini()
{
	if (!perfkitInitialised_)
		return;

	GetNvPmApi()->Shutdown();
}

void GPUProfiler::perfkitSetupExperimentCounters()
{
	if (!perfkitInitialised_)
		return;
}

void GPUProfiler::perfkitSetupCounters( PerfkitProfileType type )
{
	if (!perfkitInitialised_)
		return;

	GetNvPmApi()->RemoveAllCounters( g_hNVPMContext );

	// Add the counters to the NVPerfKit
    int index = 0;
    while (g_perfkitCounters_[index].name != NULL) 
    {
		if (g_perfkitCounters_[index].profile_type == type)
		{
			if (NVPM_OK != GetNvPmApi()->AddCounterByName( g_hNVPMContext, g_perfkitCounters_[index].name ) )
            {
                // error
                DEBUG_MSG( "NVidia PerfKit : Failed to add counter %s\n", g_perfkitCounters_[index].name );
            }
		}
        ++index;
    }	
}

void GPUProfiler::perfkitAddProfileEntry( PerfkitCounter& counter )
{
	if (!perfkitInitialised_)
		return;

	NVPMUINT64 value, cycle;
    NVPMRESULT result = GetNvPmApi()->GetCounterValueByName( g_hNVPMContext, counter.name, 0, &value, &cycle );

    if(result != NVPM_OK)
    {
        // this is common on cards that do not expose all features so just spam
        // DEBUG_MSG( "NVidia PerfKit : Failed to get value for counter %s\n", counter.name );
        counter.value = -1;
        counter.valid = false;
        return;
    }

	int32 pvalue;

	if (counter.type == perfkitCounterPercentage)
		pvalue = (int32)((float)value / cycle * 100.0f); 
	else
		pvalue = (int32)value;

    counter.value = pvalue;
    counter.valid = true;
	g_profiler.addEntry(counter.display_name, Profiler::EVENT_COUNTER, pvalue);	
}

void GPUProfiler::perfkitFrameUpdate()
{
	if (!perfkitInitialised_)
		return;

    NVPMUINT nCount = 0;
    GetNvPmApi()->Sample( g_hNVPMContext, NULL, &nCount );

	int index = 0;
    while (g_perfkitCounters_[index].name != NULL) 
    {
	    perfkitAddProfileEntry(g_perfkitCounters_[index]);
	    ++index;
    }

	if (perkitPerformanceAnalyse_)
	{
		perkitPerformanceAnalyse_ = false;
	}
}


void GPUProfiler::perfkitGetStats( PerfkitStatsArray& stats ) const
{
    int index = 0;
	while (g_perfkitCounters_[index].name != NULL) 
    {
        PerfkitCounter& counter = g_perfkitCounters_[index];

        if (counter.valid)
        {
            PerfkitStat stat;
            stat.first = counter.name;
            stat.second = counter.value;
            stats.push_back(stat);
        }
        ++index;
    }
}

void GPUProfiler::perfkitPerformExperiment()
{
    // switch counters
    perfkitSetupCounters( perfkitProfileTypeExperiment ); 
    perkitPerformanceAnalyse_ = true;
}

#endif // ENABLE_NVIDIA_PERFKIT

#endif // ENABLE_GPU_PROFILER

} // namespace Moo

BW_END_NAMESPACE

// gpu_profiler.cpp
