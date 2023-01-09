#ifdef CODE_INLINE
    #define INLINE    inline
#else
	/// INLINE macro.
    #define INLINE
#endif


INLINE void EntityProfiler::start() const
{
	if (callDepth_++ > 0)
	{
		// we're already profiling
		return;
	}
	startTime_ = timestamp();

}


INLINE void EntityProfiler::stop() const
{
	MF_ASSERT( callDepth_ > 0 );
	if (--callDepth_ > 0)
	{
		// we were nested inside another start/stop
		return;
	}
	uint64 dt = timestamp() - startTime_;
	elapsedTickTime_ += dt;
	startTime_ = 0;
}

// entity_profiler.ipp
