#include "pch.hpp"

#include "stream_psa.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "stream_psa.ipp"
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
void StreamPSA::execute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( StreamPSA_execute );

	// Do nothing if the dTime passed was zero or if the particle system
	// is not quite old enough to be active.
	if ( ( age_ < delay() ) || ( dTime <= 0.0f ) )
	{
		age_ += dTime;
		return;
	}

	// Pre-calculate our decay multiplier.
	// As dTime approaches zero, decay becomes 1.0, as it approaches infinity,
	// decay becomes zero.
	float decay;
	if ( halfLife_ < 0.0f )
	{
		decay = 1.0f;
	}
	else if ( halfLife_ == 0.0f )
	{
		decay = 0.0f;
	}
	else
	{
		decay = powf( 0.5f, dTime / halfLife_ );
	}

	Particles::iterator current = particleSystem.begin();
	Particles::iterator endOfParticles = particleSystem.end();
	while (current != endOfParticles)
	{
		Particle & particle = *current++;
		if (particle.isAlive())
		{
			Vector3 velocity;
			particle.getVelocity( velocity );
			particle.setVelocity( vector_ + decay * (velocity - vector_) );
		}
	}
}


/**
 *	This is the serialiser for StreamPSA properties
 */
template < typename Serialiser >
void StreamPSA::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, StreamPSA, vector );
	BW_PARITCLE_SERIALISE( s, StreamPSA, halfLife );
}


void StreamPSA::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< StreamPSA >( pSect, this ) );
}


void StreamPSA::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< StreamPSA >( pSect, this ) );
}


ParticleSystemActionPtr StreamPSA::clone() const
{
	BW_GUARD;
	StreamPSA * psa = new StreamPSA();
	ParticleSystemAction::clone( psa );
	this->serialise( BW::CloneObject< StreamPSA >( this, psa ) );
	return psa;
}


// -----------------------------------------------------------------------------
// Section: Python Interface to the PyStreamPSA.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyStreamPSA )

/*~ function Pixie.StreamPSA
 *	Factory function to create and return a new PyStreamPSA object. StreamPSA is a
 *	ParticleSystemAction the converges particles to the velocity of the stream.
 *	@return A new PyStreamPSA object.
 */
PY_FACTORY_NAMED( PyStreamPSA, "StreamPSA", Pixie )

PY_BEGIN_METHODS( PyStreamPSA )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyStreamPSA )
	/*~ attribute PyStreamPSA.vector
	 *	Vector describing the direction and magnitude of the stream velocity to 
	 *	which particle velocity is to be converged. Default is
	 *	( 0.0, 0.0, 0.0 ).
	 *	@type Sequence of 3 floats.
	 */
	PY_ATTRIBUTE( vector )
	/*~ attribute PyStreamPSA.halfLife
	 *	Half-life of convergence for the particles velocity to the stream 
	 *	velocity in seconds. Default is -1.0. Values should be < 0 for no 
	 *	effect, and >= 0 to have particles converge. 0 indicates instant
	 *	convergence to the stream velocity.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( halfLife )
PY_END_ATTRIBUTES()


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be a vector3 and a float in python.
 */
PyObject *PyStreamPSA::pyNew( PyObject *args )
{
	BW_GUARD;
	Vector3 newVector( 0.0f, 0.0f, 0.0f );
	float halfLife = 3600.0f;

	if ( PyTuple_Size( args ) > 0 )
	{
		if ( Script::setData( PyTuple_GetItem( args, 0 ), newVector,
			"StreamPSA() first argument" ) != 0 )
		{
			return NULL;
		}

		if ( PyTuple_Size( args ) > 1 )
		{
			if ( Script::setData( PyTuple_GetItem( args, 1 ), halfLife,
				"StreamPSA() second argument" ) != 0 )
			{
				return NULL;
			}
			else
			{
				StreamPSAPtr pAction = new StreamPSA( newVector, halfLife );
				return new PyStreamPSA(pAction);
			}
		}
		else
		{
			StreamPSAPtr pAction = new StreamPSA( newVector, halfLife );
			return new PyStreamPSA(pAction);
		}
	}
	else
	{
		StreamPSAPtr pAction = new StreamPSA;
		return new PyStreamPSA(pAction);
	}
}


PY_SCRIPT_CONVERTERS( PyStreamPSA )

BW_END_NAMESPACE

// stream_psa.cpp
