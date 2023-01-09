#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for ParticleSystemRenderer.
// -----------------------------------------------------------------------------

/**
 *	This is the constructor for ParticleSystemRenderer. 
 */
INLINE ParticleSystemRenderer::ParticleSystemRenderer() :	
	viewDependent_( false ),
	local_( false )
{
}

/**
 *	This is the destructor for ParticleSystemRenderer.
 */
INLINE ParticleSystemRenderer:: ~ParticleSystemRenderer()
{
}


// -----------------------------------------------------------------------------
// Section: Renderer Interface methods.
// -----------------------------------------------------------------------------

/**
 *	This is the Get-Accessor for the viewDependent property. If viewDependent
 *	is set to true, the coordinates of the particle system are in camera
 *	space rather than world space.
 *
 *	@return	The current status of the viewDependent flag.
 */
INLINE bool ParticleSystemRenderer::viewDependent() const
{
	return viewDependent_;
}

/**
 *	This is the Set-Accessor for the viewDependent property.
 *
 *	@param flag		The new value for the viewDependent property.
 */
INLINE void ParticleSystemRenderer::viewDependent( bool flag )
{
	viewDependent_ = flag;
	if ( viewDependent_ )
		local_ = false;
}


/**
 *	This is the Get-Accessor for the local property. If local
 *	is set to true, the coordinates of the particle system are in
 *	local space rather than world space.
 *
 *	@return	The current status of the local flag.
 */
INLINE bool ParticleSystemRenderer::local() const
{
	return local_;
}

/**
 *	This is the Set-Accessor for the local property.
 *
 *	@param flag		The new value for the local property.
 */
INLINE void ParticleSystemRenderer::local( bool flag )
{
	local_ = flag;
	if ( local_ )
		viewDependent_ = false;
}

// particle_system_renderer.ipp
