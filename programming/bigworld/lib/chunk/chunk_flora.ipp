
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

INLINE Ecotype::ID ChunkFlora::ecotypeAt( const Vector2& chunkLocalPosition ) const
{
	if (ecotypeIDs_ != NULL)
	{	
		float xf = floorf( chunkLocalPosition.x / spacing_.x );
		float zf = floorf( chunkLocalPosition.y / spacing_.y );		

		uint32 x = uint32(xf+1) % width_;
		uint32 z = uint32(zf+1) % height_;
		uint32 index = x + ( z * width_ );

		return ecotypeIDs_[index];
	}
	else
	{
		return Ecotype::ID_AUTO;
	}
}


INLINE void ChunkFloraManager::chunkToGrid( Chunk* pChunk, int32& gridX, int32& gridZ )
{
	Vector3 chunkPos = pChunk->transform().applyToOrigin();
	const float gridSize = pChunk->space()->gridSize();
	gridX = (int32)floorf(chunkPos.x/gridSize + 0.5f);
	gridZ = (int32)floorf(chunkPos.z/gridSize + 0.5f);
}


INLINE
void ChunkFloraManager::chunkLocalPosition( const Vector3& pos, int32 gridX, 
	int32 gridZ, Vector2& ret )
{
	// TODO: At the moment, assuming the space the camera is in.
	ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
	const float gridSize = pSpace->gridSize();
	ret.x = pos.x - gridX * gridSize;
	ret.y = pos.z - gridZ * gridSize;
}

// chunk_flora.ipp