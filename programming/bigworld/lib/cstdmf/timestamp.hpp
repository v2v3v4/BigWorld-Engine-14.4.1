#ifndef TIMESTAMP_HPP
#define TIMESTAMP_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"

// Indicates whether or not to use a call to RDTSC (Read Time Stamp Counter)
// to calculate timestamp. The benefit of using this is that it is fast and
// accurate, returning actual clock ticks. The downside is that this does not
// work well with CPUs that use Speedstep technology to vary their clock speeds.
#ifndef _XBOX360
#ifdef __unix__
// #define BW_USE_RDTSC
#elif !defined(_WIN64)
# define BW_USE_RDTSC
#endif // !_WIN64
#endif // _XBOX360

/**
 * This function returns the processor's (real-time) clock cycle counter.
 */
#if defined( __APPLE__ ) || defined( __ANDROID__ ) || defined( EMSCRIPTEN )

#include <sys/time.h>

BW_BEGIN_NAMESPACE

inline uint64 timestamp()
{
	timeval tv;
	gettimeofday( &tv, NULL );

	return 1000000ULL * uint64( tv.tv_sec ) + uint64( tv.tv_usec );
}

BW_END_NAMESPACE

#elif defined( __linux__ )

#include <sys/time.h>
#include <time.h>
#include <asm/unistd.h>

BW_BEGIN_NAMESPACE

enum BWTimingMethod
{
	RDTSC_TIMING_METHOD,
	GET_TIME_OF_DAY_TIMING_METHOD,
	GET_TIME_TIMING_METHOD,
	NO_TIMING_METHOD,
};

extern BWTimingMethod g_timingMethod;

inline uint64 timestamp_rdtsc()
{
	uint32 rethi, retlo;
	__asm__ __volatile__ (
		// Read Time-Stamp Counter
		// loads current value of processor's timestamp counter into EDX:EAX
		"rdtsc":
		"=d"    (rethi),
		"=a"    (retlo)
						  );
	return uint64(rethi) << 32 | retlo;
}

// Alternate Linux implementation uses gettimeofday. In rough tests, this can
// be between 20 and 600 times slower than using RDTSC. Also, there is a problem
// under 2.4 kernels where two consecutive calls to gettimeofday may actually
// return a result that goes backwards.

inline uint64 timestamp_gettimeofday()
{
	timeval tv;
	gettimeofday( &tv, NULL );

	return 1000000ULL * uint64( tv.tv_sec ) + uint64( tv.tv_usec );
}

inline uint64 timestamp_gettime()
{
	timespec tv;
	// Using a syscall here so we don't have to link with -lrt
	MF_VERIFY(syscall( __NR_clock_gettime, CLOCK_MONOTONIC, &tv ) == 0);

	return 1000000000ULL * tv.tv_sec + tv.tv_nsec;
}

inline uint64 timestamp()
{
#ifdef BW_USE_RDTSC
	return timestamp_rdtsc();
#else // BW_USE_RDTSC
	if (g_timingMethod == RDTSC_TIMING_METHOD)
		return timestamp_rdtsc();
	else if (g_timingMethod == GET_TIME_OF_DAY_TIMING_METHOD)
		return timestamp_gettimeofday();
	else //if (g_timingMethod == GET_TIME_TIMING_METHOD)
		return timestamp_gettime();

#endif // BW_USE_RDTSC
}

BW_END_NAMESPACE

#elif defined(_WIN32)

#ifdef BW_USE_RDTSC
#pragma warning (push)
#pragma warning (disable: 4035)

BW_BEGIN_NAMESPACE

inline uint64 timestamp()
{
	__asm rdtsc
	// MSVC complains about no return value here.
	// According to the help, this warning is 'harmless', and
	//  they even have example code which does it. Go figure.
}
#pragma warning (pop)

BW_END_NAMESPACE

#else // BW_USE_RDTSC

#ifdef _XBOX360
#include <xtl.h>
#else // _XBOX360
#include "cstdmf_windows.hpp"
#endif // _XBOX360

BW_BEGIN_NAMESPACE

inline uint64 timestamp()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter( &counter );
	return counter.QuadPart;
}

BW_END_NAMESPACE

#endif // BW_USE_RDTSC

#elif defined( PLAYSTATION3 )

BW_BEGIN_NAMESPACE

inline uint64 timestamp()
{
	uint64 ts;
	SYS_TIMEBASE_GET( ts );
	return ts;
}

BW_END_NAMESPACE

#else
	#error Unsupported platform!
#endif

BW_BEGIN_NAMESPACE

CSTDMF_DLL uint64 stampsPerSecond();
CSTDMF_DLL double stampsPerSecondD();

CSTDMF_DLL uint64 stampsPerSecond_rdtsc();
CSTDMF_DLL double stampsPerSecondD_rdtsc();

CSTDMF_DLL uint64 stampsPerSecond_gettimeofday();
CSTDMF_DLL double stampsPerSecondD_gettimeofday();

inline double stampsToSeconds( uint64 stamps )
{
	return double( stamps )/stampsPerSecondD();
}


// -----------------------------------------------------------------------------
// Section: TimeStamp
// -----------------------------------------------------------------------------

/**
 *	This class stores a value in stamps but has access functions in seconds.
 */
class TimeStamp
{
public:
	TimeStamp( uint64 stamps = 0 ) : stamp_( stamps ) {}

	operator uint64 &()				{ return stamp_; }
	operator uint64() const			{ return stamp_; }

	inline double inSeconds() const;
	inline void setInSeconds( double seconds );

	inline TimeStamp ageInStamps() const;
	inline double ageInSeconds() const;

	// Utility methods
	inline static double toSeconds( uint64 stamps );
	inline static TimeStamp fromSeconds( double seconds );

	uint64	stamp_;
};


/**
 *	This static method converts a timestamp value into seconds.
 */
inline double TimeStamp::toSeconds( uint64 stamps )
{
	return double( stamps )/stampsPerSecondD();
}


/**
 *	This static method converts seconds into timestamps.
 */
inline TimeStamp TimeStamp::fromSeconds( double seconds )
{
	return uint64( seconds * stampsPerSecondD() );
}


/**
 *	This method returns this timestamp in seconds.
 */
inline double TimeStamp::inSeconds() const
{
	return toSeconds( stamp_ );
}


/**
 *	This method sets this timestamp from seconds.
 */
inline void TimeStamp::setInSeconds( double seconds )
{
	stamp_ = fromSeconds( seconds );
}


/**
 *	This method returns the number of stamps from this TimeStamp to now.
 */
inline TimeStamp TimeStamp::ageInStamps() const
{
	return timestamp() - stamp_;
}


/**
 *	This method returns the number of seconds from this TimeStamp to now.
 */
inline double TimeStamp::ageInSeconds() const
{
	return toSeconds( this->ageInStamps() );
}

#if ENABLE_WATCHERS

BW_END_NAMESPACE

#include "watcher.hpp"

BW_BEGIN_NAMESPACE

inline
void watcherValueToStream( BinaryOStream & result, const TimeStamp & stamp,
						   const Watcher::Mode & mode )
{
	watcherValueToStream( result, stamp.inSeconds(), mode );
}

inline
bool watcherStreamToValue( BinaryIStream & stream, TimeStamp & value )
{
	float inSeconds;

	if (!watcherStreamToValue( stream, inSeconds ))
	{
		return false;
	}

	value.setInSeconds( inSeconds );

	return true;
}

inline BW::string watcherValueToString( const TimeStamp & value )
{
	return watcherValueToString( value.inSeconds() );
}

inline
bool watcherStringToValue( const char * valueStr, TimeStamp & stamp )
{
	float inSeconds;

	if (!watcherStringToValue( valueStr, inSeconds ))
	{
		return false;
	}

	stamp.setInSeconds( inSeconds );
	return true;
}

#endif // ENABLE_WATCHERS

BW_END_NAMESPACE

#endif // TIMESTAMP_HPP

