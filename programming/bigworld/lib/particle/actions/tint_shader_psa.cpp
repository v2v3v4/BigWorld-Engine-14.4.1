#include "pch.hpp"

#include "tint_shader_psa.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"
#include "romp/fog_controller.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "tint_shader_psa.ipp"
#endif


namespace
{

const BW::StringRef TintsString( "tints_" );
const BW::StringRef TintsElementString( "Tint" );
const BW::StringRef TintColor( "color" );
const BW::StringRef TintTime( "time" );

bool tintCompare( const TintShaderPSA::Tint & a, const TintShaderPSA::Tint & b )
{
	return a.first < b.first;
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
void TintShaderPSA::execute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( TintShaderPSA_execute );

	// Do nothing if the dTime passed was zero or if the particle system
	// is not quite old enough to be active.
	if ( ( age_ < delay() ) || ( dTime <= 0.0f ) )
	{
		age_ += dTime;
		return;
	}

	// Do nothing if the map is empty.
	if ( tints_.empty() )
	{
		return;
	}

	Vector4 multiplier(1.f,1.f,1.f,1.f);
	Vector4 colour;
	if ( modulator_ )
	{
		modulator_->output( multiplier );
	}

	Vector4 fogColour( FogController::instance().v4colour() );
	fogColour *= Vector4( 1.f/255.f, 1.f/255.f, 1.f/255.f, 1.f );

	Particles::iterator current = particleSystem.begin();
	Particles::iterator endOfParticles = particleSystem.end();
	while ( current != endOfParticles )
	{
		Particle &particle = *current;

		if (!particle.isAlive())
		{
			++current;
			continue;
		}

		// Check repeat to calculate effective age.
		float effectiveAge;
		if ( repeat() && period_ != 0.0f )
		{
			effectiveAge = fmodf( particle.age(), period_ );
		}
		else
		{
			effectiveAge = particle.age();
		}

		Tints::const_iterator lower = std::upper_bound( std::begin( tints_ ),
			std::end( tints_ ), Tint( effectiveAge, Vector4() ), tintCompare );
		Tints::const_iterator upper;

		// Calculate our two poles for tint blending.
		if (lower != tints_.end())
		{
			upper = lower == tints_.begin() ? lower : lower--;
		}
		else
		{
			upper = --lower;
		}


		// Blend between the two poles.
		if (upper == lower)
		{
			colour = upper->second * multiplier;			
		}
		else
		{
			// Interpolate linearly between the two poles.

			// Get the colour difference.
			colour = upper->second;
			colour -= lower->second;

			// Multiply by the ratios to get the percent of the difference.
			colour *= ( effectiveAge - lower->first ) /
				( upper->first - lower->first);

			// Add the lower bound to get the linear interpolation.
			colour += lower->second;
			colour *= multiplier;			
		}

		// And blend in some fog colour, if requested.
		if (fogAmount_ > 0.f)
		{			
			fogColour[3] = colour[3];
			colour *= (1.f - fogAmount_);
			colour += (fogAmount_ * fogColour);
		}

		particle.colour( Colour::getUint32FromNormalised( colour ) );

		++current;
	}

}


/**
 *	This method determines the memory footprint of the action
 */
size_t TintShaderPSA::sizeInBytes() const
{
	return sizeof(TintShaderPSA) + tints_.size() * sizeof(Tint); 
}


/**
 *	This is the serialiser for TintShaderPSA properties
 */
template < typename Serialiser >
void TintShaderPSA::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, TintShaderPSA, repeat );
	BW_PARITCLE_SERIALISE( s, TintShaderPSA, period );
	BW_PARITCLE_SERIALISE( s, TintShaderPSA, fogAmount );
}


void TintShaderPSA::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< TintShaderPSA >( pSect, this ) );

	// Save this period since clearAllTints will change the period value to 0.0
	float period = period_; 

	//make sure we can load repeatedly, and not
	//accumulate keyframes
	this->clearAllTints();

	// Read it in
	const DataSectionPtr & tintsSect = pSect->openSection( TintsString );

	//Firstly lets catch any tint shaders which have no tints (use the tint shader creation defaults)
	if (tintsSect.getObject() == NULL)
	{
		tints_.reserve( 2 );
		tints_.push_back( Tint( 0.f, Vector4( 0.f, 0.f, 0.f, 1.f ) ) );
		tints_.push_back( Tint( period, Vector4( 1.f, 1.f, 1.f, 1.f ) ) );
	}
	//Now lets catch any tint shaders which only have one tint
	else if (tintsSect->countChildren() == 1)
	{
		DataSectionIterator it = tintsSect->begin();
		const DataSectionPtr & tintSubSect = *it;

		Vector4 color = tintSubSect->readVector4( TintColor, Vector4::zero() );

		if (period == 0.0f)
		{
			period = 1.0f;
		}

		tints_.reserve( 2 );
		tints_.push_back( Tint( 0.0f, color ) );
		tints_.push_back( Tint( period, color ) );
	}
	else // All is well...
	{
		tints_.reserve( tintsSect->countChildren() );
		for (DataSectionIterator it = tintsSect->begin(),
			itEnd = tintsSect->end(); it != itEnd; ++it)
		{
			const DataSectionPtr & tintSubSect = *it;
			float time = tintSubSect->readFloat( TintTime, 0.0f );
			Vector4 color = tintSubSect->readVector4( TintColor,
				Vector4::zero() );
			tints_.push_back( Tint( time, color ) );
		}
		std::sort( std::begin( tints_ ), std::end( tints_ ), tintCompare );
	}

	period_ = period; // Restore the period value
}


void TintShaderPSA::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< TintShaderPSA >( pSect, this ) );

	DataSectionPtr tintsSect = pSect->newSection( TintsString );

	for (Tints::const_iterator it = tints_.begin(), itEnd = tints_.end();
		it != itEnd; ++it)
	{
		DataSectionPtr tintSubSect = tintsSect->newSection( 
			TintsElementString );

		float time = it->first;
		Vector4 color = it->second; 
		tintSubSect->writeFloat( TintTime, time );
		tintSubSect->writeVector4( TintColor, color );
	}
}


ParticleSystemActionPtr TintShaderPSA::clone() const
{
	BW_GUARD;
	TintShaderPSA * psa = new TintShaderPSA();
	ParticleSystemAction::clone( psa );
	this->serialise( BW::CloneObject< TintShaderPSA >( this, psa ) );
	psa->tints_ = tints_;
	return psa;
}


/**
 *	This method adds a new tint to the particles at a given time. The tint
 *	is specified by a 4D vector, R, G, B and Alpha values.
 *
 *	@param time		Age in seconds for the particle.
 *	@param tint		Tint value for that time.
 */
void TintShaderPSA::addTintAt( float time, const Vector4 & tint )
{
	Tints::const_iterator it = std::upper_bound( std::begin( tints_ ),
		std::end( tints_ ), Tint( time, Vector4() ), tintCompare );
	tints_.insert( it, Tint( time, tint ) );
	if (time > period_)
	{
		period_ = time;
	}
}

// -----------------------------------------------------------------------------
// Section: Python Interface to the PyTintShaderPSA.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyTintShaderPSA )

/*~ function Pixie.TintShaderPSA
 *	Factory function to create and return a new PyTintShaderPSA object.
 *	TintShaderPSA is a ParticleSystemAction that alters a particles tint
 *	according to its age. 
 *	@return A new PyTintShaderPSA object.
 */
PY_FACTORY_NAMED( PyTintShaderPSA, "TintShaderPSA", Pixie )

PY_BEGIN_METHODS( PyTintShaderPSA )
	/*~ function PyTintShaderPSA.addTintAt
	 *	Adds a colour (RGBA) to blend the tint to at a particular time. Particle
	 *	colour gets changed linearly between each tint and the previous tint.
	 *	You can think of the tint like a key frame for an animation.
	 *	@param time Float. Time at which the tint key should be set in the colour
	 *	cycle.
	 *	@param colour Sequence of 4 floats. RGBA
	 */
	PY_METHOD( addTintAt )
	/*~	function PyTintShaderPSA.clearAllTints
	 *	Removes all tint values from the particles.
	 */
	PY_METHOD( clearAllTints )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyTintShaderPSA )
	/*~ attribute PyTintShaderPSA.repeat
	 *	Specifies whether the tint cycle loops over the total time sequence.
	 *	Default is 0 (false).
	 *	@type Integer as boolean. 0 or 1.
	 */
	PY_ATTRIBUTE( repeat )
	/*~ attribute PyTintShaderPSA.period
	 *	Specifies the the tint cycle loop time.	 
	 *	@type Float The period in seconds.
	 */
	PY_ATTRIBUTE( period )
	/*~ attribute PyTintShaderPSA.fogAmount
	 *	Specifies the amount of fog to blend in with the animated colour.
	 *	@type Float The amount between 0.0 and 1.0.
	 */
	PY_ATTRIBUTE( fogAmount )
	/*~	attribute PyTintShaderPSA.modulator
	 *	This modulates the tint colour with an external vector 4 provider.  
	 *	this gives python scripts the ability to fade in particles over time
	 *	(in addition to the tint being based on the particles age).
	 *	@type	Vector4Provider
	 */
	PY_ATTRIBUTE( modulator )
PY_END_ATTRIBUTES()


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python. The TintShader
 *					action takes no parameters and ignores any passed to this
 *					method.
 */
PyObject * PyTintShaderPSA::pyNew( PyObject * args )
{
	BW_GUARD;
	TintShaderPSAPtr pAction = new TintShaderPSA;
	return new PyTintShaderPSA(pAction);
}

/**
 *	This Python method allows the script to add a tint at a particular age
 *	for the particles.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be a float (time/age in seconds) followed by a four-tuple
 *					of floats (RGB-Alpha values.)
 */
PyObject * PyTintShaderPSA::py_addTintAt( PyObject * args )
{
	BW_GUARD;
	float r, g, b, alpha;
	float age;

	if ( PyArg_ParseTuple( args, "f(ffff)", &age, &r, &g, &b, &alpha ) )
	{
		Vector4 tint( r, g, b, alpha );
		pAction_->addTintAt( age, tint );
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "TintShader.addTintAt: "
			"Expected a float followed by a four-tuple of floats." );
		return NULL;
	}
}

/**
 *	This Python method allows the script to clear all tints added to the
 *	TintShader action.
 *
 *	@param args		The list of parameters passed from Python. All arguments
 *					are ignored in this method.
 */
PyObject * PyTintShaderPSA::py_clearAllTints( PyObject * args )
{
	BW_GUARD;
	pAction_->clearAllTints();
	Py_RETURN_NONE;
}


PY_SCRIPT_CONVERTERS( PyTintShaderPSA )

BW_END_NAMESPACE

// tint_shader_psa.cpp
