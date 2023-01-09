
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: Accessors
// -----------------------------------------------------------------------------

/**
 *	This static method returns the singleton instance of this class.
 */
INLINE EngineStatistics & EngineStatistics::instance()
{
	return EngineStatistics::instance_;
}

// engine_statistics.ipp
