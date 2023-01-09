#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

INLINE const ChunkSpace::GeometryMappings & ChunkSpace::getMappings()
{
	// Non-locked reads of mappings_ are only allowed from the Main Thread
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

	return mappings_; 
}


INLINE bool ChunkSpace::isMapped() const 
{ 
	// Non-locked reads of mappings_ are only allowed from the Main Thread
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

	return mappings_.size() > 0; 
}


/**
 *  This method checks if there is a pending mapping operation.
 */
INLINE bool ChunkSpace::isMappingPending() const
{
	return !backgroundTasks_.empty();
}

// chunk_space.ipp
