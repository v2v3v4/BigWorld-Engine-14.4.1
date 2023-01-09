#ifdef CODE_INLINE
    #define INLINE    inline
#else
	/// INLINE macro.
    #define INLINE
#endif

INLINE void EntityTypeProfiler::tick()
{
	currSmoothedLoad_ = accSmoothedLoad_;
	currRawLoad_ = accRawLoad_;
	currAddedLoad_ = accAddedLoad_;
	numEntities_ = accEntityCount_;

	if (maxRawLoad_ < currRawLoad_)
	{
		maxRawLoad_ = currRawLoad_;
	}

	accSmoothedLoad_ = 0;
	accRawLoad_ = 0;
	accEntityCount_ = 0;
	accAddedLoad_ = 0;
}


INLINE void EntityTypeProfiler::addEntityLoad( float entitySmoothedLoad,
												float entityRawLoad,
												float addedLoad )
{
	accSmoothedLoad_ += entitySmoothedLoad;
	accRawLoad_ += entityRawLoad;
	accAddedLoad_ += addedLoad;
	accEntityCount_++;
}

// entity_type_profiler.ipp




