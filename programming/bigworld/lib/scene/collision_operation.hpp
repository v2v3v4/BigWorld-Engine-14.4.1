#ifndef COLLISION_OPERATION_HPP
#define COLLISION_OPERATION_HPP

#include "forward_declarations.hpp"

#include "scene_type_system.hpp"
#include "scene_object.hpp"

BW_BEGIN_NAMESPACE

	class WorldTriangle;
	class Vector3;
	class SweepParams;
	class CollisionState;

BW_END_NAMESPACE

namespace BW
{

class ICollisionTypeHandler
{
public:
	virtual ~ICollisionTypeHandler(){}

	virtual bool doCollide( const SceneObject & object,
		const Vector3 & source,
		const Vector3 & extent,
		const SweepParams& sp,
		CollisionState & state ) const = 0;

	virtual bool doCollide( const SceneObject & object,
		const WorldTriangle & source,
		const Vector3 & extent,
		const SweepParams& sp,
		CollisionState & state ) const = 0;
};


class CollisionOperation
	: public SceneObjectOperation<ICollisionTypeHandler>
{
public:
	bool collide( 
		const SceneObject & object,
		const Vector3 & source,
		const Vector3 & extent,
		const SweepParams& sp,
		CollisionState & state ) const;

	bool collide( 
		const SceneObject & object,
		const WorldTriangle & source,
		const Vector3 & extent,
		const SweepParams& sp,
		CollisionState & state ) const;
};

} // namespace BW

#endif // _DRAW_OPERATION_HPP
