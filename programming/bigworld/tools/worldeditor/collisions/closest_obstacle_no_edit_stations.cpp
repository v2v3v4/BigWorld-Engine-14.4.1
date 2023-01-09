#include "pch.hpp"
#include "worldeditor/collisions/closest_obstacle_no_edit_stations.hpp"
#include "physics2/collision_obstacle.hpp"
#include "chunk/chunk_item.hpp"

BW_BEGIN_NAMESPACE

ClosestObstacleNoEditStations ClosestObstacleNoEditStations::s_default;


/*virtual*/ int ClosestObstacleNoEditStations::operator()
( 
    CollisionObstacle   const &obstacle,
    WorldTriangle   const &/*triangle*/, 
    float           /*dist*/ 
)
{
	BW_GUARD;

	ChunkItem * pItem = obstacle.sceneObject().isType<ChunkItem>() ?
		obstacle.sceneObject().getAs<ChunkItem>() : NULL;
	MF_ASSERT( pItem );
	if (!pItem)
	{
		return COLLIDE_ALL;
	} 

	if (
		pItem->isEditorChunkStationNode()
		||
		pItem->isEditorChunkLink()
		||
		pItem->isEditorUserDataObject()
		||
		pItem->isEditorEntity()
    )
    {
        return COLLIDE_ALL;
    }
    else
    {
        return COLLIDE_BEFORE;
    }
}   
BW_END_NAMESPACE

