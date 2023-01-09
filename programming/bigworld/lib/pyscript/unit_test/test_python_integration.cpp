#include "pch.hpp"
#include "test_harness.hpp"
#include "cstdmf/bw_namespace.hpp"
#include "pyscript/script.hpp"


namespace BW
{

// import math module, call cos function and check output result
TEST_F( PyScriptUnitTestHarness, Python_ImportMathModule )
{
	PyObject *module = PyImport_ImportModule("math");
	CHECK(module);
	PyObject *func = PyObject_GetAttrString( module, "cos" );
	CHECK(func);
	PyObject* args = Py_BuildValue( "(f)", 0.0f);
	CHECK(args);
	PyObject* res = 
		Script::ask( func, args, "Python_ImportNativeModule", false );

	CHECK(PyFloat_Check(res));
	float fRes = (float)PyFloat_AsDouble(res);
	CHECK_CLOSE( 1.0f, fRes, 0.001f );
	Py_XDECREF( res );
	Py_XDECREF( module );
}

// import custom script and call a couple of simple functions on it
TEST_F( PyScriptUnitTestHarness, Python_ImportCustomModule )
{
	PyObject *module = PyImport_ImportModule("test_script");
	CHECK(module);
	// function which returns nothing
	{
		PyObject* res = 
			Script::ask( PyObject_GetAttrString( module, "funcDoNothing" ) , PyTuple_New(0),
			"Python_ImportCustomModule", false );
		CHECK(res);
		Py_XDECREF( res );
	}
	// test function which returns an integer
	{
		PyObject* res = 
			Script::ask(  PyObject_GetAttrString( module, "funcReturnInt" ) , PyTuple_New(0),
			"Python_ImportCustomModule", false );
		CHECK(PyInt_Check(res));
		int iRes = PyInt_AsLong(res);
		CHECK_EQUAL(42, iRes);
		Py_XDECREF( res );

	}
	// test function which divides first input parameter by the second one and returns result
	{
		PyObject* args = PyTuple_New( 2 );
		PyTuple_SetItem( args, 0, PyInt_FromLong( 6 ) );
		PyTuple_SetItem( args, 1, PyInt_FromLong( 2 ) );

		PyObject* res = 
			Script::ask( PyObject_GetAttrString( module, "funcDiv" ) , args,
			"Python_ImportCustomModule", false );
		CHECK(PyInt_Check(res));
		int iRes = PyInt_AsLong(res);
		CHECK_EQUAL(3, iRes);
		Py_XDECREF( res );
	}

	// test function which concatenates two strings and returns result
	{
		PyObject* args = PyTuple_New( 2 );
		PyTuple_SetItem( args, 0, PyString_FromString( "hello " ) );
		PyTuple_SetItem( args, 1, PyString_FromString( "world" ) );
		//PyTuple_SetItem( args, 1, PyString_FromString( msg.c_str() ) );

		PyObject* res = 
			Script::ask( PyObject_GetAttrString( module, "funcSum" ) , args,
			"Python_ImportCustomModule", false );
		CHECK(PyString_Check(res));
		char* resStr = PyString_AsString(res);
		CHECK_EQUAL("hello world", BW::string(resStr));
		Py_XDECREF( res );
	}

	Py_XDECREF( module );
}

} // namespace BW 
