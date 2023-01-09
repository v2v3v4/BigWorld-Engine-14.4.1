#ifndef STAT_WATCHER_CREATOR_HPP
#define STAT_WATCHER_CREATOR_HPP

#include "cstdmf/config.hpp"

#if ENABLE_WATCHERS

#include "stat_with_rates_of_change.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class Watcher;
typedef SmartPointer< Watcher > WatcherPtr;

typedef StatWithRatesOfChange< unsigned int > UintStatWithRatesOfChange;
typedef IntrusiveStatWithRatesOfChange< unsigned int >::Container UintStatWithRatesOfChangeContainer;

namespace StatWatcherCreator
{
	void initRatesOfChangeForStats(
			const UintStatWithRatesOfChangeContainer & stats );

	void initRatesOfChangeForStat( UintStatWithRatesOfChange & stat );

	void addWatchers( WatcherPtr pWatcher,
			const char * name, UintStatWithRatesOfChange & stat );

} // namespace StatWatcherCreator

BW_END_NAMESPACE

#endif // ENABLE_WATCHERS

#endif // STAT_WATCHER_CREATOR_HPP
