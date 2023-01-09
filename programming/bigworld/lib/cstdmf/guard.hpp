#ifndef CSTDBW_GUARD__H
#define CSTDBW_GUARD__H

#include "config.hpp"

#include "bw_namespace.hpp"
#include "stack_tracker.hpp"
#include "profiler.hpp"
#include "slot_tracker.hpp"

BW_BEGIN_NAMESPACE

#if ENABLE_STACK_TRACKER
	#define BW_GUARD		ScopedStackTrack _BW_GUARD(__FUNCTION__, __FILE__, __LINE__)
	#define BW_GUARD_BEGIN	StackTracker::push(__FUNCTION__, __FILE__, __LINE__)
	#define BW_GUARD_END	StackTracker::pop()
	#define BW_CONCATENATE_DIRECT(s1, s2) s1##s2
	#define BW_ANNOTATE_HELPER(s1, s2, s3) ScopedStackTrackAnnotation BW_CONCATENATE_DIRECT(s1,s2)(s3)
	#define BW_ANNOTATE( str )		BW_ANNOTATE_HELPER( _BW_VAR_ANNOTATE_, __LINE__, (str))
#else
	#define BW_GUARD
	#define BW_GUARD_BEGIN
	#define BW_GUARD_END
	#define BW_CONCATENATE_DIRECT(s1, s2)
	#define BW_ANNOTATE_HELPER(s1, s2, s3)
	#define BW_ANNOTATE(str)
#endif // ENABLE_STACK_TRACKER

// Macro for marking intended BW_GUARD removal
#define BW_GUARD_DISABLED

#define BW_GUARD_ANNOTATE( str ) \
	BW_GUARD; \
	BW_ANNOTATE(str)

#define BW_GUARD_PROFILER( id ) \
	BW_GUARD; \
	PROFILER_SCOPED(id)

#define BW_GUARD_MEMTRACKER( id ) \
	BW_GUARD; \
	MEMTRACKER_SCOPED( id )

#define BW_GUARD_PROFILER_MEMTRACKER( id ) \
	BW_GUARD; \
	PROFILER_SCOPED( id ); \
	MEMTRACKER_SCOPED( id )

BW_END_NAMESPACE

#endif // CSTDBW_GUARD__H

