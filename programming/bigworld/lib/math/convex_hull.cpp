#include "pch.hpp"
#include "convex_hull.hpp"

#include "vector3.hpp"
#include "vector4.hpp"
#include "matrix.hpp"
#include "boundbox.hpp"
#include "planeeq.hpp"

namespace BW
{

ConvexHull::ConvexHull( size_t numPlanes, PlaneEq * planes) :
	planes_( planes ),
	numPlanes_(numPlanes)
{
	MF_ASSERT( numPlanes <= 32 );
}


size_t ConvexHull::size() const 
{
	return numPlanes_;
}


const PlaneEq & ConvexHull::plane( size_t index ) const 
{ 
	MF_ASSERT( index < numPlanes_ );
	return planes_[index]; 
}


bool ConvexHull::intersects( const Vector3 & center, float radius ) const
{
	uint16 allPlanes = ~0;
	return intersects( center, radius, allPlanes );
}


bool ConvexHull::intersects( const AABB & bb ) const
{
	uint16 allPlanes = ~0;
	return intersects( bb, allPlanes );
}


bool ConvexHull::intersects( const Vector3 & center, float radius,
	uint16& planeEqFlags) const
{
	uint32 curPlaneBit = 1;
	for (const PlaneEq * p = planes_; 
		p < (planes_ + numPlanes_); ++p)
	{
		if (planeEqFlags & curPlaneBit)
		{
			float dist = p->distanceTo( center );
			if (dist > radius)
			{
				// Then fully outside
				return false;
			}
			else if (dist < -radius)
			{
				// Then fully inside
				planeEqFlags &= ~curPlaneBit;
			}
		}
		curPlaneBit <<= 1;
	}

	return true;
}


namespace Despair
{
	namespace Math
	{
		inline float SelectFloatGE0(float condition, float x, float y)		
		{ return (condition >= 0.f) ? x : y; }
		inline double SelectFloatGE0(double condition, double x, double y)	
		{ return (condition >= 0.f) ? x : y; }
	}
}


bool ConvexHull::intersects( const AABB & bb, uint16 & planeEqFlags ) 
	const
{
	Vector3 min = bb.minBounds();
	Vector3 max = bb.maxBounds();

	const PlaneEq * p = planes_;
	uint32 curPlaneBit = 1;
	const uint32 validPlanesMask = ((1 << numPlanes_) - 1);
	uint32 inFlags = planeEqFlags & validPlanesMask;
	uint32 outFlags = 0;

	while (curPlaneBit <= inFlags)
	{
		if (inFlags & curPlaneBit)
		{
			Vector3 neg;
			neg.x = Despair::Math::SelectFloatGE0(p->normal().x, min.x, max.x);
			neg.y = Despair::Math::SelectFloatGE0(p->normal().y, min.y, max.y);
			neg.z = Despair::Math::SelectFloatGE0(p->normal().z, min.z, max.z);

			if (p->distanceTo(neg) >= 0.f)
			{
				// Then fully outside
				return false;
			}

			Vector3 pos;
			pos.x = Despair::Math::SelectFloatGE0(p->normal().x, max.x, min.x);
			pos.y = Despair::Math::SelectFloatGE0(p->normal().y, max.y, min.y);
			pos.z = Despair::Math::SelectFloatGE0(p->normal().z, max.z, min.z);

			if (p->distanceTo(pos) > 0.f)
			{
				// Then we are intersecting
				outFlags |= curPlaneBit;
			}
		}
		curPlaneBit <<= 1;
		++p;
	}

	planeEqFlags = outFlags;
	return true;
}

} // namespace BW