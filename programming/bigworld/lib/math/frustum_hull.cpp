#include "pch.hpp"
#include "frustum_hull.hpp"

#include <math/matrix.hpp>
#include <math/vector4.hpp>
#include <math/convex_hull.hpp>
#include <math/planeeq.hpp>

namespace BW {

FrustumHull::FrustumHull( const Matrix & viewProjection )
{
	createPlanesFromViewProjection( viewProjection, planes_ );
}

ConvexHull FrustumHull::hull()
{
	return ConvexHull( 6, &planes_[0] );
}

void FrustumHull::createPlanesFromViewProjection( const Matrix & viewProjection,
	PlaneEq planes[6] )
{
	// From Despair::Frustum::SetFromProjectionMatrix
	Vector4 nearPlaneEq( 0.f, 0.f, -1.f, 0.f );
	Vector4 farPlaneEq( 0.f, 0.f, 1.f, -1.f );
	Vector4 leftPlaneEq( -1.f, 0.f, 0.f, -1.f );
	Vector4 rightPlaneEq( 1.f, 0.f, 0.f, -1.f );
	Vector4 topPlaneEq( 0.f, 1.f, 0.f, -1.f );
	Vector4 bottomPlaneEq( 0.f, -1.f, 0.f, -1.f );

	Matrix mtx = viewProjection;
	mtx.transpose();

	// transform the vectors and normalize by the length of the transformed 
	// normal
	Vector4 transformed;
	Vector3 normal;
	float recipricolLength;

	mtx.applyPoint( transformed, nearPlaneEq );
	normal.set( transformed.x, transformed.y, transformed.z );
	recipricolLength = 1 / normal.length();
	transformed.w *= recipricolLength;
	normal *= recipricolLength;
	planes[0].init( normal, -transformed.w );

	mtx.applyPoint( transformed, farPlaneEq );
	normal.set( transformed.x, transformed.y, transformed.z );
	recipricolLength = 1 / normal.length();
	transformed.w *= recipricolLength;
	normal *= recipricolLength;
	planes[1].init( normal, -transformed.w );

	mtx.applyPoint( transformed, leftPlaneEq );
	normal.set( transformed.x, transformed.y, transformed.z );
	recipricolLength = 1 / normal.length();
	transformed.w *= recipricolLength;
	normal *= recipricolLength;
	planes[2].init( normal, -transformed.w );

	mtx.applyPoint( transformed, rightPlaneEq );
	normal.set( transformed.x, transformed.y, transformed.z );
	recipricolLength = 1 / normal.length();
	transformed.w *= recipricolLength;
	normal *= recipricolLength;
	planes[3].init( normal, -transformed.w );

	mtx.applyPoint( transformed, topPlaneEq );
	normal.set( transformed.x, transformed.y, transformed.z );
	recipricolLength = 1 / normal.length();
	transformed.w *= recipricolLength;
	normal *= recipricolLength;
	planes[4].init( normal, -transformed.w );

	mtx.applyPoint( transformed, bottomPlaneEq );
	normal.set( transformed.x, transformed.y, transformed.z );
	recipricolLength = 1 / normal.length();
	transformed.w *= recipricolLength;
	normal *= recipricolLength;
	planes[5].init( normal, -transformed.w );
}

} // namespace BW
