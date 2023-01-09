#include "pch.hpp"

#include "node_clamp_psa.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "node_clamp_psa.ipp"
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

void NodeClampPSA::execute( ParticleSystem &particleSystem, float dTime )
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
void NodeClampPSA::lateExecute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( NodeClampPSA_execute );

	// Calculate the object-to-world transform.
	Matrix objectToWorld = particleSystem.objectToWorld();

	// Get the position in world space of the particle system.
	Vector3 positionOfPS = objectToWorld.applyToOrigin();

	if ( fullyClamp_ )
	{
		Particles::iterator current = particleSystem.begin();
		Particles::iterator endOfParticles = particleSystem.end();
		while ( current != endOfParticles )
		{
			Particle &particle = *current++;
			if (particle.isAlive())
				particle.position() = positionOfPS;
		}
	}
	else
	{
		// Calculate our delta position..
		Vector3 displacement = positionOfPS - ( firstUpdate_ ? positionOfPS :
			lastPositionOfPS_ );


		// Move the particles accordingly if the particle system has moved.
		// Drones and leaders are affected by this action.
		if ( displacement.lengthSquared() > 0.0f )
		{
			Particles::iterator current = particleSystem.begin();
			Particles::iterator endOfParticles = particleSystem.end();
			while ( current != endOfParticles )
			{
				Particle &particle = *current++;
				if (particle.isAlive())
					particle.position() += displacement;
			}
		}

		firstUpdate_ = false;
		lastPositionOfPS_ = positionOfPS;
	}	
}


/**
 *	This is the serialiser for NodeClampPSA properties
 */
template < typename Serialiser >
void NodeClampPSA::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, NodeClampPSA, fullyClamp );
}


void NodeClampPSA::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< NodeClampPSA >( pSect, this ) );
}


void NodeClampPSA::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< NodeClampPSA >( pSect, this ) );
}


ParticleSystemActionPtr NodeClampPSA::clone() const
{
	BW_GUARD;
	NodeClampPSA * psa = new NodeClampPSA();
	ParticleSystemAction::clone( psa );
	this->serialise( BW::CloneObject< NodeClampPSA >( this, psa ) );
	return psa;
}

// -----------------------------------------------------------------------------
// Section: Python Interface to the PyNodeClampPSA.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyNodeClampPSA )

/*~ function Pixie.NodeClampPSA
 *	Factory function to create and return a new PyNodeClampPSA object.
 *	NodeClampPSA is a ParticleSystemAction that locks a particle to a Node.
 *	@return A new PyNodeClampPSA object.
 */
PY_FACTORY_NAMED( PyNodeClampPSA, "NodeClampPSA", Pixie )

PY_BEGIN_METHODS( PyNodeClampPSA )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyNodeClampPSA )
	/*~ attribute PyNodeClampPSA.fullyClamp
	 *	If set to true, the particles are clamped directly to the particle system
	 *	position.  If false, the particles are moved relatively each frame by the
	 *	amount the particle system moved by.
	 *
	 *	@type Bool
	 */
	PY_ATTRIBUTE( fullyClamp )
PY_END_ATTRIBUTES()


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python. The NodeClamp
 *					action takes no parameters and ignores any passed to this
 *					method.
 */
PyObject *PyNodeClampPSA::pyNew( PyObject *args )
{
	BW_GUARD;
	NodeClampPSAPtr pAction = new NodeClampPSA;
	return new PyNodeClampPSA(pAction);
}


PY_SCRIPT_CONVERTERS( PyNodeClampPSA )

BW_END_NAMESPACE

// node_clamp_psa.cpp

