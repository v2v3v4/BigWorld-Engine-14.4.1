#ifndef CONVEX_HULL_HPP
#define CONVEX_HULL_HPP

#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

class PlaneEq;
class Vector3;
class AABB;

BW_END_NAMESPACE

namespace BW
{

/**
 * Convex hull class. The volume of the convex hull is made up of the
 * intersection of all the negative half-space regions of each plane.
 */
class ConvexHull
{
public:
	ConvexHull( size_t numPlanes, PlaneEq * planes );

	size_t size() const;
	const PlaneEq & plane( size_t index ) const;

	/** 
	 * Intersection functionality.
	 * Intersects the convex hull with a bounding sphere.
	 *
	 * @return true if the sphere is partially or completely contained
	 *		   within the negative half-spaces of all the planes.
	 */
	bool intersects( const Vector3 & center, float radius ) const;

	/** 
	 * Intersection functionality.
	 * Intersects the convex hull with a bounding box.
	 *
	 * @return true if the bounding box is partially or completely contained
	 *		   within the negative half-spaces of all the planes.
	 */
	bool intersects( const AABB & bb ) const;

	/** 
	 * Intersection functionality.
	 * Intersects the convex hull with a bounding sphere.
	 *
	 * @param planeFlags bit field indicating which planes to test against.
	 *					 planes that do not intersect the sphere will have 
	 *					 their bits turned off.
	 * @return true if the sphere is partially or completely contained
	 *		   within the negative half-spaces of all the planes.
	 */
	bool intersects( const Vector3 & center, float radius,
		uint16& planeFlags ) const;

	/** 
	 * Intersection functionality.
	 * Intersects the convex hull with a bounding box.
	 *
	 * @param planeFlags bit field indicating which planes to test against.
	 *					 planes that do not intersect the sphere will have 
	 *					 their bits turned off.
	 * @return true if the bounding box is partially or completely contained
	 *		   within the negative half-spaces of all the planes.
	 */
	bool intersects( const AABB & bb, 
		uint16& planeFlags ) const;

private:
	size_t numPlanes_;
	PlaneEq* planes_;
};


} // namespace BW

#endif
