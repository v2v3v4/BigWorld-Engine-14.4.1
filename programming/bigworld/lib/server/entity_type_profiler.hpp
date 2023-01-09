#ifndef ENTITY_TYPE_PROFILER_HPP
#define ENTITY_TYPE_PROFILER_HPP

#include "entity_profiler.hpp"


BW_BEGIN_NAMESPACE

/**
 * This class represents profiling per entity type
 * in BaseApp or CellApp.
 */
class EntityTypeProfiler
{
public:
	EntityTypeProfiler();

	/**
	 * This method should be called once per entity per tick.
	 */
	void addEntityLoad( float entitySmoothedLoad,
						float entityRawLoad,
						float entityAddedLoad );

	/**
	 * This method should be called every tick after
	 * all entity loads have been added.
	 */
	void tick();

	// Section: Accessors
	float load() const { return currSmoothedLoad_; }
	float rawLoad() const { return currRawLoad_; }
	float maxRawLoad() const { return maxRawLoad_; }
	float addedLoad() const { return currAddedLoad_; }
	int numEntities() const { return numEntities_; }

#if ENABLE_WATCHERS
	// Section: static
	static WatcherPtr pWatcher();
#endif // ENABLE_WATCHERS

private:
	float currSmoothedLoad_;
	float currRawLoad_;
	float currAddedLoad_;

	float maxRawLoad_;
	int numEntities_;

	float accSmoothedLoad_;
	float accRawLoad_;
	float accAddedLoad_;
	int accEntityCount_;
};

#ifdef CODE_INLINE
#include "entity_type_profiler.ipp"
#endif

BW_END_NAMESPACE

#endif /* ENTITY_TYPE_PROFILER_HPP */
