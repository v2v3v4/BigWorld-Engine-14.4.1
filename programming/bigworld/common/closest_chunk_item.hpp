#ifndef CLOSEST_CHUNK_ITEM_HPP
#define CLOSEST_CHUNK_ITEM_HPP

#include "closest_triangle.hpp"

#include "chunk/chunk_item.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_vlo.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This is the base class for ChunkItem collision callbacks.
 */
class ClosestChunkItem : public ClosestTriangle
{
public:
	ClosestChunkItem() :
		ClosestTriangle(),
		pChunkItem_( NULL )
	{
	}

	ChunkItem * pChunkItem() const	{ return pChunkItem_; }

	virtual int doVisit( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float /*dist*/ )
	{
		if (!(triangle.flags() & this->triangleFlags()))
		{
			return COLLIDE_ALL;
		}

		this->triangle( obstacle, triangle );

		pChunkItem_ = obstacle.sceneObject().isType< ChunkItem >() ?
			obstacle.sceneObject().getAs< ChunkItem >() : NULL;
		MF_ASSERT( pChunkItem_ );

		return COLLIDE_BEFORE;
	}

protected:
	ChunkItem * pChunkItem_;

private:
	virtual WorldTriangle::Flags triangleFlags() const = 0;
};


/**
 *	This class finds the closest water triangle.
 */
class ClosestWater : public ClosestChunkItem
{
public:
	VeryLargeObject * pWater() const
	{
		ChunkVLO * pVLO = pChunkItem_ ? (ChunkVLO*) pChunkItem_ : NULL;
		return pVLO ? pVLO->object().get() : NULL;
	}

private:
	virtual WorldTriangle::Flags triangleFlags() const
	{
		return TRIANGLE_WATER;
	}
};


/**
 *	This class finds the closest terrain triangle.
 */
class ClosestTerrain : public ClosestChunkItem
{
private:
	virtual WorldTriangle::Flags triangleFlags() const
	{
		return TRIANGLE_TERRAIN;
	}
};

BW_END_NAMESPACE

#endif
