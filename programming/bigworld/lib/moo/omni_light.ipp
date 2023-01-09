
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


namespace Moo {

INLINE
OmniLight::GPU OmniLight::gpu( ) const
{
	BW_GUARD;

	GPU gpuData;

	gpuData.m_pos		  = Vector4(worldPosition_, 0.0f);
	gpuData.m_attenuation = Vector4(outerRadius_, 1.0f / (outerRadius_ - innerRadius_), 0.0f, 0.0f);

#ifdef EDITOR_ENABLED
	gpuData.m_color		  = Vector4(colour_.r, colour_.g, colour_.b, colour_.a) * multiplier_;
#else
	gpuData.m_color	      = Vector4(colour_.r, colour_.g, colour_.b, colour_.a);
#endif //-- EDITOR_ENABLED

	return gpuData;
}

INLINE
const BoundingBox& OmniLight::worldBounds( ) const
{
	BW_GUARD;
	return worldBounds_;
}

INLINE
const Vector3& OmniLight::position( ) const
{
	return position_;
}

INLINE
void OmniLight::position( const Vector3& position )
{
	position_ = position;
}

INLINE
float OmniLight::innerRadius( ) const
{
	return innerRadius_;
}

INLINE
void OmniLight::innerRadius( float innerRadius )
{
	innerRadius_ = innerRadius;
	if( innerRadius_ > outerRadius_ )
		outerRadius( innerRadius_ );
}

INLINE
float OmniLight::outerRadius( ) const
{
	return outerRadius_;
}

INLINE
void OmniLight::outerRadius( float outerRadius )
{
	outerRadius_ = outerRadius;
	if( innerRadius_ > outerRadius_ )
		innerRadius( outerRadius_ );
}

INLINE
const Colour& OmniLight::colour( ) const
{
	return colour_;
}

INLINE
void OmniLight::colour( const Colour& colour )
{
	colour_ = colour;
}

INLINE
const Vector3& OmniLight::worldPosition( void ) const
{
	return worldPosition_;
}

INLINE
float OmniLight::worldInnerRadius( void ) const
{
	return worldInnerRadius_;
}

INLINE
float OmniLight::worldOuterRadius( void ) const
{
	return worldOuterRadius_;
}

INLINE
bool OmniLight::intersects( const BoundingBox& worldSpaceBB ) const
{
	return worldSpaceBB.intersects( worldPosition_, worldOuterRadius_ );
}

INLINE
Vector4* OmniLight::getTerrainLight( uint32 timestamp, float lightScale )
{
	if( terrainTimestamp_ != timestamp )
	{
		createTerrainLight( lightScale );
		terrainTimestamp_ = timestamp;
	}
	return terrainLight_;
}

}

/*omni_light.ipp*/
