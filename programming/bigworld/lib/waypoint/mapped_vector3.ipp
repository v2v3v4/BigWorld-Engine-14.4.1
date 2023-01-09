inline GeoSpaceVector3::GeoSpaceVector3( const MappedVector3& v )
{
	Vector3::operator=( v.asGeoSpace() );
}


inline WorldSpaceVector3::WorldSpaceVector3( const MappedVector3& v )
{
	Vector3::operator=( v.asWorldSpace() );
}


inline MappedVector3::MappedVector3( const Vector3& point, const Chunk* pChunk, Coordinate coordinate )
{
	init( point, pChunk, coordinate );
}


inline MappedVector3::MappedVector3( float x, float y, float z, const Chunk* pChunk, Coordinate coordinate )
{
	init( Vector3( x, y, z ), pChunk, coordinate );
}


inline MappedVector3::MappedVector3( const WorldSpaceVector3& point, const Chunk* pChunk )
	: pChunk_( pChunk )
{
	v_ = GeoSpaceVector3(
			pChunk_->mapping()->invMapper().applyPoint( point ) );
}


inline Vector3 MappedVector3::asChunkSpace() const
{
	Vector3 v = pChunk_->mapping()->mapper().applyPoint( v_ );
	return pChunk_->transformInverse().applyPoint( v );
}


inline void MappedVector3::init( const Vector3& v, const Chunk* pChunk, Coordinate coordinate )
{
	pChunk_ = pChunk;

	if (coordinate == WORLD_SPACE)
	{
		v_ = GeoSpaceVector3(
			pChunk_->mapping()->invMapper().applyPoint( v ) );
	}
	else if (coordinate == CHUNK_SPACE)
	{
		Vector3 temp = pChunk_->transform().applyPoint( v );
		v_ = GeoSpaceVector3(
			pChunk_->mapping()->invMapper().applyPoint( temp ) );
	}
	else
	{
		v_ = GeoSpaceVector3( v );
	}
}
