#ifndef COLLISION_TERRAIN_ONLY_HPP
#define COLLISION_TERRAIN_ONLY_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "chunk/chunk_obstacle.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This callback is used to filter out collisions with all objects expect the
 *	terrain.
 */
class CollisionTerrainOnly : public CollisionCallback
{   
public:
    virtual int operator()(
		const CollisionObstacle& /*obstacle*/, const WorldTriangle& triangle,
		float /*dist*/ ); 

    static CollisionTerrainOnly s_default;
};

BW_END_NAMESPACE

#endif // COLLISION_TERRAIN_ONLY_HPP
