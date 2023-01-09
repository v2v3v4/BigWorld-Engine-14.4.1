#include "script/first_include.hpp"

#include "cell_profiler.hpp"

#include "entity.hpp"
#include "cell.hpp"


BW_BEGIN_NAMESPACE

CellProfiler::CellProfiler() :
	currSmoothedLoad_( 0 ),
	currRawLoad_( 0 ),
	maxRawLoad_( 0 ),
	accSmoothedLoad_( 0 ),
	accRawLoad_( 0 )
{
}


#if ENABLE_WATCHERS

/**
 * 	This method returns the generic watcher for CellProfiler.
 */
WatcherPtr CellProfiler::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();

		pWatcher->addChild( "load",
							makeWatcher( &CellProfiler::load ) );

		pWatcher->addChild( "rawLoad",
							makeWatcher( &CellProfiler::rawLoad ) );

		pWatcher->addChild( "maxRawLoad",
							makeWatcher( &CellProfiler::maxRawLoad ) );
	}

	return pWatcher;
}

#endif // ENABLE_WATCHERS

#ifndef CODE_INLINE
#include "cell_profiler.ipp"
#endif

BW_END_NAMESPACE

// cell_profiler.cpp
