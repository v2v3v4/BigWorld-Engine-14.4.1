#include "pch.hpp"

#include "profiler.hpp"

#include "bw_namespace.hpp"
#include "guard.hpp"
#include "bw_util.hpp"
#include "dprintf.hpp"
#include "stdmf.hpp"
#include "bw_memory.hpp"
#include "bw_util.hpp"
#include "debug.hpp"
#include "watcher.hpp"
#include "allocator.hpp"

#include <time.h>
#include <stack>
#include <errno.h>

DECLARE_DEBUG_COMPONENT2( "CStdMF", 0 )

BW_BEGIN_NAMESPACE


#if ENABLE_PROFILER

Profiler g_profiler;

namespace
{

THREADLOCAL( bwThreadID ) tls_ThreadIndex_=-1;

uint32 colourHash( uint32 a)
{
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return a | 0xff000000; // set alpha
}

// -----------------------------------------------------------------------------
// Section: Sorting methods
// -----------------------------------------------------------------------------
/**
 * Sort by most costly (number of cycles)
 */
bool SortByCostliest( const Profiler::SortedEntry & a,
					const Profiler::SortedEntry & b )
{
	return a.numCycles_ > b.numCycles_;
}


/**
 * Sort by number of calls
 */
bool SortByNumCalls( const Profiler::SortedEntry & a,
					const Profiler::SortedEntry & b )
{
	return a.numCalls_ > b.numCalls_;
}


/**
 * Sort by name pointer - fast but order is indeterminate
 */
bool SortByNamePtr( const Profiler::SortedEntry & a,
					const Profiler::SortedEntry & b )
{
	return a.entryName_ < b.entryName_; // to match BW::set<> order
}


/**
 * Sort alphabetically - slow
 */
bool SortByName( const Profiler::SortedEntry & a,
				const Profiler::SortedEntry & b )
{
	return strcmp( a.entryName_, b.entryName_ ) < 0;
}


/**
 * Sort by const char*
 */
bool SortByNameChar( const char* a, const char* b )
{
	return strcmp( a, b ) < 0;
}

BW::string indexToString2Digit( const void * base, int index )
{
	char temp[32];
	bw_snprintf( temp, sizeof(temp), "%02d", index );
	return temp;
}

int stringToIndex2Digit( const void * base, const BW::string & name )
{
	return atoi( name.c_str() );
}

#if ENABLE_WATCHERS

typedef BW::vector< Profiler::ProfileLine > ProfileLines;

/**
 * This class for profile watcher, to display the output results
 * by calling Profiler::getStatistics before it iterates over the
 * children.
 */
class ProfileLinesWatcher : public SequenceWatcher<ProfileLines>
{
public:
	ProfileLinesWatcher() : 
		SequenceWatcher<ProfileLines>( profileOutput_ ),
		profileOutput_()
	{
		this->setStringIndexConverter( &stringToIndex2Digit,
			&indexToString2Digit );
		this->addChild( "*",  makeWatcher( &Profiler::ProfileLine::text_ ) );
	}

	virtual bool visitChildren( const void * base, const char * path,
		WatcherPathRequest & pathRequest )
	{
		g_profiler.getStatistics( &profileOutput_ );
		return SequenceWatcher<ProfileLines>::
			visitChildren( base, path, pathRequest );
	}
private:
	ProfileLines profileOutput_;
};


bool watcherDummyEntry()
{
	return false;
}

void watcherSetNextEntry( bool newValue )
{
	g_profiler.nextHierEntry();
}

void watcherSetPrevEntry( bool newValue )
{
	g_profiler.prevHierEntry();
}

void watcherSetToggleGraph( bool newValue )
{
	g_profiler.toggleHierChildren();
}

void watcherSetPrevCategory( bool newValue )
{
	g_profiler.prevCategory();
}

void watcherSetNextCategory( bool newValue )
{
	g_profiler.nextCategory();
}

void watcherSetCurrentCategory( BW::string newCategory )
{
	g_profiler.setCurrentCategory( newCategory );
}

BW::string watcherGetCurrentCategory()
{
	return g_profiler.getCurrentCategory();
}

/* ';' separated category names*/
BW::string watcherGetCategories()
{
	return g_profiler.getCategories();
}

int watcherGetJsonDumpState()
{
	return g_profiler.getJsonDumpState();
}

#endif // ENABLE_WATCHERS

} // anonymous namespace


std::ostream& operator<<(std::ostream & s, const Profiler::ProfileLine& t)
{
	BW_GUARD;
	s << t.text_;
	return s;
}

std::istream& operator>>( std::istream & s, Profiler::ProfileLine& t)
{
	BW_GUARD;
	s >> t.text_;
	return s;
}

// -----------------------------------------------------------------------------
// Section: Hitch Detector
// -----------------------------------------------------------------------------
#if ENABLE_HITCH_DETECTION

/**
 * Hitch Detector is used to detect slow frames and trigger a JSON dump of the
 * frame in order to figure out what went wrong.
 */
HitchDetector::HitchDetector() : 
	accumulatedTime_( 0.0 ),
	runningAverage_( 0.0 ),
	overAverageFactor_( 1.5 ),
	lastDTime_( 0.0 ),
	frameMSLimit_( 100.0),
	numTimeSamples_( 10 )
{
	BW_GUARD;

	// add watchers
	MF_WATCH( "profiler/hitch/overAverageFactor", overAverageFactor_ );
	MF_WATCH( "profiler/hitch/numTimeSamples",
		*this, MF_ACCESSORS( int, HitchDetector, numTimeSamples ) );
	MF_WATCH( "profiler/hitch/frameMSLimit", frameMSLimit_ );
}


/**
 * Initalize the hitch detection system
 */
void HitchDetector::init( int numSamples, double frameMSLimit,
						 double overAverageFactor )
{
	BW_GUARD;

	// read in vars from xml
	overAverageFactor_ = overAverageFactor;
	frameMSLimit_ = frameMSLimit;
	numTimeSamples_ = numSamples;

	this->reset();
}


/**
 * Get the number of time samples
 */
int HitchDetector::numTimeSamples( void ) const
{
	return numTimeSamples_;
}


/**
 * Set the number of time samples
 */
void HitchDetector::numTimeSamples( int numSamples )
{
	BW_GUARD;
	numTimeSamples_ = numSamples;
	this->reset();
}


/**
 * Get the frame millisecond limit
 */
double HitchDetector::frameMSLimit()
{
	return frameMSLimit_;
}


/**
 * Get the over average frame factor
 */
double HitchDetector::overAverageFactor()
{
	return overAverageFactor_;
}


/**
 * Reset the times
 */
void HitchDetector::reset()
{
	BW_GUARD;
	accumulatedTime_ = lastDTime_ * numTimeSamples_;
	runningAverage_ = lastDTime_;
}


/**
 * Accumulate time
 */
bool HitchDetector::accumulateTime( double dt )
{
	BW_GUARD;
	if ( accumulatedTime_ == 0.0 )
	{
		accumulatedTime_ = dt * numTimeSamples_;
		runningAverage_ = dt;
	}

	accumulatedTime_ -= runningAverage_;
	accumulatedTime_ += dt;
	runningAverage_ = accumulatedTime_ / numTimeSamples_;

	lastDTime_ = dt;

	// if the frame time is over the limit then we need to trigger a dump.
	// For the moment, spike detection has been disabled as it triggers too often.
	if ((lastDTime_ > frameMSLimit_) /*|| (lastDTime_ > runningAverage_ * overAverageFactor_)*/)
	{
		reset();
		return true;
	}
	return false;
}
#endif // ENABLE_HITCH_DETECTION


// -----------------------------------------------------------------------------
// Section: ProcessEntriesTask
// -----------------------------------------------------------------------------
class ProcessEntriesTask : public BackgroundTask
{
public:
	ProcessEntriesTask() : BackgroundTask( "Process Entries" )
	{
	}

	void doBackgroundTask( TaskManager & mgr )
	{
		g_profiler.processEntries();
		g_profiler.processEntrySemaphore_.push();
	}

	void cancel( TaskManager & mgr )
	{
		// If we are being cancelled, push the semaphore
		// to allow the Profiler to finish cleanly
		g_profiler.processEntrySemaphore_.push();
	}
};


//------------------------------------------------------------------------------
// Section: Hierarchy Management
//------------------------------------------------------------------------------

HierarchyManager* HierEntry::s_Manager = NULL;

/**
 * Hierarchy Manager handles the profile hierarchy view data.  Keeps a pool of
 * HierEntries that are allocated from and deals with creation and destruction
 * of those entries. Is only used within the Profiler.
 */
class HierarchyManager
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param numHEntries	the maximum number of entries allowed.
	 *	@param numSlices	the number of time slices for graphing.
	 */
	HierarchyManager( int numHEntries, int numSlices )
	{
		maxEntryCount_ = numHEntries;
		// allocate the buffer from which the hierarchy entries are allocated.
		pEntryPool_ = (HierEntry *)bw_malloc( sizeof( HierEntry ) *
			maxEntryCount_ );

		MF_ASSERT( pEntryPool_ );

		entryCount_ = 0;
		maxNumHTimeEntries_ = numSlices;

		HierEntry::s_Manager = this;
	}

	/**
	 * Reset the hierarchy after a filter change
	 */
	void reset( HierEntry * pParent )
	{
		MF_ASSERT( &pEntryPool_[ 0 ] == pParent );
		// Reset to 1 as 0 is parent/dummy
		entryCount_ = 1;
		pParent->child_ = NULL;
		pParent->sibling_ = NULL;
	}


	/**
	 *	Destructor.
	 */
	~HierarchyManager()
	{
		bw_free( pEntryPool_ );
	}

	/**
	 *	This method creates a new HierEntry based on entry.
	 *
	 *	@param pParent
	 *	@param pEntry
	 */
	HierEntry * newEntry( HierEntry * pParent, const char * pEntry )
	{
		if (entryCount_ >= maxEntryCount_)
		{
			return NULL;
		}
		
		HierEntry * newEntry = new( &pEntryPool_[ entryCount_++ ] )
			HierEntry( pParent, pEntry );

		MF_ASSERT( newEntry != NULL );

		return newEntry;
	}

	/**
	 *	This method given an entry, finds the corresponding hierarchical
	 *	child of parent.
	 *
	 *	If it doesn't exist yet, create one.
	 *
	 *	@param pParent
	 *	@param pEntry
	 */
	HierEntry * findChild( HierEntry * pParent,
		const char * pEntry )
	{
		HierEntry * current = pParent->child_;
		HierEntry * lastSib = NULL;

		while (current)
		{
			if (current->name_ == pEntry)
			{
				break;
			}

			lastSib = current;
			current = current->sibling_;
		}

		if (!current)
		{
			// couldn't find existing hierarchical entry so add a new one.
			current = this->newEntry( pParent, pEntry );

			if (!pParent->child_)
			{
				// parent had no children so this is the first one.
				pParent->child_ = current;
			}
			else
			{
				// children exist, so add this to the sibling list.
				MF_ASSERT( lastSib );
				lastSib->sibling_ = current;
			}
		}

		return current;
	}


	/**
	 *	This method resets all times to zero.
	 *
	 *	We do this at the beginning of each frame as the same entry can be 
	 *	duplicated (add the times) or may not be in this frame (so the time 
	 *	needs to be reset to zero).
	 */
	void zeroHierarchy()
	{
		// We could traverse the hierarchy in a depth first manner or, we could
		// do it the smart way. We already have a contiguous array of the
		// entries so we can iterate through that linearly. 
		for (int i = 0; i < entryCount_; i++)
		{
			HierEntry * h = &pEntryPool_[ i ];
			h->time_ = 0.0f;

			if (h->timeArray_)
			{
				h->timeArray_[ g_profiler.getFrameNumber() %
					maxNumHTimeEntries_] = 0.0f;
			}

			h->numCalls_ = 0;

			h->state_ &= ~HierEntry::HIGHLIGHTED; // clear highlighted field

		}
	}

	/**
	 *	This method returns the maxmimum number of time entries.
	 */
	int32 getMaxNumberOfTimeEntries() { return maxNumHTimeEntries_; }

private:
	friend class HierEntry;

	int32 entryCount_;
	int32 maxEntryCount_;
	int maxNumHTimeEntries_;

	HierEntry * pEntryPool_;
	//float * pHierarchicalTimeBuffers;
};


//------------------------------------------------------------------------------
//  HierEntry
//------------------------------------------------------------------------------

/**
 *	Constructor.
 */
HierEntry::HierEntry( HierEntry * pParent, const char * pEntry )
{
	parent_ = pParent;
	child_ = NULL;
	sibling_ = NULL;
	name_ = pEntry;
	state_ = 0;
	time_ = 0;
	timeArray_ = NULL;
	numCalls_ = 0;
};


/**
 *	Destructor.
 */
HierEntry::~HierEntry() 
{
	if (timeArray_)
	{
		bw_free( timeArray_ );
	}
};



/**
 *	This method changes the visibility of the direct children of this entry.
 */
void HierEntry::toggleChildren()
{
	HierEntry * pEntry = child_;

	//	bool currentState = entry->state_&VISIBLE;
	while (pEntry)
	{
		pEntry->state_ ^= VISIBLE;
		pEntry = pEntry->sibling_;
	}
}


/**
 *	This method turns on the graphing (history storage for this entry).
 *
 *	@param recursive	if true, then all visible children are toggled as well.
 */
void HierEntry::toggleGraph( bool recursive )
{
	if (!timeArray_)
	{
		timeArray_ = (float*)bw_malloc(
			s_Manager->maxNumHTimeEntries_ * sizeof( float ) );
		memset( timeArray_, 0,
			s_Manager->maxNumHTimeEntries_ * sizeof( float ) );
	}
	else
	{
		bw_free( timeArray_ );
		timeArray_ = NULL;
	}

	if (recursive)
	{
		HierEntry * c = child_;

		if (c && (c->state_ & VISIBLE))
		{
			c->toggleGraph( recursive );

			while ((c = c->sibling_))
			{
				c->toggleGraph( recursive );
			}
		}
	}

}


/**
 *	This method returns the next HierEntry.
 */
HierEntry * HierEntry::next()
{
	if (child_ && (child_->state_ & VISIBLE))
	{
		return child_;
	}

	// otherwise, is a sibling
	else if (sibling_)
	{
		return sibling_;
	}

	// otherwise, no siblings left and no children, therefore step up to the 
	// parent's next sibling. Note that if this node is visible, then it's 
	// parent and sibs should also be visible.
	else 
	{
		HierEntry * pEntry = parent_;

		while (pEntry && !pEntry->sibling_)
		{
			pEntry = pEntry->parent_;
		}

		if (pEntry)
		{
			return pEntry->sibling_;
		}
		else
		{
			return NULL;
		}
	}
}


/**
 *	This method returns the previous HierEntry.
 */
HierEntry * HierEntry::prev()
{
	// will either be the sibling that points to it or its parent
	HierEntry * prevOne = parent_;

	if (!prevOne) // no parent. This is the root entry
	{
		return NULL; 
	}

	// first child of parent is current, so parent is prev
	if (prevOne->child_ == this)
	{
		return prevOne;
	}

	prevOne = prevOne->child_;

	// look for the child that points to this entry. 
	while (prevOne->sibling_!=this)
	{
		prevOne = prevOne->sibling_;
	}

	// get last sibling of last child of that previous sibling
	while (prevOne->child_ && (prevOne->child_->state_ & VISIBLE))
	{
		prevOne = prevOne->child_;

		while (prevOne->sibling_)
		{
			prevOne = prevOne->sibling_;
		}
	}


	return prevOne;
}


// -----------------------------------------------------------------------------
// Section: Profiler
// -----------------------------------------------------------------------------

float Profiler::s_profileBarWidth = 66.666f;


/**
 *	Profiler constructor
 *	@see Profiler::init
 */
Profiler::Profiler() :
	taskManager_(),
	doJsonDump_( false ),
	jsonDumpIndex_( 0 ),
	jsonDumpCount_( 1 ),
	jsonDumpStartTime_( 0 ),
	jsonFileHasData_( false ),
	jsonOutputFile_( NULL ),
	jsonDumpState_( JSON_DUMPING_INACTIVE ),
	hasGeneratedProfile_( false ),
	isActive_( false ),
	profileFrameNumber_( 0 ),
	isPendingActive_( false ),
	profileEntryBuffer_( NULL ),
	finalProfileEntryBuffer_( NULL ),
	processEntrySemaphore_(),
	displayEntryBuffer_( NULL ),
	maxNumThreads_( MAXIMUM_NUMBER_OF_THREADS ),
	threads_( NULL ),
#if ENABLE_PER_CORE_PROFILER
	cores_( NULL ),
	numCores_( 0 ),
#endif
	maxEntryCount_( 0 ),
	highWatermarkOfEntryCount_( 0 ),
	displayEntryCount_( 0 ),
	maxDisplayEntryCount_( 0 ),
	numThreads_( 0 ),
	gpuThreadID_( 0 ),
	calcExclusiveTimes_( false ),
	profileMode_( SORT_BY_TIME ),
	freezeProfiling_( false ),
	hitchFreeze_( false ),
	allowHitchFreezing_( false ),
	timeTakenForProfiling_( 0.0f ),
	screenWidth_( 0.0f ),
	threadMutex_(),
	meshMutex_(),
	memoryUsed_( 0 ),
	csvOutputTask_( NULL ),
	processEntriesTask_(),
#if ENABLE_HITCH_DETECTION
	hitchDetector_(),
#endif
	frameStartTime_( 0 ),
	frame_( 0 ),
	inited_( false ),
	forceJsonDump_( false )
{
	BW_GUARD;

	csvOutputTask_ = new ProfileCSVOutputTask( taskManager_ );


}


/**
 *	Destructor for the profiler, this method will dump the memory usage stats of
 *	the profiler
 */
Profiler::~Profiler()
{
	BW_GUARD;

	if (hasGeneratedProfile_)
	{
		dprintf( "---------------------Profile System Stats---------------------\n");
		dprintf( " Max number of entries = %i\n", highWatermarkOfEntryCount_ );
		dprintf( " Max number of display entries = %i\n",
			highWatermarkOfDisplayEntryCount_);

		int32 maxMemoryUsed = highWatermarkOfEntryCount_ *
				sizeof( ProfileEntry ) * 2 +
				highWatermarkOfDisplayEntryCount_ * sizeof(DisplayEntry);
		dprintf( " Max memory used / Memory allocated = %2.2fMB/%2.2fMB\n",
			maxMemoryUsed / (1024.0f * 1024.0f),
			memoryUsed_ / (1024.0f * 1024.0f) );
		dprintf( "--------------------------------------------------------------\n" );
	}

	if (inited_)
	{
		this->fini();
	}
}


/**
 *
 */
void Profiler::watcherSetSortMode( BW::string newValue )
{
	if (newValue == "SORT_BY_NAME")
	{
		this->setProfileMode( SORT_BY_NAME, !calcExclusiveTimes_ );
	}
	else if (newValue == "SORT_BY_TIME")
	{
		this->setProfileMode( SORT_BY_TIME, !calcExclusiveTimes_ );
	}
	else if (newValue == "SORT_BY_NUMCALLS")
	{
		this->setProfileMode( SORT_BY_NUMCALLS, !calcExclusiveTimes_ );
	}
	else if (newValue == "HIERARCHICAL")
	{
		this->setProfileMode( HIERARCHICAL, !calcExclusiveTimes_ );
	}

}


/**
 *
 */
BW::string Profiler::watcherGetSortMode() const
{
	if (profileMode_ == SORT_BY_NAME)
	{
		return "SORT_BY_NAME";
	}
	else if (profileMode_ == SORT_BY_TIME)
	{
		return "SORT_BY_TIME";
	}
	else if (profileMode_ == SORT_BY_NUMCALLS)
	{
		return "SORT_BY_NUMCALLS";
	}
	else if (profileMode_ == HIERARCHICAL)
	{
		return "HIERARCHICAL";
	}

	return "";
}


/*
 * Add the json dump file path watcher
 */
void Profiler::addSafeDumpFilePathWatcher()
{
	Watcher & rootWatcher = Watcher::rootWatcher();	
	
	WatcherPtr pDumpFilePathWatcher = makeWatcher( jsonOutputFilePath_, 
											Watcher::WT_READ_ONLY );									
	WatcherPtr pSafeDumpFilePathWatcher = new SafeWatcher(
			pDumpFilePathWatcher, jsonOutputFilePathMutex_ );
			
	rootWatcher.addChild( "profiler/dumpFilePath", pSafeDumpFilePathWatcher);
}


/**
 *	Initalize the profiler
 *	This method will alllocate memory for the profiler, into two seperate
 *	buffers which will be swapped each Profiler::tick
 *
 *	@param memorySize	The amount of memory to allocate for the profiler
 *	@param maxNumThreads	The maximum number of threads in the profiler
 */
bool Profiler::init(int memorySize, int maxNumThreads)
{
	BW_GUARD;
	memoryUsed_ = memorySize;

	TRACE_MSG( "ProfilerSystem: %dmb memory used.\n",
		(memorySize / 1024) / 1024 );

	taskManager_.initWatchers( "Profiler" );

#ifndef MF_SERVER
	MF_WATCH( "profiler/profileBarWidth",
		s_profileBarWidth, Watcher::WT_READ_WRITE,
		"Profiler bar width (in ms)" );
#endif
	MF_WATCH( "profiler/enabled", *this, 
		&Profiler::watcherEnabled,
		&Profiler::enable, 
		"Dump the profiling data for this frame" );
	MF_WATCH( "profiler/dumpFrameCount", 
		jsonDumpCount_,
		Watcher::WT_READ_WRITE,
		"The amount of frames to store in the JSON dump" );
	MF_WATCH( "profiler/dumpFrameIndex", 
		jsonDumpIndex_,
		Watcher::WT_READ_ONLY,
		"The amount of frames that have been store in the JSON dump" );
	MF_WATCH( "profiler/dumpProfile", *this, 
		&Profiler::watcherGetDumpProfile,
		&Profiler::watcherSetDumpProfile, 
		"Dump the profiling data for this frame" );
	MF_WATCH( "profiler/sortMode", *this, 
		&Profiler::watcherGetSortMode,
		&Profiler::watcherSetSortMode,
		"The sorting mode of the profiler" );
	MF_WATCH( "profiler/exclusive", calcExclusiveTimes_,
		Watcher::WT_READ_WRITE,
		"Calculating exclusive or inclusive" );
	MF_WATCH("profiler/controls/nextEntry",
		&watcherDummyEntry,
		&watcherSetNextEntry,
		"Move to the next entry in the list" );
	MF_WATCH("profiler/controls/prevEntry",
		&watcherDummyEntry,
		&watcherSetPrevEntry,
		"Move to the prev entry in the list" );
	MF_WATCH("profiler/controls/toggleEntry",
		&watcherDummyEntry,
		&watcherSetToggleGraph,
		"Toggle the current entry in the list" );
	MF_WATCH("profiler/controls/prevCategory",
		&watcherDummyEntry,
		&watcherSetPrevCategory,
		"Move to the previous category" );
	MF_WATCH("profiler/controls/nextCategory",
		&watcherDummyEntry,
		&watcherSetNextCategory,
		"Move to the next category" );
	MF_WATCH("profiler/controls/currentCategory",
		&watcherGetCurrentCategory,
		&watcherSetCurrentCategory,
		"Current category" );
	MF_WATCH("profiler/controls/categories",
		&watcherGetCategories,
		"The category names" );
	MF_WATCH("profiler/dumpState",
		&watcherGetJsonDumpState,
		"Current Json Dump State" );
	this->addSafeDumpFilePathWatcher();

	eventCategories_[ CATEGORY_CPP ]	= "C++";
	eventCategories_[ CATEGORY_PYTHON ]	= "Python";

#ifndef MF_SERVER
	eventCategories_[ CATEGORY_GPU ]	= "GPU";
#endif // MF_SERVER

	currentEventCategory_ = 
		eventCategories_.insert( std::make_pair( CATEGORY_ALL, "All" ) ).first;

#if ENABLE_WATCHERS
	Watcher::rootWatcher().addChild( "profiler/statistics",
		new ProfileLinesWatcher() );
#endif // ENABLE_WATCHERS

	// We need 2 buffers for the profile entries (double buffered) and
	// one of the display entries there are half as many display entries
	// as profile entries (approx)
	int num = memorySize / (sizeof(ProfileEntry)*2 + sizeof(DisplayEntry)/2);

	maxEntryCount_ = num;
	maxDisplayEntryCount_ = num/2;

	// 2 buffers
	profileEntryBuffers_[0].entries_ = 
		(ProfileEntry*)bw_malloc( maxEntryCount_*2*sizeof(ProfileEntry) );
	profileEntryBuffers_[1].entries_ = profileEntryBuffers_[0].entries_ + num;

	displayEntryBuffer_ =
		(DisplayEntry*)bw_malloc( maxDisplayEntryCount_*sizeof(DisplayEntry) );

	profileEntryBuffer_ = &profileEntryBuffers_[0];
	profileEntryBuffer_->entryCount_ = 0;

	finalProfileEntryBuffer_ = &profileEntryBuffers_[1];
	finalProfileEntryBuffer_->entryCount_ = 0;

	maxNumThreads_ = maxNumThreads;
	threads_ = new ThreadEntry[maxNumThreads_];

	for (int i=0; i < maxNumThreads_; ++i)
	{
		threads_[i].thread_ = 0;
		threads_[i].head_ = NULL;
		threads_[i].hierHead_ = NULL;
		threads_[i].name_ = "Dead Thread";
		threads_[i].visible_ = false;
#if !defined( MF_SERVER )
		threads_[i].vertexData_.clear();
#endif //!defined( MF_SERVER )
	}

	numThreads_ = 0;
	displayEntryCount_ = 0;

	topEntries_.resize( maxNumThreads_,
		BW::vector<SortedEntry> ( MAX_NUM_TOP_ENTRIES ) );
	numTopEntries_.resize( maxNumThreads_, 0 );
	perThreadHistory_.resize( maxNumThreads_,
		BW::vector<float> ( HISTORY_LENGTH, 0.0 ) );

	//pre-allocate for Profiler::processEntries
	dStack_.resize( maxNumThreads_,
		BW::vector<DisplayEntry *> ( MAX_STACK_DEPTH, (DisplayEntry *)NULL ) );
	dChildTails_.resize( maxNumThreads_,
		BW::vector<DisplayEntry *> ( MAX_STACK_DEPTH, (DisplayEntry *)NULL ) );
	dHeads_.resize( maxNumThreads_, (DisplayEntry *)NULL );
	totalStackDepth_.resize( maxNumThreads_, 0 );

#if ENABLE_PER_CORE_PROFILER
	numCores_ = cpuInfo_.numberOfSystemCores() + cpuInfo_.numberOfLogicalCores() + 1;	// +1 for GPU
	cores_ = new ThreadEntry[numCores_];
	for (int i=0; i < numCores_; ++i)
	{
		char buffer[32];

		cores_[i].thread_ = i+1;
		cores_[i].head_ = NULL;
		cores_[i].hierHead_ = NULL;
		bw_snprintf(buffer, sizeof(buffer), "Core %d: %s", i, cpuInfo_.isLogical(i)?"Logical":"Physical" );
		cores_[i].name_ = buffer;
		cores_[i].visible_ = false;
#if !defined( MF_SERVER )
		cores_[i].vertexData_.clear();
#endif //!defined( MF_SERVER )
		numTopEntries_[i] = 0;
	}
#endif
	hitchFreeze_ = false;
	allowHitchFreezing_ = false;
	freezeProfiling_ = false;

	calcExclusiveTimes_ = false;
	profileMode_ = SORT_BY_TIME;
	isActive_ = false;
	isPendingActive_ = false;

	// Add the main thread to the profile system
	g_profiler.addThisThread( "MainThread" );

	timeTakenForProfiling_ = 0.0f;

	processEntrySemaphore_.push();

	processEntriesTask_ = new ProcessEntriesTask;

	inited_ = true;
	frame_ = 0;

	const int maxEntries = 1000;
	const int numTimeSlices = 256;

	hierarchyManager_ = new HierarchyManager( maxEntries, numTimeSlices );

	parentHierEntry_ = hierarchyManager_->newEntry( NULL, "All Threads" );
	currentHierEntry_ = parentHierEntry_;
	profileBarVisibility_ = true;
	
	// By default, dumping to the executable directory
	jsonDumpDir_ = BWUtil::executableDirectory();

	return true;
}


/**
 * Finialize the profiler
 */
void Profiler::fini()
{
	BW_GUARD;

	taskManager_.stopAll();

	this->enable( false );

	// make sure we have finished processing entries
	processEntrySemaphore_.pull();

	bw_free( profileEntryBuffers_[ 0 ].entries_ );
	bw_free( displayEntryBuffer_ );
	bw_safe_delete_array( threads_ );
	bw_safe_delete( hierarchyManager_ );
	processEntriesTask_  = NULL;
#if ENABLE_PER_CORE_PROFILER
	bw_safe_delete_array( cores_ );
#endif
	
	inited_ = false;
}


/**
 * This method will swap the entry buffers and start the processing of the
 * current entry buffer, this should be called once per frame in order for
 * hitch detection to work correctly.
 */
void Profiler::tick()
{
	BW_GUARD;
	if (!isPendingActive_ && !isActive_)
	{
		return;
	}

	if (taskManager_.numRunningThreads() == 0)
	{
		taskManager_.startThreads( "Profiler", 1 );
	}

	if (isActive_)
	{
		// this will always be positive, but just in case.
		if (profileEntryBuffer_->entryCount_)
		{
			// wait for the processEntries function to finish on another thread.
			if (!processEntrySemaphore_.pullTry())
			{
				int64 start = timestamp();
				processEntrySemaphore_.pull();
				WARNING_MSG( "***** Profiler::tick() stalled "
					"for %fms waiting for lock\n",
					stampsToSeconds( timestamp() - start ) * 1000.0f );
				//doJsonDump = true;
			}

			finalProfileEntryBuffer_->entryCount_ = 0;
			std::swap( finalProfileEntryBuffer_, profileEntryBuffer_ );
			profileFrameNumber_++;
			finalProfileEntryBuffer_->endOfFrameTimeStamp_ = timestamp();
			finalEventCategory_ = currentEventCategory_;

			int32 entryCount = finalProfileEntryBuffer_->entryCount_;
			highWatermarkOfEntryCount_ = MF_MAX( highWatermarkOfEntryCount_,
				entryCount );

			MF_ASSERT( entryCount <= maxEntryCount_ &&
					"Profile buffer overflow. Reduce the amount of profiling or"
					" increase the buffer size  in <debug/profilerBufferSize> in engine_config.");

#if ENABLE_HITCH_DETECTION
			double totalFrameTime = stampsToSeconds(
				timestamp()-frameStartTime_ ) * 1000.0f;

			if (allowHitchFreezing_)
			{
				// only hitch if it is enabled.
				bool doFreeze = hitchDetector_.accumulateTime( totalFrameTime );

				if (doFreeze && !hitchFreeze_)
				{
					freezeProfiling_ = true;
					doJsonDump_ = true;
					hitchFreeze_ = true;
				}
			}
#endif // ENABLE_HITCH_DETECTION

			if (forceJsonDump_)
			{
				// handy in cases where there is a lot of profile processing
				// (json fileIO for example) and you need to ensure that the
				// profiling has finished before starting the next frame.
				processEntries();
				processEntrySemaphore_.push();
			}
			else
			{
				taskManager_.addBackgroundTask( 
					processEntriesTask_, TaskManager::HIGH );
			}
		}
	}
	isPendingActive_ = false;

	isActive_ = true;

	frameStartTime_ = timestamp();
}


/**
 *	Writes profiling data out file in json format that can be read by
 *	Chrome://tracing.
 *
 *	Docs here: http://code.google.com/p/trace-viewer/wiki/TraceEventFormat
 */
void Profiler::jsonDump()
{
	PROFILER_SCOPED( jsonDump );
	BW_GUARD;

	++jsonDumpIndex_;

	if (jsonDumpIndex_ == 1)
	{
		PROFILER_SCOPED( jsonDump_open );
		if(jsonOutputFile_ != NULL)
		{
			fprintf( jsonOutputFile_, "]");
			fclose( jsonOutputFile_ );
			jsonOutputFile_ = NULL;
		}

		jsonDumpStartTime_ = finalProfileEntryBuffer_->entries_->time_;

		jsonOutputFile_ = createNewJsonDumpFile();
		if ( jsonOutputFile_ == NULL )
		{
			jsonDumpIndex_ = 0;
			doJsonDump_ = false;
			this->setJsonDumpState( JSON_DUMPING_FAILED );

			return;
		}

		this->setJsonDumpState( JSON_DUMPING_ACTIVE );

		jsonFileHasData_ = false;

		fprintf( jsonOutputFile_, "[");
	}


	this->writeJsonEntries();

	fflush( jsonOutputFile_ );

	if (jsonDumpIndex_ == jsonDumpCount_)
	{
		this->stopJsonDump();
	}
}


/**
 *  Stop dumping the Json file
 */
void Profiler::stopJsonDump()
{
	PROFILER_SCOPED( jsonDump_close );
	fprintf( jsonOutputFile_, "]" );
	fclose( jsonOutputFile_ );
	jsonOutputFile_ = NULL;
	jsonDumpIndex_ = 0;
	doJsonDump_ = false;
	this->setJsonDumpState( JSON_DUMPING_COMPLETED );
}


/**
 *  Set the json dump state 
 */
void Profiler::setJsonDumpState( JsonDumpState jsonDumpState )
{
	jsonDumpState_ = jsonDumpState;
}


/**
 *  Get the json dump state 
 */
Profiler::JsonDumpState Profiler::getJsonDumpState()
{
	return jsonDumpState_;
}


/**
 *  Set the directory that profile json should be dumped to
 */
void Profiler::setJsonDumpDir( const BW::string & jsonDumpDir )
{
	SimpleMutexHolder smh( jsonDumpDirMutex_ );

	jsonDumpDir_ = jsonDumpDir;
}


/**
 *  Create a json dump file under jsonDumpDir_
 *
 *	@return the FILE pointer of the file just created or NULL if fails
 *
 */
FILE* Profiler::createNewJsonDumpFile()
{
	time_t now = time( NULL );
	tm* t = localtime( &now );
	char fullFilePath[BW_MAX_PATH];

	// please keep this naming schema because WebConsole
	// relies on this schema to download json dump file
	// compose the JSON file path
	{
		SimpleMutexHolder smh( jsonDumpDirMutex_ );

		bw_snprintf( fullFilePath, sizeof(fullFilePath),
			"%s/%s_profile_%04d%02d%02d_%02d%02d%02d.json",
			jsonDumpDir_.c_str(),
			BWUtil::executableBasename().c_str(),
			1900+t->tm_year, t->tm_mon+1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec );
	}


	// try to create the file
	FILE* fp = bw_fopen( fullFilePath, "w" );
	if (fp != NULL)
	{
		{
			SimpleMutexHolder smh( jsonOutputFilePathMutex_ );
			jsonOutputFilePath_ = fullFilePath;
		}
		INFO_MSG( "Profiler::createNewJsonDumpFile: "
			"Created new JSON dump file: %s\n", fullFilePath );

		return fp;
	}
	else
	{
		ERROR_MSG( "Profiler::createNewJsonDumpFile: "
			"Unable to open file '%s': %s\n",
			fullFilePath, strerror( errno ) );

		return NULL;
	}
}


/**
 * start or stop the continuous json profile output.
 * multiple frames can be in the same file via use of thejsonDumpCount_.
 * if dump is false then json output will cease after the next call to 
 * jsonDump() where it will close the json file for us. 
 * 15 frames is chosen as larger files can cause Chrome to crash.
 * 
 * @param  dump true to start the continuous json output. False to stop.
 *
 */
void Profiler::setForceJsonDump( bool dump )
{
	static int oldDumpCount = 1;
	forceJsonDump_ = dump;
	if (dump)
	{
		oldDumpCount = jsonDumpCount_;
		jsonDumpCount_ = 15; 
	}
	else
	{
		jsonDumpCount_ = oldDumpCount;
	}
}


/**
 *
 */
void Profiler::writeJsonEntries()
{
	ProfileEntry* entry = finalProfileEntryBuffer_->entries_;
	ThreadEntry* threads = threads_;
	int32 numThreads = maxNumThreads_;
#if ENABLE_PER_CORE_PROFILER
	if (profileMode_ == CORES)
	{
		threads = cores_;
		numThreads = numCores_;
	}
#endif
	for (int t = 0; t < numThreads; ++t)
	{
#if ENABLE_PER_CORE_PROFILER
		if (threads[t].thread_ || profileMode_== CORES)
#else
		if (threads[t].thread_)
#endif
		{
			if (jsonFileHasData_)
			{
				fprintf( jsonOutputFile_, "," );
			}
			fprintf( jsonOutputFile_, "{\"pid\":%i,\"tid\":%i, ", 0 , t);
			fprintf( jsonOutputFile_,
				"\"ph\": \"M\", \"name\":\"thread_name\"," );
			fprintf( jsonOutputFile_, "\"args\":{\"name\": \"%s\"} } ",
				threads[t].name_.c_str());
			jsonFileHasData_  = true;
		}
	}
	// keep a stack of the valid entries so that we can detect unclosed 
	// profile scopes
	BW::vector < std::stack<ProfileEntry*> >  entryStack ( maxNumThreads_ );

	int32 entryCount = finalProfileEntryBuffer_->entryCount_;
	for (int32 i = 0; i < entryCount; i++, entry++)
	{
		const char * eventString = NULL;

		if (entry->threadIdx_ == -1)
		{
			continue;
		}

		if (jsonFileHasData_)
		{
			fprintf( jsonOutputFile_, "," );
		}

		fprintf( jsonOutputFile_, "{ \"cat\":\"%s\", \"pid\":%i,", 
			eventCategories_[entry->category_], 0 );
		fprintf( jsonOutputFile_, "\"tid\":%i, \"ts\":%i,",
#if ENABLE_PER_CORE_PROFILER
			(profileMode_==CORES)? entry->coreNumber_ : 
#endif
			entry->threadIdx_,
			(int)(stampsToSeconds( entry->time_ - jsonDumpStartTime_ )
					* 1000000) );

		switch(entry->event_)
		{
		case EVENT_START:
		case EVENT_START_IDLE:
			{
				eventString = "\"B\"";	
				entryStack[entry->threadIdx_].push(entry);
			}
			break;
		case EVENT_END:
			{
				eventString = "\"E\"";	
				if (entryStack[entry->threadIdx_].size() > 0
					&& (entryStack[entry->threadIdx_].top()->name_ == entry->name_))
				{
					entryStack[entry->threadIdx_].pop();
				}
			}
			break;
		case EVENT_COUNTER:	
			{
				eventString = "\"C\"";	
			}
			break;
		}

		fprintf( jsonOutputFile_, "\"ph\":%s,\"name\":\"%s\"",
			eventString, entry->name_ );

		if (entry->event_ == EVENT_COUNTER)
		{
			fprintf( jsonOutputFile_, ",\"args\":{\"%s\": %i}}",
				"counter", entry->value_ );
		}
		else
		{
			fprintf( jsonOutputFile_, "}" );
		}

		jsonFileHasData_  = true;
	}
}


/**
 *	Take all of the raw, flat ProfileEntry data and massage it into a hierarchy
 *	so that we know what called what and how many times.
 */
void Profiler::processEntries()
{
	BW_GUARD;
	uint64 profileStart = timestamp();

	if (doJsonDump_ || forceJsonDump_)
	{
		this->jsonDump();
	}
	else if (!doJsonDump_ && jsonOutputFile_ != NULL)
	{
		this->stopJsonDump();
	}

	if (freezeProfiling_)
	{
		// time taken to profile.
		timeTakenForProfiling_ =
			(float)stampsToSeconds( timestamp() - profileStart ) * 1000.0f;
		return;
	}


	int32 numThreads = maxNumThreads_;
#if ENABLE_PER_CORE_PROFILER
	if ( profileMode_== CORES )
	{
		numThreads = numCores_;
		threads = cores_;
	}
#endif

	for (int i = 0; i < numThreads; ++i)
	{
		dStack_[i][0] = NULL;
		dChildTails_[i][0] = NULL;
		dHeads_[i] = NULL;
		totalStackDepth_[i] = 0;
	}

	displayEntryCount_ = 0;

	// First, build up the entries for display from the raw profile entries.
	ProfileEntry * entry = finalProfileEntryBuffer_->entries_;
	int32 entryCount = finalProfileEntryBuffer_->entryCount_;

	MF_ASSERT( entryCount <= maxEntryCount_ );

	// The currentFrameIdentifier should be the same for all entries in the 
	// finalProfileEntryBuffer_. If an entry has a value that doesn't match the 
	// first one, then it is incomplete and should be ignored. This is a very
	// rare occurrence and will only affect samples at the very end of the 
	// frame
	int currentFrameIdentifier = -1;

	if (entryCount > 1)
	{
		currentFrameIdentifier = entry->frameNumber_;
	}


	for (int32 i = 0; i < entryCount; i++, entry++)
	{
		// first grab and check the thread ID of the entry
		int32 threadID;
		threadID = entry->threadIdx_;
#if ENABLE_PER_CORE_PROFILER
		if ( profileMode_ == CORES )
		{
			threadID = entry->coreNumber_;
		}
#endif
		if (threadID == -1)
		{
			continue;
		}

		MF_ASSERT( threadID < numThreads );

		// if this entry has an invalid frameNumber_ then it should be discarded
		if (entry->frameNumber_ != currentFrameIdentifier)
		{
			//DEBUG_MSG("unfinished profile entry - skipping!\n");
			continue;
		}

		if ((entry->category_ & finalEventCategory_->first) == 0)
		{
			continue;
		}

		// if its not the EndColour, then it must be an 'open' entry
		if (entry->event_ == EVENT_START ||
			entry->event_ == EVENT_START_IDLE)
		{
			// process the ProfileEntrys into DisplayEntries
			// Display entries have hierarchical info and could be used
			// for graphical displays. Currently only used internally.
			if (displayEntryCount_ >= maxDisplayEntryCount_)
			{
				profileErrorString_ = "ERROR: Too many display entries\n"
					"increase your buffer size in <debug/profileBufferSize>"
					" in engine_config.\n";
				continue;
			}

			DisplayEntry * d = &displayEntryBuffer_[displayEntryCount_];
			d->name_ = entry->name_;
			d->start_ = entry->time_;
			d->child_ = NULL;
			d->sibling_ = NULL;
			d->numInclusiveCycles_ = 0;
			d->numChildCycles_ = 0;
			d->numInclusiveIdleCycles_ = 0;
			d->isIdle_ = entry->event_ == EVENT_START_IDLE;

			int stackDepth = totalStackDepth_[threadID];

			if (stackDepth >= MAX_STACK_DEPTH)
			{
				++totalStackDepth_[threadID];
				continue;
			}

			MF_ASSERT(stackDepth >= 0);

			++displayEntryCount_;

			// add the display entry to the stack
			dStack_[threadID][stackDepth] = d;
			++totalStackDepth_[threadID];


			if (stackDepth == 0)
			{
				d->parent_ = NULL;
				if (dHeads_[threadID] == NULL)
				{
					dHeads_[threadID] = d;
				}
				else
				{
					MF_ASSERT( dChildTails_[threadID][stackDepth] );
					dChildTails_[threadID][stackDepth]->sibling_ = d;
				}
			}
			else
			{
				d->parent_ = dStack_[threadID][stackDepth - 1];
				if (d->parent_->child_ == NULL)
				{
					d->parent_->child_ = d;
				}
				else
				{
					MF_ASSERT( dChildTails_[threadID][stackDepth] );
					// if there is already a sibling, link that to this
					dChildTails_[threadID][stackDepth]->sibling_ = d;
				}
			}

			// set this display entry up to be potentially used as a sibling
			dChildTails_[threadID][stackDepth] = d;
		}
		// this marks the end of the profile section
		else if (entry->event_ == EVENT_END)
		{
			int32 stackDepth = totalStackDepth_[threadID];
			if (stackDepth >= MAX_STACK_DEPTH)
			{
				--totalStackDepth_[threadID];
				continue;
			}

			if (stackDepth == 0)
			{
				// this happens quite often due to tasks running on
				// other threads during the profile::tick()
				continue;
			}
			
			DisplayEntry* openDisplayEntry = dStack_[threadID][stackDepth-1];

			if (!openDisplayEntry ||
				openDisplayEntry->name_ != entry->name_)
			{
				MF_ASSERT( displayEntryCount_ >= maxDisplayEntryCount_ );
				continue;
			}

			MF_ASSERT( entry->time_ >= (openDisplayEntry->start_ -
				openDisplayEntry->numInclusiveIdleCycles_) );

			
			openDisplayEntry->numInclusiveCycles_ = entry->time_ -
				openDisplayEntry->start_;

			if (openDisplayEntry->isIdle_)
			{
				openDisplayEntry->numInclusiveIdleCycles_ = 
					openDisplayEntry->numInclusiveCycles_;
			}
			else
			{
				openDisplayEntry->numInclusiveCycles_ -= 
					openDisplayEntry->numInclusiveIdleCycles_;
			}

			MF_ASSERT( openDisplayEntry->numInclusiveCycles_ >=
				openDisplayEntry->numChildCycles_ );

			totalStackDepth_[threadID]--;

			DisplayEntry * pParent = openDisplayEntry->parent_;

			if (pParent)
			{
				pParent->numInclusiveIdleCycles_ 
					+= openDisplayEntry->numInclusiveIdleCycles_;

				if (!openDisplayEntry->isIdle_)
				{
					pParent->numChildCycles_ 
						+= openDisplayEntry->numInclusiveCycles_;
				}
			}
		}
	}

	highWatermarkOfDisplayEntryCount_ = MF_MAX(
		highWatermarkOfDisplayEntryCount_, displayEntryCount_ );

	// Close any entries that are still open (due to mismatched open-close
	// input entries)
	uint64 endOfFrameTimeStamp = finalProfileEntryBuffer_->endOfFrameTimeStamp_;
	for (int thread_id = 0; thread_id < numThreads; ++thread_id)
	{
		while (totalStackDepth_[thread_id] > 0)
		{
			int stackDepth = totalStackDepth_[thread_id]--;

			if (stackDepth < MAX_STACK_DEPTH)
			{
				DisplayEntry* top_entry =
					dStack_[ thread_id ][ stackDepth - 1 ];

				// clamp it to the end of the frame (start of the next one) so
				// that it at least exists
				// TODO: manage times that span multiple frames.
				if (top_entry->isIdle_)
				{
					top_entry->numInclusiveIdleCycles_ = MF_MAX( 
						endOfFrameTimeStamp - top_entry->start_, (uint64)0 );
				}
				else
				{
					top_entry->numInclusiveCycles_ = MF_MAX( 
						endOfFrameTimeStamp - top_entry->start_
						- top_entry->numInclusiveIdleCycles_, (uint64)0 );
				}

				if (top_entry->parent_)
				{
					top_entry->parent_->numChildCycles_ +=
						top_entry->numInclusiveCycles_;
					top_entry->parent_->numInclusiveIdleCycles_ += 
						top_entry->numInclusiveIdleCycles_;
				}
			}
		}

		threads_[thread_id].head_ = dHeads_[thread_id];
	}

	this->outputReport();
	frame_++;

	// add current frame's times to appropriate history array.
	for (int i = 0; i < numThreads; ++i)
	{
		DisplayEntry * node = dHeads_[i];

		if (node)
		{
			int64 start = node->start_;
			while (node->sibling_)
			{
				node = node->sibling_;
			}

			perThreadHistory_[i][frame_%HISTORY_LENGTH] =
				(float)stampsToSeconds( node->numInclusiveCycles_ +
					node->start_ - start) * 1000.0f;
		}
		else // no children. Set timer to zero.
		{
			perThreadHistory_[i][frame_%HISTORY_LENGTH] = 0.0f;
		}
	}

	this->doRemoveThreads();

	// time taken to profile.
	timeTakenForProfiling_ =
		(float)stampsToSeconds(timestamp() - profileStart) * 1000.0f;
}


/**
 *
 */
void Profiler::toggleHierChildren()
{
	if (currentHierEntry_)
	{
		currentHierEntry_->toggleChildren();
	}
}


/**
 *
 */
void Profiler::toggleGraph( bool recursive )
{
	if (currentHierEntry_)
	{
		currentHierEntry_->toggleGraph( recursive );
	}
}


/**
 *
 */
int Profiler::nextHierEntry()
{
	if (!currentHierEntry_)
	{
		return 0;
	}

	HierEntry* nextOne = currentHierEntry_->next();

	if (nextOne)
	{
		currentHierEntry_ = nextOne;
		return 1;
	}
	else
	{
		// there is no next entry. Keep currentHierEntry the same
	}

	return 0;
}


/**
 *	This method moves to the previous entry.
 *
 *	@return 1 if sucessful, zero otherwise.
 */
int Profiler::prevHierEntry()
{
	if (!currentHierEntry_)
	{
		return 0;
	}

	// will either be the sibling that points to it or its parent
	HierEntry * prevOne = currentHierEntry_->prev();

	if (prevOne) // no parent. This is the root entry
	{
		currentHierEntry_ = prevOne;
		return 1;
	}
	else
	{
		// no previous entry. Keep current entry.
	}

	return 0;
}


//------------------------------------------------------------------------------
// Builds and updates a hierarchy of the entries visible in the hierarchy view
// Adds entries to be graphed to vector for later use
//------------------------------------------------------------------------------
void Profiler::updateHierarchy()
{
	graphableHierEntrys_.clear();

	ThreadEntry* threads = threads_;
#if ENABLE_PER_CORE_PROFILER
	if ( profileMode_== CORES )
	{
		threads = cores_;
	}
#endif

	for (int t = 0; t < maxNumThreads_; ++t)
	{
		if (!threads[t].visible_)
		{
			continue;
		}

		if (!threads[t].thread_) // only output valid threads.
		{
			continue;
		}

		// grab the head to start with
		DisplayEntry * pCurrentEntry = threads[ t ].head_;

		if (!pCurrentEntry)
		{
			continue;
		}


		threads[ t ].hierHead_ = hierarchyManager_->findChild(
			parentHierEntry_, threads[t].name_.c_str() );

		HierEntry * pHierEntry = threads[ t ].hierHead_;
		if (!pHierEntry)
		{
			continue;
		}

		pHierEntry->numCalls_ = 1;
		DisplayEntry * pSibling = pCurrentEntry;
		while (pSibling)
		{
			if (calcExclusiveTimes_)
			{
				pHierEntry->time_ += (float)(stampsToSeconds(
					pSibling->numInclusiveCycles_ -
					pSibling->numChildCycles_ ) * 1000.0);
			}
			else
			{
				pHierEntry->time_ += (float)(stampsToSeconds(
					pSibling->numInclusiveCycles_ ) * 1000.0);
			}
			pSibling = pSibling->sibling_;
		}

		if (!currentHierEntry_)
		{
			currentHierEntry_ = pHierEntry;
		}

		pHierEntry = hierarchyManager_->findChild(
			pHierEntry, pCurrentEntry->name_ );

		// if pHierEntry is NULL then it is probably due to running out of
		// space in the pool
		while (pCurrentEntry && pHierEntry)
		{
			MF_ASSERT( pHierEntry->name_ == pCurrentEntry->name_ );

			if (calcExclusiveTimes_)
			{
				pHierEntry->time_ += (float)(stampsToSeconds(
					pCurrentEntry->numInclusiveCycles_ -
					pCurrentEntry->numChildCycles_ ) * 1000.0);
			}
			else
			{
				pHierEntry->time_ += (float)(stampsToSeconds(
					pCurrentEntry->numInclusiveCycles_ ) * 1000.0);
			}

			pHierEntry->numCalls_++;

			if (pHierEntry->timeArray_)
			{
				pHierEntry->timeArray_[ frame_ %
						hierarchyManager_->getMaxNumberOfTimeEntries() ] =
					pHierEntry->time_;

				if (pHierEntry->numCalls_ == 1) // only add this entry once.
				{
					graphableHierEntrys_.push_back( pHierEntry );
				}
			}

			if (pCurrentEntry->child_)  // if this entry has a child
			{
				pCurrentEntry = pCurrentEntry->child_;
				pHierEntry = hierarchyManager_->findChild( pHierEntry,
					pCurrentEntry->name_ );
			}
			else if (pCurrentEntry->sibling_) // or it has a sibling
			{
				pCurrentEntry = pCurrentEntry->sibling_;
				pHierEntry = hierarchyManager_->findChild( pHierEntry->parent_,
					pCurrentEntry->name_ );
			}
			else  // otherwise, go back up the tree
			{
				pCurrentEntry = pCurrentEntry->parent_;

				pHierEntry = pHierEntry->parent_;

				while (pCurrentEntry)
				{
					if (pCurrentEntry->sibling_)
					{
						pCurrentEntry = pCurrentEntry->sibling_;
						pHierEntry = hierarchyManager_->findChild(
							pHierEntry->parent_, pCurrentEntry->name_ );
						break;
					}
					pCurrentEntry = pCurrentEntry->parent_;
					pHierEntry = pHierEntry->parent_;
				}
			}

		}

	}
}


/**
 *	Generate the output report (CSV, display mesh, etc)
 */
void Profiler::outputReport()
{
	BW_GUARD;
	// required as getStatistics uses the results of this function and may be
	// called at the same time on another thread.
	SimpleMutexHolder mutex( getStatsMutex_ );

	// if dumping to CSV, then always sort the names alphabetical
	// (they must be in the same order each frame)
	// and also calculate the exclusive costs (subtract children's
	// cost from parent)
	ProfileMode localSortType = profileMode_;
	bool localExclusiveTimes = calcExclusiveTimes_;


	if (localSortType == HIERARCHICAL)
	{
		hierarchyManager_->zeroHierarchy();
		this->updateHierarchy();
		return;
	}

	SortedEntry * pSortedEntries = NULL;

	//TODO: The interaction between this function and 
	// ProfileCSVOutputTask::addProfileData could be improved. 
	// ProfileCSVOutputTask already has a fixed data buffer that could be used
	// instead of allocating and destroying one here. This would reduce
	// dynamic memory allocation. Could sort the entries in
	// ProfileCSVOutputTask, it does some work on another thread so won't steal
	// cycles away from the main thread to sort these entries.
	// where the entries go - dynamic ATM, could easily be fixed
	pSortedEntries = new SortedEntry[ displayEntryCount_ ];

	ThreadEntry* threads = threads_;
	int32 numThreads = maxNumThreads_;

#if ENABLE_PER_CORE_PROFILER
	if (profileMode_== CORES)
	{
		numThreads = numCores_;
		threads = cores_;
	}
#endif
	for (int t = 0; t < numThreads; ++t)
	{
		if (!threads[t].visible_)
		{
			continue;
		}

		if (!threads[t].thread_) // only output valid threads.
		{
			continue;
		}
		
		DisplayEntry * currentEntry = threads[t].head_;
		
		if (!currentEntry)
		{
			continue;
		}


		if (localSortType)
		{
			uint32 numSortedEntries = 0;

			while (currentEntry)
			{
				int64 numCycles = currentEntry->numInclusiveCycles_;

				// subtract children's cost from parent
				if (localExclusiveTimes && numCycles)
				{
					numCycles -= currentEntry->numChildCycles_;
				}

				pSortedEntries[ numSortedEntries ].numCycles_= numCycles;
				pSortedEntries[ numSortedEntries ].entryName_ =
					currentEntry->name_;
				pSortedEntries[ numSortedEntries ].numCalls_ = 1;
				++numSortedEntries;

				MF_ASSERT( numSortedEntries <= displayEntryCount_ );

				if (currentEntry->child_) // if this entry has a child
				{
					currentEntry = currentEntry->child_;
				}
				else if (currentEntry->sibling_) // or it has a sibling
				{
					currentEntry = currentEntry->sibling_;
				}
				else // otherwise, go back up the tree
				{
					currentEntry = currentEntry->parent_;

					while (currentEntry)
					{
						if (currentEntry->sibling_)
						{
							currentEntry = currentEntry->sibling_;
							break;
						}

						currentEntry = currentEntry->parent_;
					}
				}
			}

			// Before sorting we need to loop through and condense
			// any entries that have the same name
			SortedEntry* sort_entry = pSortedEntries;

			for (uint32 i = 0; i < numSortedEntries; ++i, ++sort_entry)
			{
				const char* name = sort_entry->entryName_;

				SortedEntry* match_entry = sort_entry + 1;

				for (uint32 imatch_entry = i + 1;
					imatch_entry < numSortedEntries; )
				{
					const char* match_name = match_entry->entryName_;

					// TODO: Comparing pointers fails on somethings like
					// e.g. TSpeedTreeType_setEffectParam
					if (match_name == name)
					{
						sort_entry->numCycles_ += match_entry->numCycles_;
						sort_entry->numCalls_++;
						--numSortedEntries;
						*match_entry = pSortedEntries[numSortedEntries];
					}
					else
					{
						imatch_entry++;
						match_entry++;
					}
				}
			}

			switch (localSortType)
			{
			case SORT_BY_NAME:
				std::sort( pSortedEntries,
					&pSortedEntries[numSortedEntries],
					SortByName );
				break;
			case SORT_BY_TIME:
			case CPU_GPU:
				std::sort( pSortedEntries,
					&pSortedEntries[numSortedEntries],
					SortByCostliest );
				break;
			case SORT_BY_NUMCALLS:
				std::sort( pSortedEntries,
					&pSortedEntries[numSortedEntries],
					SortByNumCalls );
				break;
			default:
				break;
			}

			// Loop through the top N sorted entries and display to stdout
			sort_entry = pSortedEntries;
			numTopEntries_[t] =
				MF_MIN( (uint32)MAX_NUM_TOP_ENTRIES, numSortedEntries );

			for (int32 i = 0; i < numTopEntries_[t]; ++i, ++sort_entry)
			{
				topEntries_[t][i] = *sort_entry;
			}

			if (csvOutputTask_->isRunning())
			{
				if (csvOutputTask_->getProfileThreadIndex() == (uint32)t)
				{
					csvOutputTask_->addProfileData( pSortedEntries,
						numSortedEntries );
				}
			}
		}
	}

#if !defined( MF_SERVER )
	if (!csvOutputTask_->isRunning())
	{
		this->generateDisplayMesh();
	}
#endif // !defined( MF_SERVER )

	bw_safe_delete_array( pSortedEntries );
}


/**
 *	Set up a new file for CSV profiling output.
 */
bool Profiler::setNewHistory( const char * historyFileName, 
							const char * threadName, BW::string * msgString)
{
	BW_GUARD;
	this->setProfileMode( SORT_BY_NAME, false );

	return csvOutputTask_->setNewHistory( historyFileName,
		threadName, msgString );
}


/**
 *	Close the history file
 */
void Profiler::closeHistory()
{
	BW_GUARD;
	csvOutputTask_->closeHistory();
}


/**
 *	Add this thread to the profiler with a specified name
 *	@param name		The name of this thread
 */
void Profiler::addThisThread( const char* name )
{
	BW_GUARD;
	if (!threads_)
	{
		return;
	}

	SimpleMutexHolder threadSmh( threadMutex_ );
	// this mutex is a little heavy handed, but it is in a function
	// which is called only a handful of times, so shouldn't be a problem.
	uint thisThread = OurThreadID();
	int firstEmptySlot = -1;
	MF_ASSERT( thisThread != 0 );

	// first check to see if its already been added
	for (int i = 0; i < maxNumThreads_; ++i)
	{
		if (thisThread == threads_[i].thread_)
		{
			tls_ThreadIndex_ = i;
			// erase the thread from being removed 
			// just incase it was marked for removal
			threadsToRemove_.erase( thisThread );
			WARNING_MSG( "Profiler::addThisThread:"
				" Thread %s already added (%i)!\n", name, i );
			return;
		}

		if (threads_[i].thread_ == 0 && firstEmptySlot == -1)
		{
			firstEmptySlot = i;
			break;
		}
	}

	if (firstEmptySlot == -1) // too many threads
	{
		WARNING_MSG( "Profiler::addThisThread: "
			"Thread %s could not be added due to no enough space.\n", name );
		return;
	}

	// find an empty slot
	threads_[firstEmptySlot].thread_ = thisThread;
	threads_[firstEmptySlot].name_ = name;
	threads_[firstEmptySlot].head_ = NULL;
	threads_[firstEmptySlot].visible_ = true;
	tls_ThreadIndex_ = firstEmptySlot;
	numThreads_++;

	MF_ASSERT( numThreads_ <= maxNumThreads_ );
}


/**
 * Move the previous category
 */
void Profiler::prevCategory()
{
	{
		SimpleMutexHolder smh( getStatsMutex_ );
		if (currentEventCategory_ == eventCategories_.begin())
		{
			currentEventCategory_ = eventCategories_.end();
		}

		--currentEventCategory_;
	}

	this->resetHierarchy();
}


/**
 * Move to the next category
 */
void Profiler::nextCategory()
{
	{
		SimpleMutexHolder smh( getStatsMutex_ );

		currentEventCategory_++;

		if (currentEventCategory_ == eventCategories_.end())
		{
			currentEventCategory_ = eventCategories_.begin();
		}
	}
	this->resetHierarchy();
}


/**
 * Set the current category
 */
void Profiler::setCurrentCategory( const BW::string & newCategory )
{
	EventCategoryNames::const_iterator iter = eventCategories_.begin();
	for (; iter != eventCategories_.end(); ++iter)
	{
		if (iter->second == newCategory)	
		{
			{
				SimpleMutexHolder smh( getStatsMutex_ );
				currentEventCategory_ = iter;
			}

			this->resetHierarchy();
			break;
		}
	}

	if (iter == eventCategories_.end())
	{
		WARNING_MSG( "Profiler::setCurrentCategory: invalid category: %s\n", 
					newCategory.c_str() );
	}
}


/**
 * Return current category name
 */
BW::string Profiler::getCurrentCategory() 
{
	SimpleMutexHolder smh( getStatsMutex_ );
	return currentEventCategory_->second;
}


/**
 * Return all the category names, ';' seperated string
 */
BW::string Profiler::getCategories() const 
{
	BW::string categories;

	EventCategoryNames::const_iterator iter = eventCategories_.begin();
	for (; iter != eventCategories_.end(); ++iter)
	{
		if (iter != eventCategories_.begin())
		{
			categories += ";";
		}
		categories += iter->second;
	}

	return categories;
}


/**
 * Reset hierarchy after changing category
 */
void Profiler::resetHierarchy()
{
	SimpleMutexHolder smh( getStatsMutex_ );
	// Reset our hierarchy to prevent issues with where things are located
	hierarchyManager_->reset( parentHierEntry_ );
	currentHierEntry_ = parentHierEntry_;
	for (int t = 0; t < maxNumThreads_; ++t)
	{
		threads_[ t ].hierHead_ = NULL;
	}
}


/**
 *	Set a threads visibility
 *
 *	@param threadId		The thread to change the visiblity on
 *	@param isVisible	True if the thread should be shown, false otherwise
 *	@see getThreadVisibility
 */
void Profiler::setThreadVisibility( int threadId, bool isVisible )
{
	BW_GUARD;
	MF_ASSERT( threadId < maxNumThreads_ );
	threads_[threadId].visible_ = isVisible;
}


/**
 *	Get a threads visibility
 *
 *	@param threadID		The thread to get the visibility of
 *	@return				True if the thread is visible, false otherwise
 *	@see setThreadVisibility
 */
bool Profiler::getThreadVisibility( int threadId )
{
	BW_GUARD;
	MF_ASSERT( threadId < maxNumThreads_ );
	return threads_[threadId].visible_;
}


/**
 *	Remove a thread from the profiler, if Profiler is enable,
 *	only add it to the threadsToRemove_ list and wait until
 *	the full processEntries has completed before it's really
 *	removed.
 *
 *	@param threadID The ID of the thread
 */
void Profiler::removeThread( uint threadId )
{
	BW_GUARD;
	if (!threads_)
	{
		return;
	}

	MF_ASSERT( threadId );

	SimpleMutexHolder threadSmh( threadMutex_ );

	// If Profiler is  not enabled, then we don't need
	// wait until the full processEntries has completed,
	// so we can remove it immediately.
	if (!isActive_ && !isPendingActive_)
	{
		this->internalRemoveThread( threadId );
		return ;
	}
	// Otherwise add it into threadsToRemove_ and
	// it will be removed later.
	threadsToRemove_.insert( threadId );
}


/**
 *	Do remove a thread from the profiler, this is internal only
 *	the outside caller should be looking after thread safe.
 *
 *	@param threadID The ID of the thread
 */
void Profiler::internalRemoveThread( uint threadId )
{
	BW_GUARD;
	if (!threads_)
	{
		return;
	}

	for (size_t i = 0; i < (size_t)maxNumThreads_; i++)
	{
		if (threads_[i].thread_ == threadId)
		{
			numThreads_--;
			DEBUG_MSG( "Thread %s deleted\n", threads_[i].name_.c_str() );

			threads_[i].head_ = NULL;
			threads_[i].name_ = "Deleted";
			threads_[i].thread_ = 0;
			threads_[i].visible_ = false;
			return;
		}
	}
}


void Profiler::doRemoveThreads()
{
	BW_GUARD;
	if (!threads_)
	{
		return;
	}

	SimpleMutexHolder threadSmh( threadMutex_ );

	for (BW::set<uint>::iterator it = threadsToRemove_.begin();
		it != threadsToRemove_.end(); ++it)
	{
		this->internalRemoveThread( *it );
	}
	threadsToRemove_.clear();
}


/**
 *	This static method exposes the tls_ThreadIndex_ for inlined
 *	methods in profiler.ipp.
 */
/* static */ const bwThreadID & Profiler::getThreadIndex()
{
	return tls_ThreadIndex_;
}


/**
 * Enable/Disable the profiler
 * @param state		True to enable the profiler, false otherwise
 */
void Profiler::enable( bool state )
{
	BW_GUARD;

	if (state == true)
	{
		// turn it on at the next tick()
		if (!isActive_)
		{
			isPendingActive_ = true;
			hasGeneratedProfile_ = true;

			this->setJsonDumpState( JSON_DUMPING_INACTIVE );
		}
	}
	else
	{
		// turn it off now and ignore all current data.
		isActive_ = false;
		profileEntryBuffer_->entryCount_ = 0;

		this->setJsonDumpState( JSON_DUMPING_INACTIVE );
	}
}


/**
 *	Set the profiler mode
 *
 *	@param mode			The mode to set the profiler to
 *	@param inclusive	True if the profiler should be inclusive,
 *						false otherwise.
 */
void Profiler::setProfileMode( ProfileMode mode, bool inclusive )
{
	BW_GUARD;
	MF_ASSERT( mode < MAX_MODES );
	if (!csvOutputTask_->isRunning())
	{
		static BW::vector<bool> visibleThreads;
		if (visibleThreads.size() != (size_t)maxNumThreads_)
		{
			visibleThreads.resize(maxNumThreads_, false);
		}

		if (mode == CPU_GPU && profileMode_ != CPU_GPU)
		{
			// store thread visibility when changing to CPU/GPU view
			for (int t = 0; t < maxNumThreads_; t++)
			{
				visibleThreads[t] = threads_[t].visible_;
				threads_[t].visible_ = 
					(threads_[t].name_ == "MainThread" ||
					 threads_[t].name_ == "GPU") ? true : false;
			}
		}
		else if (mode != CPU_GPU && profileMode_ == CPU_GPU)
		{
			// restore thread visibility when changing from CPU/GPU view
			for (int t = 0;t < maxNumThreads_; t++)
			{
				threads_[t].visible_=visibleThreads[t];
			}
		}
		profileMode_ = mode;
		this->enable( mode != PROFILE_OFF );

		calcExclusiveTimes_ = !inclusive;
	}
	else
	{
		// when profiling to CSV we need to force this setting.
		calcExclusiveTimes_ = true;
		profileMode_ = SORT_BY_NAME;
		this->enable( true );
	}
}


// only used by the following function.
// returns the depth (number of parents) of a given HierEntry.
static int GetDepth( HierEntry * e )
{
	int depth=0;

	while ((e = e->parent_))
	{
		depth++;
	}

	return depth;
}


/**
 *	Get the statistics of the profiler
 *	@param profileOutput		[out] The profiler statistics output
 */
void Profiler::getStatistics( BW::vector<ProfileLine> * profileOutput )
{
	// required as OutputReport might still be being called (unlikely though),
	SimpleMutexHolder mutex( getStatsMutex_ );

	BW_GUARD_PROFILER( getStatistics );
	char tempBuffer[256];
	char tempBuffer2[256];

	profileOutput->clear();

	if (csvOutputTask_->isRunning())
	{
		profileOutput->push_back(
			ProfileLine( 0xff0000ff,
				"Profile data not available while profiling to CSV file\n" ) );
	}
	else if (profileMode_ == HIERARCHICAL)
	{
		profileOutput->push_back( "Hierarchical Profile data" );
		profileOutput->push_back( 
			calcExclusiveTimes_ ? "(Exclusive)\n" : "(Inclusive)\n" );
		profileOutput->push_back( "Current Filter: " );
		profileOutput->push_back( currentEventCategory_->second );
		profileOutput->push_back( "\n\n" );
		// All threads are added to a single dummy entry to simplify
		// navigation (and code)
		{
			HierEntry * e = parentHierEntry_;

			while (e)
			{
#ifdef MF_SERVER
				sprintf( tempBuffer2, "%c%*s%s(%i)%s",
					currentHierEntry_ == e ? '*' : ' ',
#else
				sprintf( tempBuffer2, "%*s%s(%i)%s",
#endif
					GetDepth( e ), 
					"", e->name_, e->numCalls_,
					e->child_ ? "..." : "" );
				if (e->numCalls_)
				{
					sprintf( tempBuffer, "%-40s %7.3fms\n", tempBuffer2, e->time_ );
				}
				else
				{
					sprintf( tempBuffer, "%-40s\n", tempBuffer2 );
				}

				// We use a bitfield here and rely on the calling function in 
				// console to construct the appropriate colours for the 
				// different states.
				uint32 colour = 0;

				if (currentHierEntry_ == e)
				{
					colour |= HierEntry::HIGHLIGHTED; // highlight selected
				}

				if (e->timeArray_)
				{
					colour |= HierEntry::GRAPHED;
				}

				profileOutput->push_back( ProfileLine( colour, tempBuffer ) );
				e = e->next();
			}
		}
		return;
	}
	else
	{
		profileOutput->push_back( "Profile data - " );
		switch (profileMode_)
		{
		case SORT_BY_TIME:
			profileOutput->push_back( "Sorted By Time" );
			break;
		case SORT_BY_NUMCALLS:
			profileOutput->push_back( "Sorted By Number of Calls" );
			break;
		case SORT_BY_NAME:
			profileOutput->push_back( "Sorted By Name" );
			break;
		case CPU_GPU:
			profileOutput->push_back( "CPU and GPU" );
			break;
#if ENABLE_PER_CORE_PROFILER
		case CORES:
			profileOutput->push_back( "Core Usage" );
			break;
#endif
		default:
			profileOutput->push_back(
				ProfileLine( 0xff0000ff,"Invalid Sort!" ) );
			break;
		}

		sprintf( tempBuffer, "(%s):\n", 
			calcExclusiveTimes_ ? "Exclusive" : "Inclusive" );
		profileOutput->push_back( ProfileLine( 0xffffffff, tempBuffer ) );
#if ENABLE_HITCH_DETECTION
		sprintf( tempBuffer, "Hitch Detection (%s)",
			allowHitchFreezing_ ? "Enabled" : "Disabled" );
		profileOutput->push_back( ProfileLine( 0xffffffff, tempBuffer ) );
#endif // ENABLE_HITCH_DETECTION

		profileOutput->push_back( "Current Filter: " );
		profileOutput->push_back( currentEventCategory_->second );
		profileOutput->push_back( "\n" );

		ThreadEntry* threads = threads_;
		int32 numThreads = maxNumThreads_;
#if ENABLE_PER_CORE_PROFILER
		if ( profileMode_== CORES )
		{
			numThreads = numCores_;
			threads = cores_;
		}
#endif
		for (int t = 0; t < numThreads; t++)
		{
			if (!threads[t].visible_ || threads[t].head_ == NULL)
			{
				continue;
			}
			sprintf( tempBuffer, "\n%i: %s\n",
				t, threads[t].name_.c_str() );
			profileOutput->push_back( tempBuffer );
			
			for (int i = 0; i < numTopEntries_[t]; i++)
			{
				sprintf( tempBuffer, "%2i: %-40.40s %8.2fms %6i calls\n", i,
					topEntries_[t][i].entryName_,
					(float)stampsToSeconds(
						topEntries_[t][i].numCycles_ ) * 1000.0f,
					topEntries_[t][i].numCalls_ );

				profileOutput->push_back(
					ProfileLine(
						colourHash( *(uint32*)topEntries_[t][i].entryName_ ),
							tempBuffer ) );
			}
			
			for (int i = numTopEntries_[t]; i < 2; i++)
			{
				profileOutput->push_back("\n");
			}
			if(profileBarVisibility_)
			{			
				profileOutput->push_back("\n");
				profileOutput->push_back("\n");
				profileOutput->push_back("\n");
			}
		}
	}

	sprintf( tempBuffer, "\nTime spent profiling = %8.2fms\n",
		timeTakenForProfiling_ );
	profileOutput->push_back( tempBuffer );

	int32 entryCount = finalProfileEntryBuffer_->entryCount_;
	sprintf( tempBuffer, "   %6u profile entries, using %3.2fMB "
		"(out of %3.2fMB in each buffer)\n",
		entryCount,
		entryCount * sizeof(ProfileEntry)/(1024.0f*1024.0f),
		(maxEntryCount_*sizeof(ProfileEntry))/(1024.0f*1024.0f) );
	profileOutput->push_back( tempBuffer );

	sprintf( tempBuffer, "   %6u display entries, using %3.2fMB "
		"(out of %3.2fMB)\n",
		displayEntryCount_,
		displayEntryCount_*sizeof(DisplayEntry)/(1024.0f*1024.0f),
		(maxDisplayEntryCount_*sizeof(DisplayEntry))/(1024.0f*1024.0f) );
	profileOutput->push_back( tempBuffer );
}


#if !defined( MF_SERVER )
/**
 * Generate the mesh
 */
void Profiler::generateDisplayMesh()
{
	// each thread has a mesh into which the profile bars are built.
	int charHeight = 17;
	int top = 0 * charHeight;
	// if there is an error, then we need to move the bars down one line.
	if (!profileErrorString_.empty())
		top += charHeight;
	// we want to be careful here that we aren't drawing this mesh as we're
	// writing to it so we have a meshMutex which is locked when we draw
	// (unlikely to happen)
	SimpleMutexHolder mutex( meshMutex_ );

	ThreadEntry* threads = threads_;
	int32 numThreads = maxNumThreads_;
#if ENABLE_PER_CORE_PROFILER
	if ( profileMode_== CORES )
	{
		numThreads = numCores_;
		threads = cores_;
	}
#endif
	for (int t = 0; t < numThreads; t++)
	{
		threads[t].vertexData_.clear();
		if (!threads[t].visible_)
		{
			continue;
		}

		DisplayEntry * entry = threads[t].head_;

		if (!entry)
		{
			continue;
		}

		int64 frameStart = entry->start_;
		int depth = 0;

		// blank + title + entries (at least 2) + 3 blanks
		top += (MF_MAX( numTopEntries_[t], 2 ) + 5) * charHeight;

		while (entry)
		{
			this->addMeshEntry( &threads[t].vertexData_,
				entry, depth, frameStart, top );

			if (entry->child_ && depth < 6)
			{
				depth++;
				entry = entry->child_;
			}
			else if (entry->sibling_)
			{
				entry = entry->sibling_;
			}
			else
			{
				entry = entry->parent_;
				depth--;

				while (entry)
				{
					if (entry->sibling_)
					{
						entry = entry->sibling_;
						break;
					}
					entry = entry->parent_;
					depth--;
				}
			}
		}
	}
}


/**
 *	Get the display mesh for a thread
 *
 *	@param thread	The thread to get the display mesh for
 *
 *	@return		A pointer to the ProfileVertex data for the specified thread,
 *				otherwise NULL if this does not exist
 */
BW::vector<Profiler::ProfileVertex> * Profiler::getDisplayMesh( int thread )
{
	BW_GUARD;

	int32 numThreads = maxNumThreads_;
	ThreadEntry* threads = threads_;
#if ENABLE_PER_CORE_PROFILER
	if ( profileMode_== CORES )
	{
		numThreads = numCores_;
		threads = cores_;
	}
#endif
	if (thread < 0 ||
		thread >= numThreads ||
		!threads[thread].visible_ ||
		threads[thread].head_ == NULL ||
		!profileBarVisibility_
		)
	{
		return NULL;
	}
	else
	{
		return &threads[thread].vertexData_;
	}
}


/**
 *	Get the graph for a thread
 *
 *	@param thread	The thread to get the display mesh for
 *	@param offset	[out] The offset within the returned data
 *
 *	@return		A pointer to values of the graph which should be read from
 *				offset up until HISTORY_LENGTH.
 */
float * Profiler::getGraph(int thread, int *offset)
{
	MF_ASSERT( thread< maxNumThreads_ && thread >= 0);

	*offset = frame_ % HISTORY_LENGTH;
	return perThreadHistory_[thread].data();
}


/**
 *	Add a mesh entry to vData
 *	@param vData		The vertex data to add the mesh to
 *	@param entry		The display entry to add to vData
 *	@param depth		The depth at which entry is located
 *	@param frameStart	The frame start time
 *	@param top			The y offset of the mesh
 */
void Profiler::addMeshEntry( BW::vector<ProfileVertex> * vData,
								DisplayEntry * entry, int depth,
								int64 frameStart, int top )
{
	BW_GUARD;

	float timeScale = screenWidth_ * 0.8f / s_profileBarWidth;
	float height = 10; // height of profile bars

	float left = (float)stampsToSeconds( entry->start_ - frameStart )
		* 1000.0f * timeScale;
	float width = (float)stampsToSeconds( entry->numInclusiveCycles_ )
		* 1000.0f * timeScale;

	ProfileVertex tlv;
	tlv.colour_ = colourHash( *((uint32*)entry->name_) );
	tlv.z = 0.0f;
	tlv.w = 1;

	tlv.x = left;
	tlv.y = top + depth * height;
	vData->push_back( tlv );

	tlv.x = left + width;
	vData->push_back( tlv );

	tlv.x = left;
	tlv.y = top + (depth + 1) * height;
	vData->push_back( tlv );

	tlv.x = left + width;
	vData->push_back( tlv );
}


/**
 * Add the GPU thread to the list of threads
 */
void Profiler::addGPUThread()
{
	BW_GUARD;
	MF_ASSERT( threads_ );

	SimpleMutexHolder threadSmh( threadMutex_ );

	int firstEmptySlot = -1;
	// first check to see if its already been added
	for (int i = 0; i < maxNumThreads_; ++i)
	{

		if (threads_[i].thread_==0)
		{
			firstEmptySlot = i;
			break;
		}
	}

	MF_ASSERT( firstEmptySlot != -1 ); // too many threads

	// find an empty slot
	gpuThreadID_ = firstEmptySlot;
	threads_[firstEmptySlot].thread_ = firstEmptySlot;
	threads_[firstEmptySlot].name_ = "GPU";
	threads_[firstEmptySlot].head_ = NULL;
	threads_[firstEmptySlot].visible_ = true;

	numThreads_++;

	MF_ASSERT( numThreads_ <= maxNumThreads_ );
}


/**
 *	Add a GPU entry to the profiler
 *
 *	@param text		The text of the entry
 *	@param time		The time of the entry
 *	@param e		The event type of the entry
 *	@param val		The value of the entry
 */
void Profiler::addGPUEntry( const char* text, int64 time,
								EventType e, int32 val )
{
	BW_GUARD;

	if (!isActive_)
	{
		return;
	}

	ProfileEntry * newEntry = this->startNextEntry();

	if (newEntry == NULL)
	{
		return;
	}

	newEntry->name_ = text;
	newEntry->event_ = e;
	newEntry->category_ = CATEGORY_GPU;

	//int64 cycles = (uint64)((time-startOfGPUFrameTimeStamp_)*stampsPerSecond()/(double)gpuFrequency_);
	// TODO: time_ might not be set on this entry..
	int64 cycles = profileEntryBuffer_->entries_->time_ + time;

	newEntry->time_ = cycles;
	newEntry->value_ = val;
	// store the index of the thread so we know where it came from
	newEntry->threadIdx_ = gpuThreadID_;
#if ENABLE_PER_CORE_PROFILER
	newEntry->coreNumber_ = numCores_ - 1;
#endif
	newEntry->frameNumber_ = profileFrameNumber_;
}


#endif // !defined(MF_SERVER)


// -----------------------------------------------------------------------------
// Section: Profile CSV Output Task
// -----------------------------------------------------------------------------

/**
 * Profiler CSV Output task
 */
ProfileCSVOutputTask::ProfileCSVOutputTask( TaskManager & taskManager ) :
	BackgroundTask( "ProfileCSVOutputTask" ),
	taskManger_( taskManager ),
	historyFile_ ( NULL ),
	profileThreadIndex_(0),
	frameCount_( 0 ),
	slotsNamesWritten_( false ),	
	timeTakenForProfiling_( 0.0f ),	
	uniqueNames_( SortByNameChar ),
	maxHistoryDataEntries_(280),
	maxPerFrameEntries_(256),
	maxNumFramesAdded_(0),
	lock_()
{
	BW_GUARD;

	// as we use a semaphore for synchronisation, we need 
	// to prime it here so we can lock it later.
	this->unlock();
}


/**
 *
 */
ProfileCSVOutputTask::~ProfileCSVOutputTask()
{
	BW_GUARD;
	// stall in case we are currently writing to file on a bg thread.
	this->lock();
}


/**
 * Called from main thread to add profile data to be output in the CSV file.
 *
 * @param sortedEntries		The entries to add to the CSV file
 * @param numSortedEntries	The number of entries in sortedEntries
 */
void ProfileCSVOutputTask::addProfileData(
	Profiler::SortedEntry * sortedEntries,
	uint numSortedEntries )
{
	BW_GUARD;

	// if we are still outputting to file on the background
	// thread then we will need to stall here.
	this->lock(); 

	if (!this->isRunning())
	{
		INFO_MSG( "ProfileCSVOutputTask::addProfileData - trying to add "
			"data when csv profiling has been stopped\n" );

		this->unlock();
		return;
	}

	// copy the new data into the history buffer
	MF_ASSERT( frameCount_ < perFrameSortedEntries_.size() );
	const uint32 destIndex = perFrameSortedEntries_[frameCount_];
	const uint32 finalIndex = destIndex + numSortedEntries;
	if (finalIndex >= historyData_.size())
	{
		// ran out of room in history data
		ERROR_MSG( "ProfileCSVOutputTask::historyData_ overflow\n" );
		ERROR_MSG( "Increase maxHistoryDataEntries_\n" );
		MF_ASSERT( finalIndex < historyData_.size() );
	}
	

	SortedEntry* const destEntry = &historyData_[destIndex];
	const uint32 copySize = numSortedEntries * sizeof(Profiler::SortedEntry);	
	memcpy( destEntry, sortedEntries, copySize );

	this->unlock();

	frameCount_++;

	if (frameCount_ >= maxPerFrameEntries_)
	{
		// ran out of room in the per frame entry index list
		ERROR_MSG( "ProfileCSVOutputTask::perFrameSortedEntries_ overflow\n" );
		ERROR_MSG( "Increase maxPerFrameEntries_\n" );
		MF_ASSERT( frameCount_ < perFrameSortedEntries_.size() );
	}

	const uint32 currFrameIndex = 
		perFrameSortedEntries_[frameCount_-1] + numSortedEntries;

	perFrameSortedEntries_[frameCount_] = currFrameIndex;
	//Keep track of the largest number of frames we have stored.
	maxNumFramesAdded_ = std::max( maxNumFramesAdded_, currFrameIndex );

	// can turn this back on if you want check utilisation
	//TRACE_MSG( "Testing - maxNumFramesAdded_ %d %d\n", maxNumFramesAdded_, 
	//	maxHistoryDataEntries_*maxPerFrameEntries_ );

	if (frameCount_ >= maxPerFrameEntries_-1)
	{
		// every so often, write all this data file file.
		this->flushHistory();
	}
}


/**
 *	Called from main thread to set the history file
 *	@param historyFileName		The filename to use for the CSV output
 */
bool ProfileCSVOutputTask::setNewHistory( const char* historyFileName, const char* threadName, BW::string* msgString )
{
	BW_GUARD;
	this->closeHistory();
	for (int i = 0; i < g_profiler.numThreads_; i++)
	{
		if (strstr( g_profiler.threads_[i].name_.c_str(), threadName ))
		{
			*msgString = BW::string("Profiling thread '") + g_profiler.threads_[i].name_ + "'\n";
			profileThreadIndex_=i;
			break;
		}
	}
	historyFile_ = bw_fopen( historyFileName, "w" );
	if(!historyFile_)
	{
		*msgString += BW::string("Unable to open file '")+historyFileName + "'\n";
		return false;
	}
	*msgString += BW::string("Opened file '")+historyFileName+"' for profile output\n";
	slotsNamesWritten_ = false;
	frameCount_= 0;

	perFrameSortedEntries_.resize(maxPerFrameEntries_+1);
	historyData_.resize(maxPerFrameEntries_*maxHistoryDataEntries_);

	perFrameSortedEntries_[0] = 0;
	maxNumFramesAdded_ = 0;
	return true;
}


/**
 *	Called from main thread to close the history file
 */
void ProfileCSVOutputTask::closeHistory()
{
	BW_GUARD;
	if (historyFile_)
	{
		// this is to ensure that the outputReport and associated
		// processEntries task isn't running and hasn't been queued to run
		// (otherwise the flushHistory in this function can be run before 
		// the one triggered by Processentries and things get crashy and locky)
		if (!g_profiler.processEntrySemaphore_.pullTry())
		{
			int64 start = timestamp();
			g_profiler.processEntrySemaphore_.pull();
			WARNING_MSG( "***** ProfileCSVOutputTask::closeHistory() stalled "
				"for %fms waiting for lock\n",
				stampsToSeconds( timestamp() - start ) * 1000.0f );
		}

		this->flushHistory();
		this->lock(); // We lock here as we want to be sure that the task that was
		// run for flushHistory has finished

		fclose( historyFile_ );
		historyFile_ = NULL;

		historyData_.clear();
		perFrameSortedEntries_.clear();

		this->reset();
		this->unlock();

		g_profiler.processEntrySemaphore_.push();
	}
}


/**
 *	Called from main thread to flush the history into the history file
 *	this is done in the background.
 */
void ProfileCSVOutputTask::flushHistory()
{
	BW_GUARD;
	if (historyFile_)
	{
		this->lock();

		// this is a potentially long process (10ms+) so we'll add it as a task
		taskManger_.addBackgroundTask( this );
	}
}


/**
 *	Called as a task on background thread to write the data to the history file
 */
void ProfileCSVOutputTask::doBackgroundTask( TaskManager & mgr )
{
	BW_GUARD;
	//  lock(); // this is locked from the main thread when it is queued.
	// TODO: dump the top 20 slowest functions (exclusive)
	// 
	if (!historyFile_)
	{
		this->unlock();
		return;
	}

	bool interleaved = false;

	// Save slot names (twice - for times and counts,
	// skipping unaccounted for counts)
	if (!slotsNamesWritten_)
	{
		// since the number of entries is variable (if a function isn't called, 
		// it won't be in the data). we need to do a once off preprocess to find
		// all the names of the profile entries What we do is iterate through
		// all of the frames and store the names of all the entries
		for (uint32 entryIndex = perFrameSortedEntries_[0];
			entryIndex != perFrameSortedEntries_[frameCount_];
			++entryIndex)
		{
			const SortedEntry& entry = historyData_[entryIndex];
			uniqueNames_.insert( entry.entryName_ );
		}

		// now output the column headings.
		UniqueNamesSet::iterator it;
		fprintf( historyFile_, "Total, profiling," );
		if (interleaved)
		{
			// for each entry, the time and then the number of times
			// it was called that frame.
			for (it = uniqueNames_.begin();
				it != uniqueNames_.end();
				++it)
			{
				fprintf( historyFile_, "%s (ms),%s (calls),", *it, *it );
			}

			fprintf( historyFile_, "Missing (ms),Missing (calls),");
		}
		else
		{
			// all of the entries' times and then the number of
			// times each entry eas called.
			for (it = uniqueNames_.begin();
				it != uniqueNames_.end();
				++it)
			{
				fprintf( historyFile_, "%s (ms),", *it );
			}

			fprintf( historyFile_, "Missing (ms),");
			
			for (it = uniqueNames_.begin();
				it != uniqueNames_.end();
				++it)
			{
				fprintf( historyFile_, "%s (calls),", *it );
			}

			fprintf( historyFile_, "Missing (calls),");
		}

		fprintf( historyFile_, "\n" );
		slotsNamesWritten_ = true;
	}

	// iterate through all the frames' data and dump the time in ms and the
	// num calls the SortedEntries are in alphabetical name order
	// as are the names in m_UniqueNames
	// so we should be able to do a single pass and output the frame data
	// into the correct slots

	// turn this on for time/count format, rather than the default 
	// time/time/time... count/count/count...
	// #define INTERLEAVED_CSV

	const int maxLineLength = 1024 * 16;
	// we sprintf all our output into hear so we can prepend the 'Total Time'.
	char tempBuffer[maxLineLength*2];
	// tempBuffers 1 and 2 are for the non interleaved CSV output
	char *tempBuffer1 = tempBuffer;
	char *tempBuffer2 = tempBuffer + maxLineLength;

	double time = 0.0;
	for (uint32 i = 0; i < frameCount_; ++i)
	{
		UniqueNamesSet::iterator currentName = uniqueNames_.begin();
		const uint32 startEntryIndex = perFrameSortedEntries_[i];
		const uint32 finalEntryIndex = perFrameSortedEntries_[i+1];
		MF_ASSERT( startEntryIndex <= finalEntryIndex );
		uint32 currEntryIndex = startEntryIndex;

		int count=0;
		double totalTime=0.0f;
		int missingCount=0;
		float missingTime=0.0f;

		tempBuffer1[0]='\0';
		tempBuffer2[0]='\0';
		char* currentChar1 = tempBuffer1;
		char* currentChar2 = tempBuffer2;

		// iterate through all the entries and the unique names of the entries
		while (currEntryIndex != finalEntryIndex && 
			currentName != uniqueNames_.end())
		{
			const SortedEntry& currentEntry = historyData_[currEntryIndex];

			// entry and name match
			if (currentEntry.entryName_ == *currentName)
			{
				time = stampsToSeconds( currentEntry.numCycles_ ) * 1000;
				MF_ASSERT( time < 100000.0f ); // sanity checking
				totalTime += time;
				
				if (interleaved)
				{
					currentChar1 += sprintf( currentChar1, "%f,%i,", 
						time, currentEntry.numCalls_ );
				}
				else
				{
					currentChar1 += sprintf( currentChar1, "%f,", time );
					currentChar2 += sprintf( currentChar2, "%i,", 
						currentEntry.numCalls_ );
				}

				++currEntryIndex;
				++currentName;
			}
			// missing slot.
			else if (strcmp( currentEntry.entryName_, *currentName ) < 0)
			{
				// WARNING_MSG("Missing slot %i %s\n",count, 
				//		currentEntry->entryName_);
				++missingCount;
				missingTime += 
					(float)stampsToSeconds( currentEntry.numCycles_ ) * 1000;
				++currEntryIndex;
			}
			// the function corresponding to this entry wasn't called this frame
			else
			{
				// DEBUG_MSG("Skipping slot %i %s\n", count, *currentName);
				if (interleaved)
				{
					currentChar1 += sprintf( currentChar1, "%f,%i,", 0.0f, 0);
				}
				else
				{
					currentChar1 += sprintf( currentChar1, "%f,", 0.0f );
					currentChar2 += sprintf( currentChar2, "%i,", 0 );
				}
				++currentName;
			}
			
			++count;

			if (interleaved) 
			{
				MF_ASSERT( currentChar1 - tempBuffer1 < maxLineLength * 2 );
			}
			else
			{
				MF_ASSERT( currentChar1 - tempBuffer1 < maxLineLength );
				MF_ASSERT( currentChar2 - tempBuffer2 < maxLineLength );
			}
		}

		if (interleaved)
		{
			fprintf( historyFile_, "%f,%f,%s %f, %i\n", totalTime, 
				timeTakenForProfiling_, tempBuffer,missingTime, missingCount );
		}
		else
		{
			fprintf( historyFile_, "%f,%f,%s%f,%s%i\n",totalTime, 
				timeTakenForProfiling_, tempBuffer1, missingTime, tempBuffer2, 
				missingCount);
		}
	}

	frameCount_ = 0;
	unlock();
}

/**
 * @param amt Must be >= 0.
 *
 * @warning Overflow is NOT detected.
 */
void ProfilerLevelCounter::increase( int amt )
{
	MF_ASSERT( amt >= 0 );
	if (amt == 0) 
	{
		return;
	}
	bw_atomic32_t prev = BW_ATOMIC32_FETCH_AND_ADD( &level_, amt );
	g_profiler.addEntry( desc_, Profiler::EVENT_COUNTER, prev + amt );
}

/**
 * @param amt Must be >= 0.
 *
 * @note Underflow of the counter will result in an assert in debug
 * builds.
 */
void ProfilerLevelCounter::decrease( int amt )
{
	MF_ASSERT( amt >= 0 );
	if (amt == 0) 
	{
		return;
	}
	bw_atomic32_t prev = BW_ATOMIC32_FETCH_AND_SUB( &level_, amt );
	MF_ASSERT_DEBUG( (prev - amt) >= 0 );
	g_profiler.addEntry( desc_, Profiler::EVENT_COUNTER, prev - amt );
}


#endif // ENABLE_PROFILER


BW_END_NAMESPACE


// profiler.cpp
