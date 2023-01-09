#include "pch.hpp"

#include "collision_helpers.hpp"

#include "physics2/sweep_helpers.hpp"
#include "physics2/bsp.hpp"


namespace BW {


BSPCollisionInterface::BSPCollisionInterface(
	const CollisionObstacle& ob,
	const BSPTree& bsp ) :
		ob_( ob ),
		bsp_( bsp )
{
}


bool BSPCollisionInterface::collide( const Vector3 & source,
	const Vector3 & extent,
	CollisionState & state ) const
{
	SweepHelpers::BSPSweepVisitor visitor( ob_, state );

	float rd = 1.0f;
	bsp_.intersects( source, extent, rd, NULL, &visitor );
	return visitor.shouldStop(); 
}


bool BSPCollisionInterface::collide( const WorldTriangle & source,
	const Vector3 & extent,
	CollisionState & state ) const
{
	SweepHelpers::BSPSweepVisitor visitor( ob_, state );

	bsp_.intersects( source, extent - source.v0(), &visitor );
	return visitor.shouldStop(); 
}


} // namespace BW