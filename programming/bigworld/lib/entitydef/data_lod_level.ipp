#ifdef CODE_INLINE
#define INLINE    inline
#else
/// INLINE macro.
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: DataLoDLevels
// -----------------------------------------------------------------------------

/**
 *	This method returns the number of LoD levels.
 *
 *	@note A "Level of Detail" refers to the range and not the step. That is,
 *		if an entity type specifies two detail level steps at 20 metres and
 *		100 metres, say, it has 3 levels. [0, 20), [20, 100), [100, ...).
 */
BWENTITY_API INLINE int DataLoDLevels::size() const
{
	return size_;
}


/**
 *	This methods returns whether or not the input priority threshold needs more
 *	detail than the input level.
 */
INLINE bool DataLoDLevels::needsMoreDetail( int level, float priority ) const
{
	return priority < level_[ level ].low();
}


/**
 *	This methods returns whether or not the input priority threshold needs less
 *	detail than the input level.
 */
INLINE bool DataLoDLevels::needsLessDetail( int level, float priority ) const
{
	return priority > level_[ level ].high();
}

// data_lod_level.ipp
