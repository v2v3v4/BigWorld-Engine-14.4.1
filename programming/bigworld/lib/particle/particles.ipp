#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


/**
 *	Constructor.
 */
INLINE ContiguousParticles::ContiguousParticles():
	particleIdx_( 0 )
{
}


/**
 *	This method clears the container of particles, leaving its capacity
 *	unchanged.
 */
INLINE void ContiguousParticles::clear()
{
	particleIdx_ = 0;
	BASE_CONTAINER::clear();
}


/**
 *	This method adds a newly created particle to the container.  It will 
 *  not be added if adding the particle pushes the particle population beyond 
 *  the capacity of the system.
 *
 *	@param particle		The new particle to be added.
 *	@param isMesh		Whether the new particle is mesh-style.
 *	@return particle	Pointer to the newly added particle.
 */
INLINE
Particle * ContiguousParticles::addParticle( const Particle & particle,
	bool isMesh )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( size() < capacity() )
	{
		MF_EXIT( "size >= capacity - possible memory corruption?" );
	}
	
	IF_NOT_MF_ASSERT_DEV( !isMesh )
	{
		MF_EXIT( "can't use ContiguousParticle containers with mesh particles" );
	}

	this->push_back( particle );
	Particle * p = &this->back();
	p->index( particleIdx_ );
	particleIdx_ = (particleIdx_ + 1) % capacity();

	return p;
}


/**
 *	This method removes a particle from the particle system. The particle
 *	to be removed must be referenced via an iterator.
 *
 *	@param particle		The iterator to the particle to be removed.
 *	@return iterator	The iterator to the next valid particle.
 */
INLINE
ContiguousParticles::iterator ContiguousParticles::removeParticle(
	ContiguousParticles::iterator particle )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( this->size() > 0 )
	{
		return end();
	}

	//doesn't matter if we copy over ourselves - this only happens if
	//we erase the last particle in the vector
	*particle = back();
	pop_back();

	return particle;
}


INLINE
size_t ContiguousParticles::index( Particles::iterator particle )
{
	return (*particle).index();
}


/**
 *	Constructor.
 */
INLINE FixedIndexParticles::FixedIndexParticles():
	size_(0),
	next_(0)
{
}


/**
 *	This method adds a newly created particle to the container.  It will 
 *  not be added if adding the particle pushes the particle population beyond
 *  the capacity of the system.
 *
 *	@param particle		The new particle to be added.
 *	@param isMesh		Whether the new particle is mesh-style.
 *	@return particle	Pointer to the newly added particle or NULL if limit is hit.
 */
INLINE
Particle * FixedIndexParticles::addParticle( const Particle & particle,
	bool isMesh )
{
	BW_GUARD;
	size_t available = BASE_CONTAINER::size();
	
	if (size_ < available)
	{
		Particles & store = *this;

		//TODO :  This search is O(n) worst-case.  Possibly fix by maintaining
		//a separate store of free particle indices (which has 2*capacity bytes
		//worst-case memory usage).
		//For constant sink-times, the search is always O(1)
		//For random sink-times, the worst case is O(n) but on average will much less
		while ( store[next_].isAlive() )
		{
			next_ = (next_ + 1) % available;
		}

		store[next_] = particle;
		Particle * p = &store[ next_ ];
		
		if (!isMesh)
		{
			p->index( next_ );
		}

		++size_;		
		next_ = (next_ + 1) % available;
		return p;
	}
	return NULL;
}


/**
 *	This method removes a particle from the particle system. The particle
 *	to be removed must be referenced via an iterator.
 *
 *	@param particle		The iterator to the particle to be removed.
 *	@return iterator	The iterator to the next valid particle.
 */
INLINE
FixedIndexParticles::iterator FixedIndexParticles::removeParticle(
	FixedIndexParticles::iterator particle )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( this->size() > 0 )
	{
		return end();
	}
	
	--size_;
	particle->kill();
	return ++particle;
}


INLINE
size_t FixedIndexParticles::index( Particles::iterator particle )
{
	return (particle - this->begin());
}


/**
 *	This method clears the container of particles, leaving its capacity
 *	unchanged.
 */
INLINE void FixedIndexParticles::clear()
{
	BW_GUARD;
	iterator it = BASE_CONTAINER::begin();
	iterator end = BASE_CONTAINER::end();
	while (it != end)
	{
		it->kill();
		++it;			
	}
	size_ = next_ = 0;
}


/**
 *	This method clears the container of particles, and changes its capacity
 *	to the new size.  For FixedIndexParticles, reserve acts more like a
 *	resize, all particles are now present and we use the kill() and isAlive()
 *	on particles to represent presence in the container or not.  However
 *	the container internally keeps track of it's own 'size', representing the
 *	number of live particles present.
 *
 *	@param	amount	The new size of the container.
 */
INLINE void FixedIndexParticles::reserve( size_t amount )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( amount > 0 )
	{
		return;
	}
	
	Particle p;
	BASE_CONTAINER::clear();
	Particles::reserve( amount );
	BASE_CONTAINER::resize( amount );
	size_ = next_ = 0;	
	IF_NOT_MF_ASSERT_DEV( amount == BASE_CONTAINER::size() )
	{
		MF_EXIT( "resize container failed" );
	}
}	
