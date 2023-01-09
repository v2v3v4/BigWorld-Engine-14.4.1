#include "pch.hpp"
#include "py_gfx_script_function.hpp"
#include "py_gfx_value.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW {

PyGFxScriptFunction::PyGFxScriptFunction(PyObject *pyScript, PyMovieView *pyMovieView, const BW::string &funcName)
{
	BW_GUARD;

	MF_ASSERT(pyMovieView != NULL);

	funcName_ = funcName;
	pyMovieView_ = pyMovieView;
	
	pyFunction_ = PyObject_GetAttrString(pyScript, funcName.c_str());
	Py_XDECREF(pyFunction_.get());
}

void PyGFxScriptFunction::releasePointers()
{
	BW_GUARD;

	pyMovieView_ = NULL;
	pyFunction_ = NULL;
}

void PyGFxScriptFunction::Call(const GFx::FunctionHandler::Params &params)
{
	BW_GUARD;

	if (pyMovieView_ == NULL || pyFunction_ == NULL)
		return;

	PyObject *pyArgs = PyTuple_New(params.ArgCount);
	for (uint i = 0; i < params.ArgCount; i++)
	{
		PyObject *pyArg = PyGFxValue::convertValueToPython(params.pArgs[i], pyMovieView_);
		PyTuple_SetItem(pyArgs, i, pyArg);
	}

	PyObject *pyFn = pyFunction_.get();
	Py_XINCREF(pyFn);
	PyObject *pyRet = Script::ask(pyFn, pyArgs);

	if (pyRet != NULL && pyRet != Py_None && params.pRetVal != NULL)
		*params.pRetVal = PyGFxValue::convertValueToGFx(pyRet, pyMovieView_);
}

}

BW_END_NAMESPACE