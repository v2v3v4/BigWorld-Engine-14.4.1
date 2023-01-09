#include "pch.hpp"

#include "splat_psa.hpp"
#include "source_psa.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"
#include "romp/romp_collider.hpp"
#include "physics2/worldtri.hpp"
#include "pyscript/py_callback.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "splat_psa.ipp"
#endif


/**
 *	This method executes the action for the given frame of time. The dTime
 *	parameter is the time elapsed since the last call.
 *
 *	@param particleSystem	The particle system on which to operate.
 *	@param dTime			Elapsed time in seconds.
 */
void SplatPSA::execute( ParticleSystem & particleSystem, float dTime )
{
	BW_GUARD;

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
void SplatPSA::lateExecute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( SplatPSA_execute );

	const SourcePSA * pSource = static_cast< const SourcePSA * >(
		&*particleSystem.pAction( PSA_SOURCE_TYPE_ID ) );

	if (!pSource)
	{
		return;
	}

	const RompColliderPtr pGS = pSource->groundSpecifier();
	if (!pGS)
	{
		return;
	}

	uint64 tend = timestamp() + stampsPerSecond() / 2000;

	Particles::iterator it = particleSystem.begin();
	Particles::iterator end = particleSystem.end();

	WorldTriangle tri;

	//bust out of the loop if we take more than 0.5 msec
	while (it != particleSystem.end() && timestamp() < tend)
	{
		Particle & particle = *it;

		if (!particle.isAlive())
		{		
			continue;
		}

		//note - particles get moved after actions.
		Vector3 velocity;
		particle.getVelocity( velocity );
		Vector3 newPos;
		particleSystem.predictPosition( particle, dTime, newPos );
		float tValue = pGS->collide( particle.position(), newPos, tri );
		if (tValue >= 0.f && tValue <= 1.f)
		{
#ifndef EDITOR_ENABLED
			tri.bounce( velocity, 1.f );
			particle.setVelocity( velocity );

			if ( callback_ )
			{
				PyObject * pFn = PyObject_GetAttrString(
					&*callback_, "onSplat" );
				PyObject * pTuple = PyTuple_New( 3 );
				Vector3 collidePos( particle.position() * tValue );
				collidePos += (newPos * (1.f - tValue));
				PyTuple_SetItem( pTuple, 0, Script::getData( collidePos ) );

				Vector3 velocity;
				particle.getVelocity( velocity );
				PyTuple_SetItem( pTuple, 1, Script::getData( velocity ) );

				PyTuple_SetItem( pTuple, 2, Script::getData( 
					Colour::getVector4Normalised( particle.colour() ) ) );
				Script::callNextFrame( pFn, pTuple, "SplatPSA::execute" );
			}
#endif
			it = particleSystem.removeParticle( it );
		}
		else
		{
			++it;
		}
	}
}


void SplatPSA::loadInternal( DataSectionPtr pSect )
{
}


void SplatPSA::saveInternal( DataSectionPtr pSect ) const
{
}


ParticleSystemActionPtr SplatPSA::clone() const
{
	SplatPSA * psa = new SplatPSA();
	ParticleSystemAction::clone( psa );
	return psa;
}


// -----------------------------------------------------------------------------
// Section: Python Interface to the PySplatPSA.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PySplatPSA )

/*~ function Pixie.SplatPSA
 *	Factory function to create and return a new PySplatPSA object. SplatPSA is a
 *	ParticleSystemAction that splats particles onto the collision scene.
 *	@return A new PySplatPSA object.
 */
PY_FACTORY_NAMED( PySplatPSA, "SplatPSA", Pixie )

PY_BEGIN_METHODS( PySplatPSA )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PySplatPSA )
	/*~ attribute PySplatPSA.callback
	 *	The class that is called when a particle hits the collision scene.
	 *	The class must have a method onSplat( self, (position3),(velocity3),
	 *	(colour4) )
	 */
	PY_ATTRIBUTE( callback )
PY_END_ATTRIBUTES()


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python.
 */
PyObject *PySplatPSA::pyNew( PyObject *args )
{
	SplatPSAPtr pAction = new SplatPSA;
	return new PySplatPSA(pAction);
}


PY_SCRIPT_CONVERTERS( PySplatPSA )

BW_END_NAMESPACE

// splat_psa.cpp
