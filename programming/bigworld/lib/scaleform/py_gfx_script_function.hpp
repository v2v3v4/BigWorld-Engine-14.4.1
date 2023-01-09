#ifndef PY_GFXSCRIPTFUNCTION_HPP
#define PY_GFXSCRIPTFUNCTION_HPP

#include "config.hpp"
#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
	class PyMovieView;

	class PyGFxScriptFunction : public GFx::FunctionHandler
	{
	public:
		PyGFxScriptFunction(PyObject *pyScript, PyMovieView *pyMovieView, const BW::string &funcName);
		~PyGFxScriptFunction() { }

		void releasePointers();
		void Call(const GFx::FunctionHandler::Params &params);
	
	private:
		PyObjectPtr pyFunction_;
		PyMovieView *pyMovieView_;
		BW::string funcName_;
	};

	typedef SF::Ptr<PyGFxScriptFunction> PyGFxScriptFunctionPtr;
}

BW_END_NAMESPACE

#endif