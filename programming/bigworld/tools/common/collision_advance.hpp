#ifndef COLLISION_ADVANCE_HPP
#define COLLISION_ADVANCE_HPP

#include "physics2/collision_callback.hpp"

#include "math/planeeq.hpp"
#include "physics2/worldtri.hpp"
#include "chop_poly.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used by preventCollision.
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

	void ignoreFlags( uint8 flags )				{ ignoreFlags_ = flags; }

	bool shouldFindCentre() const				{ return shouldFindCentre_; }
	void shouldFindCentre( bool value )			{ shouldFindCentre_ = value; }

private:
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist );

	const Vector3 origin_;
	const PlaneEq planeEq0_;
	const PlaneEq planeEq1_;
	const PlaneEq planeEq2_;
	const PlaneEq planeEq3_;
	const PlaneEq planeEq4_;
	const Vector3 dir_;
	float advance_;

	WorldTriangle hitTriangle_;

	uint8 ignoreFlags_;

	bool shouldFindCentre_;
};

BW_END_NAMESPACE

#endif
