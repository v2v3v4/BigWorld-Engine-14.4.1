#ifndef EFFECT_HPP
#define EFFECT_HPP

#include "phase.hpp"
#include "pyscript/stl_to_py.hpp"

BW_BEGIN_NAMESPACE

#ifdef EDITOR_ENABLED
class GeneralEditor;
#endif // EDITOR_ENABLED


namespace PostProcessing
{
	/**
	 *	This class allows the post-processing
	 *	manager to work with a list of effects; it also ensures
	 *	that all effects are python objects.
	 */
	class Effect : public PyObjectPlus
	{
		Py_Header( Effect, PyObjectPlus )
	public:
		Effect( PyTypeObject *pType = &s_type_ );
		virtual ~Effect();

		virtual void tick( float dTime );
		virtual void draw( class Debug* );

		virtual bool load( DataSectionPtr );
		virtual bool save( DataSectionPtr );

		typedef BW::vector<PhasePtr> Phases;
		const Phases& phases() const { return phases_; }

#ifdef EDITOR_ENABLED
		virtual void edEdit( GeneralEditor * editor );
#endif // EDITOR_ENABLED

		PY_FACTORY_DECLARE()
		PY_RW_ATTRIBUTE_DECLARE( phasesHolder_, phases )
		PY_RW_ATTRIBUTE_DECLARE( name_, name )
		PY_RW_ATTRIBUTE_DECLARE( bypass_, bypass )

	private:
		Phases phases_;
		PySTLSequenceHolder<Phases>	phasesHolder_;
		BW::string name_;
		Vector4ProviderPtr bypass_;
	};

	typedef SmartPointer<Effect> EffectPtr;
};

PY_SCRIPT_CONVERTERS_DECLARE( PostProcessing::Effect )

#ifdef CODE_INLINE
#include "effect.ipp"
#endif

BW_END_NAMESPACE

#endif //#ifndef EFFECT_HPP
