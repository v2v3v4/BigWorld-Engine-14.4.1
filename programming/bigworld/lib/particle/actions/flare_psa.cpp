#include "pch.hpp"

#include "flare_psa.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"
#include "particle/renderers/particle_system_renderer.hpp"
#include "romp/lens_effect_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "flare_psa.ipp"
#endif


/*static*/ void FlarePSA::prerequisites(
		DataSectionPtr pSection,
		BW::set<BW::string>& output )
{
	BW_GUARD;
	const BW::string & flareName = pSection->readString( "flareName_" );
	if (flareName.length())
		output.insert(flareName);
}


/**
 *	This is the serialiser for FlarePSA properties
 */
template < typename Serialiser >
void FlarePSA::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, FlarePSA, flareName );
	BW_PARITCLE_SERIALISE( s, FlarePSA, flareStep );
	BW_PARITCLE_SERIALISE( s, FlarePSA, colourize );
	BW_PARITCLE_SERIALISE( s, FlarePSA, useParticleSize );
}


void FlarePSA::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< FlarePSA >( pSect, this ) );
}


void FlarePSA::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< FlarePSA >( pSect, this ) );
}


ParticleSystemActionPtr FlarePSA::clone() const
{
	BW_GUARD;
	FlarePSA * psa = new FlarePSA();
	ParticleSystemAction::clone( psa );
	this->serialise( BW::CloneObject< FlarePSA >( this, psa ) );
	return psa;
}


/**
 *	Load the associated lens effect from the name already set
 */
void FlarePSA::loadLe( void )
{
	BW_GUARD;
	if ( !flareName_.length() )
	{
		return;
	}

	DataSectionPtr flareSection = BWResource::openSection( flareName_ );
	if ( !flareSection || !LensEffect::isLensEffect( flareName_ ) )
	{
		ASSET_MSG( "FlarePSA::loadLe: Couldn't load lens effect %s\n",
			flareName_.c_str() );
		return;
	}

	le_.load( flareSection );
}

/**
 *	This method attaches flare to given particle
 *
 *  @param particleSystem			Particle system
 *  @param matrix					World transform
 *	@param Particles::iterator		The particle to attach flare to.
 */
void FlarePSA::addFlareToParticle( ParticleSystem & particleSystem,
	const Matrix & worldTransform, Particles::iterator & pItr )
{
	const Particle & particle = *pItr;

	bool isMesh = particleSystem.pRenderer()->isMeshStyle();

	if (useParticleSize_ && !isMesh)
	{
		le_.size( particle.size() );
	}

	if (colourize_)
	{
		le_.colour( particle.colour() );
	}
	else
	{
		le_.defaultColour();
	}

	Vector3 lePosition( worldTransform.applyPoint( particle.position() ) );
	uint32 pId = ParticleSystem::getUniqueParticleID( pItr, particleSystem );

	LensEffectManager::instance().add( pId , lePosition, le_ );
	particleSystem.addFlareID( pId );
}

/**
 *	This method executes the action for the given frame of time. The dTime
 *	parameter is the time elapsed since the last call.
 *
 *	@param particleSystem	The particle system on which to operate.
 *	@param dTime			Elapsed time in seconds.
 */
void FlarePSA::execute( ParticleSystem & particleSystem, float dTime )
{
	BW_GUARD;

	// This is a special case condition since particle systems do not have
	// any control over the rendering of their own flares.
	if (!particleSystem.enabled()) 
	{
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
void FlarePSA::lateExecute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( FlarePSA_execute );

	Matrix worldTransform = particleSystem.pRenderer()->local() ? 
		particleSystem.worldTransform() : Matrix::identity;

	if (flareStep_ == 0)
	{
		// This now uses a very new method to determine the last particle added (bug 32689)
		// find the youngest particle
		Particles::iterator begItr = particleSystem.begin();
		Particles::iterator endItr = particleSystem.end();

		Particles::iterator current = endItr;
		float minAge = FLT_MAX;

		for (Particles::iterator itr = begItr; itr != endItr; ++itr)
		{
			const Particle & p = *itr;
			if (p.age() <= minAge)
			{
				minAge = p.age();
				current = itr;
			}
		}
		if (current != endItr)
		{
			addFlareToParticle( particleSystem, worldTransform, current );
		}
	}
	else if (flareStep_ > 0)
	{
		Particles::iterator current = particleSystem.begin();
		Particles::iterator endOfParticles = particleSystem.end();

		while (current != endOfParticles)
		{
			Particle & particle = *current;

			if (particle.isAlive())
			{
				// Use the particle index to ensure that particles keep their flare
				// in the case where there is a sink (bug 5236).
				size_t index = particleSystem.particles()->index(current);
				if (index % flareStep_ == 0)
				{

					addFlareToParticle( particleSystem, worldTransform, current );
				}
			}

			++current;
		}
	}
}


// -----------------------------------------------------------------------------
// Section: Python Interface to the PyFlarePSA.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyFlarePSA )

/*~ function Pixie.FlarePSA
 *	Factory function to create and return a new PyFlarePSA object. FlarePSA is a
 *	ParticleSystemAction that draws a lens flare at the location of the oldest
 *	particle.
 *	@param filename Name of the flare resource to use.
 *	@return A new PyFlarePSA object.
 */
 PY_FACTORY_NAMED( PyFlarePSA, "FlarePSA", Pixie )

PY_BEGIN_METHODS( PyFlarePSA )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyFlarePSA )
	/*~ attribute PyFlarePSA.flareName
	 *	This value determines which lens effect resource to use for flares.
	 *	Default is empty string.
	 *	@type String.
	 */
	PY_ATTRIBUTE( flareName )
	/*~ attribute PyFlarePSA.flareStep
	 *	This value determines which particles have a flare drawn on them. 0 is 
	 *	leader only, 1 is all particles, 2..x is every X particles.
	 *	Default is 0.
	 *	@type Integer. 0..n.
	 */
	PY_ATTRIBUTE( flareStep )
	/*~ attribute PyFlarePSA.colourize
	 *	Specifies whether the flare should be colourized from the colour of the
	 *	particle.
	 *	Default is 0 (false).
	 *	@type Integer as boolean. 0 or 1.
	 */
	PY_ATTRIBUTE( colourize )
	/*~ attribute PyFlarePSA.useParticleSize
	 *	Specifies whether the effect should use the size of the particle for its
	 *	size.
	 *	Default is 0 (false).
	 *	@type Integer as boolean.
	 */
	PY_ATTRIBUTE( useParticleSize )
PY_END_ATTRIBUTES()


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python.
 */
PyObject *PyFlarePSA::pyNew( PyObject *args )
{
	BW_GUARD;
	char * flareType = NULL;
	int flareStep = 0;	//leader only
	int col = 1;
	int size = 0;

	if ( PyArg_ParseTuple( args, "s|iii", &flareType, &flareStep, &col, &size ) )
	{
		FlarePSAPtr pAction = new FlarePSA( flareType, flareStep, col, size );
		return new PyFlarePSA(pAction);
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "FlarePSA:"
			"Expected a lens flare filename and optional params (flare step),(colourize),(useParticleSize)." );
		return NULL;
	}
}


PY_SCRIPT_CONVERTERS( PyFlarePSA )

BW_END_NAMESPACE

// flare_psa.cpp
