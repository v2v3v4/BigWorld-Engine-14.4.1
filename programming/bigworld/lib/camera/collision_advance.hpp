#ifndef COLLISON_ADVANCE_HPP
#define COLLISON_ADVANCE_HPP


#include "chunk/chunk_obstacle.hpp"
#include "math/planeeq.hpp"
#include "physics2/worldtri.hpp"
#include "scene/scene_object.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This is a helper class is used with preventCollision.
 */
class CollisionAdvance : public CollisionCallback
{
public:
	CollisionAdvance( const Vector3 & origin,
			const Vector3 & axis1, const Vector3 & axis2,
			const Vector3 & direction,
			float maxAdvance );
	~CollisionAdvance();

	float advance() const { return max( 0.f, advance_); }

	const WorldTriangle & hitTriangle() const	{ return hitTriangle_; }
	const SceneObject & hitObject() const		{ return hitObject_; }

	void ignoreFlags( uint8 flags )				{ ignoreFlags_ = flags; }

	void ignoreTerrain() { ignoreTerrain_ = true; }
	void ignoreObjects() { ignoreObjects_ = true; }

private:
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist );

	PlaneEq planeEq0_;
	PlaneEq planeEq1_;
	PlaneEq planeEq2_;
	PlaneEq planeEq3_;
	PlaneEq planeEqBase_;
	const Vector3 dir_;
	float advance_;
	float adjust_;

	WorldTriangle		hitTriangle_;
	SceneObject			hitObject_;

	uint8 ignoreFlags_;
	bool ignoreTerrain_;
	bool ignoreObjects_;
};

BW_END_NAMESPACE


#endif // COLLISON_ADVANCE_HPP
