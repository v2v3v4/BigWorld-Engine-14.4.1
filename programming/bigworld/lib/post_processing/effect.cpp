#include "pch.hpp"
#include "effect.hpp"
#include "debug.hpp"
#include "phase_factory.hpp"


#ifdef EDITOR_ENABLED
#include "gizmo/general_editor.hpp"
#endif //EDITOR_ENABLED


DECLARE_DEBUG_COMPONENT2( "PostProcessing", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "effect.ipp"
#endif

namespace PostProcessing
{

// Python statics
PY_TYPEOBJECT( Effect )

PY_BEGIN_METHODS( Effect )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Effect )
	/*~ attribute Effect.phases
	 *	@components{ client, tools }
	 *
	 *	This attribute represents the individual phases within an effect.  It
	 *	is managed as a list, so you can add, remove and re-order the phases
	 *	within the effect.
	 *
	 *	@type List.
	 */
	PY_ATTRIBUTE( phases )
	/*~ attribute Effect.name
	 *	@components{ client, tools }
	 *	Arbitrary name for the effect.
	 *	@type String.
	 */
	PY_ATTRIBUTE( name )
	/*~ attribute Effect.bypass
	 *	@components{ client, tools }
	 *	[Optional] Vector4 Providing a bypass signal.
	 *	If an effect is given a bypass provider, it will automatically stop
	 *	drawing its phases if the bypass.w reaches 0.  For example, say
	 *	you create a Vector4 and use it to fade out the transfer phase for a
	 *	complex effect.  Once the Vector4 alpha value reaches 0, the transfer
	 *	has no effect, so you may as well not run any of the intermediate phases
	 *	either.  So hook up that fader Vector4 to the effect bypass value, and
	 *	the effect can automatically self-optimise.
	 *	@type Vector4Provider.
	 */
	PY_ATTRIBUTE( bypass )
PY_END_ATTRIBUTES()


/*~ class PostProcessing.Effect
 *	@components{ client, tools }
 *	This class allows the post-processing
 *	manager to work with a generic list of effects; it also ensures
 *	that all effects are python objects.
 */
/*~ function PostProcessing.Effect
 *	@components{ client, tools }
 *	Factory function to create and return a PostProcessing Effect object. 
 *	@return A new PostProcessing Effect object.
 */
PY_FACTORY_NAMED( Effect, "Effect", _PostProcessing )


Effect::Effect( PyTypeObject *pType ):
	PyObjectPlus( pType ),
	phasesHolder_( phases_, this, true )
{
}


Effect::~Effect()
{
}


/**
 *	This method ticks the effect.
 *	@param	dTime	delta frame time, in seconds.
 */
void Effect::tick(float dTime)
{
	Phases::iterator it = phases_.begin();
	Phases::iterator en = phases_.end();
	for(; it!=en; it++)
	{
		PhasePtr& phase = *it;
		phase->tick(dTime);
	}
}


/**
 *	This method draws the effect.
 *	@param	debug	optional debugging object to records each phase's results.
 */
void Effect::draw( Debug* debug )
{
	if ( debug )
	{
		debug->beginEffect( static_cast<uint32>( phases_.size() ) );
	}

	bool bypass = false;
	if ( bypass_.hasObject() )
	{
		Vector4 b;
		bypass_->output(b);
		bypass = almostEqual( b.w, 0.f );
	}

	if (!bypass)
	{
		Phases::iterator it = phases_.begin();
		Phases::iterator en = phases_.end();
		for(; it!=en; it++)
		{
			PhasePtr& phase = *it;
			phase->draw(debug);
		}
	}
	else
	{
		if (debug)
		{
			for (size_t i=0; i < phases_.size(); i++)
			{
				debug->drawDisabledPhase();
			}
		}
	}
}


bool Effect::load( DataSectionPtr pDS )
{
	name_ = pDS->asString(name_);

	DataSectionPtr bSect = pDS->findChild("bypass");
	if (bSect.hasObject())
	{
		Vector4 bp = bSect->asVector4();
		this->bypass_ = Vector4ProviderPtr( new Vector4Basic(bp), true );
	}	

	phases_.clear();

	DataSection::iterator it = pDS->begin();
	DataSection::iterator en = pDS->end();
	for(; it!=en; ++it)
	{
		DataSectionPtr phase = *it;

		// Don't try to load 'bypass' as a phase (must check by name, packed
		// sections returns a different pointer hear than from its findChild).
		if (phase->sectionName() == "bypass")
			continue;

		PhasePtr p( PhaseFactory::loadItem(phase), true );
		if (p.hasObject())
		{
			phases_.push_back(p);
		}
		else
		{
			ERROR_MSG( "Unknown phase type %s\n", phase->sectionName().c_str() );
		}
	}

	return true;
}


bool Effect::save( DataSectionPtr pDS )
{
	DataSectionPtr pSect = pDS->newSection( "Effect" );
	pSect->setString( name_ );
	if ( bypass_.hasObject() )
	{
		//note - this freezes the bypass, thus on load, it
		//becomes a static value representing the value it was at save time.
		Vector4 b;
		bypass_->output(b);
		pSect->writeVector4( "bypass", b );
	}
	Phases::iterator it = phases_.begin();
	Phases::iterator en = phases_.end();
	for(; it!=en; it++)
	{
		PhasePtr& phase = *it;
		phase->save(pSect);
	}

	return true;
}


#ifdef EDITOR_ENABLED

void Effect::edEdit( GeneralEditor * editor )
{
	// TODO: implement
}

#endif // EDITOR_ENABLED


PyObject* Effect::pyNew(PyObject* args)
{
	return new Effect;
}


}	//namespace PostProcessing

PY_SCRIPT_CONVERTERS( PostProcessing::Effect )

BW_END_NAMESPACE

// effect.cpp
