#ifndef PLANEEQ_HPP
#define PLANEEQ_HPP

#include "cstdmf/stdmf.hpp"
#include "mathdef.hpp"
#include "vector3.hpp"
#include "lineeq.hpp"

BW_BEGIN_NAMESPACE

class Vector2;

/**
 *	This class is used to represent a plane in 3 dimensional space.
 *
 *	@ingroup Math
 */
class PlaneEq
{
public:
	enum ShouldNormalise
	{
		SHOULD_NORMALISE,
		SHOULD_NOT_NORMALISE
	};

	PlaneEq();
	PlaneEq( const Vector3 & normal, const float d );
	PlaneEq( const Vector3 & point, const Vector3 & normal );
	PlaneEq( const Vector3 & v0,
		const Vector3 & v1,
		const Vector3 & v2,
		ShouldNormalise normalise = SHOULD_NORMALISE );

	INLINE void init( const Vector3 & p0,
		const Vector3 & p1,
		const Vector3 & p2,
		ShouldNormalise normalise = SHOULD_NORMALISE );
	INLINE void init( const Vector3 & point,
				 const Vector3 & normal );
	INLINE void init( const Vector3 & normal,
		const float d );

	float distanceTo( const Vector3 & point ) const;
	bool isInFrontOf( const Vector3 & point ) const;
	bool isInFrontOfExact( const Vector3 & point ) const;
	float y( float x, float z ) const;

	Vector3 intersectRay( const Vector3 & source, const Vector3 & dir ) const;
	INLINE float
		intersectRayHalf( const Vector3 & source, float normalDotDir ) const;
	INLINE float
		intersectRayHalfNoCheck( const Vector3 & source, float oneOverNormalDotDir ) const;

	LineEq intersect( const PlaneEq & slice ) const;

	void basis( Vector3 & xdir, Vector3 & ydir ) const;
	Vector3 param( const Vector2 & param ) const;
	Vector2 project( const Vector3 & point ) const;

	const Vector3 & normal() const;
	float d() const;

	void setHidden();
	void setAlwaysVisible();

private:
	Vector3 normal_;
	float d_;
};

#ifdef CODE_INLINE
	#include "planeeq.ipp"
#endif

BW_END_NAMESPACE

#endif // PLANEEQ_HPP
