
#include "entity_type_profiler.hpp"


BW_BEGIN_NAMESPACE

EntityTypeProfiler::EntityTypeProfiler() :
	currSmoothedLoad_( 0 ),
	currRawLoad_( 0 ),
	currAddedLoad_( 0 ),
	maxRawLoad_( 0 ),
	numEntities_( 0 ),
	accSmoothedLoad_( 0 ),
	accRawLoad_( 0 ),
	accAddedLoad_( 0 ),
	accEntityCount_( 0 )
{
}

#if ENABLE_WATCHERS

/**
 * 	This method returns the generic watcher for EntityTypeProfiler.
 */
WatcherPtr EntityTypeProfiler::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();

		pWatcher->addChild( "load",
							makeWatcher( &EntityTypeProfiler::load ) );

		pWatcher->addChild( "rawLoad",
							makeWatcher( &EntityTypeProfiler::rawLoad ) );

		pWatcher->addChild( "maxRawLoad",
							makeWatcher( &EntityTypeProfiler::maxRawLoad ) );

		pWatcher->addChild( "numEntities",
							makeWatcher( &EntityTypeProfiler::numEntities ) );
	}

	return pWatcher;
}

#endif // ENABLE_WATCHERS


#ifndef CODE_INLINE
#include "entity_type_profiler.ipp"
#endif

BW_END_NAMESPACE

// entity_type_profiler.cpp
