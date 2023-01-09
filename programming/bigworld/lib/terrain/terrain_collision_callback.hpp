#ifndef TERRAIN_TERRAIN_COLLISION_CALLBACK_HPP
#define TERRAIN_TERRAIN_COLLISION_CALLBACK_HPP

#include "cstdmf/bw_namespace.hpp"
#include "chunk/chunk_obstacle.hpp"


BW_BEGIN_NAMESPACE

class WorldTriangle;

namespace Terrain
{
    /**
     *  This interface provides a callback for collisions with terrain.
     */
    class TerrainCollisionCallback
    {
    public:
		CollisionCallback &ccb_;
		TerrainCollisionCallback(CollisionCallback &ccb)
			: ccb_(ccb)
		{ }

        /**
         *  This is the TerrainCollisionCallback destructor.
         */
        virtual ~TerrainCollisionCallback();

        /**
         *  This is the callback for terrain collisions.
         *
         *  @param triangle     The impacted triangle.
         *  @param tValue       The t-value of collision (e.g. t-value along
         *                      a line-segment or prism.
         *  @returns            True if the collision was accepted, false if
         *                      more collisions are required.
         */
        virtual bool 
        collide
        (
            WorldTriangle       const &triangle,
            float               tValue
        ) = 0;
    };
}

BW_END_NAMESPACE

#endif // TERRAIN_TERRAIN_COLLISION_CALLBACK_HPP
