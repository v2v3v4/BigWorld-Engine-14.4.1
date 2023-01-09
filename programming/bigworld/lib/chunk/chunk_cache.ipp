
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

INLINE ChunkCache::ChunkCache()
#ifdef EDITOR_ENABLED
	:invalidateFlag_( InvalidateFlags::FLAG_NONE )
#endif //EDITOR_ENABLE
{
}

INLINE ChunkCache::~ChunkCache()
{
}


/**
 *	Draw the contents of the chunk cache.
 *	Eg. draw navmesh.
 */
INLINE void ChunkCache::draw( Moo::DrawContext& drawContext )
{
}


/**
 *	Focus the chunk.
 *	@return focusCount
 */
INLINE int ChunkCache::focus()
{
	return 0;
}


/**
 *	Bind or unbind chunk.
 *	@param isUnbind true if we are unbinding.
 */
INLINE void ChunkCache::bind( bool isUnbind )
{
}	


/**
 *	Load the chunk.
 *	@param chunk the chunk to load.
 *	@param cdata
 *	@return true on success.
 */
INLINE bool ChunkCache::load( DataSectionPtr chunk, DataSectionPtr cdata )
{
	return true;
}


/**
 *	Get the chunk cache's identifier.
 *	@return name of the type of chunk cache.
 */
INLINE const char* ChunkCache::id() const
{
	return "ChunkCache";
}


#ifdef EDITOR_ENABLED
/**
 *	Save the cache's information for a chunk.
 *	@param chunk the chunk to save our info into.
 */
INLINE void ChunkCache::saveChunk( DataSectionPtr chunk )
{
}


/**
 *	Save the cache's cdata information for a chunk. 
 *	@param cdata the datasection to save our info into.
 */
INLINE void ChunkCache::saveCData( DataSectionPtr cdata )
{
}

/**
 *	Allows the cache to read their dirty state from ChunkCleanFlags. 
 *	@param cf The ChunkCleanFlags to read from
 */
INLINE void ChunkCache::loadCleanFlags( const ChunkCleanFlags& cf )
{
}

/**
 *	Allows the cache to write their dirty state into ChunkCleanFlags.
 *	@param cf The ChunkCleanFlags to write to
 */
INLINE void ChunkCache::saveCleanFlags( ChunkCleanFlags& cf )
{
}

/**
 *	Check that this chunk cache can be processed now.
 *	Checks if dirty, readyToCalculate (ie. loaded) and !isBeingCalculated.
 *	@param mgr this chunk cache's manager.
 *	@return true when ready.
 */
INLINE bool ChunkCache::canCalculateNow( ChunkProcessorManager* mgr )
{
	return this->dirty() &&
		this->readyToCalculate( mgr ) &&
		!this->isBeingCalculated();
}

/**
 *	Check that the manager is ready to process this chunk.
 *	For example, if it still needs to be loaded or its neighbouring chunks
 *	need to be loaded then this will return false.
 *	@param mgr this chunk cache's manager.
 *	@return true when ready.
 */
INLINE bool ChunkCache::readyToCalculate( ChunkProcessorManager* mgr )
{
	return true;
}


/**
 *	Load the chunk ready for calculation.
 *	@param mgr manager
 *	@return true on success.
 */
INLINE bool ChunkCache::loadChunkForCalculate( ChunkProcessorManager* mgr )
{
	return true;
}


/**
 *	Invalidate to recalculate data.
 *	@param mgr				manager
 *	@param spread			spread
 *	@param pChangedItem		the changed item that caused the chunk changed,
 *								NULL means it's other reason caused the chunk changed
 *	@return true on success.
 */
INLINE bool ChunkCache::invalidate( 
				ChunkProcessorManager* mgr,
				bool spread,
				EditorChunkItem* pChangedItem/* = NULL*/ )
{
	return false;
}


/**
 *	Check if this cache requires processing in the main thread.
 *	@return true if this cache requires processing in the main thread.
 */
INLINE bool ChunkCache::requireProcessingInMainThread() const
{
	return false;
}


/**
 *	Check if this cache requires processing in the main thread.
 *	@return true if this cache requires processing in the main thread.
 */
INLINE bool ChunkCache::requireProcessingInBackground() const
{
	return false;
}


/**
 *	Check if this cache requires processing.
 *	@return true if this cache requires processing.
 */
INLINE bool ChunkCache::dirty()
{
	return false;
}


/**
 *	Check if this cache is currently  being processed.
 *	@return true if this cache is currently  being processed.
 */
INLINE bool ChunkCache::isBeingCalculated() const
{
	return false;
}


/**
 *	Process this cache.
 *	@param mgr manager.
 *	@param unsaved unsaved list.
 *	@return true on success.
 */
INLINE bool ChunkCache::recalc(
	ChunkProcessorManager* mgr,
	UnsavedList& unsaved )
{
	return true;
}


#endif //EDITOR_ENABLE


/**
 *	Chunk touching this cache type.
 *	@param chunk
 */
INLINE void ChunkCache::touch( Chunk& chunk )
{
}


/**
 *	Get a cache id.
 *	@return cache id.
 */
INLINE int ChunkCache::cacheNum()
{
	return s_nextCacheIndex_;
}

