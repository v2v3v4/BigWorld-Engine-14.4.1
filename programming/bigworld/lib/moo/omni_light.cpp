#include "pch.hpp"

#include "omni_light.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "omni_light.ipp"
#endif

namespace Moo {

/**
 *	Constructor
 */
OmniLight::OmniLight()
: position_( 0, 0, 0 ),
  innerRadius_( 0 ),
  outerRadius_( 0 ),
  colour_( 0, 0, 0, 0 ),
  terrainTimestamp_( 0xffffffff ),
  priority_( 0 )
#ifdef EDITOR_ENABLED
  , multiplier_( 1.f )
#endif
{
	// give the worldTransformed attributes default values.
	worldPosition_ = position_;
	worldInnerRadius_= innerRadius_;
	worldOuterRadius_= outerRadius_;
}

/**
 *	Constructor
 */
OmniLight::OmniLight( const D3DXCOLOR& colour, const Vector3& position, float innerRadius, float outerRadius )
: position_( position ),
  innerRadius_( innerRadius ),
  outerRadius_( outerRadius ),
  colour_( (const Colour&)colour ),
  priority_( 0 )
#ifdef EDITOR_ENABLED
  ,multiplier_(1.f)
#endif
{
	// give the worldTransformed attributes default values.
	worldPosition_ = position;
	worldInnerRadius_= innerRadius;
	worldOuterRadius_= outerRadius;
}

/**
 *	Destructor
 */
OmniLight::~OmniLight()
{
}

/**
 *  Transform the the light to worldspace. The worldTransform of the light must
 *  be an orthogonal matrix with uniform scale for the light to scale properly.
 */
void OmniLight::worldTransform( const Matrix& transform )
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
	
	//-- calculate world bounds.
	{
		Vector3 expand(outerRadius_, outerRadius_, outerRadius_);
		BoundingBox localBounds(position_ - expand, position_ + expand);

		worldBounds_ = localBounds;
		worldBounds_.transformBy(transform);
	}
}

/**
 * Create the vertex shader constants for a terrain light.
 *
 */
void OmniLight::createTerrainLight( float lightScale )
{
	terrainLight_[ 0 ] = Vector4( worldPosition_.x * lightScale,
		worldPosition_.y * lightScale,
		worldPosition_.z * lightScale, 0 );

	terrainLight_[ 1 ] = Vector4( colour_.r * multiplier(), colour_.g * multiplier(), colour_.b * multiplier(), colour_.a * multiplier() );

	float outer = worldOuterRadius_ * lightScale;
	float inner = worldInnerRadius_ * lightScale;

	terrainLight_[ 2 ] = Vector4( outer, 1.f / ( outer - inner ), 0, 0 );
}

/**
 *	Calculate attenuation factor from distance of this light to given bounding
 *	box (in world space).
 *
 *	@param worldSpaceBB	The bounding box of target object.
 */
float OmniLight::attenuation( const BoundingBox& worldSpaceBB ) const
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

} // namespace Moo

BW_END_NAMESPACE

// omni_light.cpp
