#include "pch.hpp"

#include "sphere.hpp"

#include "vector3.hpp"
#include "vector4.hpp"
#include "boundbox.hpp"


BW_BEGIN_NAMESPACE

Sphere::Sphere()
	: center_( Vector3::ZERO )
	, radius_( 0.0f )
{

}

Sphere::Sphere( const Vector3& center, float radius )
	: center_( center ) 
	, radius_( radius )
{

}

Sphere::Sphere( const AABB& bb )
{
	// Calculate the center point and radius
	center_ = bb.centre();
	radius_ = (bb.maxBounds() - bb.minBounds()).length() / 2.0f;
}

bool Sphere::intersect( const Vector3& origin, const Vector3& travel ) const
{
	return intersect::intersectRaySphere( origin, travel, center_, radius_ );
}

namespace intersect
{

bool intersectRaySphere( const Vector3& origin, const Vector3& travel, 
						const Vector3& center, float radius )
{
	// travel = destination - origin;
	float r2 = radius * radius;

	// From Realtime Rendering 3, pg 741
	Vector3 l = center - origin;
	float s = travel.dotProduct( l );
	float l2 = l.dotProduct( l ); // distOriginToCenterSqr
	if (s < 0.0f && 
		l2 > r2)
	{
		return false;
	}

	float s2 = s * s;
	float m2 = l2 * s2;

	if (m2 > r2)
	{
		return false;
	}
	return true;
}

}

BW_END_NAMESPACE

// sphere.cpp
