#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE

INLINE void Tool::size( float s )
{
	size_ = s;
}


INLINE float Tool::size() const
{
	return size_;
}


INLINE void Tool::strength( float s )
{
	strength_ = s;
}


INLINE float Tool::strength() const
{
	return strength_;
}


INLINE ChunkPtrVector& Tool::relevantChunks()
{
	return relevantChunks_;
}


INLINE const ChunkPtrVector& Tool::relevantChunks() const
{
	return relevantChunks_;
}


INLINE ChunkPtr& Tool::currentChunk()
{
	return currentChunk_;
}


INLINE const ChunkPtr& Tool::currentChunk() const
{
	return currentChunk_;
}

INLINE bool Tool::applying() const
{
	return functor_.exists() && functor_->applying();
}

BW_END_NAMESPACE

