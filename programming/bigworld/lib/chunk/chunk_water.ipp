#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE


INLINE Chunk * ChunkWater::chunk() const
{
	return pChunk_;
}


INLINE void ChunkWater::chunk( Chunk * pChunk )
{
	pChunk_ = pChunk;
}


BW_END_NAMESPACE
