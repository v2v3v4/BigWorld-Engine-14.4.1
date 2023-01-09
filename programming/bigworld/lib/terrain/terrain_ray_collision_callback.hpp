#ifndef TERRAIN_RAY_COLLISION_CALLBACK_HPP
#define TERRAIN_RAY_COLLISION_CALLBACK_HPP

#include "terrain_collision_callback.hpp"

#include "math/vector3.hpp"
#include "math/matrix.hpp"

BW_BEGIN_NAMESPACE

class CollisionState;
class SceneObject;

namespace Terrain
{

class BaseTerrainBlock;

class TerrainRayCollisionCallback : public Terrain::TerrainCollisionCallback
{
public:
	TerrainRayCollisionCallback( CollisionState& cs,
		const Matrix& transform,
		const Matrix& inverseTransform,
		const Terrain::BaseTerrainBlock& block_,
		const SceneObject & sceneObject,
		const Vector3& start, const Vector3& end );

	~TerrainRayCollisionCallback();


	virtual bool collide( const WorldTriangle& triangle, float dist );

	bool finishedColliding() const;

private:
	CollisionState& cs_;
	const Terrain::BaseTerrainBlock& block_;
	const SceneObject & sceneObject_;

	Matrix transform_;
	Matrix inverseTransform_;
	Vector3 start_;
	Vector3 end_;
	Vector3 dir_;
	float dist_;

	bool finishedColliding_;
};

}

BW_END_NAMESPACE

#endif // TERRAIN_RAY_COLLISION_CALLBACK_HPP
