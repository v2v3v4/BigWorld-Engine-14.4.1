/**
 *	@internal
 *	@file
 */

#ifndef PROFILER_HPP
#define PROFILER_HPP

#include "cstdmf/config.hpp"

// -----------------------------------------------------------------------------
// Section: Macros
// -----------------------------------------------------------------------------
#if !ENABLE_PROFILER

#define PROFILER_SCOPED( name )

#define PROFILER_COUNTER( name, value )

#define PROFILE_FILE_SCOPED( name )

#define PROFILER_SCOPED_DYNAMIC_STRING( name )

#define PROFILER_IDLE_SCOPED( name )

#define PROFILER_LEVEL_COUNTER( name )

#define PROFILER_LEVEL_COUNTER_INC( name, amt )

#define PROFILER_LEVEL_COUNTER_DEC( name, amt )

#else // #if !ENABLE_PROFILER

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/cpuinfo.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/stringmap.hpp"
#include "cstdmf/timestamp.hpp"

#include <stdio.h>

#define PROFILER_SCOPED( name ) \
	ScopedProfiler prof_##name( #name )

#define PROFILER_IDLE_SCOPED( name ) \
	ScopedIdleProfiler prof_##name ( #name )

#define PROFILER_COUNTER( name, value ) \
	g_profiler.addEntry( #name, Profiler::EVENT_COUNTER, value );

// The file profiling is quite extensive and probably isn't needed all the time.
// If you are profiling the fileIO, then turn this on.
#define FILE_PROFILE_ENABLED

#ifdef FILE_PROFILE_ENABLED
	#define PROFILE_FILE_SCOPED( name ) \
		ScopedProfiler prof_##name( #name )
#else
	#define PROFILE_FILE_SCOPED( name )
#endif

#define PROFILER_SCOPED_DYNAMIC_STRING( name  ) \
	ScopedDynamicStringProfiler prof_##__FILE__##__LINE__( name )

#define PROFILER_SCOPED_DYNAMIC_STRING_CATEGORY( name, category ) \
	ScopedDynamicStringProfiler prof_##__FILE__##__LINE__( name, category )

#define PROFILER_LEVEL_COUNTER( name ) \
	ProfilerLevelCounter prof_##name( #name )

#define PROFILER_LEVEL_COUNTER_INC( name, amt ) \
	prof_##name.increase( amt )

#define PROFILER_LEVEL_COUNTER_DEC( name, amt ) \
	prof_##name.decrease( amt )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: HitchDetector
// -----------------------------------------------------------------------------

/**
 *	@internal
 */
class ProfileCSVOutputTask;
class ProcessEntriesTask;
#if ENABLE_PROFILER && ENABLE_HITCH_DETECTION
class HitchDetector
{
public:
	HitchDetector();
	void init( int numSamples, double frameMSLimit, double overAverageFactor );

	int numTimeSamples( void ) const;
	void numTimeSamples( int numSamples );

	double frameMSLimit();
	double overAverageFactor();

	void reset();
	bool accumulateTime( double dt );

private:
	double accumulatedTime_;
	double runningAverage_;
	double overAverageFactor_;
	double lastDTime_;
	double frameMSLimit_;

	int numTimeSamples_;
};
#endif

// -----------------------------------------------------------------------------
// Section: Profiler
// -----------------------------------------------------------------------------

class HierarchyManager;
//class ProfileGraph;
class HierEntry;

class Profiler
{
	friend class ProcessEntriesTask;
	friend class ProfileCSVOutputTask;
	friend class ScopedDynamicStringProfiler;
public:
	enum ProfileMode
	{
		PROFILE_OFF      = 0,
		HIERARCHICAL,
		SORT_BY_TIME,
		SORT_BY_NUMCALLS,
		SORT_BY_NAME,
		CPU_GPU,
		GRAPHS,
#if ENABLE_PER_CORE_PROFILER
		CORES,
#endif
		MAX_MODES
	};

	enum
	{
		/// Arbitrary limit, increase/decrease if necessary
		MAXIMUM_NUMBER_OF_THREADS	= 96,
		MAX_STACK_DEPTH				= 64,
		MAX_NUM_TOP_ENTRIES			= 10,
		HISTORY_LENGTH				= 512
	};

	enum EventType
	{
		EVENT_START,
		EVENT_START_IDLE,
		EVENT_END,
		EVENT_COUNTER
	};

	enum EventCategory
	{
		CATEGORY_CPP	= (1 << 0),
		CATEGORY_GPU	= (1 << 1),
		CATEGORY_PYTHON	= (1 << 2),

		CATEGORY_ALL	= (1 << 3)-1
	};

	enum JsonDumpState
	{
		JSON_DUMPING_INACTIVE,
		JSON_DUMPING_ACTIVE,
		JSON_DUMPING_COMPLETED,
		JSON_DUMPING_FAILED
	};

	struct ProfileEntry
	{
		EventType		event_;
		EventCategory	category_;
		const char *	name_;
		uint64			time_;
		bwThreadID		threadIdx_;
		int32			value_;
#if ENABLE_PER_CORE_PROFILER
		int32			coreNumber_;
#endif
		int32			frameNumber_;
	};


	struct DisplayEntry
	{
		DisplayEntry*	parent_;
		DisplayEntry*	sibling_;
		DisplayEntry*	child_;
		const char*		name_;
		uint64			start_;
		uint64			numInclusiveCycles_;
		uint64			numChildCycles_;
		uint64			numInclusiveIdleCycles_;
		bool			isIdle_;
	};


#if !defined ( MF_SERVER )
	struct ProfileVertex
	{
		float		x;
		float		y;
		float		z;
		float		w;

		uint32		colour_;
	};
#endif

	struct ThreadEntry
	{
		uint			thread_;
		BW::string		name_;
		DisplayEntry *	head_;
		HierEntry *		hierHead_;
		bool			visible_;
#if !defined ( MF_SERVER )
		BW::vector<ProfileVertex> vertexData_;
#endif
	};

	struct ProfileLine
	{
		friend std::ostream& operator<<(std::ostream&, const ProfileLine&);
		friend std::istream& operator>>(std::istream&, ProfileLine&);
		ProfileLine( const char* text )
		{
			colour_ = 0xffffffff;
			text_ = text;
		}
		ProfileLine( uint32 colour, const char* text )
		{
			colour_ = colour;
			text_ = text;
		}

		uint32 colour_;
		BW::string text_;
	};

	struct SortedEntry
	{
		int64 numCycles_;
		const char * entryName_;
		uint32 numCalls_;
	};

	struct EntryBuffer
	{
		bw_atomic32_t entryCount_;
		uint64 endOfFrameTimeStamp_;
		ProfileEntry * entries_;
	};

	Profiler();
	~Profiler();

	CSTDMF_DLL bool init( int memorySize, int32 maxNumThreads = MAXIMUM_NUMBER_OF_THREADS );

	CSTDMF_DLL void fini();

	void addEntry( const char* text, EventType e, int32 value, 
		EventCategory category = CATEGORY_CPP );


	CSTDMF_DLL void tick();

	CSTDMF_DLL void addThisThread( const char* name );
	CSTDMF_DLL void removeThread( uint threadId );
	void doRemoveThreads();

	const char* getThreadName( int threadId ) const;

	CSTDMF_DLL void setProfileMode( ProfileMode mode, bool inclusive );

	CSTDMF_DLL void getStatistics( BW::vector<ProfileLine> *profileOutput );

	BW::vector< HierEntry * >& getHierGraphs()
	{
		return graphableHierEntrys_;
	}

#if !defined( MF_SERVER )
	CSTDMF_DLL BW::vector<ProfileVertex>* getDisplayMesh( int thread );
	CSTDMF_DLL float* getGraph( int thread, int * offset );
	CSTDMF_DLL void addGPUThread();
	CSTDMF_DLL void addGPUEntry( const char* text, int64 time, EventType e, int32 val = 0 );
#endif
	

	void freeze( bool freeze, bool dump )
	{
		if (freeze && !freezeProfiling_)
		{
			doJsonDump_ = dump;
		}
		freezeProfiling_ = freeze;
	}

	bool allowHitchFreezing()
	{
		return allowHitchFreezing_;
	}

	void allowHitchFreezing( bool allow )
	{
		allowHitchFreezing_ = allow;
	}

	bool hitchFreeze()
	{
		return hitchFreeze_;
	}

	void hitchFreeze( bool hitch )
	{
		this->freeze( hitch, (hitch != hitchFreeze_ && hitch) );
		hitchFreeze_ = hitch;
	}

	bool getProfileBarVisibility() { return profileBarVisibility_; }
	void setProfileBarVisibility( bool state )
	{
		profileBarVisibility_ = state;
	}

	bool setNewHistory( const char* historyFileName, const char* threadName, BW::string* msgString );
	void closeHistory();
	void flushHistory();
	void setScreenWidth( float screenWidth )
	{
		screenWidth_ = screenWidth;
	}

	SimpleMutex& getMeshMutex()
	{
		return meshMutex_;
	}

#if ENABLE_HITCH_DETECTION
	HitchDetector& hitchDetector()
	{
		return hitchDetector_;
	}
#endif // ENABLE_HITCH_DETECTION

	void dumpThisFrame()
	{
		doJsonDump_ = true;
	}

	void setJsonDumpCount( int count )
	{
		jsonDumpCount_ = count;
	}

	void watcherSetSortMode( BW::string newValue );
	BW::string watcherGetSortMode() const;
	
	void watcherSetDumpProfile( bool newValue )
	{
		jsonDumpIndex_ = 0;
		doJsonDump_ = newValue;
	}
	
	bool watcherGetDumpProfile() const
	{
		return doJsonDump_;
	}
	
	bool watcherEnabled() const
	{
		return (isActive_ || isPendingActive_);
	}

	CSTDMF_DLL void prevCategory();
	CSTDMF_DLL void nextCategory();
	CSTDMF_DLL void setCurrentCategory( const BW::string & newCategory );
	CSTDMF_DLL BW::string getCurrentCategory();
	CSTDMF_DLL BW::string getCategories() const;

	CSTDMF_DLL void setThreadVisibility( int threadId, bool isVisible );
	CSTDMF_DLL bool getThreadVisibility( int threadId );

	CSTDMF_DLL void toggleHierChildren();
	CSTDMF_DLL void toggleGraph(bool recursive);

	CSTDMF_DLL int nextHierEntry();
	CSTDMF_DLL int prevHierEntry();

	int getFrameNumber() {return frame_;}
	
	void setForceJsonDump( bool dump );

	BW::string getErrorString() { return profileErrorString_; }

	void setJsonDumpDir( const BW::string & jsonDumpDir );

	CSTDMF_DLL JsonDumpState getJsonDumpState();

	void enable( bool state );

	BW::string getJsonOutputFilePath() { return jsonOutputFilePath_; }

private:
	CSTDMF_DLL static const bwThreadID & getThreadIndex();

	ProfileEntry * startNextEntry();

	void internalRemoveThread( uint threadId );

	void addSafeDumpFilePathWatcher();
	
	void jsonDump();
	void stopJsonDump();
	FILE* createNewJsonDumpFile();
	void setJsonDumpState( JsonDumpState jsonDumpState );
	void writeJsonEntries();
	void processEntries();
	void outputReport();

	void zeroHierarchy();
	void updateHierarchy();
	void resetHierarchy();
	HierEntry* findChild(HierEntry* parent, DisplayEntry* entry);

#if !defined( MF_SERVER )
	void generateDisplayMesh();
	void addMeshEntry( BW::vector<ProfileVertex> *vData,
		DisplayEntry * entry,
		int depth,
		int64 frameStart,
		int top );
#endif

	TaskManager taskManager_;

	bool doJsonDump_;
	int jsonDumpIndex_;
	int jsonDumpCount_;
	int64 jsonDumpStartTime_;
	bool jsonFileHasData_;
	FILE * jsonOutputFile_;
	BW::string jsonOutputFilePath_;
	BW::string jsonDumpDir_;
	JsonDumpState jsonDumpState_;

	bool hasGeneratedProfile_;
	// isActive_ needs to be volatile as it set in one thread and
	// read from another.
	volatile bool isActive_;
	int profileFrameNumber_;

	// true when we are waiting for a tick() before turning the system on.
	bool isPendingActive_;

	EntryBuffer profileEntryBuffers_[2];

	EntryBuffer * profileEntryBuffer_;
	EntryBuffer * finalProfileEntryBuffer_;

	SimpleSemaphore processEntrySemaphore_;

	DisplayEntry * displayEntryBuffer_;

	int32 maxNumThreads_;
	ThreadEntry* threads_;
	BW::set<uint> threadsToRemove_;
#if ENABLE_PER_CORE_PROFILER
	ThreadEntry * cores_;		// can we treat each core as a thread. Tasks run on them the same.
	int32 numCores_;
	CpuInfo cpuInfo_;
#endif
	int32 maxEntryCount_;
	int32 highWatermarkOfEntryCount_;

	uint32 displayEntryCount_;
	uint32 maxDisplayEntryCount_;
	uint32 highWatermarkOfDisplayEntryCount_;

	HierEntry* currentHierEntry_;
	HierEntry* parentHierEntry_;
	HierarchyManager* hierarchyManager_;

	BW::vector< HierEntry * > graphableHierEntrys_;

	int32 numThreads_;
	bwThreadID gpuThreadID_;
	bool calcExclusiveTimes_; 
	ProfileMode profileMode_;
	bool freezeProfiling_;
	bool hitchFreeze_;
	bool allowHitchFreezing_;
	float timeTakenForProfiling_;
	float screenWidth_;
	static float s_profileBarWidth;
	bool profileBarVisibility_;

	BW::vector< BW::vector<SortedEntry> > topEntries_;
	BW::vector<int> numTopEntries_;
	SimpleMutex threadMutex_;
	SimpleMutex meshMutex_;
	SimpleMutex getStatsMutex_;
	SimpleMutex jsonOutputFilePathMutex_;
	SimpleMutex jsonDumpDirMutex_;

	int memoryUsed_;
	SmartPointer<ProfileCSVOutputTask> csvOutputTask_;
	SmartPointer<ProcessEntriesTask> processEntriesTask_;

#if ENABLE_HITCH_DETECTION
	HitchDetector hitchDetector_;
#endif // ENABLE_HITCH_DETECTION
	uint64 frameStartTime_;
	BW::vector< BW::vector<float> > perThreadHistory_;
	int frame_;
	bool inited_;
	bool forceJsonDump_;

	BW::string profileErrorString_;
	StringSet<>	dynamicStrings_;
	SimpleMutex dynamicStringsMutex_;
	typedef BW::map<EventCategory, const char*> EventCategoryNames;
	EventCategoryNames eventCategories_;
	EventCategoryNames::const_iterator currentEventCategory_;
	EventCategoryNames::const_iterator finalEventCategory_;

	BW::vector< BW::vector<DisplayEntry *> > dStack_;
	BW::vector< BW::vector<DisplayEntry *> > dChildTails_;
	BW::vector<DisplayEntry *> dHeads_;
	BW::vector<int> totalStackDepth_;

};

CSTDMF_DLL extern Profiler g_profiler;

void bgProcessEntries( void* This );

//------------------------------------------------------------------------------
class ProfileCSVOutputTask : public BackgroundTask
{
public:
	ProfileCSVOutputTask( TaskManager & taskManger );
	~ProfileCSVOutputTask();

	void addProfileData( Profiler::SortedEntry * sortedEntries,
		uint numSortedEntries );

	bool setNewHistory( const char* historyFileName, const char* threadName, BW::string* msgString );
	void closeHistory();
	void flushHistory();
	bool isRunning()
	{
		return historyFile_ != NULL;
	}

	void reset()
	{
		slotsNamesWritten_ = false;
		historyFile_ = NULL;
	}

	virtual void doBackgroundTask( TaskManager & mgr );

	uint32 getProfileThreadIndex() {return profileThreadIndex_;}
private:

	void lock()
	{
		if (lock_.pullTry())
		{
			return;
		}
		int64 start = timestamp();
		lock_.pull();
		WARNING_MSG( "***** ProfileCSV stalled for %fms waiting for lock\n",
			stampsToSeconds( timestamp() - start )*1000.0f );
	}

	void unlock()
	{
		lock_.push();
	}

	typedef Profiler::SortedEntry SortedEntry;
	typedef BW::set<const char*, bool(*)(const char*, const char*)>
		UniqueNamesSet;

	TaskManager & taskManger_;
	FILE * historyFile_;
	uint32 profileThreadIndex_;
	uint32 frameCount_;
	bool slotsNamesWritten_;	
	float timeTakenForProfiling_;

	// unique names associated with the entries added
	UniqueNamesSet uniqueNames_;

	// list of entries submitted so far
	BW::vector<SortedEntry> historyData_;
	const uint32 maxHistoryDataEntries_;

	// list of indices into historyData_
	BW::vector<uint32> perFrameSortedEntries_;
	const uint32 maxPerFrameEntries_;
	
	uint32 maxNumFramesAdded_; // keep track of max number of active frames.

	// we have to use a semaphore as the mutex is reenetrant within the same thread
	// and we need to lock in thread A and unlock in thread B, blocking thread A
	// during future locks.
	SimpleSemaphore lock_;
};

//------------------------------------------------------------------------------

// This class is always inlined, sorry.
#include "profiler.ipp"

// -----------------------------------------------------------------------------
// Section: ScopeProfiler
// -----------------------------------------------------------------------------

/**
 *	@internal
 */

class ScopedProfiler
{
public:
	inline ScopedProfiler( const char* text )
	{
		text_ = text;
		g_profiler.addEntry( text,Profiler::EVENT_START, 0);
	}

	inline ~ScopedProfiler()
	{
		g_profiler.addEntry( text_, Profiler::EVENT_END, 0);
	}

private:
	const char* text_;
};

class ScopedIdleProfiler
{
public:
	inline ScopedIdleProfiler( const char* text )
	{
		text_ = text;
		g_profiler.addEntry( text,Profiler::EVENT_START_IDLE, 0);
	}

	inline ~ScopedIdleProfiler()
	{
		g_profiler.addEntry( text_, Profiler::EVENT_END, 0);
	}

private:
	const char* text_;
};

class ScopedDynamicStringProfiler
{
public:
	inline ScopedDynamicStringProfiler( const char* dynamicText,
		Profiler::EventCategory category = Profiler::CATEGORY_CPP )
	{
		// This code could be called from multiple threads, need to protect g_profiler.dynamicStrings_
		// note that we don't need to protect g_profiler.dynamicStrings_ when we print profiler info
		// because allocated strings are persistent and we are not accessing set when printing/saving data
		SimpleMutexHolder smh( g_profiler.dynamicStringsMutex_ );
		StringSet<>& dynamicStrings = g_profiler.dynamicStrings_;
		// add annotation to our dynamic set if it's still not in there
		// check if we have this string in the cache already
		StringSet<>::iterator it = dynamicStrings.find( dynamicText );
		if (it == dynamicStrings.end())
		{
			// new string, adding it to the dynamic string buffer
			it = dynamicStrings.insert( dynamicText ).first;
		}
		text_ = *it;
		category_ = category;

		g_profiler.addEntry( text_, Profiler::EVENT_START, 0, category_ );
	}

	inline ~ScopedDynamicStringProfiler()
	{
		g_profiler.addEntry( text_, Profiler::EVENT_END, 0, category_ );
	}
private:
	const char* text_;
	Profiler::EventCategory category_;
};


/**
 * Track the level of something (like a buffer) using a Chrome Trace
 * counter.
 *
 * @note The level is output in the profiling data in response to a
 * change in the level, that is via corresponding call to Increase()
 * or Decrease(). If the level never changes, the counter data will be
 * entirely *absent* from the profiling data.
 *
 * @note Counter range is: [0, INT32_MAX]. WARNING: overflow is NOT
 * detected.
 *
 * @note Thread safe.
 */
class ProfilerLevelCounter 
{
public:
	ProfilerLevelCounter( const char *desc )
	{
		desc_ = desc;
		level_ = 0;
	}

	CSTDMF_DLL void increase( int amt );
	CSTDMF_DLL void decrease( int amt );

private:
	bw_atomic32_t level_;		// Assumed to be a signed type.
	const char *desc_;
};


class HierEntry
{
public:
	enum HierState
	{
		HIDDEN		= 0x0,
		VISIBLE		= 0x1,
		HIGHLIGHTED = 0x2,
		GRAPHED		= 0x4,
	};

	HierEntry( HierEntry * pParent, const char * pEntry );
	~HierEntry();

	// change the visibility of the direct children of this entry
	void toggleChildren();

	// turn on graphing for this entry
	// if recursive is true, turn it on for all visible children too.
	void toggleGraph( bool recursive );

	HierEntry* next();

	HierEntry* prev();

	// Given an entry, find the corresponding hierarchical child of parent.
	// If it doesn't exist yet, create one.
	HierEntry* findChild( Profiler::DisplayEntry * pEntry );

	HierEntry*  parent_;
	HierEntry*  sibling_;
	HierEntry*  child_;
	Profiler::EventCategory category_;
	const char* name_;
	uint32		state_;
	float		time_;
	float*		timeArray_;
	int			numCalls_;

	static HierarchyManager* s_Manager;
};

BW_END_NAMESPACE

#endif // #if !ENABLE_PROFILER

#include "external_profiler.hpp"

#endif // PROFILER_HPP
