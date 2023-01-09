#ifndef TERRAIN_TERRAIN_QUAD_TREE_CELL_HPP
#define TERRAIN_TERRAIN_QUAD_TREE_CELL_HPP

#include "math/boundbox.hpp"

BW_BEGIN_NAMESPACE

namespace Terrain
{
    class TerrainCollisionCallback;
    class TerrainHeightMap2;
}

class WorldTriangle;

namespace Terrain
{
    /**
     *  This class is used in TerrainHeightMap2 collisions calculations.
     */
    class TerrainQuadTreeCell
    {
    public:
		TerrainQuadTreeCell();
        ~TerrainQuadTreeCell();

		void init( const TerrainHeightMap2* map,
			uint32 xOffset, uint32 zOffset, uint32 xSize, uint32 zSize,
			float xMin, float zMin, float xMax, float zMax, float minCellSize, 
			TerrainQuadTreeCell** pCellArray, TerrainQuadTreeCell* cellArrayEnd );

		bool collide( const Vector3& source, const Vector3& extent,
			const TerrainHeightMap2* pOwner,
            TerrainCollisionCallback* pCallback ) const;

		bool collide( const WorldTriangle& source, const Vector3& extent,
			const TerrainHeightMap2* pOwner, TerrainCollisionCallback* pCallback
			) const;

    private:
		BoundingBox                         boundingBox_;
		TerrainQuadTreeCell*				children_;
	};
}

BW_END_NAMESPACE

#endif // TERRAIN_TERRAIN_QUAD_TREE_CELL_HPP
