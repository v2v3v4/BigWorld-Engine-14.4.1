#include "pch.hpp"

#include "boundbox.hpp"

BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "boundbox.ipp"
#endif

/**
 *	This method calculates the outcode when the bounding box is
 *	viewed by the given projection matrix
 */
void AABB::calculateOutcode( const Matrix & m, 
	Outcode &outCode, Outcode& combinedOutcode ) const
{
	Outcode oc = 0;
	Outcode combinedOc = OUTCODE_MASK;

	Vector4 vx[2];
	Vector4 vy[2];
	Vector4 vz[2];

	vx[0] = m.row( 0 );
	vx[1] = vx[0];
	vx[0] *= min_[0];
	vx[1] *= max_[0];

	vy[0] = m.row( 1 );
	vy[1] = vy[0];
	vy[0] *= min_[1];
	vy[1] *= max_[1];

	vz[0] = m.row( 2 );
	vz[1] = vz[0];
	vz[0] *= min_[2];
	vz[1] *= max_[2];

	const Vector4 &vw = m.row( 3 );

	for( uint32 i=0 ; i < 8 ; i++ )
	{
		Vector4 v = vw;
		v += vx[ i &1 ];
		v += vy[( i >> 1 ) &1 ];
		v += vz[( i >> 2 ) &1 ];

		Outcode outcode = v.calculateOutcode();

		combinedOc &= outcode;

		oc |= outcode;
	}

	outCode = oc;
	combinedOutcode = combinedOc;
}


/**
 *	This method calculates the outcode when the bounding box is
 *	viewed by the given projection matrix
 */
void BoundingBox::calculateOutcode( const Matrix & m ) const
{
	AABB::calculateOutcode( m, oc_, combinedOc_ );
}


/**
 *	This method transforms this bounding box by the input transform.
 *	The new bounding box is set to the bounding box of the rotated box.
 */
void AABB::transformBy( const Matrix & transform )
{
	MF_ASSERT( !this->insideOut() );

	// Need to remember to save the size that we'll use to scale the unit axes
	const Vector3 size = this->maxBounds() - this->minBounds();

	// Calculate the new location of the minimum point.
	min_ = transform.applyPoint(min_);
	max_ = min_;

	// Go through each axis. Each component of each axis either adds to the
	// max or takes from the min.

	for (int axis = X_AXIS; axis <= Z_AXIS; axis++)
	{
		const Vector3 transformedAxis =
			size[axis] * transform.applyToUnitAxisVector(axis);

		for (int resultDirection = X_COORD;
			resultDirection <= Z_COORD;
			resultDirection++)
		{
			// Go through each coordinate of the axis and add to the
			// appropriate point.

			if (transformedAxis[resultDirection] > 0)
			{
				max_[resultDirection] += transformedAxis[resultDirection];
			}
			else
			{
				min_[resultDirection] += transformedAxis[resultDirection];
			}
		}
	}
}


/**
 *	This method tests if the given ray intersects with the bounding box
 */
bool AABB::intersectsRay( const Vector3 & origin, const Vector3 & dir ) const
{
	Vector3 bounds[2] = {min_, max_};
	Vector3 ptOnPlane;

	for (int bound = 0; bound < 2; bound++)
	{
		for (int axis = X_AXIS; axis <= Z_AXIS; axis++)
		{
			if (!isZero( dir[axis] ))
			{
				float t = (bounds[bound][axis] - origin[axis]) / dir[axis];

				if (t > 0.f)
				{
					ptOnPlane = origin + t * dir;

					int nextAxis = (axis + 1) % 3;
					int prevAxis = (axis + 2) % 3;

					if (min_[nextAxis] < ptOnPlane[nextAxis] && ptOnPlane[nextAxis] < max_[nextAxis] &&
						min_[prevAxis] < ptOnPlane[prevAxis] && ptOnPlane[prevAxis] < max_[prevAxis])
					{
						return true;
					}

				}
			}
		}
	}

	return false;
}


/**
 *	This method tests if the given line segment intersects with the bounding box
 */
bool AABB::intersectsLine( const Vector3 & origin, const Vector3 & dest ) const
{
	const Vector3 direction = dest - origin;
	Vector3 bounds[2] = {this->minBounds(), this->maxBounds()};
	Vector3 ptOnPlane;

	for (int bound = 0; bound < 2; bound++)
	{
		for (int axis = X_AXIS; axis <= Z_AXIS; axis++)
		{
			if (!isZero( direction[axis] ))
			{
				float t = (bounds[bound][axis] - origin[axis]) / direction[axis];

				if ( t > 0.f && t < 1.f )
				{
					ptOnPlane = origin + t * direction;

					int nextAxis = (axis + 1) % 3;
					int prevAxis = (axis + 2) % 3;

					if (min_[nextAxis] < ptOnPlane[nextAxis] && ptOnPlane[nextAxis] < max_[nextAxis] &&
						min_[prevAxis] < ptOnPlane[prevAxis] && ptOnPlane[prevAxis] < max_[prevAxis])
					{
						return true;
					}

				}
			}
		}
	}

	return false;
}


/**
 *	A method to compute a ray-AABB intersection.
 *	Original code by Andrew Woo, from "Graphics Gems", Academic Press, 1990
 *	Optimized code by Pierre Terdiman, 2000 (~20-30% faster on my Celeron 500)
 *	Epsilon value added by Klaus Hartmann. (discarding it saves a few cycles only)
 *
 *	Hence this version is faster as well as more robust than the original one.
 *
 *	Should work provided:
 *	1) the integer representation of 0.0f is 0x00000000
 *	2) the sign bit of the float is the most significant one
 *
 *	Report bugs: p.terdiman@codercorner.com
 *
 *	\param		aabb		[in] the axis-aligned bounding box
 *	\param		origin		[in] ray origin
 *	\param		dir			[in] ray direction
 *	\param		coord		[out] impact coordinates
 *	\return		true if ray intersects AABB
 */

//! Integer representation of a floating-point value.
uint32 IR(float x)
{
	return ((uint32&)x);
}

#define RAYAABB_EPSILON 0.00001f
AABB::RayIntersectionType AABB::intersectsRay(
	const Vector3 & origin, const Vector3 & dir, Vector3 & coord) const
{
	bool isInside = true;
	Vector3 minBounds = this->minBounds();
	Vector3 maxBounds = this->maxBounds();
	Vector3 maxT( -1.0f, -1.0f, -1.0f );
	
	// Find candidate planes.
	for (uint32 i=0;i<3;i++)
	{
		if (origin[i] < minBounds[i])
		{
			coord[i] = minBounds[i];
			isInside = false;

			// Calculate T distances to candidate planes
			if (IR(dir[i]))
			{
				maxT[i] = (minBounds[i] - origin[i]) / dir[i];
			}
		}
		else if (origin[i] > maxBounds[i])
		{
			coord[i] = maxBounds[i];
			isInside = false;

			// Calculate T distances to candidate planes
			if (IR(dir[i]))
			{
				maxT[i] = (maxBounds[i] - origin[i]) / dir[i];
			}
		}
	}

	// Ray origin inside bounding box
	if (isInside)
	{
		coord = origin;
		return ORIGIN_INSIDE;
	}

	// Get smallest of the maxT's for final choice of intersection
	uint32 whichPlane = 0;
	if (maxT[1] > maxT[whichPlane])
	{
		whichPlane = 1;
	}
	if (maxT[2] > maxT[whichPlane])
	{
		whichPlane = 2;
	}

	// Check final candidate actually inside box
	if (IR(maxT[whichPlane]) & 0x80000000)
	{
		return NO_INTERSECTION;
	}

	for (uint32 i = 0; i<3; i++)
	{
		if (i!=whichPlane)
		{
			coord[i] = origin[i] + maxT[whichPlane] * dir[i];
#ifdef RAYAABB_EPSILON
			if (coord[i] < minBounds[i] - RAYAABB_EPSILON || 
				coord[i] > maxBounds[i] + RAYAABB_EPSILON)	
			{
				return NO_INTERSECTION;
			}
#else
			if (coord[i] < minBounds[i] || 
				coord[i] > maxBounds[i])
			{
				return NO_INTERSECTION;
			}
#endif
		}
	}
	return INTERSECTION;
}


/**
 *	This method clips the given line segment to this bounding box.
 *	i.e. it makes it start and end inside it. For this calculation, the
 *	bounding box is first expanded by a bloat amount in all directions.
 *
 *	@return false if the line segment doesn't intersects the box at all
 *			(like intersectsLine)
 */
bool AABB::clip( Vector3 & source, Vector3 & extent, float bloat ) const
{
	Vector3	mt;	// min params
	Vector3	Mt; // max params

	float sMin = 0.f;
	float eMax = 1.f;

	Vector3 delta = extent - source;

	// check the x faces
	if (!isZero( delta.x ))
	{
		// find the parameter along the line as it crosses these faces
		mt.x = (min_.x - bloat - source.x) / delta.x;
		Mt.x = (max_.x + bloat - source.x) / delta.x;

		// see if it moves in the min or max,
		//  (depends on line dir in this axis)
		if (delta.x > 0.f)
		{
			if (mt.x > sMin) sMin = mt.x;
			if (Mt.x < eMax) eMax = Mt.x;
		}
		else
		{
			if (Mt.x > sMin) sMin = Mt.x;
			if (mt.x < eMax) eMax = mt.x;
		}
	}
	else if (source.x < min_.x - bloat || source.x >= max_.x + bloat)
		return false;

	// check the y faces
	if (!isZero( delta.y ))
	{
		mt.y = (min_.y - bloat - source.y) / delta.y;
		Mt.y = (max_.y + bloat - source.y) / delta.y;
		if (delta.y > 0.f)
		{
			if (mt.y > sMin) sMin = mt.y;
			if (Mt.y < eMax) eMax = Mt.y;
		}
		else
		{
			if (Mt.y > sMin) sMin = Mt.y;
			if (mt.y < eMax) eMax = mt.y;
		}
	}
	else if (source.y < min_.y - bloat || source.y >= max_.y + bloat)
		return false;

	// check the z faces
	if (!isZero( delta.z ))
	{
		mt.z = (min_.z - bloat - source.z) / delta.z;
		Mt.z = (max_.z + bloat - source.z) / delta.z;
		if (delta.z > 0.f)
		{
			if (mt.z > sMin) sMin = mt.z;
			if (Mt.z < eMax) eMax = Mt.z;
		}
		else
		{
			if (Mt.z > sMin) sMin = Mt.z;
			if (mt.z < eMax) eMax = mt.z;
		}
	}
	else if (source.z < min_.z - bloat || source.z >= max_.z + bloat)
		return false;

	// get out if it's not inside
	if (sMin >= eMax) return false;

	// otherwise set the points the the calculated parameters
	extent = source + eMax * delta;
	source = source + sMin * delta;

	// and that's it
	return true;
}

/**
 * This method calculates the distance from a point to the bounding box
 * @param point point to calculate the distance to
 * @return distance to the point
 */
float AABB::distance( const Vector3& point ) const
{
	Vector3 p(0,0,0);

	if (point.x > max_.x)
		p.x = point.x - max_.x;
	else if (point.x < min_.x)
		p.x = point.x - min_.x;

	if (point.y > max_.y)
		p.y = point.y - max_.y;
	else if (point.y < min_.y)
		p.y = point.y - min_.y;

	if (point.z > max_.z)
		p.z = point.z - max_.z;
	else if (point.z < min_.z)
		p.z = point.z - min_.z;

	return p.length();
}

// Static initialiser
const BoundingBox BoundingBox::s_insideOut_;

BW_END_NAMESPACE

// boundbox.cpp
