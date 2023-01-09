#include "pch.hpp"
#include "test_harness.hpp"
#include "pyscript/stl_to_py.hpp"
#include "pyscript/script.hpp"

namespace BW
{

// test passing stl container to python 
TEST_F( PyScriptUnitTestHarness, PassSTLVectorToPython )
{
	PyObject *module = PyImport_ImportModule("test_stl");
	CHECK(module);
	BW::vector<int> inputVec(4);
	inputVec[0] = 1;
	inputVec[1] = 2;
	inputVec[2] = 3;
	inputVec[3] = 4;

	PySTLSequenceHolder<BW::vector<int> > pyVecHolder(inputVec, NULL, true);
	// calc sum of all elements
	{
		PyObject* args = PyTuple_New( 1 );
		PyTuple_SetItem( args, 0, Script::getData( pyVecHolder ) );

		PyObject* res = 
			Script::ask( PyObject_GetAttrString( module, "sumVector" ) , args,
			"Python_ImportCustomModule", false );
		CHECK(PyInt_Check(res));
		int iRes = PyInt_AsLong(res);
		CHECK_EQUAL(10, iRes);
		Py_XDECREF( res );
	}

	Py_XDECREF( module );
}

TEST_F( PyScriptUnitTestHarness, RecieveSTLVectorFromPython )
{
	PyObject *module = PyImport_ImportModule("test_stl");
	CHECK(module);
	// build a range vector for given min and max values including min and max values itself
	{
		PyObject* args = PyTuple_New( 2 );
		PyTuple_SetItem( args, 0, PyInt_FromLong( -2 ) );
		PyTuple_SetItem( args, 1, PyInt_FromLong( 1 ) );

		PyObject* res = 
			Script::ask( PyObject_GetAttrString( module, "buildRangeVector" ) , args,
			"Python_ImportCustomModule", false );

		BW::vector<int>  resVec;
		Script::setData( res, resVec );
		CHECK_EQUAL( 4u, resVec.size() );
		CHECK_EQUAL(-2, resVec[0] );
		CHECK_EQUAL( -1, resVec[1] );
		CHECK_EQUAL( 0, resVec[2] );
		CHECK_EQUAL( 1, resVec[3] );

		Py_XDECREF( res );
	}
	Py_XDECREF( module );
}

TEST_F( PyScriptUnitTestHarness, PassAndModifySTLVector )
{
	PyObject *module = PyImport_ImportModule("test_stl");
	CHECK(module);
	BW::vector<float> inputVec(3);
	inputVec[0] = -1.5f;
	inputVec[1] = 0.75f;
	inputVec[2] = 2.25f;

	PySTLSequenceHolder<BW::vector<float> > pyVecHolder(inputVec, NULL, true);
	// clamp all values into range [0.0, 1.0]
	// modifies passed array of floats
	{
		PyObject* args = PyTuple_New( 3 );
		PyTuple_SetItem( args, 0, Script::getData( pyVecHolder ) );
		PyTuple_SetItem( args, 1, PyFloat_FromDouble( 0.0f ) );
		PyTuple_SetItem( args, 2, PyFloat_FromDouble( 1.0f )  );

		PyObject* res = 
			Script::ask( PyObject_GetAttrString( module, "clampVec" ) , args,
			"Python_ImportCustomModule", false );

		CHECK_EQUAL( 3u, inputVec.size() );
		CHECK_CLOSE( 0.0f, inputVec[0], 0.0001f );
		CHECK_CLOSE( 0.75f, inputVec[1], 0.0001f );
		CHECK_CLOSE(1.0f, inputVec[2], 0.0001f );

		Py_XDECREF( res );
	}
	Py_XDECREF( module );
}

} // BW namespace
