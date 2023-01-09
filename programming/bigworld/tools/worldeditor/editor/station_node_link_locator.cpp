#include "pch.hpp"
#include "worldeditor/editor/station_node_link_locator.hpp"
#include "worldeditor/editor/chunk_obstacle_locator.hpp"
#include "worldeditor/collisions/closest_obstacle_no_edit_stations.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "chunk/chunk_item.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "physics2/collision_obstacle.hpp"
#include "gizmo/snap_provider.hpp"
#include <limits>

BW_BEGIN_NAMESPACE

namespace
{
    /**
     *  This is a helper class that finds the closest linkable chunk item.
     */
    class ClosestLinkableCatcher : public CollisionCallback
    {
    public:
        explicit ClosestLinkableCatcher(StationNodeLinkLocator::Type type) : 
            chunkItem_(NULL),
            distance_(std::numeric_limits<float>::max()),
            type_(type)
        {
        }

        /*virtual*/ int operator()
        ( 
            CollisionObstacle   const &obstacle,
		    WorldTriangle   const &/*triangle*/, 
            float           dist
        )
        {
			BW_GUARD;

            if (dist < distance_)
            {
                distance_ = dist;

				ChunkItem * pItem = obstacle.sceneObject().isType<ChunkItem>() ?
					obstacle.sceneObject().getAs<ChunkItem>() : NULL;
				MF_ASSERT( pItem );
				if (!pItem)
				{
					return COLLIDE_ALL;
				}

                EditorChunkItem *item = pItem;
                
                if 
                (
                    (type_ & StationNodeLinkLocator::LOCATE_ENTITIES) != 0
                    &&
                    item->isEditorEntity()
                )
                {
                    EditorChunkEntity *entity = (EditorChunkEntity *)item;
                    int idx = entity->patrolListPropIdx();
                    if (idx != -1)
                    {
                        chunkItem_ = pItem;
                    }
                }
                else if 
                (
                    (type_ & StationNodeLinkLocator::LOCATE_NODES) != 0
                    &&
                    item->isEditorChunkStationNode()
                )
                {
                    chunkItem_ = pItem;
                }           
            }
            return COLLIDE_BEFORE;
        }

        ChunkItemPtr                    chunkItem_;
        float                           distance_;
        StationNodeLinkLocator::Type    type_;
    };
}


/**
 *  StationNodeLinkLocator constructor.
 */
/*explicit*/ StationNodeLinkLocator::StationNodeLinkLocator
(
    Type    type        /*= LOCATE_BOTH*/
) :
    chunkItem_(NULL),
    initialPos_(true),
    type_(type)
{
	BW_GUARD;

    subLocator_ = ToolLocatorPtr(
        new ChunkObstacleToolLocator(ClosestObstacleNoEditStations::s_default),
		PyObjectPtr::STEAL_REFERENCE );
}


/**
 *  StationNodeLinkLocator destrcutor.
 */
/*virtual*/ StationNodeLinkLocator::~StationNodeLinkLocator()
{
}


/**
 *  Calculate the location given a ray through worldray.
 *
 *  @param worldRay     The world ray.
 *  @param tool         The originating tool.
 */
/*virtual*/ void 
StationNodeLinkLocator::calculatePosition
(
    Vector3             const &worldRay, 
    Tool                &tool
)
{
	BW_GUARD;

    chunkItem_ = NULL; // reset item

    // Allow the sub-locator to find the position first:
	subLocator_->calculatePosition(worldRay, tool);
	transform_ = subLocator_->transform();

	Vector3 extent = 
        Moo::rc().invView().applyToOrigin() 
        +
		worldRay*Moo::rc().camera().farPlane();

    ClosestLinkableCatcher clc(type_);

	float distance = 
        ChunkManager::instance().cameraSpace()->collide
        ( 
		    Moo::rc().invView().applyToOrigin(),
		    extent,
		    clc
        );

    chunkItem_ = clc.chunkItem_;
}


/**
 *  Return the selected chunk item.
 */
ChunkItemPtr StationNodeLinkLocator::chunkItem()
{
    return chunkItem_;
}
BW_END_NAMESPACE

