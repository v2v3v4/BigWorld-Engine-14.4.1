#include "pch.hpp"

#include "test_harness.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"


namespace BW
{

class PyTestClass : public PyObjectPlus
{
	Py_Header( PyTestClass, PyObjectPlus );
public:
	static int CHECK_INT;

	PyTestClass() : PyObjectPlus( &s_type_ ), intAttribute_( 0 )
	{
	}

	int testMethod()
	{
		return CHECK_INT;
	}
	PY_AUTO_METHOD_DECLARE( RETDATA, testMethod, END );

	int intAttribute_;
	PY_RW_ATTRIBUTE_DECLARE( intAttribute_, intAttribute );
};

int PyTestClass::CHECK_INT = 123456;

PY_TYPEOBJECT( PyTestClass )

PY_BEGIN_ATTRIBUTES( PyTestClass )
	PY_ATTRIBUTE( intAttribute )
PY_END_ATTRIBUTES()

PY_BEGIN_METHODS( PyTestClass )
	PY_METHOD( testMethod )
PY_END_METHODS()

typedef ScriptObjectPtr<PyTestClass> PyTestClassPtr;

TEST_F( ScriptUnitTestHarness, ScriptObjectPtr_general )
{
	PyTestClassPtr p = PyTestClassPtr( new PyTestClass(),
		ScriptObject::FROM_NEW_REFERENCE );
	CHECK( PyTestClassPtr::check( p ) );
	CHECK_EQUAL( PyTestClass::CHECK_INT, p->testMethod() );

	ScriptObject result = p.callMethod( "testMethod", ScriptErrorPrint() );
	CHECK( result );
	CHECK( ScriptInt::check( result ) );
	ScriptInt iResult = ScriptInt( result );
	CHECK_EQUAL( PyTestClass::CHECK_INT, iResult.asLong() );
}

TEST_F( ScriptUnitTestHarness, ScriptObject_getAttribute_setAttribute )
{
	PyTestClassPtr p = PyTestClassPtr( new PyTestClass(),
		ScriptObject::FROM_NEW_REFERENCE );
	CHECK_EQUAL( 0, p->intAttribute_ );

	int testInt;
	CHECK( p.getAttribute( "intAttribute", testInt, ScriptErrorPrint() ) );
	CHECK_EQUAL( 0, testInt );

	CHECK( p.setAttribute( "intAttribute", 1, ScriptErrorPrint() ) );
	CHECK_EQUAL( 1, p->intAttribute_ );

	CHECK( p.getAttribute( "intAttribute", testInt, ScriptErrorPrint() ) );
	CHECK_EQUAL( 1, testInt );

	BW::string testString;
	CHECK( !p.getAttribute( "intAttribute", testString, ScriptErrorPrint() ) );
	CHECK( !p.setAttribute( "intAttribute", "Bananas", ScriptErrorPrint() ) );
}


TEST_F( ScriptUnitTestHarness, ScriptObject_convertTo_None )
{
	// This is testing that ScriptObject::none().convertTo( ScriptObject )
	// sets ScriptObject::none() and not ScriptObject()
	// If this fails, Script::setData( ..., SmartPointer< PyObject > ) is
	// being run instead of Script::setData( ..., ScriptObject & )
	ScriptObject none = ScriptObject::none();
	ScriptObject result;
	CHECK( none.convertTo( result, ScriptErrorClear() ) );
	CHECK_EQUAL( none, result );
}


TEST_F( ScriptUnitTestHarness, ScriptObject_createFrom_None )
{
	// This is testing that ScriptObject::createFrom( ScriptObject::none )
	// returns ScriptObject::none() and not ScriptObject()
	// If this fails, Script::getData( ..., SmartPointer< PyObject > ) is
	// being run instead of Script::getData( ..., ScriptObject & )
	ScriptObject none = ScriptObject::none();
	ScriptObject result = ScriptObject::createFrom( none );
	CHECK( result.exists() );
	CHECK_EQUAL( none, result );
}


} // namespace BW 
