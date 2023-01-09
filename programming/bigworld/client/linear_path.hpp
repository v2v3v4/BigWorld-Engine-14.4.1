#ifndef LINEAR_PATH_HPP
#define LINEAR_PATH_HPP

#include "math/vector3.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Path between two points given as a parametric equation.
 *	P = Pstart + dir * t
 */
class LinearPath
{
public:
	LinearPath( const Vector3& start, const Vector3& end );
	float length() const;
	Vector3 getPointAt( float t ) const;
	Vector3 getDirection( float t ) const;
	const Vector3& getStart() const;
	const Vector3& getEnd() const;

private:
	const Vector3 dir_;
	const Vector3 start_;
	const Vector3 end_;
};

BW_END_NAMESPACE

#endif // LINEAR_PATH_HPP
