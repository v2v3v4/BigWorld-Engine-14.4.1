#ifndef _FRUSTUM_HULL_HPP
#define _FRUSTUM_HULL_HPP

#include <math/planeeq.hpp>

BW_BEGIN_NAMESPACE
class Matrix;
BW_END_NAMESPACE

namespace BW {

class ConvexHull;

/**
 * Frustum hull class, which currently lives only to be queried as a ConvexHull.
 */
class FrustumHull
{
public:
	FrustumHull( const Matrix & viewProjection );

	ConvexHull hull();

	static void createPlanesFromViewProjection( const Matrix & viewProjection,
		PlaneEq planes[6] );

private:
	PlaneEq planes_[6];
};


} // namespace BW

#endif
