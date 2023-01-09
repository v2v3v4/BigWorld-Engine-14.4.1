#include "pch.hpp"
#include "test_harness.hpp"
#include "cstdmf/bw_namespace.hpp"
#include "pyscript/script.hpp"
#include "pyscript/pickler.hpp"


namespace BW
{

// test cPickle module integration
TEST_F( PyScriptUnitTestHarness, Python_cPickle )
{
	PyObject *module = PyImport_ImportModule("test_cpickle");
	CHECK(module);
	if (module)
	{
		PyObject* res = 
			Script::ask( PyObject_GetAttrString( module, "testPickleAndUnpickle" ) , PyTuple_New( 0 ),
			"", false );

		CHECK_EQUAL(true, PyInt_AsLong(res) != 0);
		Py_XDECREF( res );
	}
	Py_XDECREF( module );
}

// test pyscript::cPickler class
TEST_F( PyScriptUnitTestHarness, Python_PicklerClass )
{
	// we don't need to explicitly initialise cPickle module if we are using Pickler class
	PyObject* args = PyTuple_New( 2 );
	PyTuple_SetItem( args, 0, PyString_FromString( "teststring" ) );
	PyTuple_SetItem( args, 1, PyInt_FromLong( 42 ));

	ScriptObject obj(args, false);
	BW::string pickledData = Pickler::pickle( obj );
	CHECK( pickledData != "" );

	ScriptObject unpickledObj = Pickler::unpickle( pickledData );
	CHECK( unpickledObj.get() );
	PyObject* param0 = PyTuple_GetItem( unpickledObj.get(), 0 );
	PyObject* param1 = PyTuple_GetItem( unpickledObj.get(), 1 );
	//static ScriptObject 	
	CHECK( PyString_Check(param0) );
	CHECK( PyInt_Check(param1) );
	CHECK_EQUAL( "teststring", BW::string( PyString_AsString( param0 ) ) );
	CHECK_EQUAL( 42, PyInt_AsLong( param1 ) );
}

// test zlib module integration
TEST_F( PyScriptUnitTestHarness, Python_zlib )
{
	PyObject *module = PyImport_ImportModule("test_zlib");
	CHECK(module);
	if (module)
	{
		PyObject* res = 
			Script::ask( PyObject_GetAttrString( module, "testZlib" ) , PyTuple_New( 0 ),
			"", false );

		CHECK_EQUAL(true, PyInt_AsLong(res) != 0);
		Py_XDECREF( res );
	}
	Py_XDECREF( module );
}

// encrypt a simple string using hashlib and check output hash
TEST_F( PyScriptUnitTestHarness, Python_ImportHashlibModule )
{
	PyObject *module = PyImport_ImportModule("test_hashlib");
	CHECK(module);
	if (module)
	{
		PyObject* args = PyTuple_New( 1 );
		PyTuple_SetItem( args, 0, PyString_FromString( "test source string" ) );
		PyObject* res = 
			Script::ask( PyObject_GetAttrString( module, "encryptTest" ) , args,
			"Python_ImportHashlibModule", false );

		CHECK(PyString_Check(res));
		CHECK_EQUAL("e1537ae4352f712da08b93d34310f6de", BW::string(PyString_AsString(res)));

		Py_XDECREF( res );
		Py_XDECREF( module );
	}
}

// loading PYD is not supported even in current dynamically linked python

} // namespace BW
