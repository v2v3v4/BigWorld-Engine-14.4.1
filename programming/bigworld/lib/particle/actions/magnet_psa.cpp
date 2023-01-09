#include "pch.hpp"

#include "magnet_psa.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"
#include "particle/renderers/particle_system_renderer.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "magnet_psa.ipp"
#endif


/**
 *	This method executes the action for the given frame of time. The dTime
 *	parameter is the time elapsed since the last call.
 *
 *	@param particleSystem	The particle system on which to operate.
 *	@param dTime			Elapsed time in seconds.
 */
void MagnetPSA::execute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD;

	// Do nothing if the dTime passed was zero or if the particle system
	// is not quite old enough to be active.
	if ( ( age_ < delay() ) || ( dTime <= 0.0f ) )
	{
		age_ += dTime;
		return;
	}

	if (!source_.hasObject() && particleSystem.isLocal())
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
void MagnetPSA::lateExecute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( MagnetPSA_execute );

	// If there is no source matrix provider, use the origin of the 
	// particle system itself.
	Vector3 origin;
	if ( source_ )
	{
		Matrix m;
		source_->matrix(m);
		origin = m.applyToOrigin();
	}
	else
	{
		if ( !particleSystem.pRenderer() || !particleSystem.pRenderer()->local() )
		{
			origin = particleSystem.worldTransform().applyToOrigin();
		}
		else
		{
			origin.set(0,0,0);
		}
	}

	Particles::iterator current = particleSystem.begin();
	Particles::iterator endOfParticles = particleSystem.end();
	while ( current != endOfParticles )
	{
		Particle & particle = *current++;

		if (particle.isAlive())
		{
			// Here we go: Apply acceleration to velocity based on magnet position.
			Vector3 dv (origin-particle.position());
			float separation = max(dv.length(), minDist_);
			dv /= separation;
			float factor = (dTime * strength_) / separation;
			Vector3 velocity;
			particle.getVelocity( velocity );
			particle.setVelocity( velocity + (dv * factor) );
		}
	}
}


/**
 *	This is the serialiser for MagnetPSA properties
 */
template < typename Serialiser >
void MagnetPSA::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, MagnetPSA, strength );
	BW_PARITCLE_SERIALISE( s, MagnetPSA, minDist );
}


void MagnetPSA::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< MagnetPSA >( pSect, this ) );
}


void MagnetPSA::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< MagnetPSA >( pSect, this ) );
}


ParticleSystemActionPtr MagnetPSA::clone() const
{
	BW_GUARD;
	MagnetPSA * psa = new MagnetPSA();
	ParticleSystemAction::clone( psa );
	this->serialise( BW::CloneObject< MagnetPSA >( this, psa ) );
	return psa;
}

// -----------------------------------------------------------------------------
// Section: Python Interface to the PyMagnetPSA.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyMagnetPSA )

/*~ function Pixie.MagnetPSA
 *
 *	Factory function to create and return a new PyMagnetPSA object.
 *	@return A new PyMagnetPSA object.
 */
PY_FACTORY_NAMED( PyMagnetPSA, "MagnetPSA", Pixie )

PY_BEGIN_METHODS( PyMagnetPSA )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyMagnetPSA )
	/*~ attribute PyMagnetPSA.strength
	 *	The strength attribute is applied as a scaling factor to force of
	 *	attraction between particles, and the MagnetPSA's source. Default value
	 *	is 1.0.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( strength )
	/*~ attribute PyMagnetPSA.source
	 *	Source is a MatrixProvider specifying the point to which particles
	 *	are attracted. Default value is the origin of the ParticleSystem to
	 *	which this ParticleSystemAction is attached.
	 *	@type MatrixProvider.
	 */
	PY_ATTRIBUTE( source )
	/*~ attribute PyMagnetPSA.minDist
	 *	minDist exists to avoid singularity type effects when a particle
	 *	approaches the source of the attraction. It is used when calculating
	 *	attraction instead of the particles distance to the source, when the
	 *	particles distance to the source < minDist. Default value is 1.0.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( minDist )
PY_END_ATTRIBUTES()


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be a vector3 in python.
 */
PyObject *PyMagnetPSA::pyNew( PyObject *args )
{
	BW_GUARD;
	float  s;

	if ( PyTuple_Size( args ) > 0 )
	{
		if ( Script::setData( PyTuple_GetItem( args, 0 ), s,
			"MagnetPSA() argument " ) != 0 )
		{
			return NULL;
		}
		else
		{
			MagnetPSAPtr pAction = new MagnetPSA( s );
			return new PyMagnetPSA(pAction);
		}
	}
	else
	{
		MagnetPSAPtr pAction = new MagnetPSA();
		return new PyMagnetPSA(pAction);
	}
}


PY_SCRIPT_CONVERTERS( PyMagnetPSA )

BW_END_NAMESPACE

// magnet_psa.cpp
