#include "pch.hpp"

#include "force_psa.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"
#include "vector_generator.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "force_psa.ipp"
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

void ForcePSA::execute( ParticleSystem &particleSystem, float dTime )
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
void ForcePSA::lateExecute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( ForcePSA_execute );

	Vector3 scaledVector( vector_ * dTime );

	Particles::iterator current = particleSystem.begin();
	Particles::iterator endOfParticles = particleSystem.end();
	while ( current != endOfParticles )
	{
		Particle &particle = *current++;
		if (particle.isAlive())
		{
			Vector3 velocity;
			particle.getVelocity( velocity );
			particle.setVelocity( velocity + scaledVector );
		}
	}
}


/**
 *	This is the serialiser for ForcePSA properties
 */
template < typename Serialiser >
void ForcePSA::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, ForcePSA, vector );
}


void ForcePSA::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< ForcePSA >( pSect, this ) );
}


void ForcePSA::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< ForcePSA >( pSect, this ) );
}


ParticleSystemActionPtr ForcePSA::clone() const
{
	BW_GUARD;
	ForcePSA * psa = new ForcePSA();
	ParticleSystemAction::clone( psa );
	this->serialise( BW::CloneObject< ForcePSA >( this, psa ) );
	return psa;
}


// -----------------------------------------------------------------------------
// Section: Python Interface to the PyForcePSA.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyForcePSA )

/*~ function Pixie.ForcePSA
 *	Factory function to create and return a new PyForcePSA object. ForcePSA is a
 *	ParticleSystemAction that applies a constant acceleration to particles in a
 *	particular direction.
 *	@return A new PyForcePSA object.
 */
PY_FACTORY_NAMED( PyForcePSA, "ForcePSA", Pixie )

PY_BEGIN_METHODS( PyForcePSA )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyForcePSA )
	/*~ attribute PyForcePSA.vector
	 *	This is a vector describing the direction and magnitude of the
	 *	acceleration to be applied to the particles. The default value
	 *	is (0.0, 0.0, 0.0).
	 *	@type Sequence of 3 floats.
	 */
	PY_ATTRIBUTE( vector )
PY_END_ATTRIBUTES()


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be a vector3 in python.
 */
PyObject *PyForcePSA::pyNew( PyObject *args )
{
	BW_GUARD;
	Vector3 newVector( 0.0f, 0.0f, 0.0f );

	if ( PyTuple_Size( args ) > 0 )
	{
		if ( Script::setData( PyTuple_GetItem( args, 0 ), newVector,
			"ForcePSA() argument " ) != 0 )
		{
			return NULL;
		}
		else
		{
			ForcePSAPtr pAction = new ForcePSA( newVector );
			return new PyForcePSA(pAction);
		}
	}
	else
	{
		ForcePSAPtr pAction = new ForcePSA( newVector );
		return new PyForcePSA(pAction);
	}
}


PY_SCRIPT_CONVERTERS( PyForcePSA )

BW_END_NAMESPACE

// force_psa.cpp
