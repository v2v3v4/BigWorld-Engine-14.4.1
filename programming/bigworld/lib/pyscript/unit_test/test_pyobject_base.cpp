#include "pch.hpp"
#include "test_harness.hpp"
#include "pyscript/pyobject_base.hpp"
#include "pyscript/pyobject_plus.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	
class BaseObj : public PyObjectPlus
{
	Py_Header( BaseObj, PyObjectPlus )

public:
	BaseObj( PyTypeObject * pType, PyObject * ref = NULL );
	~BaseObj();

	int pyTraverse( visitproc visit, void * arg );
	int pyClear();

	static int s_instCount;

private:
	PyObject * ref_;
};

int BaseObj::s_instCount = 0;

PY_BASETYPEOBJECT( BaseObj )

PY_BEGIN_METHODS( BaseObj )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( BaseObj )
PY_END_ATTRIBUTES()


BaseObj::BaseObj( PyTypeObject * pType, PyObject * ref ) :
	PyObjectPlus( pType, true ),
	ref_(ref)
{
	Py_XINCREF(ref_);
	TRACE_MSG( "BaseObj ctor\n" );
	++s_instCount;
}


BaseObj::~BaseObj()
{
	TRACE_MSG( "BaseObj dtor\n" );
	Py_XDECREF(ref_);
	--s_instCount;
}

int BaseObj::pyTraverse( visitproc visit, void * arg )
{
	TRACE_MSG( "BaseObj::pyTraverse\n" );
	Py_VISIT( ref_ );
	return 0; // TODO: check that parent shouldn't be called
}

int BaseObj::pyClear()
{
	TRACE_MSG( "BaseObj::pyClear\n" );
	Py_CLEAR( ref_ );
	return 0; // TODO: check that parent shouldn't be called
}

} // end anonymous namespace


TEST_F( PyScriptUnitTestHarness, BaseCreation )
{
	BaseObj::s_instCount = 0;
	PyObject * obj = PyType_GenericAlloc( &BaseObj::s_type_, 0 );
	CHECK( obj );
	CHECK( BaseObj::Check( obj ) );

	if (obj)
	{
		new ( obj ) BaseObj( &BaseObj::s_type_ );
		while (PyGC_Collect() > 0);
	}
	
	Py_XDECREF( obj );
	while (PyGC_Collect() > 0);
	CHECK_EQUAL( 0, BaseObj::s_instCount );
}


#if 0
/* This test should be enabled when PY_TYPEOBJECT_SPECIALISE_GC macro is implemented*/
TEST_F( PyScriptUnitTestHarness, CyclicReferences )
{
	TRACE_MSG( "---- START Cyclic References ----\n" );

	BaseObj::s_instCount = 0;
	PyObject * obj1 = PyType_GenericAlloc( &BaseObj::s_type_, 0 );
	CHECK( obj1 );
	CHECK( BaseObj::Check( obj1 ) );

	PyObject * obj2 = PyType_GenericAlloc( &BaseObj::s_type_, 0 );
	CHECK( obj2 );
	CHECK( BaseObj::Check( obj2 ) );

	new ( obj1 ) BaseObj( &BaseObj::s_type_, obj2 ); 
	new ( obj2 ) BaseObj( &BaseObj::s_type_, obj1 );

	TRACE_MSG( "Objects created\n" );

	while (PyGC_Collect() > 0);

	TRACE_MSG( "Releasing objects\n" );

	Py_XDECREF( obj1 );
	Py_XDECREF( obj2 );

	TRACE_MSG( "Objects released\n" );

	while (PyGC_Collect() > 0);

	CHECK_EQUAL( 0, BaseObj::s_instCount );

	TRACE_MSG( "---- END Cyclic References ----\n" );
}
#endif // if 0

BW_END_NAMESPACE
