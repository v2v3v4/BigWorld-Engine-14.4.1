#include "pch.hpp"

#include "python_input_substituter.hpp"

#include "personality.hpp"

DECLARE_DEBUG_COMPONENT2( "Script", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PythonInputSubstituter
// -----------------------------------------------------------------------------


/**
 *	Perform dollar substitution on this line, using the named function from the
 *	provided module.  If the module isn't provided, the personality module is
 *	used.
 */
BW::string PythonInputSubstituter::substitute( const BW::string & line,
	PyObject* pModule, const char * funcName )
{
	// Use the personality module if none provided
	if (pModule == NULL)
	{
		pModule = Personality::instance().get();

		if (pModule == NULL)
		{
			return line;
		}
	}

	PyObjectPtr pFunc = PyObjectPtr(
		PyObject_GetAttrString( pModule, funcName ),
		PyObjectPtr::STEAL_REFERENCE );

	if (!pFunc)
	{
		return line;
	}

	if (!PyCallable_Check( pFunc.getObject() ))
	{
		PyErr_Format( PyExc_TypeError,
			"Macro expansion function '%s' is not callable", funcName );
		PyErr_Print();
		return "";
	}

	PyObjectPtr pExpansion = PyObjectPtr(
		PyObject_CallFunction( pFunc.getObject(), "s", line.c_str() ),
		PyObjectPtr::STEAL_REFERENCE );

	if (pExpansion == NULL)
	{
		PyErr_Print();
		return "";
	}

	else if (!PyString_Check( pExpansion.getObject() ))
	{
		PyErr_Format( PyExc_TypeError, "Macro expansion returned non-string" );
		PyErr_Print();
		return "";
	}

	else
	{
		return BW::string( PyString_AsString( pExpansion.getObject() ) );
	}
}

BW_END_NAMESPACE

// python_input_substituter.cpp
