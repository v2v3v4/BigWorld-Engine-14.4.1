#include "pch.hpp"

#include "spot_light.hpp"

#include "math/planeeq.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "spot_light.ipp"
#endif

namespace Moo {

/**
 *	Constructor
 */
SpotLight::SpotLight( )
: dirty_( true ),
  position_( 0, 0, 0 ),
  direction_( 0, 0, 1 ),
  innerRadius_( 0 ),
  outerRadius_( 0 ),
  cosConeAngle_( 1 ),
  colour_( 0, 0, 0, 0 ),
  terrainTimestamp_( 0xffffffff ),
  priority_( 0 )
#ifdef EDITOR_ENABLED
  , multiplier_( 1.f )
#endif
{
	// give the worldTransformed attributes default values.
	worldPosition_ = position_;
	worldDirection_ = direction_;

	worldInnerRadius_ = innerRadius_;
	worldOuterRadius_ = outerRadius_;
}

/**
 *	Constructor
 */
SpotLight::SpotLight( const Colour& colour, const Vector3& position, const Vector3& direction,
	 float innerRadius, float outerRadius, float cosConeAngle )
: dirty_( true ),
  position_( position ),
  direction_( direction ),
  innerRadius_( innerRadius ),
  outerRadius_( outerRadius ),
  cosConeAngle_( cosConeAngle ),
  colour_( colour ),
  priority_( 0 )
#ifdef EDITOR_ENABLED
  , multiplier_( 1.f )
#endif
{
	// give the worldTransformed attributes default values.
	worldPosition_ = position;
	worldDirection_ = direction;
	worldInnerRadius_ = innerRadius;
	worldOuterRadius_ = outerRadius;
}

/**
 *  Transform the the light to worldspace. The worldTransform of the light must
 *  be an orthogonal matrix with uniform scale for the light to scale properly.
 */
void SpotLight::worldTransform( const Matrix& transform )
{
	// transform light position
	XPVec3TransformCoord( &worldPosition_, &position_, &transform );

	// calculate the scale of the matrix by calculating the scale of the
	// first row of the world transform matrix (only works on orthogonal
	// uniform scale matrices)
	MF_ASSERT(transform.isUniformlyScaled());
	float scale = transform.uniformScale();

	//scale radiuses
	worldInnerRadius_ = innerRadius_ * scale;
	worldOuterRadius_ = outerRadius_ * scale;

	// transform the direction, assumes that transform is uniformally scaled and orthogonal.
	Matrix transNormal( transform );
	if ( transNormal.invert() )
	{
		transNormal.transpose();
	}
	XPVec3TransformNormal( &worldDirection_, &direction_, &transNormal );

	XPVec3Normalize( &worldDirection_, &worldDirection_ );
	dirty_ = true;
}

/**
 * Create the vertex shader constants for a terrain light.
 *
 */
void SpotLight::createTerrainLight( float lightScale )
{
	terrainLight_[ 0 ] = Vector4( worldPosition_.x * lightScale,
		worldPosition_.y * lightScale,
		worldPosition_.z * lightScale, 0 );
	terrainLight_[ 1 ] = Vector4( colour_.r * multiplier(), colour_.g * multiplier(), colour_.b * multiplier(), colour_.a * multiplier() );

	float outer = worldOuterRadius_ * lightScale;
	float inner = worldInnerRadius_ * lightScale;

	terrainLight_[ 2 ] = Vector4( outer, 1.f / ( outer - inner ), cosConeAngle_, 0 );
	terrainLight_[ 3 ] = Vector4( worldDirection_.x , worldDirection_.y, worldDirection_.z, 0 );
}

/**
 *	Calculate attenuation factor from distance of this light to given bounding
 *	box (in world space).
 *
 *	@param worldSpaceBB	The bounding box of target object.
 */
float SpotLight::attenuation( const BoundingBox& worldSpaceBB ) const
{
	float distance = worldSpaceBB.distance( worldPosition_ );

	float att = 0;
	if (distance < worldInnerRadius_)
		att = 2 - (distance / worldInnerRadius_ );
	else if (distance < worldOuterRadius_)
	{
		att = (distance - worldOuterRadius_) / (worldInnerRadius_ - worldOuterRadius_);
	}

	return att;
}

/**
 *	Make sure the internal bounds are cached.
 */
void SpotLight::updateInternalBounds() const
{
	if (!dirty_)
		return;

	float angle = acosf( cosConeAngle_ );

	// Build bounding box for the view frustum
	lightBounds_.setBounds( worldPosition_, worldPosition_ );

	Vector3 capCentre = worldPosition_ + direction_*outerRadius_;
	float capRadius   = outerRadius_ * tanf(angle);

	PlaneEq capPlane( direction_, outerRadius_ );
	Vector3 xdir, ydir;
	capPlane.basis( xdir, ydir );

	lightBounds_.addBounds( capCentre + xdir*capRadius );
	lightBounds_.addBounds( capCentre - xdir*capRadius );
	lightBounds_.addBounds( capCentre + ydir*capRadius );
	lightBounds_.addBounds( capCentre - ydir*capRadius );

	// Build light view projection
	Matrix lookat, proj;
	lookat.lookAt( worldPosition_, direction_, Vector3(0,1,0) );
	proj.perspectiveProjection( angle*2.0f, 1.0f, 0.0f, outerRadius_ );

	lightView_.multiply( lookat, proj );

	dirty_ = false;
}

/**
 *	Destructor
 */
SpotLight::~SpotLight()
{
}

} // namespace Moo

BW_END_NAMESPACE

// spot_light.cpp
