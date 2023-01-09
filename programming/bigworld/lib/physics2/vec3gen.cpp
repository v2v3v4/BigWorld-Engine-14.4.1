/**
 *	@file
 */

#include "pch.hpp"

#include "vec3gen.hpp"

#include <math.h>
#include "math/mathdef.hpp"

BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
	#include "vec3gen.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: Point generator
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param point	The point to generate Vector3 instances at.
 */
PointV3Gen::PointV3Gen(const Vector3 & point) : point_(point)
{
}


/**
 *	This method sets the input Vector3 to a fixed point.
 */
void PointV3Gen::generate( Vector3 & vec ) const
{
	vec = point_;
}


// -----------------------------------------------------------------------------
// Section: Line generator
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param start	The point at the start of the line interval.
 *	@param end		The point at the start of the line interval.
 */
LineV3Gen::LineV3Gen(const Vector3 & start,
					 const Vector3 & end) :
	start_(start),
	dir_(end - start)
{
}


/**
 *	This method sets the input Vector3 to a random point on the specified line.
 */
void LineV3Gen::generate(Vector3 & vec) const
{
	vec = start_ + unitRand() * dir_;
}


// -----------------------------------------------------------------------------
// Section: Cylinder generator
// -----------------------------------------------------------------------------

/**
 *	This constructor initialises this cylinder generator. The cylinder can be
 *	empty in the middle.
 *
 *	@param source	The centre of one of the cylinder's bases.
 *	@param target	The centre of the other cylinder's base.
 *	@param minRadius	The inner radius of the cylinder. That is, the radius of
 *						the hole in the middle.
 *	@param maxRadius	The outer radius of the cylinder.
 */
CylinderV3Gen::CylinderV3Gen(const Vector3 & source,
		const Vector3 & target,
		float minRadius,
		float maxRadius) :
	source_(source),
	dir_(target - source),
	r1_(minRadius),
	r2_(maxRadius)
{
	// Find base vectors
	Vector3 n = dir_;
	n.normalise();

	Vector3 basis(1.0f, 0.0f, 0.0f);

	if (1.0f - fabs(basis.dotProduct(n)) < 1e-5f)
		basis.set(0.0f, 1.0f, 0.0f);

	u_ = basis.crossProduct(dir_);
	u_.normalise();

	v_ = dir_.crossProduct(u_);
	v_.normalise();
}


/**
 *	This method sets the input Vector3 to a random point in the specified
 *	cylinder.
 */
void CylinderV3Gen::generate(Vector3 & vec) const
{
	// Distance along axis
	vec = source_ + unitRand() * dir_;

	// Distance from axis
	float angle = unitRand() * 2.0f * float(3.141f);
	float r = r2_ + unitRand() * (r1_ - r2_);

	float x = r * cosf(angle);
	float y = r * sinf(angle);

	vec += x * u_ + y * v_;
}


// -----------------------------------------------------------------------------
// Section: Sphere generator
// -----------------------------------------------------------------------------

/**
 *	This constructor initialises a sphere generator. The specified sphere may
 *	have a empty sphere at its centre.
 *
 * 	@param centre		The centre of the sphere.
 * 	@param maxRadius	The radius of the sphere.
 * 	@param minRadius	The radius of the hole in the centre of the sphere.
 */
SphereV3Gen::SphereV3Gen(const Vector3 & centre,
						 float maxRadius,
						 float minRadius) :
	minRadius_(minRadius),
	maxRadius_(maxRadius),
	centre_(centre)
{
}


/**
 *	This method sets the input Vector3 to a random point in the specified
 *	sphere.
 */
void SphereV3Gen::generate(Vector3 & vec) const
{
	// This generator is not quite uniform.  It perfers points towards
	// the corner of the bounding cube. For a uniform distribution we
	// could use a rejection method.

	vec.set(unitRand() - 0.5f, unitRand() - 0.5f, unitRand() - 0.5f);
	vec.normalise();

	if (maxRadius_ == minRadius_)
	{
		vec *= maxRadius_;
		vec += centre_;
	}
	else
	{
		vec *= minRadius_ + unitRand() * (maxRadius_ - minRadius_);
		vec += centre_;
	}
}


// -----------------------------------------------------------------------------
// Section: Box generator
// -----------------------------------------------------------------------------

/**
 *	This constructor initialises this box generator. The box is axis aligned.
 *
 *	@param min	One corner of the box.
 *	@param max	The opposite corner of the box.
 */
BoxV3Gen::BoxV3Gen(const Vector3 & min,
				   const Vector3 & max) :
	min_(min),
	max_(max)
{
}


/**
 *	This method sets the input Vector3 to a random point in the specified box.
 */
void BoxV3Gen::generate(Vector3 & vec) const
{
	vec.set(
		min_.x() + unitRand() * (max_.x() - min_.x()),
		min_.y() + unitRand() * (max_.y() - min_.y()),
		min_.z() + unitRand() * (max_.z() - min_.z()));
}

BW_END_NAMESPACE

// vec3gen.cpp
