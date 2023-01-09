#ifndef SPHERE_HPP
#define SPHERE_HPP

#include "vector3.hpp"

BW_BEGIN_NAMESPACE

class AABB;

class Sphere
{
public:
	Sphere();
	Sphere( const Vector3& center, float radius );
	explicit Sphere( const AABB& bb );

	const Vector3& center() const;
	void center( const Vector3& newCenter );

	float radius() const;
	void radius( float newRadius );

	bool intersect( const Vector3& origin, const Vector3& travel ) const;

private:
	Vector3 center_;
	float radius_;
};

// Helpers
namespace intersect
{
	bool intersectRaySphere( const Vector3& origin, const Vector3& travel, 
		const Vector3& center, float radius );
}

BW_END_NAMESPACE

#include "sphere.ipp"

#endif // SPHERE_HPP
