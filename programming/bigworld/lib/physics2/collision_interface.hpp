#ifndef COLLISION_INTERFACE_HPP
#define COLLISION_INTERFACE_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class Vector3;
class WorldTriangle;
class CollisionState;

class CollisionInterface
{
public:
	virtual bool collide( const Vector3 & source, const Vector3 & extent,
		CollisionState & state ) const = 0;

	virtual bool collide( const WorldTriangle & source, const Vector3 & extent,
		CollisionState & state ) const = 0;
};

BW_END_NAMESPACE


#endif // COLLISION_INTERFACE_HPP
