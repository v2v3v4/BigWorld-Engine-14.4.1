/**
 *	@file
 */

#ifndef WORLD_POLYGON_HPP
#define WORLD_POLYGON_HPP

#include "cstdmf/bw_vector.hpp"

#include "math/vector3.hpp"
#include "math/planeeq.hpp"

BW_BEGIN_NAMESPACE

typedef BW::vector< Vector3 > PolygonBase;

/**
 *	This class is used to represent a polygon.
 */
class WorldPolygon : public BW::vector< Vector3 >
{
public:
	WorldPolygon() : PolygonBase() {}
	WorldPolygon( size_t size ) : PolygonBase( size ) {}

	void split( const PlaneEq & planeEq,
		WorldPolygon & frontPoly,
		WorldPolygon & backPoly ) const;

	bool chop( const PlaneEq & planeEq );

private:
};

typedef BW::vector< WorldPolygon > WPolygonSet;

#ifdef CODE_INLINE
#include "worldpoly.ipp"
#endif

BW_END_NAMESPACE

#endif // WORLD_POLYGON_HPP
