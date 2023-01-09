#ifndef MANAGER_HPP
#define MANAGER_HPP

#include "cstdmf/init_singleton.hpp"
#include "effect.hpp"
#include "debug.hpp"


BW_BEGIN_NAMESPACE

namespace PostProcessing
{
	class Manager : public Singleton< Manager >
	{
	public:
		void tick( float dTime );
		void draw();
		void debug( DebugPtr d )	{ debug_ = d; }
		DebugPtr debug() const		{ return debug_; }
		typedef BW::vector<EffectPtr> Effects;
		const Effects& effects() const { return effects_; }
		PY_MODULE_STATIC_METHOD_DECLARE( py_chain )
		PY_MODULE_STATIC_METHOD_DECLARE( py_debug )
		PY_MODULE_STATIC_METHOD_DECLARE( py_profile )
		PY_MODULE_STATIC_METHOD_DECLARE( py_save )
		PY_MODULE_STATIC_METHOD_DECLARE( py_load )
	private:
		PyObject* getChain() const;
		PyObject* setChain( PyObject * args );
		Effects effects_;
		DebugPtr debug_;
		typedef Manager This;
	};
};

#ifdef CODE_INLINE
#include "manager.ipp"
#endif

BW_END_NAMESPACE

#endif //#ifndef MANAGER_HPP
