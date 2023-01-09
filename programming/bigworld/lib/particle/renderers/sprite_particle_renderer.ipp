#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

/**
 *	This is the Get-Accessor for the materialFX property. This property
 *	defines how the material texture is drawn with respect to the background.
 *
 *	@return	The current setting for the materialFX property.
 */
INLINE SpriteParticleRenderer::MaterialFX
	SpriteParticleRenderer::materialFX() const
{
	return materialFX_;
}

/**
 *	This is the Set-Accessor for the materialFX property.
 *
 *	@param newMaterialFX	The new setting for the materialFX property.
 */
INLINE void SpriteParticleRenderer::materialFX(
	SpriteParticleRenderer::MaterialFX newMaterialFX )
{
	materialFX_ = newMaterialFX;
	materialSettingsChanged_ = true;
}

/**
 *	This is the Get-Accessor for the texture file name of the sprite.
 *
 *	@return	A string containing the file name of the sprite texture.
 */
INLINE const BW::string &SpriteParticleRenderer::textureName() const
{
	return textureName_;
}


/**
 *	This is the Get-Accessor for the effect file name of the sprite.
 *
 *	@return	A string containing the file name of the sprite effect.
 */
INLINE const BW::string& SpriteParticleRenderer::effectName() const
{
	return effectName_;
}

/**
 *	This method sets the effect name for the renderer and marks it for loading.
 *
 *	@param value	The new file name as a string.
 */
INLINE void SpriteParticleRenderer::effectName( const BW::string& value )
{
	BW_GUARD;

	effectName_ = value;
	materialSettingsChanged_ = true;
	updateMaterial();
}


/**
 *	Get-Accessor for the softDepthRange property.
 *	Modifies range of depth difference to which softness is applied.
 *
 *	@return	The current setting for the softDepthRange property.
 */
INLINE float SpriteParticleRenderer::softDepthRange() const
{
	return softDepthRange_;
}

/**
 *	Set-Accessor for if the softDepthRange property.
 *	Modifies range of depth difference to which softness is applied.
 *
 *	@param value	The new value for the softDepthRange property.
 */
INLINE void SpriteParticleRenderer::softDepthRange( float value )
{
	softDepthRange_ = value;
	materialSettingsChanged_ = true;
}


/**
 *	Get-Accessor for the softFalloffPower property.
 *	Falloff curve power. 1 is linear. Higher values increase fade rate.
 *
 *	@return	The current setting for the softFalloffPower property.
 */
INLINE float SpriteParticleRenderer::softFalloffPower() const
{
	return softFalloffPower_;
}

/**
 *	Set-Accessor for if the softFalloffPower property.
 *	Falloff curve power. 1 is linear. Higher values increase fade rate.
 *
 *	@param value	The new value for the softFalloffPower property.
 */
INLINE void SpriteParticleRenderer::softFalloffPower( float value )
{
	softFalloffPower_ = value;
	materialSettingsChanged_ = true;
}


/**
 *	Get-Accessor for the softDepthOffset property.
 *	Shifts the softening by relative depth difference, relative to extent.
 *
 *	@return	The current setting for the softDepthOffset property.
 */
INLINE float SpriteParticleRenderer::softDepthOffset() const
{
	return softDepthOffset_;
}

/**
 *	Set-Accessor for if the softDepthOffset property.
 *	Shifts the softening by relative depth difference, relative to extent.
 *
 *	@param value	The new value for the softDepthOffset property.
 */
INLINE void SpriteParticleRenderer::softDepthOffset( float value )
{
	softDepthOffset_ = value;
	materialSettingsChanged_ = true;
}


/**
 *	Get-Accessor for the nearFadeCutoff property.
 *	Distance at which particles become completely invisible.
 *
 *	@return	The current setting for the nearFadeCutoff property.
 */
INLINE float SpriteParticleRenderer::nearFadeCutoff() const
{
	return nearFadeCutoff_;
}

/**
 *	Get-Accessor for the nearFadeStart property.
 *	Distance at which fading is full opacity and starts fading towards near cutoff
 *
 *	@return	The current setting for the nearFadeStart property.
 */
INLINE float SpriteParticleRenderer::nearFadeStart() const
{
	return nearFadeStart_;
}

/**
 *	Get-Accessor for the nearFadeFalloff property.
 *	Falloff curve power, 1 is linear. Higher values increase fade rate.
 *
 *	@return	The current setting for the nearFadeFalloff property.
 */
INLINE float SpriteParticleRenderer::nearFadeFalloffPower() const
{
	return nearFadeFalloffPower_;
}

/**
 *	Set-Accessor for if the nearFadeFalloff property.
 *	Falloff curve power, 1 is linear. Higher values increase fade rate.
 *
 *	@param value	The new value for the nearFadeFalloff property.
 */
INLINE void SpriteParticleRenderer::nearFadeFalloffPower( float value )
{
	nearFadeFalloffPower_ = value;
	materialSettingsChanged_ = true;
}

