#ifndef COMPILED_SPACE_COLLISION_HELPERS_HPP
#define COMPILED_SPACE_COLLISION_HELPERS_HPP

#include "physics2/collision_interface.hpp"

BW_BEGIN_NAMESPACE

	class Vector3;
	class WorldTriangle;
	class CollisionObstacle;
	class BSPTree;

BW_END_NAMESPACE

namespace BW {

class BSPCollisionInterface : public CollisionInterface
{
public:
	BSPCollisionInterface( const CollisionObstacle& ob,
		const BSPTree& bsp );

	virtual bool collide( const Vector3 & source,
		const Vector3 & extent,
		CollisionState & state ) const;

	virtual bool collide( const WorldTriangle & source,
		const Vector3 & extent,
		CollisionState & state ) const;

private:
	const CollisionObstacle& ob_;
	const BSPTree& bsp_;
};

} // namespace BW

#endif // COMPILED_SPACE_COLLISION_HELPERS_HPP
