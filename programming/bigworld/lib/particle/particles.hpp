#ifndef PARTICLES_HPP
#define PARTICLES_HPP


#include "cstdmf/vectornodest.hpp"
#include "particle.hpp"


BW_BEGIN_NAMESPACE

/**
 *	The Particles container class is a virtual base class for containers
 *	of particles.  Depending on the particle system's requirements,
 *	different particle containers with different behaviours will be used. 
 *
 *	The capacity is managed by this base class because STL vectors and in
 *	particular the vectorNoDestructor do not shrink the capacity even if
 *	reserve is called with a smaller amount.  Particle Systems need to know
 *	the exact specified capacity, and not its underlying memory allocation.
 */
class Particles : public AVectorNoDestructor<Particle>, public ReferenceCount
{
public:
	Particles() :
	  capacity_( 0 )
	{};

	//don't need this yet, as no overriding classes allocate memory
	//virtual ~Particles();

	/// @name Particles methods.
	//@{
	virtual Particle* addParticle( const Particle &particle, bool isMesh ) = 0;
	virtual iterator removeParticle( iterator particle ) = 0;
	virtual size_t index( iterator particle ) = 0;
	//@}

	// The following methods hide the base container class' non-virtual
	// methods, and thus allow specialisation by derived classes.

	/// @name Particles base container hidden methods.
	//@{
	virtual void clear() = 0;
	virtual void reserve( size_t amount )
	{
		BASE_CONTAINER::reserve( amount );
		capacity_ = amount;
	}
	virtual size_t size()		{return BASE_CONTAINER::size();}
	virtual size_t size() const	{return BASE_CONTAINER::size();}
	virtual size_t capacity()	{return capacity_;}
	virtual size_t capacity() const	{return capacity_;}
	//@}


protected:
	typedef AVectorNoDestructor<Particle> BASE_CONTAINER;
	size_t	capacity_;
};

typedef SmartPointer<Particles>	ParticlesPtr;


/**
 *	This Particles container class minimises fragmentation in the container,
 *	by keeping the particles in a contiguous sequence.  It minimises iteration,
 *	insert and remove time but does not guarantee that particles remain in a
 *	fixed position in the array.  This is an efficient container that works in
 *	most cases.
 */
class ContiguousParticles : public Particles
{	
public:
	ContiguousParticles();	

	/// @name Particles methods.
	//@{
	Particle* addParticle( const Particle &particle, bool isMesh );	
	iterator removeParticle( iterator particle );
	size_t index(iterator particle );
	//@}

	/// @name Particles base container hidden methods.
	//@{
	void clear();	
	//@}

private:		
	uint16	 particleIdx_;	
};


/**
 *	This Particles container class gaurantees that once a particle is in the
 *	container, it will stay in the same location.  This may cause fragmentation
 *	in the container and thus iterate/add/remove are potentially slower than in
 *	the contiguous particles container.
 *
 *	Beware of when size() is much less than capacity(); causes slow iteration.
 *
 *	This container works for particle systems that rely on particles having a
 *	fixed location, for example Trail particles and Mesh particles.
 */
class FixedIndexParticles : public Particles
{	
public:
	FixedIndexParticles();

	/// @name Particles container methods.
	//@{
	Particle* addParticle( const Particle &particle, bool isMesh );	
	iterator removeParticle( iterator particle );
	size_t index(iterator particle);


	/// @name Particles base container hidden methods.
	//@{
	void clear();
	void reserve( size_t amount );
	size_t size()		{ return size_; }
	size_t size() const	{ return size_; }
	//@}	
private:
	uint16	next_;
	uint16	size_;	
};


#ifdef CODE_INLINE
#include "particles.ipp"
#endif

BW_END_NAMESPACE

#endif //PARTICLES_HPP
