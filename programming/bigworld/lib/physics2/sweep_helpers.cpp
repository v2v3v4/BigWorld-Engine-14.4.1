#include "pch.hpp"

#include "sweep_helpers.hpp"


BW_BEGIN_NAMESPACE

namespace SweepHelpers
{
/**
 *	This is the visit method of the sweep collision visitor.
 *	It returns true to tell the BSP to stop looking for more intersections.
 *	@param	hitTriangle	the triangle hit by the ray
 *	@param	rd	the relative distance between hit point and source point
 *	@return	true if stop, false if continue searching
 */
bool BSPSweepVisitor::visit( const WorldTriangle & hitTriangle, float rd )
{
	// see how far we really travelled (handles scaling, etc.)
	float ndist = cs_.sTravel_ + (cs_.eTravel_-cs_.sTravel_) * rd;

	if (cs_.onlyLess_ && ndist > cs_.dist_) return true;	// stop now
	if (cs_.onlyMore_ && ndist < cs_.dist_) return false;	// keep looking	

	// call the callback function
	int say = cs_.cc_.visit( ob_, hitTriangle, ndist );

	// record the distance, if the collision callback reported
	// an appropriate triangle.
	if( (say & 3) != 3) cs_.dist_ = ndist;

	// see if any other collisions are wanted
	if (!say)
	{
		stop_ = true;
		return true;
	}

	// some are wanted ... see if it's only one side
	cs_.onlyLess_ = !(say & 2);
	cs_.onlyMore_ = !(say & 1);

	// stop looking in this BSP if we only want closer collisions
	return cs_.onlyLess_;
}

}

BW_END_NAMESPACE
