#include "pch.hpp"

#include "scaler_psa.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "scaler_psa.ipp"
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
void ScalerPSA::execute( ParticleSystem &particleSystem, float dTime )
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
void ScalerPSA::lateExecute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( ScalerPSA_execute );

	Particles::iterator current = particleSystem.begin();
	Particles::iterator endOfParticles = particleSystem.end();
	while (current != endOfParticles)
	{
		Particle & particle = *current;

		if (particle.isAlive())
		{
			float size = particle.size();

			if ( size > size_ )
			{
				size -= rate_ * dTime;
				if ( size < size_ )
				{
					size = size_;
				}
			}
			else if ( size < size_ )
			{
				size += rate_ * dTime;
				if ( size > size_ )
				{
					size = size_;
				}
			}

			particle.size( size );
		}

		++current;
	}
}


/**
 *	This is the serialiser for ScalerPSA properties
 */
template < typename Serialiser >
void ScalerPSA::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, ScalerPSA, size );
	BW_PARITCLE_SERIALISE( s, ScalerPSA, rate );
}


void ScalerPSA::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< ScalerPSA >( pSect, this ) );
}


void ScalerPSA::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< ScalerPSA >( pSect, this ) );
}


ParticleSystemActionPtr ScalerPSA::clone() const
{
	BW_GUARD;
	ScalerPSA * psa = new ScalerPSA();
	ParticleSystemAction::clone( psa );
	this->serialise( BW::CloneObject< ScalerPSA >( this, psa ) );
	return psa;
}


// -----------------------------------------------------------------------------
// Section: Python Interface to the PyScalerPSA.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyScalerPSA )

/*~ function Pixie.ScalerPSA
 *	Factory function to create and return a new PyScalerPSA object. ScalarPSA is a
 *	ParticleSystemAction that scales particle size over time.
 *	@return A new PyScalerPSA object.
 */
PY_FACTORY_NAMED( PyScalerPSA, "ScalerPSA", Pixie )

PY_BEGIN_METHODS( PyScalerPSA )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyScalerPSA )
	/*~ attribute PyScalerPSA.size
	 *	Eventual size of the particles as a scaling factor. It's effect is
	 *	dependant upon the ParticleSystemRenderer. For a SpriteParticleRenderer
	 *	the size is applied to the width and height of the sprites. Particles
	 *	are scaled up or down linearly over time to reach this value.
	 *	Default is 0.0.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( size )
	/*~ attribute PyScalerPSA.rate
	 *	Rate per second at which particles scale towards the value size.
	 *	Default is 0.0.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( rate )
PY_END_ATTRIBUTES()


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be two floats ( size and rate.) Both arguments are
 *					optional.
 */
PyObject *PyScalerPSA::pyNew( PyObject *args )
{
	BW_GUARD;
	float newSize = 1.0;
	float newRate = 0.0;

	if ( PyArg_ParseTuple( args, "|ff", &newSize, &newRate ) )
	{
		ScalerPSAPtr pAction = new ScalerPSA( newSize, newRate );
		return new PyScalerPSA(pAction);
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "ScalerPSA:"
			"Expected two lists describing vector generators." );
		return NULL;
	}
}


PY_SCRIPT_CONVERTERS( PyScalerPSA )


BW_END_NAMESPACE

// scaler_psa.cpp
