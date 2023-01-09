#ifndef MAPPED_VECTOR3_HPP
#define MAPPED_VECTOR3_HPP


#include "chunk/chunk.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"


BW_BEGIN_NAMESPACE

class MappedVector3;


class GeoSpaceVector3 : public Vector3
{
public:
	GeoSpaceVector3(){}
	GeoSpaceVector3( float x, float y, float z )
		: Vector3( x, y, z )
	{}
	GeoSpaceVector3( const MappedVector3& v );
	explicit GeoSpaceVector3( const Vector3& v )
		: Vector3( v )
	{}
};


class WorldSpaceVector3 : public Vector3
{
public:
	WorldSpaceVector3(){}
	WorldSpaceVector3( float x, float y, float z )
		: Vector3( x, y, z )
	{}
	WorldSpaceVector3( const MappedVector3& v );
	explicit WorldSpaceVector3( const Vector3& v )
		: Vector3( v )
	{
	}
};


class MappedVector3
{
public:
	enum Coordinate
	{
		WORLD_SPACE,
		CHUNK_SPACE,
		GEO_SPACE
	};
	MappedVector3( const Vector3& point, const Chunk* pChunk, Coordinate coordinate );
	MappedVector3( float x, float y, float z, const Chunk* pChunk, Coordinate coordinate );
	MappedVector3( const GeoSpaceVector3& point, const Chunk* pChunk )
		: v_( point ), pChunk_( pChunk )
	{}
	MappedVector3( const WorldSpaceVector3& point, const Chunk* pChunk );
	const GeoSpaceVector3& asGeoSpace() const
	{
		return v_;
	}
	WorldSpaceVector3 asWorldSpace() const
	{
		return WorldSpaceVector3( pChunk_->mapping()->mapper().applyPoint( v_ ) );
	}
	Vector3 asChunkSpace() const;
	const Chunk* pChunk() const
	{
		return pChunk_;
	}
private:
	void init( const Vector3& v, const Chunk* pChunk, Coordinate coordinate );
	GeoSpaceVector3 v_;
	const Chunk* pChunk_;
};


#include "mapped_vector3.ipp"

BW_END_NAMESPACE

#endif//MAPPED_VECTOR3_HPP
