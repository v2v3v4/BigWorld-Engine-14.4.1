#ifdef CODE_INLINE
#define INLINE    inline
#else
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: Cell
// -----------------------------------------------------------------------------

/**
 *	This method returns the number of real entities on this cell.
 */
INLINE
int Cell::numRealEntities() const
{
	return realEntities_.size();
}


/**
 *	This method returns a map of the real entities on this cell. The map is
 *	keyed by their IDs.
 */
INLINE
Cell::Entities & Cell::realEntities()
{
	return realEntities_;
}

// cell.ipp
