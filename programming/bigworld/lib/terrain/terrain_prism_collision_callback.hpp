#ifndef TERRAIN_PRISM_COLLISION_CALLBACK_HPP
#define TERRAIN_PRISM_COLLISION_CALLBACK_HPP

#include "terrain_collision_callback.hpp"

#include "math/vector3.hpp"
#include "math/matrix.hpp"

#include "physics2/worldtri.hpp"

BW_BEGIN_NAMESPACE

class CollisionState;
class SceneObject;

namespace Terrain
{

class BaseTerrainBlock;

class TerrainPrismCollisionCallback : public Terrain::TerrainCollisionCallback
{
public:
	TerrainPrismCollisionCallback( CollisionState& cs,
		const Matrix& transform,
		const Matrix& inverseTransform,
		const Terrain::BaseTerrainBlock& block_,
		const SceneObject & sceneObject,
		const WorldTriangle& start, const Vector3& end );

	~TerrainPrismCollisionCallback();


	virtual bool collide( const WorldTriangle& triangle, float dist );

private:
	CollisionState& cs_;
	const Terrain::BaseTerrainBlock& block_;
	const SceneObject & sceneObject_;

	Matrix			transform_;
	Matrix			inverseTransform_;
	WorldTriangle	start_;
	Vector3			end_;
	Vector3			dir_;
	float			dist_;
};

}

BW_END_NAMESPACE

#endif // TERRAIN_PRISM_COLLISION_CALLBACK_HPP
