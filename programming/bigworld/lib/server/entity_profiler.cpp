
#include "entity_profiler.hpp"
#include "entity_type_profiler.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/cstdmf_init.hpp"


BW_BEGIN_NAMESPACE

namespace
{
	const float ARTIFICIAL_LOAD_EPSILON = 1e-6;

	bool isArtificialLoadZero( float load )
	{
		return load < ARTIFICIAL_LOAD_EPSILON;
	}
}


EntityProfiler::EntityProfiler() :
	elapsedTickTime_( 0 ),
	startTime_( 0 ),
	callDepth_( 0 ),
	currSmoothedLoad_( 0.f ),
	currRawLoad_( 0.f ),
	maxRawLoad_( 0.f ),
	currAdjustedLoad_( 0.f ),
	artificialMinLoad_( 0.f )
{
}


/**
 * This method resets the profiler state, brings all loads to 0
 * and disables artificial load.
 */
void EntityProfiler::reset()
{
	currSmoothedLoad_ = 0.f;
	currRawLoad_ = 0.f;
	maxRawLoad_ = 0.f;
	currAdjustedLoad_ = 0.f;
	artificialMinLoad_ = 0.f;
}

#if ENABLE_WATCHERS

/**
 * 	This method returns the generic watcher for EntityProfiler.
 */
WatcherPtr EntityProfiler::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();

		pWatcher->addChild( "load",
							makeWatcher( &EntityProfiler::load ) );

		pWatcher->addChild( "rawLoad",
							makeWatcher( &EntityProfiler::rawLoad ) );

		pWatcher->addChild( "maxRawLoad",
							makeWatcher( &EntityProfiler::maxRawLoad ) );

		pWatcher->addChild( "artificialMinLoad",
							makeWatcher( &EntityProfiler::artificialMinLoad ) );
}

	return pWatcher;
}

#endif // ENABLE_WATCHERS

/**
 * This method should be called every tick
 * to recalculate current smoothed load
 */
void EntityProfiler::tick( uint64 tickDtInStamps,
						float smoothingFactor,
						EntityTypeProfiler &typeProfiler )
{
	// How many ticks it takes smoothed load to get from
	// a' to a" with error <= 1% ( |a" - a'| / 100 )?
	// numTicks = math.ceil( math.log( 0.01, (1 - SMOOTHING_FACTOR) ) )
	//
	// What should SMOOTHING_FACTOR be for smoothed load to get from a' to a"
	// in numTicks with error <= 1% ( |a" - a'| / 100 )?
	// SMOOTHING_FACTOR = 1.0 - math.pow( 0.01, (1.0 / numTicks) )

	float rawLoad = (float)((double)elapsedTickTime_ / (double)tickDtInStamps);

	currSmoothedLoad_ = smoothingFactor * rawLoad + \
						(1.f - smoothingFactor) * \
						currSmoothedLoad_;

	currAdjustedLoad_ = currSmoothedLoad_;

	// Calculate added load and apply min artificial load if necessary
	float addedLoad = 0.f;
	if (artificialMinLoad_ > currSmoothedLoad_)
	{
		addedLoad = artificialMinLoad_ - currSmoothedLoad_;
		currAdjustedLoad_ = artificialMinLoad_;
	}

	currRawLoad_ = rawLoad;

	if (maxRawLoad_ < rawLoad)
	{
		maxRawLoad_ = rawLoad;
	}

	elapsedTickTime_ = 0;

	typeProfiler.addEntityLoad( currAdjustedLoad_, currRawLoad_, addedLoad );
}


/**
 * This method sets artificial load on a profiler's instance.
 * The load returned from load() method is guaranteed to be no less
 * than the artificialMinLoad.
 * Any added on top of the artificialMinLoad load
 * contributes to the calculated load for this process and affects
 * load balancing and the reported load statistics.
 *
 * @param minLoad	minimal artificial load in range of [0, +inf)
 */
void EntityProfiler::artificialMinLoad( float minLoad )
{
	if (!isArtificialLoadZero( minLoad ))
	{
		artificialMinLoad_ = minLoad;
	}
	else
	{
		artificialMinLoad_ = 0.f;
	}
}


BinaryIStream & operator>>( BinaryIStream & data,
							EntityProfiler & profiler )
{
	data >> profiler.currSmoothedLoad_ >> profiler.maxRawLoad_;

	profiler.currAdjustedLoad_ = profiler.currSmoothedLoad_;

	bool hasArtificialLoad;
	data >> hasArtificialLoad;

	if (hasArtificialLoad)
	{
		data >> profiler.artificialMinLoad_;
		if (profiler.artificialMinLoad_ > profiler.currSmoothedLoad_)
		{
			profiler.currAdjustedLoad_ = profiler.artificialMinLoad_;
		}
	}
	else
	{
		profiler.artificialMinLoad_ = 0.f;
	}

	return data;
}


BinaryOStream & operator<<( BinaryOStream & data,
							const EntityProfiler & profiler )
{
	data << profiler.currSmoothedLoad_ << profiler.maxRawLoad_;
	if (isArtificialLoadZero( profiler.artificialMinLoad_ ))
	{
		return data << false;
	}
	else
	{
		return data << true << profiler.artificialMinLoad_;
	}
}


#ifndef CODE_INLINE
#include "entity_profiler.ipp"
#endif

BW_END_NAMESPACE

// entity_profiler.cpp
