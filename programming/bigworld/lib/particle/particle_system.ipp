#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: Accessors to Particle System Properties.
// -----------------------------------------------------------------------------

/**
 *	This is the Get-Accessor for the capacity property. This is the maximum
 *	number of particles in the system. We use a variable to store the capacity
 *	because the vector template only guarantees minimum size, so the actual
 *	capacity may be higher than the value we specified.
 *
 *	@return	The maximum number of particles allowed.
 */
INLINE int ParticleSystem::capacity( void ) const
{
	if (particles_)
		return static_cast<int>(particles_->capacity());
	else
		return 0;
}

/**
 *	This is the Get-Accessor for the size property. This is the current number
 *	of particles in the system. There is no set accessor for this property -
 *	it is read-only. We can use the size() method of the vector template for
 *	this as it is accurate.
 *
 *	@return The current number of particles in the system.
 */
INLINE int ParticleSystem::size( void ) const
{
	if (particles_)
		return static_cast<int>(particles_->size());
	else
		return 0;
}

/**
 *	This is the Get-Accessor for the particle system's wind factor.  If this
 *  value is zero then the particle is not affected by the window, if it is
 *  one then the velocity used is the velocity of the particle plus the
 *  velocity of the wind.
 *
 *	Infinity is represented by the negative number.
 *
 *	@return	The current value of the particle system's windFactor.
 */
INLINE float ParticleSystem::windFactor( void ) const
{
	return windFactor_;
}

/**
 *	This is the Get-Accessor for the particle system's wind factor
 *	property.
 *
 *	@param ratio	The new value of the particle system's wind factor.
 */
INLINE void ParticleSystem::windFactor( float ratio )
{
	windFactor_ = ratio;
}

/**
 *	This is the Get-Accessor for the particle system's preview visible flag.
 *  If this flag is set then the particle system will not be rendered as part
 *  of the meta particle system.
 *
 *	@return	The current value of the particle system's visible flag.
 */
INLINE bool ParticleSystem::enabled( void ) const
{
	return enabled_;
}


/**
 *	This is the Get-Accessor for the particle system's preview visible
 *	property.
 *
 *	@param state	The new value of the particle system's visible flag.
 */
INLINE void ParticleSystem::enabled( bool state )
{
	enabled_ = state;
}


/**
 *	This is the Get-Accessor for the particle system's renderer.
 *
 *	@return A pointer to the particle system's renderer.
 */
INLINE ParticleSystemRendererPtr ParticleSystem::pRenderer( void ) const
{
	return pRenderer_;
}


// -----------------------------------------------------------------------------
// Section: General Operations on Particle Systems.
// -----------------------------------------------------------------------------

/**
 *	This method adds a newly created particle to the particle system.  It will 
 *  not be added if adding the particle pushes the particle population beyond 
 *  the capacity of the system.
 *
 *	@param particle		The new particle to be added.
 *	@param isMesh		Boolean indicating whether the particles to add are a mesh.
 */
INLINE void ParticleSystem::addParticle( const Particle & particle,
	bool isMesh )
{
	BW_GUARD;
	if ( size() < capacity() && particles_.getObject() )
	{
		particles_->addParticle( particle, isMesh );		

		//this assumes the particle has the correct position
		//when it is added.  This may not be a good assumption.
		//checked out ok 17Mar2002
		//note : static systems only grow their bounding boxes.
		if  ( particles_->size() == 1 && !static_ )
		{
			boundingBox_.setBounds( particle.position() - Vector3(0.1f,0.1f,0.1f),
									particle.position() + Vector3(0.1f,0.1f,0.1f) );
		}
		else
		{
			boundingBox_.addBounds( particle.position() );
		}
	}
}


/**
 *	This method removes a particle from the particle system. The particle
 *	to be removed must be referenced via an iterator.
 *
 *	@param particle		The iterator to the particle to be removed.
 */
INLINE Particles::iterator ParticleSystem::removeParticle(
		Particles::iterator particle )
{
	BW_GUARD;
	//Code error.  can't remove particles and certainly shouldn't have been
	//able to add them in the first place if we have no particles storage.
	IF_NOT_MF_ASSERT_DEV( particles_.getObject() )
	{
		MF_EXIT( "container is NULL" );
	}
	
	return particles_->removeParticle( particle );
}

/**
 *	This is the Get-Accessor for the ground specifier of the particle
 *	system. The ground specifier is actually a romp-collider. It is used to
 *	determine collision for the particle system.
 *
 *	@return A smart pointer to the ground specifer.
 */
INLINE RompColliderPtr ParticleSystem::groundSpecifier( void ) const
{
	return pGS_;
}


// -----------------------------------------------------------------------------
// Section: Direct Accessors to Particles.
// -----------------------------------------------------------------------------

/**
 *	This is the accessor to the start of the vector of particles owned by the
 *	system.
 *
 *	@return	An iterator for the particles beginning at the start.
 */
INLINE Particles::iterator ParticleSystem::begin( void )
{
	BW_GUARD;
	//Code error.  Must check that particles_ is not NULL before using
	IF_NOT_MF_ASSERT_DEV( particles_.getObject() )
	{
		MF_EXIT( "container is NULL" );
	}
	
	return particles_->begin();
}

/**
 *	This is the const-accessor to the start of the vector of particles owned
 *	by the system.
 *
 *	@return	A const iterator for the particles beginning at the start.
 */
INLINE Particles::const_iterator ParticleSystem::begin( void ) const
{
	BW_GUARD;
	//Code error.  Must check that particles_ is not NULL before using
	IF_NOT_MF_ASSERT_DEV( particles_.getObject() )
	{
		MF_EXIT( "container is NULL" );
	}
	
	return particles_->begin();
}

/**
 *	This is the accessor to the end of the vector of particles owned by the
 *	system.
 *
 *	@return	An iterator for the particles that is past the end of the vector.
 */
INLINE Particles::iterator ParticleSystem::end( void )
{
	BW_GUARD;
	//Code error.  Must check that particles_ is not NULL before using
	IF_NOT_MF_ASSERT_DEV( particles_.getObject() )
	{
		MF_EXIT( "container is NULL" );
	}
	
	return particles_->end();
}

/**
 *	This is the const-accessor to the end of the vector of particles owned
 *	by the system.
 *
 *	@return	A const iterator for the particles that is past the end of the
 *			vector.
 */
INLINE Particles::const_iterator ParticleSystem::end( void ) const
{
	BW_GUARD;
	//Code error.  Must check that particles_ is not NULL before using
	IF_NOT_MF_ASSERT_DEV( particles_.getObject() )
	{
		MF_EXIT( "container is NULL" );
	}
	
	return particles_->end();
}


/**
 *	This method returns the bounding box of the particle system.
 *
 *	@return	BoundingBox	The bounding box of the particle system.
 */
INLINE const BoundingBox& ParticleSystem::boundingBox() const
{
	return boundingBox_;
}


/**
 *	This method sets the explicit position of the particle system.
 *	After calling this method, the particle system no longer uses
 *	the world space given to it; the system is always at this location.
 */
INLINE void ParticleSystem::explicitPosition( const Vector3& worldPos )
{
	BW_GUARD;
	explicitTransform_ = true;

	explicitPosition_ = worldPos;
	if ( pOwnWorld_ != NULL )
	{
		Matrix tr;
		tr.setTranslate( worldPos );
		pOwnWorld_->setMatrix(tr);		
	}
}


/**
 *	This method gets the explicit position of the particle system.
 */
INLINE const Vector3& ParticleSystem::explicitPosition() const
{
	return explicitPosition_;
}


/**
 *	This method sets the explicit position of the particle system.
 *	After calling this method, the particle system no longer uses
 *	the world space given to it; the system is always at this location.
 */
INLINE void ParticleSystem::explicitDirection( const Vector3& dir )
{
	explicitTransform_ = true;
	explicitDirection_ = dir;
}


/**
 *	This method gets the explicit position of the particle system.
 */
INLINE const Vector3& ParticleSystem::explicitDirection() const
{
	return explicitDirection_;
}


INLINE void ParticleSystem::explicitTransform( bool b )
{
	explicitTransform_ = b;
}


INLINE bool ParticleSystem::explicitTransform() const
{
	return explicitTransform_;
}


/**
 *	This method sets the maxLod parameter
 */
INLINE void ParticleSystem::maxLod(float maxLod)
{
	maxLod_ = maxLod;
}

/**
 *	This method gets the explicit position of the particle system.
 */
INLINE float ParticleSystem::maxLod() const
{
	return maxLod_;
}


/**
 *  This private method returns whether or not the particle
 *  system uses local-space particles; this affects how we
 *  calculate our bounding box for example.
 */

INLINE bool ParticleSystem::isLocal() const
{
	bool isLocal = (pOwnWorld_ == NULL);
	isLocal |= (this->pRenderer() && this->pRenderer()->local());
	return isLocal;
}

/* particle_system.ipp */
