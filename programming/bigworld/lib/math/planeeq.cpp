#include "pch.hpp"

#include "planeeq.hpp"

#ifndef CODE_INLINE

BW_BEGIN_NAMESPACE
#include "planeeq.ipp"
BW_END_NAMESPACE

#endif

#include "vector2.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/dprintf.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Plane intersection function. The line is defined in the 'this' plane.
 */
LineEq PlaneEq::intersect( const PlaneEq & slice ) const
{
	const PlaneEq & basis = *this;

	// find the direction of our line in 3D
	Vector3 dir3D;
	dir3D.crossProduct( basis.normal(), slice.normal() );

	if (almostZero( dir3D.lengthSquared(), 0.0001f ))
		return LineEq( Vector2( 0.f, 0.f ), 0.f );

	// find the normal to the line (on the first plane) in 3D
	Vector3 normal3D;
	normal3D.crossProduct( normal_, dir3D );

	// find the origin of the basis plane's coordinate system in 3D
	Vector3 origin3D = this->param( Vector2( 0, 0 ) );

	// find the intersection in the direction of normal3D through
	//  origin3D with the slice plane.
	Vector3 intersectPoint3D = slice.intersectRay( origin3D, normal3D );

	// now turn intersectionPoint and dir3D into the basis of the basis plane
	Vector2 intersectPoint2D = this->project( intersectPoint3D );

	// set the normal from the normal in 3D
	Vector2 lineNormal = this->project( origin3D + normal3D );
	lineNormal.normalise();

	// calc d_ to work for the point that we know is on the line
	float lineD = lineNormal.dotProduct( intersectPoint2D );

	Vector3 ir = this->param( intersectPoint2D );

	if ((ir-intersectPoint3D).length() > 1.f)
	{
		dprintf( "Intersecting (%f,%f,%f) > %f with (%f,%f,%f) > %f\n",
			basis.normal().x, basis.normal().y, basis.normal().z, basis.d(),
			slice.normal().x, slice.normal().y, slice.normal().z, slice.d() );

		dprintf( "  3D intersect point (%f,%f,%f)\n",
			intersectPoint3D.x, intersectPoint3D.y, intersectPoint3D.z );
		dprintf( "  intersectPoint2D (%f,%f)\n", intersectPoint2D.x, intersectPoint2D.y );
		dprintf( "  Reconstituted intersect point (%f,%f,%f)\n", ir.x, ir.y, ir.z );
		Vector3 xdir, ydir;
		this->basis( xdir, ydir );
		dprintf( "  xdir (%f,%f,%f)\n", xdir.x, xdir.y, xdir.z );
		dprintf( "  ydir (%f,%f,%f)\n", ydir.x, ydir.y, ydir.z );
		dprintf( "  xdir dot ydir: %f\n", xdir.dotProduct( ydir ) );
	}

	return LineEq( lineNormal, lineD );
}








const int biMoreL[3]={ 1, 2, 0 };
const int biLessL[3]={ 2, 0, 1 };

/**
 *	This function returns the x and y directions for the parameterised
 *	basis of the given plane.
 */
void PlaneEq::basis( Vector3 & xdir, Vector3 & ydir ) const
{
	// find a suitable x basis vector  (find biggest dimension of normal.
	//  set other dimension A to zero. set other dimension B to one.
	//  then calc what biggest dimension should be to put us on the plane.
	//  we choose the biggest because we have to divide by it)
	float fabses[3] =
		{ float(fabs(normal_.x)), float(fabs(normal_.y)), float(fabs(normal_.z)) };
	int bi = fabses[0] > fabses[1] ?
		( fabses[0] > fabses[2] ? 0 : 2 ) : (fabses[1] > fabses[2] ? 1 : 2);

	int biMore = biMoreL[ bi ];
	int biLess = biLessL[ bi ];

	xdir[ biLess ] = 0;
	xdir[ biMore ] = 1;
	xdir[ bi     ] = -normal_[ biMore ] / normal_[ bi ];

	// find a suitable y basis vector  (cross product of existing ones)
	ydir.crossProduct( normal_, xdir );

	xdir.normalise();
	ydir.normalise();
}


/**
 *	This function finds the point defined by the 'point' param on the
 *	given plane. It should be a method of PlaneEq.
 */
Vector3 PlaneEq::param( const Vector2 & param ) const
{
	// get the basis to use
	Vector3 xdir, ydir;
	this->basis( xdir, ydir );

	float dOverNormLen = d_ / normal_.lengthSquared();

	// and get the vector. the 'origin' of the plane is at
	//  normal * d / normal.lengthSquared
	return dOverNormLen * normal_ + param.x * xdir + param.y * ydir;
}


/**
 *	This function finds the param for the given point in the plane's basis
 */
Vector2 PlaneEq::project( const Vector3 & point ) const
{
	// get the basis to use
	Vector3 xdir, ydir;
	this->basis( xdir, ydir );

	// take off the origin of the plane
	Vector3 offset = point - (d_ / normal_.lengthSquared()) * normal_;

	// and project it onto our (unit length) basis vectors
	return Vector2(
		xdir.dotProduct( offset ),
		ydir.dotProduct( offset ) );
}

BW_END_NAMESPACE

// planeeq.cpp
