#ifndef CELL_PROFILER_HPP
#define CELL_PROFILER_HPP

#include "server/entity_profiler.hpp"


BW_BEGIN_NAMESPACE

// Section: forward declarations
class Cell;

/**
 * This class is responsible for tracking load on a Cell instance.
 */
class CellProfiler
{
public:
	CellProfiler();

	/**
	 * This method should be called once per entity per tick.
	 */
	void addEntityLoad( float entitySmoothedLoad,
						float entityRawLoad );

	/**
	 * This method should be called every tick after
	 * all entity loads have been added.
	 */
	void tick();

	// Section: Accessors
	float load() const { return currSmoothedLoad_; }
	float rawLoad() const { return currRawLoad_; }
	float maxRawLoad() const { return maxRawLoad_; }

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif // ENABLE_WATCHERS

private:
	float currSmoothedLoad_;
	float currRawLoad_;
	float maxRawLoad_;

	float accSmoothedLoad_;
	float accRawLoad_;
};

#ifdef CODE_INLINE
#include "cell_profiler.ipp"
#endif

BW_END_NAMESPACE

#endif /* CELL_PROFILER_HPP */
