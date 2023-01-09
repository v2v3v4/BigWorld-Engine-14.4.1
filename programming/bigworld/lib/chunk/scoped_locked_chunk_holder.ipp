
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE

INLINE ChunkSet::iterator ScopedLockedChunkHolder::begin()
{
	return chunks_.begin();
}

INLINE ChunkSet::const_iterator ScopedLockedChunkHolder::begin() const
{
	return chunks_.begin();
}

INLINE ChunkSet::iterator ScopedLockedChunkHolder::end()
{
	return chunks_.end();
}
INLINE ChunkSet::const_iterator ScopedLockedChunkHolder::end() const
{
	return chunks_.end();
}

INLINE ChunkSet::reverse_iterator ScopedLockedChunkHolder::rbegin()
{
	return chunks_.rbegin();
}

INLINE ChunkSet::const_reverse_iterator ScopedLockedChunkHolder::rbegin() const
{
	return chunks_.rbegin();
}

INLINE ChunkSet::reverse_iterator ScopedLockedChunkHolder::rend()
{
	return chunks_.rend();
}

INLINE ChunkSet::const_reverse_iterator ScopedLockedChunkHolder::rend() const
{
	return chunks_.rend();
}

INLINE bool ScopedLockedChunkHolder::empty() const
{
	return chunks_.empty();
}

INLINE ChunkSet::size_type ScopedLockedChunkHolder::size() const
{
	return chunks_.size();
}

INLINE ChunkSet::size_type ScopedLockedChunkHolder::max_size() const
{
	return chunks_.max_size();
}

INLINE ChunkSet::iterator ScopedLockedChunkHolder::find( Chunk* pChunk )
{
	return chunks_.find( pChunk );
}

INLINE ChunkSet::const_iterator ScopedLockedChunkHolder::find(
	Chunk* pChunk ) const
{
	return chunks_.find( pChunk );
}

INLINE ChunkSet::size_type ScopedLockedChunkHolder::count( Chunk* pChunk ) const
{
	return chunks_.count( pChunk );
}

BW_END_NAMESPACE

