
#include "pch.hpp"
#include "matrix_swarm_psa.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "matrix_swarm_psa.ipp"
#endif


/**
 *	Put this matrix particle system actions into a queue to be updated later.
 *	
 *	Because matrix swarm actions use positions on other target objects,
 *	they need to wait for all of the targets to be updated first.
 *
 *	@param particleSystem	The particle system on which to operate.
 *	@param dTime			Elapsed time in seconds.
 */
void MatrixSwarmPSA::execute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER(MatrixSwarmPSA_execute);

	if (targets_.empty())
	{
		return;
	}

	ParticleSystemAction::pushLateUpdate( *this, particleSystem, dTime );
}


/**
 *	This method executes the action for the given frame of time. The dTime
 *	parameter is the time elapsed since the last call.
 *
 *	@param particleSystem	The particle system on which to operate.
 *	@param dTime			Elapsed time in seconds.
 */
void MatrixSwarmPSA::lateExecute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( MatrixSwarmPSA_execute );

	Particles::iterator it = particleSystem.begin();
	Particles::iterator end = particleSystem.end();

	const int m = static_cast< int >( targets_.size() );
	int i = (particleSystem.size() < m) ? rand() : 0;

	while (it != end)
	{
		Particle & particle = *it++;

		if (particle.isAlive())
		{
			const MatrixProviderPtr& target = targets_[(i++)%m];
			if (target.hasObject())
			{
				Matrix targetMatrix;
				target->matrix( targetMatrix );
				particle.position() = targetMatrix.applyToOrigin();
			}
		}
	}
}


void MatrixSwarmPSA::loadInternal( DataSectionPtr pSect )
{
}


void MatrixSwarmPSA::saveInternal( DataSectionPtr pSect ) const
{
}


ParticleSystemActionPtr MatrixSwarmPSA::clone() const
{
	BW_GUARD;
	MatrixSwarmPSA * psa = new MatrixSwarmPSA();
	ParticleSystemAction::clone( psa );
	return psa;
}


// -----------------------------------------------------------------------------
// Section: Python Interface to the PyMatrixSwarmPSA.
// -----------------------------------------------------------------------------


PY_TYPEOBJECT( PyMatrixSwarmPSA )

/*~ function Pixie.MatrixSwarmPSA
 *
 *	Factory function to create and return a new PyMatrixSwarmPSA object.
 *	MatrixSwarmPSA takes a list of target matrix providers, and
 *	for each particle, assigns the position of one of the targets to it.
 *	@return A new PyMatrixSwarmPSA object.
 */
PY_FACTORY_NAMED( PyMatrixSwarmPSA, "MatrixSwarmPSA", Pixie )

PY_BEGIN_METHODS( PyMatrixSwarmPSA )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyMatrixSwarmPSA )
	/*~ attribute PyMatrixSwarmPSA.targets
	 *	This is a list of target MatrixProvider objects to apply to particles
	 *	instead of source of the particle systems transform.
	 *	@type List of MatrixProvider objects. Default is empty [].
	 */
	PY_ATTRIBUTE( targets )
PY_END_ATTRIBUTES()


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python.
 */
PyObject *PyMatrixSwarmPSA::pyNew( PyObject *args )
{
	BW_GUARD;
	MatrixSwarmPSAPtr pAction = new MatrixSwarmPSA;
	return new PyMatrixSwarmPSA(pAction);
}


/**
 *	This is the release method for matrices.
 *
 *	@param pMatrix		The matrix to be released.
 */
void MatrixProviderPtr_release( MatrixProvider *&pMatrix )
{
	BW_GUARD;
	Py_XDECREF( pMatrix );
}


PY_SCRIPT_CONVERTERS( PyMatrixSwarmPSA )

BW_END_NAMESPACE

// matrix_swarm_psa.cpp
