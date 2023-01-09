#ifndef LINEEQ_HPP
#define LINEEQ_HPP

#include "vector2.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the equation of a line. It is not a line segment or
 *	a ray, but an infinite line (like a PlaneEq is an infinite plane)
 *	The line is defined in two dimensions, on some plane.
 *
 *	For param and all that to work, your normals must be normalised!!
 */
class LineEq
{
public:
	/// simple constructor
	LineEq( const Vector2 & normal, const float d ) :
		normal_( normal ), d_( d ) {}

	LineEq( const Vector2 & v0, const Vector2 & v1, bool needToNormalise = true );

	float intersect( const LineEq & other ) const;

	Vector2 param( float t ) const;
	float project( const Vector2 & point ) const;

	bool isMinCutter( const LineEq & cutter ) const;
	bool isParallel( const LineEq & other ) const;
	bool isInFrontOf( const Vector2 & point ) const;

	const Vector2 & normal() const	{ return normal_; }
	float d() const					{ return d_; }

private:
	Vector2		normal_;
	float		d_;
};

#include "lineeq.ipp"

BW_END_NAMESPACE

#endif // LINEEQ_HPP
