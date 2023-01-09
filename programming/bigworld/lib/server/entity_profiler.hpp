#ifndef ENTITY_PROFILER_H_
#define ENTITY_PROFILER_H_

#include "cstdmf/stdmf.hpp"
#include "cstdmf/timestamp.hpp"

#define ENABLE_ENTITY_PROFILER


#ifdef ENABLE_ENTITY_PROFILER


#define AUTO_SCOPED_ENTITY_PROFILE( ENTITY_PTR )							\
	EntityProfiler::AutoScopedHelper< BaseOrEntity >						\
		_autoEntityProfile( ENTITY_PTR )

#define AUTO_SCOPED_THIS_ENTITY_PROFILE										\
			AUTO_SCOPED_ENTITY_PROFILE( this )

#else // !ENABLE_ENTITY_PROFILER

#define AUTO_SCOPED_ENTITY_PROFILE( pEntity )
#define AUTO_SCOPED_THIS_ENTITY_PROFILE

#endif // ENABLE_ENTITY_PROFILER


BW_BEGIN_NAMESPACE

// Section: Forward declarations

class EntityTypeProfiler;

class BinaryIStream;
class BinaryOStream;


/**
 * This class is used for per entity profiling.
 * It can be a part of base and cell entities.
 */
class EntityProfiler
{
public:
	EntityProfiler();
	~EntityProfiler() { callDepth_ = 0; }

	void reset();

	void start() const;
	void stop() const;

	void tick( uint64 tickDtInStamps,
			float smoothingFactor,
			EntityTypeProfiler &typeProfiler );

	// Section: Accessors
	float load() const { return currAdjustedLoad_; }
	float rawLoad() const { return currRawLoad_; }
	float maxRawLoad() const { return maxRawLoad_; }
	float artificialMinLoad() const { return artificialMinLoad_; }

	// Section: Setters
	void artificialMinLoad( float minLoad );

	/**
	 * This class is a helper for auto-scoped profiling macros
	 */
	template < class ENTITY >
	class AutoScopedHelper
	{
	public:
		AutoScopedHelper( const ENTITY * pEntity ) : pEntity_()
		{
			if (pEntity)
			{
				pEntity_ = pEntity;
				pEntity->profiler().start();
			}
		}

		~AutoScopedHelper()
		{
			if (pEntity_)
			{
				pEntity_->profiler().stop();
			}
		}

	private:
		ConstSmartPointer< ENTITY > pEntity_;
	};

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif // ENABLE_WATCHERS

protected:
	mutable uint64 elapsedTickTime_; /* time spent in the profiling
								since the beginning of the tick */
	mutable uint64 startTime_; /* when start() was called */
	mutable uint32 callDepth_; /* the depth of nested starts and stops */

	// Variables used to calculate the load
	// We use exponential smoothing here, see
	// http://en.wikipedia.org/wiki/Exponential_smoothing
	float currSmoothedLoad_;
	float currRawLoad_;
	float maxRawLoad_;

	// smoothed load after applying artificial load
	float currAdjustedLoad_;

	// artificial load overrides calculated load
	float artificialMinLoad_;

	friend BinaryIStream & operator>>( BinaryIStream & data,
										EntityProfiler & profiler );
	friend BinaryOStream & operator<<( BinaryOStream & data,
										const EntityProfiler & profiler );

};

BinaryIStream & operator>>( BinaryIStream & data,
							EntityProfiler & profiler );
BinaryOStream & operator<<( BinaryOStream & data,
							const EntityProfiler & profiler );

#ifdef CODE_INLINE
#include "entity_profiler.ipp"
#endif

BW_END_NAMESPACE

#endif /* ENTITY_PROFILER_H_ */
