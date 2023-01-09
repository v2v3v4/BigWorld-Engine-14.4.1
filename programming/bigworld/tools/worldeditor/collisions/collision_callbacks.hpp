#ifndef COLLISION_CALLBACKS_HPP
#define COLLISION_CALLBACKS_HPP

#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "physics2/collision_callback.hpp"
#include "physics2/worldtri.hpp"

BW_BEGIN_NAMESPACE

class CollisionObstacle;

/**
 *	This class finds the first collision with a forward
 *	facing polygon
 */
class ObstacleLockCollisionCallback : public CollisionCallback
{
public:
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist );

	virtual void clear();

	float dist_;
	Vector3 normal_;
	ChunkItemPtr pItem_;
	Vector3 triangleNormal_;
	static ObstacleLockCollisionCallback s_default;
};

BW_END_NAMESPACE

#endif // COLLISION_CALLBACKS_HPP
