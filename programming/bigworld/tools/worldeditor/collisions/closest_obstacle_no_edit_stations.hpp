#ifndef CLOSEST_OBSTACLE_NO_EDIT_STATIONS_HPP
#define CLOSEST_OBSTACLE_NO_EDIT_STATIONS_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "physics2/collision_callback.hpp"

BW_BEGIN_NAMESPACE

class CollisionObstacle;

/**
 *  This callback is used to filter out collisions with 
 *  EditorChunkStationNodes and EditorChunkLinks.
 */
class ClosestObstacleNoEditStations : public CollisionCallback
{   
public:
    virtual int operator()
    ( 
        CollisionObstacle   const &obstacle,
	    WorldTriangle   const &/*triangle*/, 
        float           /*dist*/ 
    ); 

    static ClosestObstacleNoEditStations s_default;
};

BW_END_NAMESPACE

#endif // CLOSEST_OBSTACLE_NO_EDIT_STATIONS_HPP
