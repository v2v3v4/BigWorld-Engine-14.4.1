/**
 *	@file
 */

#ifndef VECTOR3GEN_HPP
#define VECTOR3GEN_HPP

#include "math/vector3.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the base class for classes used to generate random Vector3
 *	instances.
 */
class Vector3Generator
{
public:
	virtual ~Vector3Generator();

	/// This method sets the input vector to a random value.
	virtual void generate(Vector3 & vec) const = 0;
};


/**
 *	This class is used to generate Vector3 instances at a fixed point.
 */
class PointV3Gen : public Vector3Generator
{
public:
	PointV3Gen(const Vector3 & point);

	virtual void generate(Vector3 & vec) const;

protected:
	Vector3 point_;		///< Where the points are generated at.
};


/**
 *	This class is used to generate Vector3 instances randomly along a line.
 */
class LineV3Gen : public Vector3Generator
{
public:
	LineV3Gen(const Vector3 & start,
		const Vector3 & end);

	virtual void generate(Vector3 & vec) const;

	void setSource(const Vector3 & source);

private:
	Vector3 start_;
	Vector3 dir_;
};


/**
 *	This class is used to generate Vector3 instances in a cylinder.
 */
class CylinderV3Gen : public Vector3Generator
{
public:
	CylinderV3Gen(const Vector3 & source,
		const Vector3 & target,
		float minRadius,
		float maxRadius);
	virtual void generate(Vector3 & vec) const;

	void setSource(const Vector3 & source);

private:
	Vector3 source_;
	Vector3 dir_;
	float r1_;
	float r2_;

	Vector3 u_;
	Vector3 v_;
};


/**
 *	This class is used to generate Vector3 instances in a sphere.
 */
class SphereV3Gen : public Vector3Generator
{
public:
	SphereV3Gen(const Vector3 & centre,
		float maxRadius,
		float minRadius = 0.f);
	virtual void generate(Vector3 & vec) const;

private:
	Vector3 centre_;
	float minRadius_;
	float maxRadius_;
};


/**
 *	This class is used to generate Vector3 instances in a box.
 */
class BoxV3Gen : public Vector3Generator
{
public:
	BoxV3Gen(const Vector3 & min,
		const Vector3 & max);
	virtual void generate(Vector3 & vec) const;

	Vector3 min_;		///< The minimum coordinate of the box.
	Vector3 max_;		///< The maximum coordinate of the box.
private:
};

#ifdef CODE_INLINE
#include "vec3gen.ipp"
#endif

BW_END_NAMESPACE

#endif // VECTOR3GEN_HPP
