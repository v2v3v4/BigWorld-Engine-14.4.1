#ifndef COLLISION_SCENE_VIEW_HPP
#define COLLISION_SCENE_VIEW_HPP

#include "math/forward_declarations.hpp"
#include "forward_declarations.hpp"
#include "scene_provider.hpp"
#include "scene_type_system.hpp"

namespace BW
{

class CollisionCallback;
class CollisionState;
class WorldTriangle;
class SweepParams;

class ICollisionSceneViewProvider
{
public:

	virtual bool collide( const Vector3 & source,
		const Vector3 & extent,
		const SweepParams& sp,
		CollisionState & state ) const = 0;

	virtual bool collide( const WorldTriangle & source,
		const Vector3 & extent,
		const SweepParams& sp,
		CollisionState & state ) const = 0;
};


class CollisionSceneView : public 
	SceneView<ICollisionSceneViewProvider>
{
public:

	float collide( const Vector3 & start, const Vector3 & end,
		CollisionCallback & cc ) const;
	float collide( const WorldTriangle & start, const Vector3 & end,
		CollisionCallback & cc ) const;
};


} // namespace BW

#endif
