// chunk_obstacle.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


/**
 *	This method returns the chunk item that caused the creation of this obstacle
 */
INLINE ChunkItem* ChunkObstacle::pItem() const
{
	return pItem_.get();
}

/**
 *	This method returns the chunk that this obstacle is in.
 */
INLINE Chunk * ChunkObstacle::pChunk() const
{
	return pItem_->chunk();
}


/**
 *	This method returns BSPTree.
 */
INLINE BSPTree * ChunkBSPObstacle::bspTree()
{
#if ENABLE_RELOAD_MODEL
	if (bspTreeModel_)
	{
		return (BSPTree *)bspTreeModel_->decompose();
	}
	else
#endif
	{
		return bspTree_;
	}
}

// chunk_obstacle.ipp
