
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


INLINE Ecotype* Flora::ecotype( int nearOrFar, float x, float z )
{
	int32 t = floraType( x, z );
	int idx = (nearOrFar==0) ? mappings_[t].nearSet_ : mappings_[t].farSet_;

	return &ecotypes_[nearOrFar][idx];
}


INLINE int32 Flora::floraType( float x, float z )
{
	Vector3 pos( x, 0.f, z );
	Moo::TerrainBlock::FindDetails details = 
		Moo::TerrainBlock::findOutsideBlock( pos );

	if (details.pBlock_)
	{
		Vector3 relPos = details.pInvMatrix_->applyPoint( pos );

		// offset position for rounding errors -
		// all terrain block functions use floorf() based on the spacing size.
		const float round = details.pBlock_->spacing() / 2.f;

		return details.pBlock_->detailID( relPos.x + round, relPos.z + round );
	}

	return 0;
}


// flora.ipp
