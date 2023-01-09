#include "pch.hpp"

#include "sink_psa.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_overlapper.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "sink_psa.ipp"
#endif


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------

/**
 *	This method executes the action for the given frame of time. The dTime
 *	parameter is the time elapsed since the last call.
 *
 *	@param particleSystem	The particle system on which to operate.
 *	@param dTime			Elapsed time in seconds.
 */
void SinkPSA::execute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD;

	// Do nothing if the dTime passed was zero or if the particle system
	// is not quite old enough to be active.
	if ( ( age_ < delay() ) || ( dTime <= 0.0f ) )
	{
		age_ += dTime;
		return;
	}

	if (particleSystem.isLocal())
	{
		this->lateExecute( particleSystem, dTime );
	}
	else
	{
		ParticleSystemAction::pushLateUpdate( *this, particleSystem, dTime );
	}
}


/**
 *	This method executes the action for the given frame of time. The dTime
 *	parameter is the time elapsed since the last call.
 *
 *	@param particleSystem	The particle system on which to operate.
 *	@param dTime			Elapsed time in seconds.
 */
void SinkPSA::lateExecute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( SinkPSA_execute );

	if ( (maximumAge() >= 0.0f) || (minimumSpeed() >= 0.0f) || outsideOnly_ )
	{
		bool remove = false;
		float minSpdSqrd = (minimumSpeed() >= 0 ) ? minimumSpeed() * minimumSpeed() : -1.f;

		float sz = 0.01f;
		if (particleSystem.pRenderer() && particleSystem.pRenderer()->knowParticleRadius())
		{
			sz = particleSystem.pRenderer()->particleRadius();
		}

		if ( outsideOnly_ )
		{
			BoundingBox bb;
			particleSystem.worldVisibilityBoundingBox(bb);
			cacheHullInfo(bb);
		}

		//note : can't save the end() during iteration because particles
		//may be erased, thus moving the end iterator. ( i.e. this is ok )
		Particles::iterator current = particleSystem.begin();
		while ( current != particleSystem.end() )
		{
			Particle &particle = *current;
			remove = false;

			if ( particle.isAlive() )
			{
				Vector3 velocity;
				particle.getVelocity( velocity );

				if ((particle.age() > maximumAge()) ||
					(velocity.lengthSquared() < minSpdSqrd))
				{
					remove = true;
				}
				else if (outsideOnly_ && !shells_.empty())
				{
					//note - particles get moved after the actions, so find out
					//where the particle will be before checking if its inside.
					Vector3 newPos;
					particleSystem.predictPosition( particle, dTime, newPos );

					if (particleSystem.isLocal())
					{
						newPos = particleSystem.worldTransform().applyPoint( newPos );
					}

					if (!particleSystem.pRenderer()->isMeshStyle())
					{						
						//'size' of sprite particles is its radius, however since
						//they can rotate, we need to get the length of the
						//hypotenuse.  They're square and so times by root 2.
						const float root2 = 1.415f;
						float radius = particle.size() * root2;
						remove = this->isIndoors( newPos, radius );
					}
					else
					{	
						//'size' of mesh particles is calculated by the renderer,
						//and has already been adjusted to be the length of the
						//hypotenuse of the bounding box.
						//(see base mesh particle renderer)
						remove = this->isIndoors( newPos, sz );
					}					
				}			
			}

			if (remove)
			{
				if (!particleSystem.forcingSave())
				{
					particleSystem.removeFlareID(
						ParticleSystem::getUniqueParticleID( current,
						particleSystem ) );
				}
				current = particleSystem.removeParticle( current );				
			}
			else
			{
				++current;
			}
		}

		if (outsideOnly_)
		{
			clearHullInfo();
		}
	}
}


/**
 *	This method accumulates all overlapping shells for the given outdoor chunk
 *	that overlap with the given bounding box into the shells_ member.
 */
void SinkPSA::accumulateOverlappers( const BoundingBox & bb,
	Chunk * outsideChunk )
{
	if (ChunkOverlappers::instance.exists( *outsideChunk ))
	{
		const ChunkOverlappers & o = ChunkOverlappers::instance( *outsideChunk );
		const ChunkOverlappers::Overlappers & overlappers = o.overlappers();

		for (uint i = 0; i < overlappers.size(); i++)
		{
			const ChunkOverlapper * pOverlapper = &*overlappers[i];
			if (pOverlapper->focussed())
			{
				if (pOverlapper->bb().intersects( bb ))
				{
					shells_.insert( pOverlapper->pOverlapper() );
				}
			}
		}
	}
}


/**
 *	This method finds all indoor chunks that overlap with the given bounding
 *	box, and stores them in the shells_ member.
 */
void SinkPSA::cacheHullInfo( const BoundingBox& wvbb )
{
	if ( wvbb.insideOut() )
	{
		return;
	}

	ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
	if (!pSpace)
	{
		return;
	}

	//Find all columns intersecting the particle system
	BaseChunkSpace::Column* c1 = pSpace->column( Vector3(wvbb.minBounds().x, 0.f, wvbb.minBounds().z), false );
	BaseChunkSpace::Column* c2 = pSpace->column( Vector3(wvbb.minBounds().x, 0.f, wvbb.maxBounds().z), false );
	BaseChunkSpace::Column* c3 = pSpace->column( Vector3(wvbb.maxBounds().x, 0.f, wvbb.minBounds().z), false );
	BaseChunkSpace::Column* c4 = pSpace->column( Vector3(wvbb.maxBounds().x, 0.f, wvbb.maxBounds().z), false );

	//Find all shells within these columns
	if (c1 && c1->pOutsideChunk())	accumulateOverlappers( wvbb, c1->pOutsideChunk() );
	if (c2 && c2->pOutsideChunk())	accumulateOverlappers( wvbb, c2->pOutsideChunk() );
	if (c3 && c3->pOutsideChunk())	accumulateOverlappers( wvbb, c3->pOutsideChunk() );
	if (c4 && c4->pOutsideChunk())	accumulateOverlappers( wvbb, c4->pOutsideChunk() );
}


void SinkPSA::clearHullInfo()
{
	shells_.clear();
}


/**
 *	This is the serialiser for SinkPSA properties
 */
template < typename Serialiser >
void SinkPSA::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, SinkPSA, maximumAge );
	BW_PARITCLE_SERIALISE( s, SinkPSA, minimumSpeed );
	BW_PARITCLE_SERIALISE( s, SinkPSA, outsideOnly );
}


void SinkPSA::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< SinkPSA >( pSect, this ) );
}


void SinkPSA::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< SinkPSA >( pSect, this ) );
}


ParticleSystemActionPtr SinkPSA::clone() const
{
	BW_GUARD;
	SinkPSA * psa = new SinkPSA();
	ParticleSystemAction::clone( psa );
	this->serialise( BW::CloneObject< SinkPSA >( this, psa ) );
	return psa;
}


/**
 *	This method is a private helper method. It finds whether
 *	the box centred on pos, with radius radius, intersects an
 *	indoor area. It uses the cached list of shells that overlap
 *	with the bounding box of the particle system as a whole, and
 *	performs a containment test on each.
 */
bool SinkPSA::isIndoors( const Vector3& pos, float radius ) const
{
	static DogWatch isIndoorsWatch( "isIndoors" );
	ScopedDogWatch sdw( isIndoorsWatch );
	BW_GUARD;

	BW::set<Chunk*>::const_iterator it = shells_.begin();
	while ( it != shells_.end() )
	{
		const Chunk* c = *it;
		if ( c->contains(pos,radius) )
		{
			return true;
		}
		++it;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Section: Python Interface to the PySinkPSA.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PySinkPSA )


/*~ function Pixie.SinkPSA
 *	Factory function to create and return a new PySinkPSA object. SinkPSA is a
 *	ParticleSystemAction that destroys particles within a ParticleSystem. 
 *	@return A new PySinkPSA object.
 */
PY_FACTORY_NAMED( PySinkPSA, "SinkPSA", Pixie )

PY_BEGIN_METHODS( PySinkPSA )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PySinkPSA )
	/*~ attribute PySinkPSA.maximumAge
	 *	Particles whose time in existence is greater then maximumAge are
	 *	removed from the ParticleSystem. Default -1.0. This attribute is ignored
	 *	if value is < 0.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( maximumAge )
	/*~ attribute PySinkPSA.minimumSpeed
	 *	Particles with a speed of less than minimumSpeed are removed from the
	 *	ParticleSystem. Default -1.0. This attribute is ignored if value is < 0.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( minimumSpeed )
PY_END_ATTRIBUTES()


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be two floats - maximumAge and minimumSpeed.
 */
PyObject *PySinkPSA::pyNew( PyObject *args )
{
	BW_GUARD;
	float maximumAge = -1.0f;
	float minimumSpeed = -1.0f;

	if ( PyArg_ParseTuple( args, "|ff", &maximumAge, &minimumSpeed ) )
	{
		SinkPSAPtr pAction = new SinkPSA( maximumAge, minimumSpeed );
		return new PySinkPSA(pAction);
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "SinkPSA:"
			"Expected two numbers." );
		return NULL;
	}
}


PY_SCRIPT_CONVERTERS( PySinkPSA )

BW_END_NAMESPACE

// sink_psa.cpp
