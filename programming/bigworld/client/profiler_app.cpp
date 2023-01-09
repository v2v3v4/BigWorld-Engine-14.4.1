#include "pch.hpp"

#include <psapi.h>

#include "app.hpp"
#include "camera_app.hpp"
#include "profiler_app.hpp"
#include "client_camera.hpp"
#include "fly_through_camera.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/bw_list.hpp"
#include "cstdmf/memory_load.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_overlapper.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_terrain.hpp"
#include "chunk/user_data_object.hpp"

#include "romp/console_manager.hpp"
#include "romp/xconsole.hpp"
#include "romp/flora.hpp"

BW_BEGIN_NAMESPACE

static const float DAMPING_RATIO = 0.97f;

typedef std::pair<BW::string, float> PairStringFloat;
typedef BW::list<PairStringFloat> ListStringFloat;

// ----------------------------------------------------------------------------
// Section ProfilerApp
// ----------------------------------------------------------------------------

ProfilerApp ProfilerApp::instance;
int ProfilerApp_token;


ProfilerApp::ProfilerApp() :
	cpuStall_( 0.f ),
	filteredDTime_( 0.f ),
	filteredFps_( 0.f )
{
	BW_GUARD;
	MainLoopTasks::root().add( this, "Profiler/App", NULL );
}


ProfilerApp::~ProfilerApp()
{
	BW_GUARD;
	/*MainLoopTasks::root().del( this, "Profiler/App" );*/
}


bool ProfilerApp::init()
{
	BW_GUARD;
	if (App::instance().isQuiting())
	{
		return false;
	}

	// Set up watches
	MF_WATCH( "Render/Performance/FPS(filtered)",
		filteredFps_,
		Watcher::WT_READ_ONLY,
		"Filtered FPS." );
	MF_WATCH( "Render/Performance/Frame Time (filtered)",
		filteredDTime_,
		Watcher::WT_READ_ONLY,
		"Filtered frame time." );
	MF_WATCH( "Render/Performance/CPU Stall",
		cpuStall_,
		Watcher::WT_READ_WRITE,
		"Stall CPU by this much each frame to see if we are GPU bound." );

#if ENABLE_WATCHERS
	Allocator::addWatchers();
#endif

	return true;
}


void ProfilerApp::fini()
{
	BW_GUARD;	
}


void ProfilerApp::tick( float /* dGameTime */, float dRenderTime )
{
	BW_GUARD;
	// Gather stats
	filteredDTime_ = ( DAMPING_RATIO * filteredDTime_ ) +
		( ( 1.f - DAMPING_RATIO ) * dRenderTime );

	double cpuFreq = stampsPerSecondD() / 1000.0;
	if (cpuStall_ > 0.f )
	{
		uint64 cur = timestamp();
		uint64 end = cur + ( uint64 )( cpuStall_ * cpuFreq );
		do
		{
			cur = timestamp();
		} 
		while ( cur < end );
	}

	MF_ASSERT( dRenderTime > 0.f );

	filteredFps_ = ( DAMPING_RATIO * filteredFps_ ) +
		( ( 1.f - DAMPING_RATIO ) * 1.f / dRenderTime );

	// Tick FlyThroughCamera-based profiler
	updateProfiler( dRenderTime );
}


void ProfilerApp::draw()
{
	BW_GUARD;	
}

// ----------------------------------------------------------------------------
// ProfilerFlyThroughSpline class
// ----------------------------------------------------------------------------
/*
 * This is a Catmull-Rom spline.
 *
 * It uses a hard-coded speed approximating SPLINE_SPEED/4 edges per second,
 * and ignores the speed attributes of the FlyThroughNodes.
 *
 * It sets endOfPath() to true when it passes nodes[nodeLen-2], but will
 * loop through the two control nodes nodes[nodeLen-1] and nodes[0] if allowed.
 */
const static float SPLINE_SPEED = 20.f;

class ProfilerFlyThroughSpline : public FlyThroughSpline 
{
public:
	ProfilerFlyThroughSpline() :
		camera_( Matrix::identity ),
		endOfPath_( false ),
		nextNodeId_( 1 ),
		edgePosition_( 1.f ),
		edgesPerSecond_( 0.f )
	{};

	bool update( float dTime, const FlyThroughNodes & nodes );

	void reset();

	bool endOfPath() const { return endOfPath_; }

	const Matrix & camera() const { return camera_; }

private:
	///	@name Data representing where we are on the path
	//@{
	Matrix camera_;
	bool endOfPath_;
	//@}

	///	@name Data controlling where we are on the path
	//@{
	// The id of the node at the end of the current edge
	unsigned int nextNodeId_;
	// Position on the current edge, as a proportion of the edge
	float edgePosition_;
	// Increase in edgePosition per second.
	float edgesPerSecond_;
	// Details about the current edge, and the edges either side.
	Vector3 p0_, p1_, p2_, p3_;
	Vector3 r0_, r1_, r2_, r3_;
	//@}

	static void clampAngles( const Vector3 & start, Vector3 & end );
};

/**
 * A helper function to clamp rotation to the acute angles between 2 angles.
 */
void ProfilerFlyThroughSpline::clampAngles( const Vector3 & start, Vector3 & end )
{
	BW_GUARD;

	Vector3 rot = end - start;
	if (rot[0] <= -MATH_PI)
	{
		end[0] += MATH_2PI;
	} 
	else if (rot[0] > MATH_PI)
	{
		end[0] -= MATH_2PI;
	}

	if (rot[1] <= -MATH_PI)
	{
		end[1] += MATH_2PI; 
	}
	else if (rot[1] > MATH_PI)
	{
		end[1] -= MATH_2PI; 
	}

	if (rot[2] <= -MATH_PI)
	{
		end[2] += MATH_2PI; 
	}
	else if (rot[2] > MATH_PI)
	{
		end[2] -= MATH_2PI; 
	}
}

bool ProfilerFlyThroughSpline::update( float dTime, 
	const FlyThroughNodes & nodes )
{
	BW_GUARD;

	/* We need at least four nodes */
	if (nodes.size() < 4 )
	{
		return false;
	}

	endOfPath_ = false;

	edgePosition_ += dTime * edgesPerSecond_;
	if (edgePosition_ >= 1.f)
	{
		const size_t nodeCount = nodes.size();
		const int steps = (int)floorf(edgePosition_);
		
		nextNodeId_ += steps;
		edgePosition_ -= steps;

		if (nextNodeId_ >= nodeCount - 1) 
		{
			endOfPath_ = true;
		}

		// Get the interpolation samples, treating the node list
		// as a loop.
		const size_t n0 = ( nextNodeId_ + nodeCount - 2 ) % nodeCount;
		const size_t n1 = ( nextNodeId_ + nodeCount - 1 ) % nodeCount;
		const size_t n2 = ( nextNodeId_ ) % nodeCount;
		const size_t n3 = ( nextNodeId_ + 1 ) % nodeCount;

		p0_ = nodes[ n0 ].pos;
		r0_ = nodes[ n0 ].rot;

		p1_ = nodes[ n1 ].pos;
		r1_ = nodes[ n1 ].rot;

		p2_ = nodes[ n2 ].pos;
		r2_ = nodes[ n2 ].rot;

		p3_ = nodes[ n3 ].pos;
		r3_ = nodes[ n3 ].rot;

		nextNodeId_ = nextNodeId_ % nodeCount;

		//Make sure the rotations only result in acute turns
		clampAngles( r0_, r1_ );
		clampAngles( r1_, r2_ );
		clampAngles( r2_, r3_ );

		//We need to blend the time increment to avoid large speed changes
		edgesPerSecond_ = SPLINE_SPEED / ((p0_ - p1_).length() +
					2.f * (p1_ - p2_).length() +
					(p2_ - p3_).length());
	}

	//Interpolate the position and rotation using a Catmull-Rom spline
	const float t2 = edgePosition_ * edgePosition_;
	const float t3 = edgePosition_ * t2;
	Vector3 pos =	0.5 *((2 * p1_) +
					(-p0_ + p2_) * edgePosition_ +
					(2*p0_ - 5*p1_ + 4*p2_ - p3_) * t2 +
					(-p0_ + 3*p1_ - 3*p2_ + p3_) * t3 );

	Vector3 rot =	0.5 *((2 * r1_) +
					(-r0_ + r2_) * edgePosition_ +
					(2*r0_ - 5*r1_ + 4*r2_ - r3_) * t2 +
					(-r0_ + 3*r1_ - 3*r2_ + r3_) * t3 );

	//Work out the direction and up vector
	Matrix m = Matrix::identity;
	m.preRotateY( rot[0] );
	m.preRotateX( rot[1] );
	m.preRotateZ( rot[2] );

	Vector3 dir = m.applyToUnitAxisVector( Z_AXIS );
	Vector3 up = m.applyToUnitAxisVector( Y_AXIS );

	//Update the transform
	camera_.lookAt( pos, dir, up );

	return true;
}

void ProfilerFlyThroughSpline::reset() 
{
	BW_GUARD;

	// This has the effect of moving the flythrough
	// to nextNodeId_ 2, edgePosition_ = 0.
	nextNodeId_ = 1;
	edgePosition_ = 1.f;
	edgesPerSecond_ = 0.f;
}

// ----------------------------------------------------------------------------
// Section Profile
// ----------------------------------------------------------------------------

static double s_soakTime;
static char s_csvFilename[256];
static FILE * s_csvFile = NULL;

static BW::vector<float> s_frameTimes;
static float s_frameTimesTotal = 0.0f;

static bool s_soakTest = false;
static int s_state = -1;

static const float FRAME_STEP = 0.1f;
static const double SOAK_SETTLE_TIME = 60.0 * 20.0;
static const double VALIDATE_TIME = 5.0;

static int  s_profilerNumRuns = 2;
static char s_profilerOutputRoot[MAX_PATH];
static bool s_profilerExitOnComplete = true;

static FlyThroughCameraPtr s_flyThroughCamera = NULL;

static time_t s_startTime;
static double s_prevValidateTime;
static int s_numSoakFrames;

#if ENABLE_MEMORY_DEBUG
static Allocator::AllocationStats s_settledStats;
static bool s_settled = false;
#endif // ENABLE_MEMORY_DEBUG

// Static crap
static void profilingLapStart() 
{
	// Advance to the next run
	char historyFileName[MAX_PATH];
	sprintf( historyFileName, "%s_%d.csv", s_profilerOutputRoot, s_state );

#if ENABLE_PROFILER
	BW::string msg;
	g_profiler.setNewHistory( historyFileName, "MainThread", &msg);
	DEBUG_MSG( "%s\n", msg.c_str() );
#endif

	s_flyThroughCamera->restart();
	std::time( &s_startTime );
	s_frameTimes.clear();
	s_frameTimesTotal = 0.0f;
	s_state++;
}

static void setupProfileRun() 
{
	CameraApp::instance().clientCamera().camera( 
		BaseCameraPtr(s_flyThroughCamera) );
	std::time( &s_startTime );
	if(s_soakTest)
	{
		s_numSoakFrames = 0;
		s_prevValidateTime = 0;
		s_csvFile = fopen(s_csvFilename, "w");
		if(s_csvFile)
		{
			fprintf( s_csvFile,
				"Time Stamp, Bytes, Blocks, Proc Bytes, Num Frames, Time,"
				" Frame Time Avg, Frame Time Std Dev, FPS Avg\n" );
			DEBUG_MSG("created csv file: %s.\n", s_csvFilename);
		}
		else
		{
			DEBUG_MSG("could not create csv file: %s.\n", s_csvFilename);
		}
	}
	else 
	{
		profilingLapStart();
	}
	s_state = 1;
}


/**
 *	Return the averge time per frame and the frame standard deviation
 */
static void calculatePerformance( float & frameTimesAvg, 
								 float & frameTimesStdDev, 
								 float & frameTimeDifferences,
								 float & frameTimePow2Differences )
{
	frameTimesAvg = s_frameTimesTotal / float( s_frameTimes.size() );
	frameTimesStdDev = 0.0f;
	frameTimeDifferences = 0.0;
	frameTimePow2Differences = 0.0;

	for ( uint i = 0; i < s_frameTimes.size(); i++ )
	{
		float dev = s_frameTimes[i] - frameTimesAvg;
		frameTimesStdDev += dev * dev;
		if (i > 0)
		{
			float diff = (s_frameTimes[i] - s_frameTimes[i-1]);
			frameTimeDifferences += abs(diff);
			frameTimePow2Differences += (diff * diff * 
				/*just to make the numbers more significant */ 100);
		}
	}

	frameTimesStdDev = sqrt( frameTimesStdDev / float( s_frameTimes.size() ) );
}

void profilerFinished(const ListStringFloat & sentList)
{
	ScriptArgs args;
	if (!sentList.empty())
	{
		ScriptList scriptList = ScriptList::create( );
		
		ListStringFloat::const_iterator iter = sentList.begin();
		for (; iter != sentList.end(); iter++)
		{
			ScriptTuple scriptPair = ScriptTuple::create( 2 );
			scriptPair.setItem( 0, ScriptObject::createFrom( iter->first ) );
			scriptPair.setItem( 1, ScriptObject::createFrom( iter->second ) );

			scriptList.append( scriptPair );
		}
		args = ScriptArgs::create( scriptList );
	}
	else
	{
		args = ScriptArgs::create( ScriptObject::none() );
	}
	 
	s_flyThroughCamera->complete( args, true );
	s_flyThroughCamera = NULL;
}

/**
* This method calls profilerFinished sending it the standard results 
* list (fps statistics)
*/
void profilerFinishedWithStats()
{
	float frameTimesAvg;
	float frameTimesStdDev;
	float frameTimeDifferences;
	float frameTimePow2Differences;
	calculatePerformance( frameTimesAvg, frameTimesStdDev, 
		frameTimeDifferences, frameTimePow2Differences );

	ListStringFloat sentList;
	sentList.push_back( PairStringFloat( 
		"Total Frames", (float)s_frameTimes.size() ) );
	sentList.push_back( PairStringFloat( 
		"Milli per frame", 1000.0f * frameTimesAvg ) );
	sentList.push_back( PairStringFloat( 
		"FPS", 1.0f / frameTimesAvg ) );
	sentList.push_back( PairStringFloat( 
		"Standard Deviation", 1000.0f * frameTimesStdDev ) );
	sentList.push_back( PairStringFloat( 
		"Frame differences", 1000.0f * frameTimeDifferences ) );
	sentList.push_back( PairStringFloat( 
		"Frame differences Pow 2", 1000.0f * frameTimePow2Differences ) );
	profilerFinished(sentList);
}


static void reportCSVStats(FILE * file, double elapsedTime )
{
	BW_GUARD;	
#if ENABLE_MEMORY_DEBUG
	// Get memory stats
	Allocator::AllocationStats curStats;
	Allocator::readAllocationStats( curStats );

	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo( GetCurrentProcess(), &pmc, sizeof( pmc ) );

	// Calculate performance stats
	float frameTimesAvg;
	float frameTimesStdDev;
	float frameTimeDifferences;
    float frameTimePow2Differences;
	calculatePerformance( frameTimesAvg, frameTimesStdDev, 
		frameTimeDifferences, frameTimePow2Differences );

	// Print stats for that block
	fprintf( file, "%g, %d, %d, %lu, %d, %f, %f, %f, %f\n", elapsedTime,
		curStats.currentBytes_, curStats.currentAllocs_,
		pmc.PagefileUsage, s_frameTimes.size(), s_frameTimesTotal * 1000.0f,
		frameTimesAvg * 1000.0f, frameTimesStdDev * 1000.0f,
		1.0f / frameTimesAvg );
#endif // ENABLE_MEMORY_DEBUG
}


static void flyThroughCompleteCallback() {
	MF_ASSERT( !s_soakTest );
	CameraApp::instance().clientCamera().camera( 
		s_flyThroughCamera->origCamera() );
}


// Returns true if this was the last lap
static bool profilingLapComplete() {
	float frameTimesAvg;
	float frameTimesStdDev;
	float frameTimeDifferences;
	float frameTimePow2Differences;
	calculatePerformance( frameTimesAvg, frameTimesStdDev, 
		frameTimeDifferences, frameTimePow2Differences );

	DEBUG_MSG( "Profile run %d done\n", s_state );
	DEBUG_MSG( "%d frames, averaging %f ms per frame (%f frames per second)\n",
		s_frameTimes.size(), 1000.0f * frameTimesAvg, 1.0f / frameTimesAvg );
	DEBUG_MSG( "Standard Deviation: %f\n", 1000.0f * frameTimesStdDev );
	DEBUG_MSG( "Frame differences: %f\n", frameTimeDifferences );
	DEBUG_MSG( "Frame differences Pow 2: %f\n", frameTimePow2Differences );

#if ENABLE_MEMORY_DEBUG
	Allocator::debugAllocStatsReport();
#endif // ENABLE_MEMORY_DEBUG

	// If we've done the last run quit out
	if ( s_state == s_profilerNumRuns )
	{
#if ENABLE_PROFILER
		g_profiler.closeHistory();
#endif
		profilerFinishedWithStats();

		if (s_profilerExitOnComplete)
		{
			App::instance().quit();
		}
		return true;
	}

	CameraApp::instance().clientCamera().camera( 
		BaseCameraPtr(s_flyThroughCamera) );
	profilingLapStart();
	return false;
}


static void soakUpdate( double elapsedTime ) {
	// Process soak test update
	if ( ( elapsedTime - s_prevValidateTime ) > VALIDATE_TIME )
	{
#if ENABLE_MEMORY_DEBUG
		Allocator::debugAllocStatsReport();

		// report stats to csv file
		if(s_csvFile)
		{
			reportCSVStats( s_csvFile, elapsedTime );
		}

		// Reset frame times
		s_frameTimes.clear();
		s_frameTimesTotal = 0.0f;

		s_prevValidateTime = elapsedTime;

		if ( ( elapsedTime >= SOAK_SETTLE_TIME ) && !s_settled )
		{
			s_settled = true;
			Allocator::readAllocationStats( s_settledStats );
			DEBUG_MSG( "Soak settle period complete\n" );
		}
#endif // ENABLE_MEMORY_DEBUG

		if ( elapsedTime >= s_soakTime )
		{
			if (s_csvFile)
			{
				fclose(s_csvFile);
				s_csvFile = NULL;
			}

			float secondsPerFrame = 
				( float )elapsedTime / ( float )s_numSoakFrames;
			float framesPerSecond = 1.0f / secondsPerFrame;

			DEBUG_MSG( "Soak Test Done\n" );
			DEBUG_MSG( "%d frames, averaging %f ms per frame "
				"(%f frames per second)\n",
				s_numSoakFrames, 1000.0f * secondsPerFrame, framesPerSecond );

#if ENABLE_MEMORY_DEBUG
			// Print leak error if we have > 110% of settled
			Allocator::AllocationStats curStats;
			Allocator::readAllocationStats( curStats );

/* TODO: Get the approximate peak based on our sampling from the app and 
 * put this back

			if (s_settled && 
				(10 * curStats.peakBytes_) > (11 * s_settledStats.peakBytes_))
			{
				WARNING_MSG( "Too much memory has been leaked\n" );
			}
*/					 
			ListStringFloat sentList;
			sentList.push_back( PairStringFloat( 
				"Total Frames", (float)s_numSoakFrames ) );
			sentList.push_back( PairStringFloat( 
				"Milli per frame", 1000.0f * secondsPerFrame ) );
			sentList.push_back( PairStringFloat( 
				"FPS", (float)framesPerSecond));
			profilerFinished(sentList);
#endif // ENABLE_MEMORY_DEBUG
			App::instance().quit();
			return;
		}
	}

}


/**
 *	Tick a profile or SoakTest run (from a FlyThroughCamera)
 */
void ProfilerApp::updateProfiler( float dTime )
{
	BW_GUARD;

	// We don't think we're running, so nothing to do.
	if (s_flyThroughCamera == NULL)
	{
		return;
	}

	//If we have a profiling job to start
	if ( s_state == 0 )
	{
		MF_ASSERT( !s_flyThroughCamera->running() );
		setupProfileRun();
		MF_ASSERT( s_state == 1 );
		MF_ASSERT( s_flyThroughCamera->running() );
	}

	if ( !s_soakTest && s_state > 0 && !s_flyThroughCamera->running() )
	{
		// New lap of the profile run.
		// Note that this shadows the next check during a profile run.
		if (profilingLapComplete())
		{
			return;
		}
	}

	// Someone switched cameras
	if (!s_flyThroughCamera->running()) {
		WARNING_MSG( "Switched camera during profiling run?"
			" Aborting profiling run\n" );
		s_frameTimes.clear();
		s_frameTimesTotal = 0.0f;
		// If the camera has been changed, don't override that.
		s_flyThroughCamera->complete( ScriptArgs(), false );
		s_flyThroughCamera = NULL;
		return;
	}

	// Store frame time
	s_frameTimes.push_back( dTime );
	s_frameTimesTotal += dTime;

	if ( s_soakTest )
	{
		time_t curTime;
		std::time( &curTime );
		s_numSoakFrames++;
		soakUpdate( difftime( curTime, s_startTime ) );
	}
}

// ----------------------------------------------------------------------------
// Section Python module functions
// ----------------------------------------------------------------------------

/**
 *
 */
#if ENABLE_MEMORY_DEBUG
void outputMemoryStats( )
{
	BW_GUARD;
	Allocator::debugAllocStatsReport();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, outputMemoryStats, END, BigWorld )
#endif // ENABLE_MEMORY_DEBUG


#if ENABLE_CONSOLES
static Memory::MemSize loadingMemoryStart = 0;

/**
 * called when loading has completed. Outputs time to python console.
 *
 * @param outputString string to be written to the python console.
 *
 */
void loadingFinishedCallback( const char* outputString )
{
	ConsoleManager::instance().find( "Python" )->print( outputString );

//	int32 memdiff = (int)(Memory::usedMemory() - loadingMemoryStart);
//	char buffer[256];
//	sprintf( buffer, "Memory leaked = %ikb\n", memdiff/1024 );
//	ConsoleManager::instance().find( "Python" )->print( buffer );
}


/**
 * unload all the visible chunks and start the timer when the first
 * one starts loading.
 */
void timeLoading()
{
	ChunkManager::instance().startTimeLoading( &loadingFinishedCallback );
	loadingMemoryStart = Memory::usedMemory();

	Chunk * pCameraChunk = ChunkManager::instance().cameraChunk();
	if (pCameraChunk)
	{
		GeometryMapping * pCameraMapping = pCameraChunk->mapping();
		ChunkSpacePtr pSpace = pCameraChunk->space();

		// unload every chunk in sight
		ChunkMap & chunks = pSpace->chunks();
		for (ChunkMap::iterator it = chunks.begin(); it != chunks.end(); it++)
		{
			for (uint i = 0; i < it->second.size(); i++)
			{
				Chunk * pChunk = it->second[i];
				if (pChunk->isBound())
				{
					BWResource::instance().purge( pChunk->resourceID() );
					pChunk->unbind( false );
					pChunk->unload();
				}
			}
		}

		// now reload the camera chunk
		ChunkManager::instance().loadChunkExplicitly(
			pCameraChunk->identifier(), pCameraMapping );

		// and repopulate the flora
		Flora::floraReset();
	}
	ConsoleManager::instance().find( "Python" )->print( "Reloading..." );

}
PY_AUTO_MODULE_FUNCTION( RETVOID, timeLoading, END, BigWorld )
#endif // ENABLE_CONSOLES

/**
 *  Prints the names and indices of the currently active threads to the 
 * console window
 */
void printThreadNames()
{
	BW_GUARD;
#if ENABLE_PROFILER && ENABLE_CONSOLES
	for(int i=0;i<Profiler::MAXIMUM_NUMBER_OF_THREADS;i++)
	{
		const char* threadName = g_profiler.getThreadName(i);
		if(threadName)
		{
			char buffer[256];
			sprintf(buffer," %i : '%s'\n",i,threadName);
			ConsoleManager::instance().find( "Python" )->print( buffer );
		}
	}
#endif
}
PY_AUTO_MODULE_FUNCTION( RETVOID, printThreadNames, END, BigWorld )

/**
 * Starts profiling to a CSV. 
 * The code looks for the first thread that has the string threadName in it
 * ie "Main" will find "MainThread", as will "M"
 * 
 */
PyObject* py_setProfilerHistory( PyObject * args )
{
	BW_GUARD;
	const char * historyFileName = NULL;
	const char * threadName = NULL;
	if (!PyArg_ParseTuple( args, "s|s:BigWorld.setProfilerHistory",
			&historyFileName, &threadName ))
	{
		return NULL;
	}

#if ENABLE_PROFILER
	if(!threadName)
	{
		threadName = "MainThread";
	}

	BW::string msgString;
	if(!g_profiler.setNewHistory( historyFileName, threadName, &msgString ))
	{
		PyErr_SetString( PyExc_IOError, "Error opening file. "
			"Profiling aborted." );
	}

#if ENABLE_CONSOLES
	if (msgString.size()>0)
	{
		ConsoleManager::instance().find( "Python" )->print( msgString );
	}
#endif // ENABLE_CONSOLES

#endif

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( setProfilerHistory, BigWorld )


/**
 * function BigWorld.closeProfilerHistory
 * @components{ client }
 * 
 * Stop profiling to CSV file.
 */
void closeProfilerHistory()
{
#if ENABLE_PROFILER
	g_profiler.closeHistory();
#endif
}
PY_AUTO_MODULE_FUNCTION( RETVOID, closeProfilerHistory, END, BigWorld )

// ----------------------------------------------------------------------------
// Section Python profiler flythough interface
// ----------------------------------------------------------------------------

/*~	function BigWorld.runProfiler
 *	@components{ client }
 *
 *	Runs a client Profile using a FlyThroughCamera
 *
 *	@param cameraNodeName	A string naming the CameraNode to start the 
 *							loop with
 *	@param numRuns (optional)	An int of the number of laps of the loop to 
 *								run, defaults to 2
 *	@param outputRoot (optional)	A string with the prefix for the profiler 
 *									output CSV (defaults to "profiler_run").
 *									This string will have "_N.csv" appended, 
 *									where N is the 0-based lap number.
 *	@param exitOnComplete (optional)	A bool of whether to exit the client 
 *										when the runs have completed.
 */
static PyObject * py_runProfiler( PyObject * args )
{
	BW_GUARD;

	char * cameraNodeName = NULL;
	s_profilerNumRuns = 2;
	strncpy( s_profilerOutputRoot, "profiler_run", MAX_PATH - 1 );
	char * profilerOutputRoot = NULL;
	int exitOnComplete = 1;

	if (!PyArg_ParseTuple( args, "s|isi:BigWorld.runProfiler",
			&cameraNodeName, &s_profilerNumRuns,
			&profilerOutputRoot, &exitOnComplete ))
	{
		Py_RETURN_NONE;
	}

	s_profilerExitOnComplete = (exitOnComplete != 0);

	if (profilerOutputRoot)
	{
		strncpy( s_profilerOutputRoot, profilerOutputRoot, MAX_PATH - 1 );
	}

	PyObjectPtr cameraNode = 
		FlyThroughCamera::findCameraNode( cameraNodeName );
	if (!cameraNode)
	{
		Py_RETURN_NONE;
	}

	s_soakTest = false;
	s_state = 0;
	s_flyThroughCamera = new FlyThroughCamera( cameraNode );

	s_flyThroughCamera->fixedFrameTime( FRAME_STEP );
	s_flyThroughCamera->loop( false );
	s_flyThroughCamera->callback( &flyThroughCompleteCallback );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( runProfiler, BigWorld )

/*~	function BigWorld.runSoakTest
 *	@components{ client }
 *
 *	Runs a SoakTest using a FlyThroughCamera
 *
 *	@param	cameraNodeName	A string naming the CameraNode to start the 
 *							loop with
 *	@param	soakTime (optional)	A float of the soakTest run length in minutes, 
 *								defaults to 24
 *	@param	filename (optional)	A string of the filename to store the 
 *								soakTest status CSV, defaults to 
 *								"soak_{timestamp}.csv". May be None to use 
 *								the default value.
 *	@param	fixedFrameTime (optional)	A bool of whether to move the camera 
 *										with a fixed delta-time per frame
 *										or at the wall-clock delta-time.
 */
static PyObject* py_runSoakTest( PyObject* args )
{
	BW_GUARD;
	char * cameraNodeName = NULL;
	char * filename = NULL;
	s_soakTime = 24.0 * 60.0;
	int fixedFrameTime = 0;

	if (!PyArg_ParseTuple( args, "s|dzi;"
				"BigWorld.runSoakTest() takes args (str)cameraNodeName "
				"[, (float)soakTime, (str)filename, (int)fixedFrameTime]",
			&cameraNodeName,
			&s_soakTime, &filename, &fixedFrameTime ))
	{
		Py_RETURN_NONE;
	}

	// read filename
	memset( s_csvFilename, 0, sizeof(s_csvFilename) );
	if(filename)
	{
		strncpy( s_csvFilename, filename, sizeof(s_csvFilename) );
	}
	else
	{
		sprintf( s_csvFilename, "soak_%d.csv", (int) std::time(NULL) );
	}

	s_soakTime *= 60.0; // make amount equal minutes

	PyObjectPtr cameraNode = 
		FlyThroughCamera::findCameraNode( cameraNodeName );
	if (!cameraNode)
	{
		Py_RETURN_NONE;
	}

	s_soakTest = true;
	s_state = 0;
	s_flyThroughCamera = new FlyThroughCamera( cameraNode );

	if (fixedFrameTime)
	{
		s_flyThroughCamera->fixedFrameTime( FRAME_STEP );
	}
	s_flyThroughCamera->loop( true );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( runSoakTest, BigWorld )

BW_END_NAMESPACE


// profiler_app.cpp
