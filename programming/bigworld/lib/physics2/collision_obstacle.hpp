#ifndef COLLISION_OBSTACLE_HPP
#define COLLISION_OBSTACLE_HPP

#include "cstdmf/bw_namespace.hpp"
#include "math/vector3.hpp"
#include "math/matrix.hpp"
#include "scene/scene_object.hpp"

namespace BW
{
	class SceneObject;
}

BW_BEGIN_NAMESPACE

class CollisionObstacle
{
public:
	enum Flags
	{
		FLAG_MOVING_OBSTACLE = (1<<0)
	};

	CollisionObstacle( const Matrix * transform,
		const Matrix * transformInverse,
		const SceneObject & sceneObject,
		uint32 flags = 0) :
			flags_( flags ),
			transform_( *transform ),
			transformInverse_( *transformInverse ),
			sceneObject_( sceneObject )
	{
		MF_ASSERT( transform );
		MF_ASSERT( transformInverse );
	}

	bool isMovingObstacle() const { return (flags_ & FLAG_MOVING_OBSTACLE) ? true : false; }

	const Matrix & transform() const { return transform_; }
	const Matrix & transformInverse() const { return transformInverse_; }
	const SceneObject & sceneObject() const { return sceneObject_; }

private:
	uint32 flags_;
	const Matrix & transform_;
	const Matrix & transformInverse_;
	SceneObject sceneObject_;
};

BW_END_NAMESPACE


#endif // COLLISION_OBSTACLE_HPP
