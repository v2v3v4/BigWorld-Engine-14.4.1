/**
 *	@file timestamp.cpp
 */

#include "pch.hpp"

#include "timestamp.hpp"
#include "debug.hpp"
#include "watcher.hpp"

#if defined( __APPLE__ ) || defined( __ANDROID__ ) || defined( EMSCRIPTEN )

BW_BEGIN_NAMESPACE

static uint64 calcStampsPerSecond()
{
	// Using gettimeofday and returning microseconds.
	return 1000000ULL;
}

BW_END_NAMESPACE

#elif defined( __linux__ )

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


BW_BEGIN_NAMESPACE

#ifdef BW_USE_RDTSC
BWTimingMethod g_timingMethod = RDTSC_TIMING_METHOD;
#else // BW_USE_RDTSC
const BWTimingMethod DEFAULT_TIMING_METHOD = GET_TIME_TIMING_METHOD;
BWTimingMethod g_timingMethod = NO_TIMING_METHOD;
#endif // BW_USE_RDTSC


namespace
{
const char * timingMethod()
{
	switch (g_timingMethod)
	{
		case NO_TIMING_METHOD:
			return "none";

		case RDTSC_TIMING_METHOD:
			return "rdtsc";

		case GET_TIME_OF_DAY_TIMING_METHOD:
			return "gettimeofday";

		case GET_TIME_TIMING_METHOD:
			return "gettime";

		default:
			return "Unknown";
	}
}
}

static uint64 calcStampsPerSecond_rdtsc()
{
	struct timeval	tvBefore,	tvSleep = {0, 500000},	tvAfter;
	uint64 stampBefore,	stampAfter;

	// Prime it (just in cache)
	gettimeofday( &tvBefore, NULL );
	gettimeofday( &tvBefore, NULL );

	/* If we do these in the same order, then as long as the offset of
	the time returned by gettimeofday is consistent, we should be ok. */
	gettimeofday( &tvBefore, NULL );
	stampBefore = timestamp();

	select( 0, NULL, NULL, NULL, &tvSleep );

	// And again
	gettimeofday( &tvAfter, NULL );
	gettimeofday( &tvAfter, NULL );

	gettimeofday( &tvAfter, NULL );
	stampAfter = timestamp();

	uint64 microDelta =
		(tvAfter.tv_usec + 1000000 - tvBefore.tv_usec) % 1000000;
	uint64 stampDelta = stampAfter - stampBefore;

	return ( stampDelta * 1000000ULL ) / microDelta;
	// the multiply above won't overflow until we get over 4THz processors :)
}

#if !defined( BW_USE_RDTSC )
static uint64 calcStampsPerSecond_gettime()
{
	// Using gettimeofday and returning microseconds.
	return 1000000000ULL;
}
#endif

static uint64 calcStampsPerSecond_gettimeofday()
{
	// Using gettimeofday and returning microseconds.
	return 1000000ULL;
}

static uint64 calcStampsPerSecond()
{
	static bool firstTime = true;
	if (firstTime)
	{
		MF_WATCH( "timingMethod", &timingMethod );
		firstTime = false;
	}

#ifdef BW_USE_RDTSC
	return calcStampsPerSecond_rdtsc();
#else // BW_USE_RDTSC
	if (g_timingMethod == RDTSC_TIMING_METHOD)
		return calcStampsPerSecond_rdtsc();
	else if (g_timingMethod == GET_TIME_OF_DAY_TIMING_METHOD)
		return calcStampsPerSecond_gettimeofday();
	else if (g_timingMethod == GET_TIME_TIMING_METHOD)
		return calcStampsPerSecond_gettime();
	else
	{
		char * timingMethod = getenv( "BW_TIMING_METHOD" );
		if (!timingMethod)
		{
			g_timingMethod = DEFAULT_TIMING_METHOD;
		}
		else if (strcmp( timingMethod, "rdtsc" ) == 0)
		{
			g_timingMethod = RDTSC_TIMING_METHOD;
		}
		else if (strcmp( timingMethod, "gettimeofday" ) == 0)
		{
			g_timingMethod = GET_TIME_OF_DAY_TIMING_METHOD;
		}
		else if (strcmp( timingMethod, "gettime" ) == 0)
		{
			g_timingMethod = GET_TIME_TIMING_METHOD;
		}
		else
		{
			WARNING_MSG( "calcStampsPerSecond: "
						 "Unknown timing method '%s', using clock_gettime.\n",
						 timingMethod );
			g_timingMethod = DEFAULT_TIMING_METHOD;
		}

		return calcStampsPerSecond();
	}
#endif // BW_USE_RDTSC
}


uint64 stampsPerSecond_rdtsc()
{
	static uint64 stampsPerSecondCache = calcStampsPerSecond_rdtsc();
	return stampsPerSecondCache;
}

double stampsPerSecondD_rdtsc()
{
	static double stampsPerSecondCacheD = double(stampsPerSecond_rdtsc());
	return stampsPerSecondCacheD;
}

uint64 stampsPerSecond_gettimeofday()
{
	static uint64 stampsPerSecondCache = calcStampsPerSecond_gettimeofday();
	return stampsPerSecondCache;
}

double stampsPerSecondD_gettimeofday()
{
	static double stampsPerSecondCacheD = double(stampsPerSecond_gettimeofday());
	return stampsPerSecondCacheD;
}

BW_END_NAMESPACE

#elif defined(_WIN32) // not defined( __linux__ )

#ifndef _XBOX360
#include "cstdmf/cstdmf_windows.hpp"
#endif // _XBOX360


BW_BEGIN_NAMESPACE

#ifdef BW_USE_RDTSC

uint64 g_busyIdleCounter = 0;	// global to avoid over-zealous optimiser
volatile static bool continueBusyIdle;
static DWORD WINAPI busyIdleThread( LPVOID arg )
{
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_LOWEST );

	while (continueBusyIdle) ++g_busyIdleCounter;

	return 0;
}

static uint64 calcStampsPerSecond()
{		// see comments for the unix version
	LARGE_INTEGER	tvBefore,	tvAfter;
	DWORD			tvSleep = 500;
	uint64 stampBefore,	stampAfter;

	// start a low-priority busy idle thread to use 100% CPU on laptops
	continueBusyIdle = true;
	DWORD busyIdleThreadID = 0;
	HANDLE thread = CreateThread( NULL, 0, &busyIdleThread, NULL, 0, &busyIdleThreadID );
	Sleep( 100 );	// a chance for CPU speed to adjust


	QueryPerformanceCounter( &tvBefore );
	QueryPerformanceCounter( &tvBefore );

	QueryPerformanceCounter( &tvBefore );
	stampBefore = timestamp();

	Sleep(tvSleep);

	QueryPerformanceCounter( &tvAfter );
	QueryPerformanceCounter( &tvAfter );

	QueryPerformanceCounter( &tvAfter );
	stampAfter = timestamp();

	uint64 countDelta = tvAfter.QuadPart - tvBefore.QuadPart;
	uint64 stampDelta = stampAfter - stampBefore;

	LARGE_INTEGER	frequency;
	QueryPerformanceFrequency( &frequency );

	continueBusyIdle = false;
	CloseHandle( thread );

	return (uint64)( ( stampDelta * uint64(frequency.QuadPart) ) / countDelta );
	// the multiply above won't overflow until we get over 4THz processors :)
	//  (assuming the performance counter stays at about 1MHz)
}

#else // BW_USE_RDTSC

static uint64 calcStampsPerSecond()
{
	LARGE_INTEGER rate;
	MF_VERIFY( QueryPerformanceFrequency( &rate ) );
	return rate.QuadPart;
}

#endif // BW_USE_RDTSC

BW_END_NAMESPACE

#elif defined( PLAYSTATION3 )

BW_BEGIN_NAMESPACE

static uint64 calcStampsPerSecond()
{
	return sys_time_get_timebase_frequency();
}

BW_END_NAMESPACE

#endif // PLAYSTATION3

BW_BEGIN_NAMESPACE


static uint64 stampsPerSecondCache = calcStampsPerSecond();
static double stampsPerSecondCacheD = static_cast< double >(stampsPerSecond());

/**
 *	This function tells you how many there are in a second. It caches its reply
 *	after being called for the first time, however that call may take some time.
 */
uint64 stampsPerSecond()
{
	return stampsPerSecondCache;
}

/**
 *	This function tells you how many there are in a second as a double precision
 *	floating point value. It caches its reply after being called for the first
 *	time, however that call may take some time.
 */
double stampsPerSecondD()
{
	return stampsPerSecondCacheD;
}

BW_END_NAMESPACE

/* timestamp.cpp */
