#include "pch.hpp"

#include "jitter_psa.hpp"
#include "vector_generator.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "jitter_psa.ipp"
#endif

namespace
{
const BW::StringRef JitterPSAPositionSourceString( "pPositionSrc" );
const BW::StringRef JitterPSAVelocitySourceString( "pVelocitySrc" );
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for JitterPSA.
// -----------------------------------------------------------------------------
/**
 *	This is the destructor for JitterPSA.
 */
JitterPSA::~JitterPSA()
{
	BW_GUARD;
	bw_safe_delete(pPositionSrc_);
	bw_safe_delete(pVelocitySrc_);
}


// -----------------------------------------------------------------------------
// Section: Methods for JitterPSA.
// -----------------------------------------------------------------------------

/**
 *	This sets a vector generator as the new generator for jitter of the
 *	particle positions.
 *
 *	@param pPositionSrc		The new source for jitter positions.
 */
void JitterPSA::setPositionSource( VectorGenerator *pPositionSrc )
{
	BW_GUARD;
	if ( pPositionSrc != pPositionSrc_ )
	{
		bw_safe_delete(pPositionSrc_);
		pPositionSrc_ = pPositionSrc;
	}
}

/**
 *	This sets a vector generator as the new generator for jitter of the
 *	particle velocities.
 *
 *	@param pVelocitySrc		The new source for jitter velocities.
 */
void JitterPSA::setVelocitySource( VectorGenerator *pVelocitySrc )
{
	BW_GUARD;
	if ( pVelocitySrc != pVelocitySrc_ )
	{
		bw_safe_delete(pVelocitySrc_);
		pVelocitySrc_ = pVelocitySrc;
	}
}


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
void JitterPSA::execute( ParticleSystem &particleSystem, float dTime )
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
void JitterPSA::lateExecute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( JitterPSA_execute );

	Particles::iterator current = particleSystem.begin();
	Particles::iterator endOfParticles = particleSystem.end();
	while ( current != endOfParticles )
	{
		Particle & particle = *current++;
		if (particle.isAlive())
		{
			if ( affectPosition() && ( pPositionSrc_ != NULL ) )
			{
				Vector3 jitter;
				pPositionSrc_->generate( jitter );

				particle.position() += jitter * dTime;
			}

			if ( affectVelocity() && ( pVelocitySrc_ != NULL ) )
			{
				Vector3 jitter;
				pVelocitySrc_->generate( jitter );

				Vector3 velocity;
				particle.getVelocity( velocity );
				particle.setVelocity( velocity + jitter );
			}
		}
	}
}


/**
 *	This method determines the memory footprint of the action
 */
size_t JitterPSA::sizeInBytes() const
{
    size_t sz = sizeof(JitterPSA);
    if (pPositionSrc_ != NULL)
        sz += pPositionSrc_->sizeInBytes();
    if (pVelocitySrc_ != NULL)
        sz += pVelocitySrc_->sizeInBytes();
    return sz;
}


/**
 *	This is the serialiser for JitterPSA properties
 */
template < typename Serialiser >
void JitterPSA::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, JitterPSA, affectPosition );
	BW_PARITCLE_SERIALISE( s, JitterPSA, affectVelocity );
}


void JitterPSA::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< JitterPSA >( pSect, this ) );

	DataSectionPtr pVGSect = pSect->openSection( JitterPSAPositionSourceString );
	if (pVGSect)
	{
		for (DataSectionIterator it = pVGSect->begin(), end = pVGSect->end();
			it != end; ++it)
		{
			DataSectionPtr pDS = *it;
			BW::string vgType = pDS->sectionName();
			pPositionSrc_ = VectorGenerator::createGeneratorOfType( vgType );
			MF_ASSERT_DEV( pPositionSrc_ );
			if (pPositionSrc_)
			{
				pPositionSrc_->load( pDS );
			}
		}
	}

	pVGSect = pSect->openSection( JitterPSAVelocitySourceString );
	if (pVGSect)
	{
		for (DataSectionIterator it = pVGSect->begin(), end = pVGSect->end();
			it != end; ++it)
		{
			DataSectionPtr pDS = *it;
			BW::string vgType = pDS->sectionName();
			pVelocitySrc_ = VectorGenerator::createGeneratorOfType( vgType );
			MF_ASSERT_DEV(pVelocitySrc_);
			if (pVelocitySrc_)
			{
				pVelocitySrc_->load( pDS );
			}
		}
	}
}


void JitterPSA::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< JitterPSA >( pSect, this ) );

	if (pPositionSrc_)
	{
		DataSectionPtr pVGSect = pSect->newSection( JitterPSAPositionSourceString );
		pPositionSrc_->save( pVGSect );
	}

	if (pVelocitySrc_)
	{
		DataSectionPtr pVGSect = pSect->newSection( JitterPSAVelocitySourceString );
		pVelocitySrc_->save( pVGSect );
	}
}


ParticleSystemActionPtr JitterPSA::clone() const
{
	BW_GUARD;
	JitterPSA * psa = new JitterPSA();
	ParticleSystemAction::clone( psa );
	this->serialise( BW::CloneObject< JitterPSA >( this, psa ) );

	if (pPositionSrc_)
	{
		psa->pPositionSrc_ = pPositionSrc_->clone();
	}

	if (pVelocitySrc_)
	{
		psa->pVelocitySrc_ = pVelocitySrc_->clone();
	}

	return psa;
}

// -----------------------------------------------------------------------------
// Section: Python Interface to the PyJitterPSA.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyJitterPSA )

/*~ function Pixie.JitterPSA
 *	Factory function to create and return a new PyJitterPSA object. JitterPSA is a
 *	ParticleSystemAction that adds random motion to the particles.
 *	@return A new PyJitterPSA object.
 */
PY_FACTORY_NAMED( PyJitterPSA, "JitterPSA", Pixie )

PY_BEGIN_METHODS( PyJitterPSA )
	/*~ function PyJitterPSA.setPositionSource
	 *	Sets a new VectorGenerator as a jitter position source for the
	 *	particles.
	 *	@param generator VectorGenerator. Jitter position source for the
	 *	particles.
	 */
	PY_METHOD( setPositionSource )
	/*~ function PyJitterPSA.setVelocitySource
	 *	Sets a new VectorGenerator as a jitter velocity source for the
	 *	particles.
	 *	@param generator VectorGenerator. Jitter velocity source for the
	 *	particles.
	 */
	PY_METHOD( setVelocitySource )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyJitterPSA )
	/*~ attribute PyJitterPSA.affectPosition
	 *	This is a flag to determine whether JitterPSA affects position. Default
	 *	value is 0 (false).
	 *	@type Integer as boolean. 0 or 1.
	 */
	PY_ATTRIBUTE( affectPosition )
	/*~ attribute PyJitterPSA.affectVelocity
	 *	This is a flag to determine whether JitterPSA affects velocity. Default
	 *	is 0 (false).
	 *	@type Integer as boolean. 0 or 1.
	 */
	PY_ATTRIBUTE( affectVelocity )
PY_END_ATTRIBUTES()


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be two lists (vector generator descriptions for position
 *					and velocity.) Both arguments are optional.
 */
PyObject *PyJitterPSA::pyNew( PyObject *args )
{
	BW_GUARD;
	PyObject *positionDesc = NULL;
	PyObject *velocityDesc = NULL;

	if ( PyArg_ParseTuple( args, "|OO", &positionDesc, &velocityDesc ) )
	{
		VectorGenerator *posGen = NULL;
		if ( positionDesc != NULL )
		{
			posGen = VectorGenerator::parseFromPython( positionDesc );
			if ( posGen == NULL )
			{
				return NULL;	// Error already set by the parser.
			}
		}

		VectorGenerator *vecGen = NULL;
		if ( velocityDesc != NULL )
		{
			vecGen = VectorGenerator::parseFromPython( velocityDesc );
			if ( vecGen == NULL )
			{
				return NULL;	// Error already set by the parser.
			}
		}

		JitterPSAPtr pAction = new JitterPSA( posGen, vecGen );
		return new PyJitterPSA(pAction);
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "JitterPSA:"
			"Expected two lists describing vector generators." );
		return NULL;
	}
}

/**
 *	This Python method allows the script to specify the parameters for a
 *	new vector generator to the jitter positions.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be a list describing a vector generator.
 */
PyObject *PyJitterPSA::py_setPositionSource( PyObject *args )
{
	BW_GUARD;
	PyObject *positionDesc = NULL;

	if ( PyArg_ParseTuple( args, "O", &positionDesc ) )
	{
		VectorGenerator *vGen = VectorGenerator::parseFromPython(
			positionDesc );
		if ( vGen != NULL )
		{
			pAction_->setPositionSource( vGen );
			Py_RETURN_NONE;
		}
		else
		{
			return NULL;	// Error already set by the parser.
		}
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "JitterPSA.setPositionSource: "
			"Expected a list describing vector generators." );
		return NULL;
	}
}

/**
 *	This Python method allows the script to specify the parameters for a
 *	new vector generator to the jitter velocities.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be a list describing a vector generator.
 */
PyObject *PyJitterPSA::py_setVelocitySource( PyObject *args )
{
	BW_GUARD;
	PyObject *velocityDesc = NULL;

	if ( PyArg_ParseTuple( args, "O", &velocityDesc ) )
	{
		VectorGenerator * vg = VectorGenerator::parseFromPython( velocityDesc );
		if ( vg != NULL )
		{
			pAction_->setVelocitySource( vg );
			Py_RETURN_NONE;
		}
		else
		{
			return NULL;	// Error already set by the parser.
		}
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "JitterPSA.setVelocitySource: "
			"Expected a list describing vector generators." );
		return NULL;
	}
}


PY_SCRIPT_CONVERTERS( PyJitterPSA )

BW_END_NAMESPACE

// jitter_psa.cpp
