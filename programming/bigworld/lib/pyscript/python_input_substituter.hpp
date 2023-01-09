#ifndef PYTHON_INPUT_SUBSTITUTER_HPP
#define PYTHON_INPUT_SUBSTITUTER_HPP

#include "Python.h"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This is a helper class to substitute strings
 */
class PythonInputSubstituter
{
public:
	static BW::string substitute( const BW::string & line,
		PyObject* pModule = NULL, const char * funcName = "expandMacros" );
};

BW_END_NAMESPACE

#endif // PYTHON_INPUT_SUBSTITUTER_HPP
