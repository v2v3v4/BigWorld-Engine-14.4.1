#ifdef CODE_INLINE
    #define INLINE    inline
#else
	/// INLINE macro.
    #define INLINE
#endif


INLINE void CellProfiler::addEntityLoad( float entitySmoothedLoad,
								float entityRawLoad )
{
	accSmoothedLoad_ += entitySmoothedLoad;
	accRawLoad_ += entityRawLoad;
}


INLINE void CellProfiler::tick()
{
	currSmoothedLoad_ = accSmoothedLoad_;
	currRawLoad_ = accRawLoad_;

	if (maxRawLoad_ < currRawLoad_)
	{
		maxRawLoad_ = currRawLoad_;
	}

	accSmoothedLoad_ = 0;
	accRawLoad_ = 0;
}


// cell_profiler.ipp
